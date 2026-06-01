#ifndef CYCLES_H
# define CYCLES_H

#include "../glad/include/glad/glad.h"
#include <GLFW/glfw3.h>
#include <lut.h>
#include <camera.h>

# define TRIANGLE_VERTS 1
# define TRIANGLE_NORMS 2
# define TEXCOORDS_SSBOS 3
# define MESHES_SSBOS 4
# define BVH_SSBOS 5
# define TLAS_SSBOS 6
# define MATERIALS_SSBOS 7
# define LIGHT_SSBOS 8
# define IMAGES_SSBOS 9
# define PIXELS_SSBOS 10
# define EMISSIVE_MESHES_SSBOS 11
# define RAY_QUEUE_SSBO_A 12
# define RAY_QUEUE_SSBO_B 13
# define RAY_COUNTERS_SSBO 14

# define MAX_TILE_PIXELS (128 * 128)
# define STATE_SIZE_BYTES 128

# define NO_TONEMAP 0
# define CUBE_LUT_TONEMAP 1

typedef struct s_cycles
{
	GLFWwindow		*win;
	t_camera		*cam;
	GLuint			compute_program;
	GLuint			fullscreen_program;
	GLuint			tex;
	GLuint			preview_tex;
	GLuint			vao;
	unsigned int	tonemap;
	GLuint			lut_tex;
	int				lut_size;
	int				width;
	int				height;
	int				preview_width;
	int				preview_height;
	int				dirty;
	int				preview;
	GLuint			ray_queue_ssbo[2];
	GLuint			counters_ssbo;
}	t_cycles;

typedef struct s_compute_uniforms
{
	GLint	loc_resolution;
	GLint	loc_tile_offset;
	GLint	loc_mesh_count;
	GLint	loc_frame_index;
	GLint	loc_reset_samples;
	GLint	loc_ambient_color;
	GLint	loc_sky_tex;
	GLint	loc_sky_intensity;
	GLint	loc_light_count;
	GLint	loc_emissive_mesh_count;
	GLint	loc_max_bounces;
	GLint	loc_pass_type;
}	t_compute_uniforms;

typedef struct s_fragment_uniforms
{
	GLint	loc_accumulation_tex_fs;
	GLint	loc_tonemap_fs;
	GLint	loc_lut_tex_fs;
	GLint	loc_lut_size_fs;
}	t_fragment_uniforms;

typedef struct s_all_uniforms
{
	t_compute_uniforms	compute;
	t_fragment_uniforms	fragment;
}	t_all_uniforms;

t_cycles		init_cycles(void);
GLuint			gen_lut_tex(t_lut lut);
t_all_uniforms	get_all_uniform_locations(GLuint compute_program, GLuint fullscreen_program);
void			resize_callback(GLFWwindow *win, int width, int height);

#endif
