#include "camera.h"
#include "cycles.h"
#include <GLFW/glfw3.h>
#include <math.h>

static int		g_first_move = 0;
static int		g_mouse_look = 0;
static double	g_last_time = 0.0;

void	mouse_callback(GLFWwindow *win, double x, double y)
{
	static double	last_x;
	static double	last_y;
	t_cycles		*cycles;
	t_camera		*cam;
	double			now;
	double			dt;
	float			dx;
	float			dy;

	cycles = glfwGetWindowUserPointer(win);
	cam = cycles->cam;
	if (!g_mouse_look)
	{
		g_first_move = 1;
		return;
	}
	cam->dirty = 1;
	now = glfwGetTime();
	dt = now - g_last_time;
	g_last_time = now;
	if (g_first_move) {
		last_x = x;
		last_y = y;
		g_first_move = 0;
		return ;
	}
	dx = (float)(x - last_x) * CAM_SENSITIVITY * (float)dt;
	dy = (float)(y - last_y) * CAM_SENSITIVITY * (float)dt;
	last_x = x;
	last_y = y;
	cam->yaw -= dx;
	cam->pitch -= dy;
	cam->pitch = fmaxf(-1.5f, fminf(1.5f, cam->pitch));
}

void	scroll_callback(GLFWwindow *win, double xoffset, double yoffset)
{
	t_cycles	*cycles;
	t_camera	*cam;

	cycles = glfwGetWindowUserPointer(win);
	cam = cycles->cam;
	cam->dirty = 1;
	(void)xoffset;
	cam->fov -= (float)yoffset * 0.05f;
	if (cam->fov < 0.17f)
		cam->fov = 0.17f;
	if (cam->fov > 2.62f)
		cam->fov = 2.62f;
}

void	mouse_button_callback(GLFWwindow *win, int button, int action, int mods)
{
    (void)win;
    (void)mods;
    if (button == GLFW_MOUSE_BUTTON_MIDDLE)
    {
		if (action == GLFW_PRESS)
		{
			g_first_move = 1;
			g_mouse_look = 1;
			glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		}
		else if (action == GLFW_RELEASE)
		{
			g_mouse_look = 0;
			glfwSetInputMode(win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}
	}
}
