#ifndef INPUT_H
# define INPUT_H
# include "camera.h"

void	mouse_callback(GLFWwindow *win, double x, double y);
void	handle_input(GLFWwindow *win, t_camera *cam);
void	scroll_callback(GLFWwindow *win, double xoffset, double yoffset);

#endif
