#include "rt_math.h"
#include "scene.h"
#include <stdlib.h>

t_scene	scene_create(uint32_t initial_capacity)
{
	t_scene	scene = {0};

	scene.mesh_capacity = initial_capacity ? initial_capacity : 8;
	scene.material_capacity = initial_capacity ? initial_capacity : 8;
	scene.material_count = 0;
	scene.ambient = vec4_create(0.45, 0.745, 0.94, 1.0);
	scene.meshes = malloc(sizeof(t_mesh) * scene.mesh_capacity);
	scene.descriptors = malloc(sizeof(t_mesh_descriptor) * scene.mesh_capacity);
	scene.materials = malloc(sizeof(t_material) * scene.material_capacity);
	scene.bvhs = malloc(sizeof(t_bvh) * scene.mesh_capacity);
	glGenBuffers(1, &scene.ssbo_triangles);
	glGenBuffers(1, &scene.ssbo_normals);
	glGenBuffers(1, &scene.ssbo_meshes);
	glGenBuffers(1, &scene.ssbo_bvh_nodes);
	glGenBuffers(1, &scene.ssbo_tlas_nodes);
	glGenBuffers(1, &scene.ssbo_materials);
	scene.tlas_dirty = 1;
	return (scene);
}
