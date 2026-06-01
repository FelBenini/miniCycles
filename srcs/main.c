#include "camera.h"
#include "cycles.h"
#include "frame.h"
#include "rt_math.h"
#include "scene.h"
#include <GLFW/glfw3.h>
#include <stdint.h>
#include <stdio.h>
#include "parser.h"
#include "input.h"
#include "screenshot.h"

static void	register_callbacks(t_cycles cycles, t_camera *cam)
{
	(void)cam;
	glfwSetCursorPosCallback(cycles.win, mouse_callback);
	glfwSetScrollCallback(cycles.win, scroll_callback);
	glfwSetMouseButtonCallback(cycles.win, mouse_button_callback);
}

	int	main(int argc, char *argv[])
{
	t_cycles			cycles;
	t_scene				scene;
	t_cam_uniforms		cam_u;
	t_all_uniforms		all_u;
	char				title[126];

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
	all_u = get_all_uniform_locations(cycles.compute_program, cycles.fullscreen_program);
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
		if (cycles.preview || was_preview != cycles.preview)
		{
			frame_index = 0;
			preview_frame_index = 0;
			reset_samples = 1;
		}
		else
			reset_samples = 0;
		upload_camera(cycles.compute_program, cam_u, &scene.camera);
		was_preview = cycles.preview;
		render_frame(
			cycles,
			all_u.compute,
			all_u.fragment,
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
		scene.camera.dirty = 0;
		cycles.dirty = 0;
	}
	scene_destroy(&scene);
	glfwTerminate();
	return (0);
}
