#include <stddef.h>
#include <stdlib.h>
#include "shader.h"
#include "cycles.h"
#include "lut.h"
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <string.h>

#define WIDTH 1920
#define HEIGHT 1080

static GLuint	gen_tex(int width, int height)
{
	GLuint	tex;

	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA,
		GL_FLOAT, NULL);
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
	cycles->width = width;
	cycles->height = height;
	cycles->tex = gen_tex(width, height);
	glViewport(0, 0, width, height);
	cycles->dirty = 1;
}

void	parse_cycles_args(t_cycles *cycles, char **args, int argv)
{
	int		i;
	t_lut	lut;
	char	*lut_path;

	i = 2;
	lut.size = 0;
	lut.data = 0;
	lut_path = NULL;
	while (i < argv)
	{
		if (strncmp(args[i], "--tonemap=", 10) == 0)
		{
			if (strncmp(args[i] + 10, "agx", 3) == 0)
				cycles->tonemap = AGX_TONEMAP;
			else if (strncmp(args[i] + 10, "cube", 4) == 0)
				cycles->tonemap = CUBE_LUT_TONEMAP;
		}
		if (strncmp(args[i], "--lut=", 6) == 0)
			lut_path = args[i] + 6;
		i++;
	}
	if (lut_path)
	{
		lut = load_lut(lut_path);
		if (lut.size == 0)
			printf("Warning: Failed to load LUT, falling back to no tonemap\n");
	}
	if (cycles->tonemap == CUBE_LUT_TONEMAP)
	{
		if (lut.size > 0)
		{
			cycles->lut_tex = gen_lut_tex(lut);
			cycles->lut_size = lut.size;
		}
		else
		{
			printf("Error: --tonemap=cube requires --lut=<file.cube>\n");
			cycles->tonemap = NO_TONEMAP;
		}
	}
	destroy_lut(&lut);
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
														"shaders/pathtrace.comp.glsl",
														NULL);
	cycles.fullscreen_program = shader_create_graphics("shaders/fullscreen.vert.glsl",
			"shaders/fullscreen.frag.glsl");
	cycles.tex = gen_tex(WIDTH, HEIGHT);
	cycles.vao = gen_vao();
	cycles.tonemap = NO_TONEMAP;
	cycles.lut_tex = 0;
	cycles.lut_size = 0;
	return (cycles);
}
