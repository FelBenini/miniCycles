#ifndef MESH_H
# define MESH_H

# include <stdint.h>
# include "rt_math.h"
# include "triangle.h"

typedef struct s_mesh {
	t_triangle	*triangles;
	uint32_t	triangle_count;
	t_vec4		position;
	t_vec4		scale;
	uint32_t	smooth;
	uint32_t	material_index;
}	t_mesh;

t_mesh generate_uv_sphere(int stacks, int slices, float radius);

t_mesh generate_plane(float x_size, float y_size);

t_mesh	generate_cube(float size);

t_mesh generate_cone(int stacks, int slices, float radius, float height);

t_mesh	generate_cylinder(int stacks, int slices, float radius, float height);

#endif
