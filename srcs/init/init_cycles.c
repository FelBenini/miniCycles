#include <stddef.h>
#include <stdlib.h>
#include "shader.h"
#include "cycles.h"
#include "lut.h"
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

t_all_uniforms	get_all_uniform_locations(GLuint compute_program, GLuint fullscreen_program)
{
	t_all_uniforms	u;

	u.compute.loc_resolution = glGetUniformLocation(compute_program, "u_resolution");
	u.compute.loc_tile_offset = glGetUniformLocation(compute_program, "u_tile_offset");
	u.compute.loc_mesh_count = glGetUniformLocation(compute_program, "u_mesh_count");
	u.compute.loc_frame_index = glGetUniformLocation(compute_program, "u_frame_index");
	u.compute.loc_reset_samples = glGetUniformLocation(compute_program, "u_reset_samples");
	u.compute.loc_ambient_color = glGetUniformLocation(compute_program, "u_ambient_color");
	u.compute.loc_sky_tex = glGetUniformLocation(compute_program, "u_sky_tex");
	u.compute.loc_sky_intensity = glGetUniformLocation(compute_program, "u_sky_intensity");
	u.compute.loc_light_count = glGetUniformLocation(compute_program, "u_light_count");
	u.compute.loc_emissive_mesh_count = glGetUniformLocation(compute_program, "u_emissive_mesh_count");
	u.compute.loc_max_bounces = glGetUniformLocation(compute_program, "u_max_bounces");
	u.compute.loc_pass_type = glGetUniformLocation(compute_program, "u_pass_type");
	u.fragment.loc_accumulation_tex_fs = glGetUniformLocation(fullscreen_program, "u_accumulation_tex");
	u.fragment.loc_tonemap_fs = glGetUniformLocation(fullscreen_program, "u_tonemap");
	u.fragment.loc_lut_tex_fs = glGetUniformLocation(fullscreen_program, "u_lut_tex");
	u.fragment.loc_lut_size_fs = glGetUniformLocation(fullscreen_program, "u_lut_size");
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
	cycles.compute_program = shader_create_compute_asm("shaders/structs.comp.glsl",
														"shaders/triangle.comp.glsl",
														"shaders/mat_from_dir.comp.glsl",
														"shaders/intersect_aabb.comp.glsl",
														"shaders/blas_intersect.comp.glsl",
														"shaders/scene_intersect.comp.glsl",
														"shaders/seed.comp.glsl",
														"shaders/sky.comp.glsl",
														"shaders/sample_light.comp.glsl",
														"shaders/mis.comp.glsl",
														"shaders/trace_textures.comp.glsl",
														"shaders/shade_hit.comp.glsl",
														"shaders/pathtrace.comp.glsl",
														NULL);
	cycles.fullscreen_program = shader_create_graphics("shaders/fullscreen.vert.glsl",
			"shaders/fullscreen.frag.glsl");
	cycles.tex = gen_tex(WIDTH, HEIGHT);
	cycles.preview_tex = gen_preview_tex(WIDTH, HEIGHT);
	cycles.preview_width = WIDTH / 4;
	cycles.preview_height = HEIGHT / 4;
	cycles.vao = gen_vao();

	int queue_size = MAX_TILE_PIXELS * STATE_SIZE_BYTES;
	glGenBuffers(2, cycles.ray_queue_ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, cycles.ray_queue_ssbo[0]);
	glBufferData(GL_SHADER_STORAGE_BUFFER, queue_size, NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, cycles.ray_queue_ssbo[1]);
	glBufferData(GL_SHADER_STORAGE_BUFFER, queue_size, NULL, GL_DYNAMIC_DRAW);
	glGenBuffers(1, &cycles.counters_ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, cycles.counters_ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 4 * sizeof(GLuint), NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	cycles.tonemap = NO_TONEMAP;
	cycles.lut_tex = 0;
	cycles.lut_size = 0;
	cycles.preview = 0;
	return (cycles);
}
