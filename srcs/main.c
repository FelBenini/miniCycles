#include "bvh.h"
#include "camera.h"
#include "cycles.h"
#include "input.h"
#include "material.h"
#include "mesh.h"
#include "parser.h"
#include "rt_math.h"
#include "scene.h"
#include <GLFW/glfw3.h>
#include <stdint.h>
#include <stdio.h>

#define WIDTH 1920
#define HEIGHT 1080

void	create_balls(t_scene *scene)
{
	uint32_t	mat_ball;
	uint32_t	mat_plane;
	uint32_t	mat_cone;
	uint32_t	mat_cube;
	uint32_t	mat_cylinder;
	t_mesh		plane;
	t_mesh		ball;
	t_mesh		cone;
	t_mesh		cube;
	t_mesh		cylinder;
	t_mesh		torus;

	mat_ball = scene_add_material(scene, (t_material){
		.albedo = (t_vec4){0.8f, 0.2f, 0.2f, 1.0f},
		.emission = (t_vec4){0.0f, 0.0f, 0.0f, 1.0f},
		.roughness = 0.05f,
		.metallic = 0.0f,
		.ior = 1.5f,
		.type = 0
	});
	mat_plane = scene_add_material(scene, (t_material){
		.albedo = (t_vec4){0.7f, 0.7f, 0.7f, 1.0f},
		.emission = (t_vec4){0.0f, 0.0f, 0.0f, 1.0f},
		.roughness = 1.0f,
		.metallic = 0.5f,
		.ior = 1.5f,
		.type = 0
	});
	mat_cone = scene_add_material(scene, (t_material){
		.albedo = (t_vec4){0.2f, 0.8f, 0.2f, 1.0f},
		.emission = (t_vec4){0.0f, 0.0f, 0.0f, 1.0f},
		.roughness = 0.4f,
		.metallic = 0.0f,
		.ior = 1.5f,
		.type = 0
	});
	mat_cube = scene_add_material(scene, (t_material){
		.albedo = (t_vec4){0.2f, 0.2f, 0.8f, 1.0f},
		.emission = (t_vec4){0.0f, 0.0f, 0.0f, 1.0f},
		.roughness = 0.6f,
		.metallic = 0.0f,
		.ior = 1.5f,
		.type = 0
	});
	mat_cylinder = scene_add_material(scene, (t_material){
		.albedo = (t_vec4){0.3f, 0.3f, 0.05f, 1.0f},
		.emission = (t_vec4){0.0f, 0.0f, 0.0f, 1.0f},
		.roughness = 0.05f,
		.metallic = 0.0f,
		.ior = 1.5f,
		.type = 0
	});

	plane = generate_plane(100, 100);
	plane.position = (t_vec4){0.0f, -1.0f, 0.0f, 0.0f};
	ball = generate_uv_sphere(32, 32, 1.0f);
	ball.position = (t_vec4){0.0f, 0.0f, 0.0f ,0.0f};
	cone = generate_cone(12, 32, 1.0f, 3.0f);
	cone.position = (t_vec4){1.5f, 0.0f, 0.0f, 0.0f};
	cube = generate_cube(2.0f);
	cube.position = (t_vec4){-2.0f, 0.0f,0.0f, 0.0f};
	cylinder = generate_cylinder(12, 32, 1.0f, 3.0f);
	cylinder.position = (t_vec4){3.0f, 0.0f, 0.0f, 0.0f};
	torus = generate_torus(32, 42, 1.5f, 0.5f);
	torus.position = (t_vec4){7.0f, -0.5f, 0.0f, 0.0f};

	scene_add_mesh(scene, ball, mat_ball);
	scene_add_mesh(scene, plane, mat_plane);
	scene_add_mesh(scene, cone, mat_cone);
	scene_add_mesh(scene, cube, mat_cube);
	scene_add_mesh(scene, cylinder, mat_cylinder);
	scene_add_mesh(scene, torus, mat_cube);
}

static void	render_frame(
	t_cycles cycles,
	GLint loc_resolution,
	GLint loc_mesh_count,
	GLint loc_frame_index,
	GLint loc_reset_samples,
	GLint loc_ambient_color,
	GLint loc_accumulation_tex_fs,
	t_scene scene,
	uint32_t frame_index,
	uint32_t reset_samples)
{
	glUseProgram(cycles.compute_program);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, scene.ssbo_triangles);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, scene.ssbo_normals);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, scene.ssbo_meshes);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, scene.ssbo_bvh_nodes);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, scene.ssbo_tlas_nodes);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, scene.ssbo_materials);

	glUniform4f(loc_ambient_color, scene.ambient.x, scene.ambient.y, scene.ambient.z, scene.ambient.w);

	glBindImageTexture(0, cycles.tex, 0, GL_FALSE, 0,
		GL_READ_WRITE, GL_RGBA32F);
	glUniform2f(loc_resolution, (float)WIDTH, (float)HEIGHT);
	glUniform1ui(loc_mesh_count, scene.mesh_count);
	glUniform1ui(loc_frame_index, frame_index);
	glUniform1ui(loc_reset_samples, reset_samples);

	glDispatchCompute((WIDTH + 7) / 8, (HEIGHT + 7) / 8, 1);

	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(cycles.fullscreen_program);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, cycles.tex);
	glUniform1i(loc_accumulation_tex_fs, 0);

	glDrawArrays(GL_TRIANGLES, 0, 6);
}

static void	register_callbacks(t_cycles cycles, t_camera *cam)
{
	glfwSetInputMode(cycles.win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetWindowUserPointer(cycles.win, cam);
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
	GLint	loc_ambient_color;

	uint32_t frame_index = 0;
	uint32_t reset_samples = 1;

	if (argc != 2)
	{
		printf("Please, pass a file as an argument.\n");
		return (1);
	}
	cycles = init_cycles();
	scene = parse_scene(argv[1]);
	scene_upload_triangles(&scene);
	scene_upload_materials(&scene);
	scene_upload_bvh_nodes(&scene);
	scene_rebuild_tlas(&scene);
	scene_upload_tlas_nodes(&scene);
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
	loc_accumulation_tex_fs = glGetUniformLocation(
		cycles.fullscreen_program, "u_accumulation_tex");
	while (!glfwWindowShouldClose(cycles.win))
	{
		glfwPollEvents();
		handle_input(cycles.win, &scene.camera);
		if (scene.camera.dirty)
		{
			frame_index = 0;
			reset_samples = 1;
		}
		else
		{
			reset_samples = 0;
			scene.camera.dirty = 0;
		}
		if (scene.desc_dirty)
			scene_upload_descriptors(&scene);
		if (scene.material_dirty)
			scene_upload_materials(&scene);
		if (scene.tlas_dirty)
		{
			scene_rebuild_tlas(&scene);
			scene_upload_tlas_nodes(&scene);
		}
		upload_camera(cycles.compute_program, cam_u, &scene.camera);
		render_frame(
			cycles,
			loc_resolution,
			loc_mesh_count,
			loc_frame_index,
			loc_reset_samples,
			loc_ambient_color,
			loc_accumulation_tex_fs,
			scene,
			frame_index,
			reset_samples);
		frame_index++;
		snprintf(title, sizeof(title), "miniCycles | sample %u", frame_index);
		glfwSetWindowTitle(cycles.win, title);
		glfwSwapBuffers(cycles.win);
		scene.camera.dirty = 0;
	}
	scene_destroy(&scene);
	glfwTerminate();
	return (0);
}
