#include <stddef.h>
#include <stdlib.h>
#include "shader.h"
#include "cycles.h"
#include "lut.h"
#include "camera.h"
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <string.h>

#define WIDTH 320
#define HEIGHT 180

static GLuint	gen_tex(int width, int height)
{
	GLuint	tex;
	
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA,
		GL_HALF_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	return (tex);
}

static GLuint	gen_preview_tex(int width, int height)
{
	GLuint	tex;
	int		preview_w = width / 4;
	int		preview_h = height / 4;
	
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, preview_w, preview_h, 0, GL_RGBA,
		GL_HALF_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	return (tex);
}

GLuint	gen_lut_tex(t_lut lut)
{
	GLuint			tex;
	float			*data;
	int				total;

	if (lut.size == 0 || !lut.data)
		return (0);
	total = lut.size * lut.size * lut.size * 3;
	data = malloc(total * sizeof(float));
	if (!data)
		return (0);
	memcpy(data, lut.data, total * sizeof(float));
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_3D, tex);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB32F, lut.size, lut.size, lut.size,
		0, GL_RGB, GL_FLOAT, data);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	free(data);
	return (tex);
}

static GLuint	gen_vao(void)
{
	GLuint	vao;

	glViewport(0, 0, WIDTH, HEIGHT);
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	return (vao);
}

void	resize_callback(GLFWwindow *win, int width, int height)
{
	t_cycles	*cycles;

	cycles = glfwGetWindowUserPointer(win);
	if (!cycles)
		return ;
	glDeleteTextures(1, &cycles->tex);
	glDeleteTextures(1, &cycles->preview_tex);
	cycles->width = width;
	cycles->height = height;
	cycles->tex = gen_tex(width, height);
	cycles->preview_tex = gen_preview_tex(width, height);
	cycles->preview_width = width / 4;
	cycles->preview_height = height / 4;
	glViewport(0, 0, width, height);
	cycles->dirty = 1;
}

static t_compute_uniforms	get_compute_uniforms(GLuint program)
{
	t_compute_uniforms	u;

	u.loc_resolution = glGetUniformLocation(program, "u_resolution");
	u.loc_tile_offset = glGetUniformLocation(program, "u_tile_offset");
	u.loc_mesh_count = glGetUniformLocation(program, "u_mesh_count");
	u.loc_frame_index = glGetUniformLocation(program, "u_frame_index");
	u.loc_reset_samples = glGetUniformLocation(program, "u_reset_samples");
	u.loc_ambient_color = glGetUniformLocation(program, "u_ambient_color");
	u.loc_sky_tex = glGetUniformLocation(program, "u_sky_tex");
	u.loc_sky_intensity = glGetUniformLocation(program, "u_sky_intensity");
	u.loc_light_count = glGetUniformLocation(program, "u_light_count");
	u.loc_emissive_mesh_count = glGetUniformLocation(program, "u_emissive_mesh_count");
	u.loc_max_bounces = glGetUniformLocation(program, "u_max_bounces");
	return (u);
}

t_cycles	init_cycles(void)
{
	t_cycles cycles;

	if (!glfwInit())
		exit(1);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    cycles.win = glfwCreateWindow(WIDTH, HEIGHT, "miniCycles", NULL, NULL);
	glfwSwapInterval(1);
	cycles.width = WIDTH;
	cycles.height = HEIGHT;
	cycles.dirty = 0;
    glfwSetFramebufferSizeCallback(cycles.win, resize_callback);
    glfwMakeContextCurrent(cycles.win);
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        printf("Failed to load GLAD\n");
        exit(1);
    }
	// --- Compile 4 compute programs ---
	cycles.gen_ray_prog = shader_create_compute_asm(
		"shaders/structs.comp.glsl",
		"shaders/seed.comp.glsl",
		"shaders/gen_ray.comp.glsl",
		NULL);
	cycles.intersect_prog = shader_create_compute_asm(
		"shaders/structs.comp.glsl",
		"shaders/triangle.comp.glsl",
		"shaders/mat_from_dir.comp.glsl",
		"shaders/intersect_aabb.comp.glsl",
		"shaders/blas_intersect.comp.glsl",
		"shaders/scene_intersect.comp.glsl",
		"shaders/sky.comp.glsl",
		"shaders/intersect_kernel.comp.glsl",
		NULL);
	cycles.shade_prog = shader_create_compute_asm(
		"shaders/structs.comp.glsl",
		"shaders/triangle.comp.glsl",
		"shaders/mat_from_dir.comp.glsl",
		"shaders/intersect_aabb.comp.glsl",
		"shaders/blas_intersect.comp.glsl",
		"shaders/scene_intersect.comp.glsl",
		"shaders/seed.comp.glsl",
		"shaders/sky.comp.glsl",
		"shaders/mis.comp.glsl",
		"shaders/sample_light.comp.glsl",
		"shaders/trace_textures.comp.glsl",
		"shaders/shade_hit.comp.glsl",
		"shaders/shade_kernel.comp.glsl",
		NULL);
	cycles.shadow_prog = shader_create_compute_asm(
		"shaders/structs.comp.glsl",
		"shaders/triangle.comp.glsl",
		"shaders/mat_from_dir.comp.glsl",
		"shaders/intersect_aabb.comp.glsl",
		"shaders/blas_intersect.comp.glsl",
		"shaders/scene_intersect.comp.glsl",
		"shaders/shadow_kernel.comp.glsl",
		NULL);
	cycles.accumulate_prog = shader_create_compute_asm(
		"shaders/structs.comp.glsl",
		"shaders/accumulate_kernel.comp.glsl",
		NULL);

	cycles.fullscreen_program = shader_create_graphics("shaders/fullscreen.vert.glsl",
			"shaders/fullscreen.frag.glsl");

	// --- Retrieve uniform locations ---
	cycles.gen_ray_u = get_compute_uniforms(cycles.gen_ray_prog);
	cycles.intersect_u = get_compute_uniforms(cycles.intersect_prog);
	cycles.shade_u = get_compute_uniforms(cycles.shade_prog);
	cycles.accumulate_u = get_compute_uniforms(cycles.accumulate_prog);
	cycles.shadow_u = get_compute_uniforms(cycles.shadow_prog);
	cycles.cam_u = get_cam_uniform_locations(cycles.gen_ray_prog);
	cycles.fragment_u.loc_accumulation_tex_fs = glGetUniformLocation(cycles.fullscreen_program, "u_accumulation_tex");
	cycles.fragment_u.loc_tonemap_fs = glGetUniformLocation(cycles.fullscreen_program, "u_tonemap");
	cycles.fragment_u.loc_lut_tex_fs = glGetUniformLocation(cycles.fullscreen_program, "u_lut_tex");
	cycles.fragment_u.loc_lut_size_fs = glGetUniformLocation(cycles.fullscreen_program, "u_lut_size");

	// --- Render targets ---
	cycles.tex = gen_tex(WIDTH, HEIGHT);
	cycles.preview_tex = gen_preview_tex(WIDTH, HEIGHT);
	cycles.preview_width = WIDTH / 4;
	cycles.preview_height = HEIGHT / 4;
	cycles.vao = gen_vao();

	// --- SSBOs ---
	int queue_size = MAX_TILE_PIXELS * STATE_SIZE_BYTES;
	glGenBuffers(2, cycles.ray_queue_ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, cycles.ray_queue_ssbo[0]);
	glBufferData(GL_SHADER_STORAGE_BUFFER, queue_size, NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, cycles.ray_queue_ssbo[1]);
	glBufferData(GL_SHADER_STORAGE_BUFFER, queue_size, NULL, GL_DYNAMIC_DRAW);
	glGenBuffers(1, &cycles.counters_ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, cycles.counters_ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 4 * sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
	glGenBuffers(1, &cycles.hit_queue_ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, cycles.hit_queue_ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, MAX_TILE_PIXELS * 80, NULL, GL_DYNAMIC_DRAW);
	glGenBuffers(1, &cycles.shadow_queue_ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, cycles.shadow_queue_ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, MAX_SHADOW_RAYS * SHADOW_RAY_SIZE, NULL, GL_DYNAMIC_DRAW);
	glGenBuffers(1, &cycles.shadow_accum_ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, cycles.shadow_accum_ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, MAX_RENDER_PIXELS * 3 * sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	cycles.tonemap = NO_TONEMAP;
	cycles.lut_tex = 0;
	cycles.lut_size = 0;
	cycles.preview = 0;
	cycles.tile_fence = NULL;
	return (cycles);
}
