#ifndef MESH_H
# define MESH_H

# include <stdint.h>
# include "rt_math.h"
# include "triangle.h"

typedef struct s_mesh {
	t_triangle	*triangles;
	uint32_t	triangle_count;
	t_vec4		position;
	t_vec4		direction;
	t_vec4		scale;
	uint32_t	smooth;
	uint32_t	material_index;
}	t_mesh;

typedef struct s_obj_counts
{
	int	vert_count;
	int	norm_count;
	int	uv_count;
	int	triangle_count;
	int	face_count;
}	t_obj_counts;

typedef struct s_obj_buffers
{
	t_vec4	*verts;
	t_vec4	*norms;
	t_vec4	*uvs;
}	t_obj_buffers;

t_mesh	generate_uv_sphere(int stacks, int slices, float radius);

t_mesh	generate_plane(float x_size, float y_size);

t_mesh	generate_cube(float size);

t_mesh generate_cone(int stacks, int slices, float radius, float height);

t_mesh	generate_cylinder(int stacks, int slices, float radius, float height);

t_mesh	generate_torus(int stacks, int slices, float major_radius, float minor_radius);

t_mesh	load_mesh_from_obj(const char *filepath, float radius);

#endif
