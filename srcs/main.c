#include "camera.h"
#include "cycles.h"
#include "rt_math.h"
#include "scene.h"
#include <GLFW/glfw3.h>
#include <stdint.h>
#include <stdio.h>
#include "parser.h"
#include "input.h"
#include "screenshot.h"

static void	render_frame(
	t_cycles cycles,
	GLint loc_resolution,
	GLint loc_mesh_count,
	GLint loc_frame_index,
	GLint loc_reset_samples,
	GLint loc_ambient_color,
	GLint loc_sky_tex,
	GLint loc_sky_intensity,
	GLint loc_light_count,
	GLint loc_emissive_mesh_count,
	GLint loc_max_bounces,
	GLint loc_accumulation_tex_fs,
	GLint loc_tonemap_fs,
	GLint loc_lut_tex_fs,
	GLint loc_lut_size_fs,
	t_scene scene,
	uint32_t frame_index,
	uint32_t reset_samples,
	int preview)
{
	GLuint	tex;
	int		render_width;
	int		render_height;

	tex = preview ? cycles.preview_tex : cycles.tex;
	render_width = preview ? cycles.preview_width : cycles.width;
	render_height = preview ? cycles.preview_height : cycles.height;

	glUseProgram(cycles.compute_program);

	glUniform4f(loc_ambient_color, scene.ambient.x, scene.ambient.y, scene.ambient.z, scene.ambient.w);

	glBindImageTexture(0, tex, 0, GL_FALSE, 0,
		GL_READ_WRITE, GL_RGBA32F);
	glUniform2f(loc_resolution, (float)render_width, (float)render_height);
	glUniform1ui(loc_mesh_count, scene.mesh_count);
	glUniform1i(loc_sky_tex, scene.sky_tex);
	glUniform1f(loc_sky_intensity, scene.sky_intensity);
	glUniform1ui(loc_frame_index, frame_index);
	glUniform1ui(loc_reset_samples, reset_samples);
	glUniform1ui(loc_light_count, scene.light_count);
	glUniform1ui(loc_emissive_mesh_count, scene.emissive_mesh_count);
	glUniform1i(loc_max_bounces, preview ? 3 : 6);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glDispatchCompute((render_width + 7) / 8, (render_height + 7) / 8, 1);

	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	glViewport(0, 0, cycles.width, cycles.height);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(cycles.fullscreen_program);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex);
	glUniform1ui(loc_accumulation_tex_fs, 0);
	glUniform1ui(loc_tonemap_fs, cycles.tonemap);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_3D, cycles.lut_tex);
	glUniform1i(loc_lut_tex_fs, 1);
	glUniform1i(loc_lut_size_fs, cycles.lut_size);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

static void	register_callbacks(t_cycles cycles, t_camera *cam)
{
	(void)cam;
	glfwSetCursorPosCallback(cycles.win, mouse_callback);
	glfwSetScrollCallback(cycles.win, scroll_callback);
	glfwSetMouseButtonCallback(cycles.win, mouse_button_callback);
}

int	main(int argc, char *argv[])
{
	t_cycles		cycles;
	t_scene			scene;
	t_cam_uniforms	cam_u;
	char			title[126];

	GLint	loc_resolution;
	GLint	loc_mesh_count;
	GLint	loc_frame_index;
	GLint	loc_reset_samples;
	GLint	loc_accumulation_tex_fs;
	GLint	loc_tonemap_fs;
	GLint	loc_lut_tex_fs;
	GLint	loc_lut_size_fs;
	GLint	loc_ambient_color;
	GLint	loc_sky_tex;
	GLint	loc_sky_intensity;
	GLint	loc_light_count;
	GLint	loc_emissive_mesh_count;
	GLint	loc_max_bounces;

	uint32_t frame_index = 0;
	uint32_t preview_frame_index = 0;
	uint32_t reset_samples = 1;
	int was_preview = 1;

	if (argc < 2)
	{
		printf("Please, pass a file as an argument.\n");
		return (1);
	}
	cycles = init_cycles();

	scene = parse_scene(argv[1], &cycles);

	scene_upload_images(&scene);
	scene_upload_triangles(&scene);
	scene_upload_materials(&scene);
	scene_upload_bvh_nodes(&scene);
	scene_rebuild_tlas(&scene);
	scene_upload_tlas_nodes(&scene);
	scene_upload_lights(&scene);
	scene_upload_emissive_meshes(&scene);
	cycles.cam = &scene.camera;
	glfwSetWindowUserPointer(cycles.win, &cycles);
	register_callbacks(cycles, &scene.camera);
	cam_u = get_cam_uniform_locations(cycles.compute_program);
	loc_resolution = glGetUniformLocation(
		cycles.compute_program, "u_resolution");
	loc_mesh_count = glGetUniformLocation(
		cycles.compute_program, "u_mesh_count");
	loc_frame_index = glGetUniformLocation(
		cycles.compute_program, "u_frame_index");
	loc_reset_samples = glGetUniformLocation(
		cycles.compute_program, "u_reset_samples");
	loc_ambient_color = glGetUniformLocation(
				cycles.compute_program, "u_ambient_color");
	loc_sky_tex = glGetUniformLocation(
				cycles.compute_program, "u_sky_tex");
	loc_sky_intensity = glGetUniformLocation(
				cycles.compute_program, "u_sky_intensity");
	loc_light_count = glGetUniformLocation(
					cycles.compute_program, "u_light_count");
	loc_emissive_mesh_count = glGetUniformLocation(
					cycles.compute_program, "u_emissive_mesh_count");
	loc_max_bounces = glGetUniformLocation(
					cycles.compute_program, "u_max_bounces");
	loc_accumulation_tex_fs = glGetUniformLocation(
		cycles.fullscreen_program, "u_accumulation_tex");
	loc_tonemap_fs = glGetUniformLocation(
			cycles.fullscreen_program, "u_tonemap");
	loc_lut_tex_fs = glGetUniformLocation(
			cycles.fullscreen_program, "u_lut_tex");
	loc_lut_size_fs = glGetUniformLocation(
			cycles.fullscreen_program, "u_lut_size");
	glfwShowWindow(cycles.win);
	while (!glfwWindowShouldClose(cycles.win))
	{
		if (glfwGetKey(cycles.win, GLFW_KEY_P) == GLFW_PRESS)
			save_screenshot(cycles.width, cycles.height);
		glfwSwapBuffers(cycles.win);
		glfwPollEvents();
		handle_input(cycles.win, &scene.camera);
		if (scene.desc_dirty)
			scene_upload_descriptors(&scene);
		if (scene.material_dirty)
		{
			scene_upload_materials(&scene);
			scene_upload_emissive_meshes(&scene);
		}
		if (scene.emissive_mesh_dirty)
			scene_upload_emissive_meshes(&scene);
		if (scene.tlas_dirty)
		{
			scene_rebuild_tlas(&scene);
			scene_upload_tlas_nodes(&scene);
		}
		cycles.preview = scene.camera.dirty || cycles.dirty;
		if (scene.camera.dirty || cycles.dirty)
		{
			if (cycles.preview)
				preview_frame_index = 0;
			else
				frame_index = 0;
			reset_samples = 1;
		}
		else
			reset_samples = 0;
		scene.camera.dirty = 0;
		cycles.dirty = 0;
		upload_camera(cycles.compute_program, cam_u, &scene.camera);
		if (was_preview && !cycles.preview)
		{
			frame_index = 0;
			reset_samples = 1;
		}
		else if (!was_preview && cycles.preview)
		{
			preview_frame_index = 0;
			reset_samples = 1;
		}
		was_preview = cycles.preview;
		render_frame(
			cycles,
			loc_resolution,
			loc_mesh_count,
			loc_frame_index,
			loc_reset_samples,
			loc_ambient_color,
			loc_sky_tex,
			loc_sky_intensity,
			loc_light_count,
			loc_emissive_mesh_count,
			loc_max_bounces,
			loc_accumulation_tex_fs,
			loc_tonemap_fs,
			loc_lut_tex_fs,
			loc_lut_size_fs,
			scene,
			cycles.preview ? preview_frame_index : frame_index,
			reset_samples,
			cycles.preview);
		if (cycles.preview)
			preview_frame_index++;
		else
			frame_index++;
		snprintf(title, sizeof(title), "miniCycles | sample %u", frame_index);
		glfwSetWindowTitle(cycles.win, title);
		glFinish();
		scene.camera.dirty = 0;
	}
	scene_destroy(&scene);
	glfwTerminate();
	return (0);
}
