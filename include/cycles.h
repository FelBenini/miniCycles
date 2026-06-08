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
# define RAY_QUEUE_SSBO_A 11
# define RAY_QUEUE_SSBO_B 12
# define RAY_COUNTERS_SSBO 13
# define HIT_QUEUE_SSBO 14
# define SHADOW_QUEUE_SSBO 15
# define SHADOW_ACCUM_SSBO 16

# define MAX_TILE_PIXELS (128 * 128)
# define STATE_SIZE_BYTES 128
# define MAX_SHADOW_RAYS (MAX_TILE_PIXELS * 16)
# define SHADOW_RAY_SIZE 80
# define MAX_RENDER_PIXELS (1920 * 1080)

# define NO_TONEMAP 0
# define CUBE_LUT_TONEMAP 1

// CPU-side mirror of the SceneParams UBO (std140 layout)
typedef struct s_scene_ubo
{
	float	resolution[2];
	float	_pad_00[2];
	float	cam_pos[3];
	float	_pad_cam_pos;
	float	cam_forward[3];
	float	_pad_cam_forward;
	float	cam_right[3];
	float	_pad_cam_right;
	float	cam_up[3];
	float	_pad_cam_up;
	float	cam_fov;
	float	lens_radius;
	float	focal_distance;
	float	_pad_fov;
	float	ambient_color[4];
	int		sky_tex;
	float	sky_intensity;
	uint32_t mesh_count;
	uint32_t light_count;
	uint32_t emissive_mesh_count;
	uint32_t frame_index;
	uint32_t reset_samples;
	int		max_bounces;
}	t_scene_ubo;

typedef struct s_cycles
{
	GLFWwindow		*win;
	t_camera		*cam;
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
	GLuint			gen_ray_prog;
	GLuint			intersect_prog;
	GLuint			shade_prog;
	GLuint			shadow_prog;
	GLuint			accumulate_prog;
	GLuint			scene_ubo;
	GLint			tile_offset_loc;
	GLint			loc_accumulation_tex_fs;
	GLint			loc_tonemap_fs;
	GLint			loc_lut_tex_fs;
	GLint			loc_lut_size_fs;
	GLuint			ray_queue_ssbo[2];
	GLuint			counters_ssbo;
	GLuint			hit_queue_ssbo;
	GLuint			shadow_queue_ssbo;
	GLuint			shadow_accum_ssbo;
	GLsync			tile_fence;
}	t_cycles;

t_cycles		init_cycles(void);
GLuint			gen_lut_tex(t_lut lut);
void			resize_callback(GLFWwindow *win, int width, int height);

#endif
