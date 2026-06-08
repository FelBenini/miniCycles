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
	float	lens_radius;
	float	focal_distance;
	int		is_active;
	int		dirty;
}	t_camera;

# define CAM_SPEED 0.25f
# define CAM_SENSITIVITY 0.05f
# define APERTURE_SPEED 0.005f
# define FOCAL_DIST_SPEED 0.5f

t_camera		camera_create(float x, float y, float z, float fov_deg);
void			camera_update_basis(t_camera *cam);
void			mouse_button_callback(GLFWwindow *win, int button, int action, int mods);

#endif
