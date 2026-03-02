#ifndef CYCLES_H
# define CYCLES_H

#include "../glad/include/glad/glad.h"
#include <GLFW/glfw3.h>

# define TRIANGLE_VERTS 1
# define TRIANGLE_NORMS 2
# define MESHES_SSBOS 3
# define BVH_SSBOS 4
# define TLAS_SSBOS 5
# define MATERIALS_SSBOS 6

typedef struct s_cycles
{
	GLFWwindow	*win;
	GLuint		compute_program;
	GLuint		fullscreen_program;
	GLuint		tex;
	GLuint		vao;
}	t_cycles;

t_cycles	init_cycles(void);

#endif
