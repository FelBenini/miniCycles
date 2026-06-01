#include "frame.h"

static void	tile_reset_counters(t_cycles cycles)
{
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, RAY_QUEUE_SSBO_A,
		cycles.ray_queue_ssbo[0]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, RAY_QUEUE_SSBO_B,
		cycles.ray_queue_ssbo[1]);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, cycles.counters_ssbo);
	GLuint zero = 0;
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(zero), &zero);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);
}

static void	tile_gen_rays(t_compute_uniforms u, int tile_x, int tile_y,
				int groups_x, int groups_y)
{
	glUniform1ui(u.loc_pass_type, 0);
	glUniform2f(u.loc_tile_offset, (float)tile_x, (float)tile_y);
	glDispatchCompute(groups_x, groups_y, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

static int	tile_bounce_loop(t_cycles cycles, t_compute_uniforms u,
				int groups_1d, int max_bounces)
{
	int	read_buf = 0;
	int	write_buf = 1;

	for (int bounce = 0; bounce < max_bounces; bounce++)
	{
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, RAY_QUEUE_SSBO_A,
			cycles.ray_queue_ssbo[read_buf]);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, RAY_QUEUE_SSBO_B,
			cycles.ray_queue_ssbo[write_buf]);

		glUniform1ui(u.loc_pass_type, 1);
		glDispatchCompute(groups_1d, 1, 1);
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		int	tmp = read_buf;
		read_buf = write_buf;
		write_buf = tmp;
	}
	return (read_buf);
}

static void	tile_accumulate(t_cycles cycles, t_compute_uniforms u,
				int read_buf, int tile_x, int tile_y, int groups_1d)
{
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, RAY_QUEUE_SSBO_A,
		cycles.ray_queue_ssbo[read_buf]);
	glUniform1ui(u.loc_pass_type, 2);
	glUniform2f(u.loc_tile_offset, (float)tile_x, (float)tile_y);
	glDispatchCompute(groups_1d, 1, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}

void	render_frame(
	t_cycles cycles,
	t_compute_uniforms compute_u,
	t_fragment_uniforms fragment_u,
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

	glUseProgram(cycles.compute_program);

	glUniform4f(compute_u.loc_ambient_color,
		scene.ambient.x, scene.ambient.y,
		scene.ambient.z, scene.ambient.w);
	glBindImageTexture(0, tex, 0, GL_FALSE, 0,
		GL_READ_WRITE, GL_RGBA32F);
	glUniform2f(compute_u.loc_resolution,
		(float)render_width, (float)render_height);
	glUniform1ui(compute_u.loc_mesh_count, scene.mesh_count);
	glUniform1i(compute_u.loc_sky_tex, scene.sky_tex);
	glUniform1f(compute_u.loc_sky_intensity, scene.sky_intensity);
	glUniform1ui(compute_u.loc_frame_index, frame_index);
	glUniform1ui(compute_u.loc_reset_samples, reset_samples);
	glUniform1ui(compute_u.loc_light_count, scene.light_count);
	glUniform1ui(compute_u.loc_emissive_mesh_count,
		scene.emissive_mesh_count);
	glUniform1i(compute_u.loc_max_bounces, max_bounces);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, RAY_QUEUE_SSBO_A,
		cycles.ray_queue_ssbo[0]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, RAY_QUEUE_SSBO_B,
		cycles.ray_queue_ssbo[1]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, RAY_COUNTERS_SSBO,
		cycles.counters_ssbo);

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

			tile_reset_counters(cycles);
			tile_gen_rays(compute_u, tile_x, tile_y,
				groups_x, groups_y);

			int read_buf = tile_bounce_loop(cycles, compute_u,
					groups_1d, max_bounces);

			tile_accumulate(cycles, compute_u, read_buf,
				tile_x, tile_y, groups_1d);
		}
	}

	glViewport(0, 0, cycles.width, cycles.height);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(cycles.fullscreen_program);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex);
	glUniform1ui(fragment_u.loc_accumulation_tex_fs, 0);
	glUniform1ui(fragment_u.loc_tonemap_fs, cycles.tonemap);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_3D, cycles.lut_tex);
	glUniform1i(fragment_u.loc_lut_tex_fs, 1);
	glUniform1i(fragment_u.loc_lut_size_fs, cycles.lut_size);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}
