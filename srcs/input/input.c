#include "input.h"
#include "camera.h"
#include <GLFW/glfw3.h>

void	handle_input(GLFWwindow *win, t_camera *cam)
{
	if (glfwGetKey(win, GLFW_KEY_W) == GLFW_PRESS)
	{
		cam->dirty = 1;
		cam->pos.x += cam->forward.x * CAM_SPEED;
		cam->pos.y += cam->forward.y * CAM_SPEED;
		cam->pos.z += cam->forward.z * CAM_SPEED;
	}
	if (glfwGetKey(win, GLFW_KEY_S) == GLFW_PRESS)
	{
		cam->dirty = 1;
		cam->pos.x -= cam->forward.x * CAM_SPEED;
		cam->pos.y -= cam->forward.y * CAM_SPEED;
		cam->pos.z -= cam->forward.z * CAM_SPEED;
	}
	if (glfwGetKey(win, GLFW_KEY_A) == GLFW_PRESS)
	{
		cam->dirty = 1;
		cam->pos.x -= cam->right.x * CAM_SPEED;
		cam->pos.z -= cam->right.z * CAM_SPEED;
	}
	if (glfwGetKey(win, GLFW_KEY_D) == GLFW_PRESS)
	{
		cam->dirty = 1;
		cam->pos.x += cam->right.x * CAM_SPEED;
		cam->pos.z += cam->right.z * CAM_SPEED;
	}
	if (glfwGetKey(win, GLFW_KEY_SPACE) == GLFW_PRESS)
	{
		cam->dirty = 1;
		cam->pos.y += CAM_SPEED;
	}
	if (glfwGetKey(win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
	{
		cam->dirty = 1;
		cam->pos.y -= CAM_SPEED;
	}
	if (glfwGetKey(win, GLFW_KEY_UP) == GLFW_PRESS)
	{
		cam->dirty = 1;
		cam->pitch += 0.01f;
	}
	if (glfwGetKey(win, GLFW_KEY_DOWN) == GLFW_PRESS)
	{
		cam->dirty = 1;
		cam->pitch -= 0.01f;
	}
	if (glfwGetKey(win, GLFW_KEY_LEFT) == GLFW_PRESS)
	{
		cam->dirty = 1;
		cam->yaw += 0.01f;
	}
	if (glfwGetKey(win, GLFW_KEY_RIGHT) == GLFW_PRESS)
	{
		cam->dirty = 1;
		cam->yaw -= 0.01f;
	}
	if (glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(win, 1);
	if (glfwGetKey(win, GLFW_KEY_Q) == GLFW_PRESS)
	{
		cam->dirty = 1;
		cam->lens_radius -= APERTURE_SPEED;
		if (cam->lens_radius < 0.0f)
			cam->lens_radius = 0.0f;
	}
	if (glfwGetKey(win, GLFW_KEY_E) == GLFW_PRESS)
	{
		cam->dirty = 1;
		cam->lens_radius += APERTURE_SPEED;
	}
	if (glfwGetKey(win, GLFW_KEY_Z) == GLFW_PRESS)
	{
		cam->dirty = 1;
		cam->focal_distance -= FOCAL_DIST_SPEED;
		if (cam->focal_distance < 0.0f)
			cam->focal_distance = 0.0f;
	}
	if (glfwGetKey(win, GLFW_KEY_C) == GLFW_PRESS)
	{
		cam->dirty = 1;
		cam->focal_distance += FOCAL_DIST_SPEED;
	}
}
