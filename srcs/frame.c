#include "frame.h"
#include <stdio.h>
#include <string.h>

static void	set_common_uniforms(GLuint prog, t_compute_uniforms u,
				t_scene scene, float rx, float ry,
				uint32_t frame_index, uint32_t reset_samples,
				int max_bounces)
{
	glUseProgram(prog);
	glUniform4f(u.loc_ambient_color,
		scene.ambient.x, scene.ambient.y,
		scene.ambient.z, scene.ambient.w);
	glUniform2f(u.loc_resolution, rx, ry);
	glUniform1ui(u.loc_mesh_count, scene.mesh_count);
	glUniform1i(u.loc_sky_tex, scene.sky_tex);
	glUniform1f(u.loc_sky_intensity, scene.sky_intensity);
	glUniform1ui(u.loc_frame_index, frame_index);
	glUniform1ui(u.loc_reset_samples, reset_samples);
	glUniform1ui(u.loc_light_count, scene.light_count);
	glUniform1ui(u.loc_emissive_mesh_count, scene.emissive_mesh_count);
	glUniform1i(u.loc_max_bounces, max_bounces);
}

static void	tile_reset(t_cycles cycles)
{
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, RAY_QUEUE_SSBO_A,
		cycles.ray_queue_ssbo[0]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, RAY_QUEUE_SSBO_B,
		cycles.ray_queue_ssbo[1]);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, cycles.counters_ssbo);
	glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
	GLuint zero = 0;
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(zero), &zero);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

static void	tile_gen_rays(t_cycles cycles, int tile_x, int tile_y,
				int groups_x, int groups_y)
{
	glUseProgram(cycles.gen_ray_prog);
	glUniform2f(cycles.gen_ray_u.loc_tile_offset, (float)tile_x, (float)tile_y);
	glDispatchCompute(groups_x, groups_y, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

static int	tile_bounce_loop(t_cycles cycles, int groups_1d,
				int shadow_groups, int max_bounces)
{
	int	read_buf = 0;
	int	write_buf = 1;

	for (int bounce = 0; bounce < max_bounces; bounce++)
	{
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, RAY_QUEUE_SSBO_A,
			cycles.ray_queue_ssbo[read_buf]);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, RAY_QUEUE_SSBO_B,
			cycles.ray_queue_ssbo[write_buf]);

		glUseProgram(cycles.intersect_prog);
		glDispatchCompute(groups_1d, 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		glUseProgram(cycles.shade_prog);
		glDispatchCompute(groups_1d, 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		glUseProgram(cycles.shadow_prog);
		glDispatchCompute(shadow_groups, 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		int	tmp = read_buf;
		read_buf = write_buf;
		write_buf = tmp;

		// Reset shadow counter for next bounce (offset 4 in combined counters)
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, cycles.counters_ssbo);
		glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
		GLuint sc_zero = 0;
		glBufferSubData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint), sizeof(sc_zero), &sc_zero);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}
	return (read_buf);
}

static void	tile_accumulate(t_cycles cycles, int read_buf, int groups_1d)
{
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, RAY_QUEUE_SSBO_A,
		cycles.ray_queue_ssbo[read_buf]);
	glUseProgram(cycles.accumulate_prog);
	glDispatchCompute(groups_1d, 1, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);
}

void	render_frame(
	t_cycles cycles,
	t_scene scene,
	uint32_t frame_index,
	uint32_t reset_samples,
	int preview)
{
	GLuint	tex;
	int		render_width;
	int		render_height;
	int		tile_width = 128;
	int		tile_height = 128;
	int		max_bounces = preview ? 3 : 6;

	tex = preview ? cycles.preview_tex : cycles.tex;
	render_width = preview ? cycles.preview_width : cycles.width;
	render_height = preview ? cycles.preview_height : cycles.height;

	set_common_uniforms(cycles.gen_ray_prog, cycles.gen_ray_u,
		scene, (float)render_width, (float)render_height,
		frame_index, reset_samples, max_bounces);
	set_common_uniforms(cycles.intersect_prog, cycles.intersect_u,
		scene, (float)render_width, (float)render_height,
		frame_index, reset_samples, max_bounces);
	set_common_uniforms(cycles.shade_prog, cycles.shade_u,
		scene, (float)render_width, (float)render_height,
		frame_index, reset_samples, max_bounces);
	set_common_uniforms(cycles.shadow_prog, cycles.shadow_u,
		scene, (float)render_width, (float)render_height,
		frame_index, reset_samples, max_bounces);
	set_common_uniforms(cycles.accumulate_prog, cycles.accumulate_u,
		scene, (float)render_width, (float)render_height,
		frame_index, reset_samples, max_bounces);

	// Clear shadow accumulation buffer (per-pixel direct lighting)
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, cycles.shadow_accum_ssbo);
	GLsizeiptr accum_size = (GLsizeiptr)MAX_RENDER_PIXELS * 3 * sizeof(GLuint);
	void *accum_data = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0,
		accum_size, GL_MAP_INVALIDATE_BUFFER_BIT | GL_MAP_WRITE_BIT);
	memset(accum_data, 0, accum_size);
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);

	glBindImageTexture(0, tex, 0, GL_FALSE, 0,
		GL_READ_WRITE, GL_RGBA32F);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, RAY_COUNTERS_SSBO,
		cycles.counters_ssbo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, HIT_QUEUE_SSBO,
		cycles.hit_queue_ssbo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SHADOW_QUEUE_SSBO,
		cycles.shadow_queue_ssbo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SHADOW_ACCUM_SSBO,
		cycles.shadow_accum_ssbo);

	int	tiles_dispatched = 0;

	for (int tile_y = 0; tile_y < render_height; tile_y += tile_height)
	{
		for (int tile_x = 0; tile_x < render_width; tile_x += tile_width)
		{
			int	tile_w = tile_x + tile_width > render_width
					? render_width - tile_x : tile_width;
			int	tile_h = tile_y + tile_height > render_height
					? render_height - tile_y : tile_height;
			int	tile_pixels = tile_w * tile_h;
			int	groups_x = (tile_w + 7) / 8;
			int	groups_y = (tile_h + 7) / 8;
			int	groups_1d = (tile_pixels + 63) / 64;
			int	shadow_groups = (groups_1d * 16 < 65535)
					? groups_1d * 16 : 65535;

			// --- Throttle every 4 tiles ---
			if (tiles_dispatched > 0 && tiles_dispatched % 4 == 0)
			{
				if (cycles.tile_fence)
				{
					GLenum result = glClientWaitSync(cycles.tile_fence,
						GL_SYNC_FLUSH_COMMANDS_BIT, 5000000000ULL);
					if (result == GL_WAIT_FAILED)
						fprintf(stderr, "warn: glClientWaitSync failed\n");
					glDeleteSync(cycles.tile_fence);
					cycles.tile_fence = NULL;
				}
				cycles.tile_fence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
			}

			// Reset shadow counter before bounce loop (offset 4 in combined counters)
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, cycles.counters_ssbo);
			glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
			GLuint sc_zero = 0;
			glBufferSubData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint), sizeof(sc_zero), &sc_zero);
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
			glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

			tile_reset(cycles);
			tile_gen_rays(cycles, tile_x, tile_y,
				groups_x, groups_y);

			int read_buf = tile_bounce_loop(cycles,
					groups_1d, shadow_groups, max_bounces);

			tile_accumulate(cycles, read_buf, groups_1d);

			tiles_dispatched++;
		}
	}

	// Cleanup fence after all tiles are done
	if (cycles.tile_fence)
	{
		glClientWaitSync(cycles.tile_fence,
			GL_SYNC_FLUSH_COMMANDS_BIT, 5000000000ULL);
		glDeleteSync(cycles.tile_fence);
		cycles.tile_fence = NULL;
	}

	glViewport(0, 0, cycles.width, cycles.height);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(cycles.fullscreen_program);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex);
	glUniform1ui(cycles.fragment_u.loc_accumulation_tex_fs, 0);
	glUniform1ui(cycles.fragment_u.loc_tonemap_fs, cycles.tonemap);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_3D, cycles.lut_tex);
	glUniform1i(cycles.fragment_u.loc_lut_tex_fs, 1);
	glUniform1i(cycles.fragment_u.loc_lut_size_fs, cycles.lut_size);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}
