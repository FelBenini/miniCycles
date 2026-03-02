#ifndef CAMERA_H
# define CAMERA_H

# include "rt_math.h"
# include "../glad/include/glad/glad.h"
# include <GLFW/glfw3.h>

typedef struct s_camera
{
	t_vec4	pos;
	t_vec4	forward;
	t_vec4	right;
	t_vec4	up;
	float	fov;
	float	yaw;
	float	pitch;
	int		is_active;
}	t_camera;

typedef struct s_cam_uniforms
{
	GLint	pos;
	GLint	forward;
	GLint	right;
	GLint	up;
	GLint	fov;
}	t_cam_uniforms;

# define CAM_SPEED 0.05f
# define CAM_SENSITIVITY 0.05f

t_camera		camera_create(float x, float y, float z, float fov_deg);
void			upload_camera(GLuint program, t_cam_uniforms u, t_camera *cam);
void			camera_update_basis(t_camera *cam);
t_cam_uniforms	get_cam_uniform_locations(GLuint program);
void			upload_camera(GLuint program, t_cam_uniforms u, t_camera *cam);
void			mouse_button_callback(GLFWwindow *win, int button, int action, int mods);

#endif
