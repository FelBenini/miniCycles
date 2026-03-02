#include "mesh.h"
#include "bvh.h"
#include "rt_math.h"
#include "scene.h"
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

static float	compute_bounding_radius(t_mesh *mesh)
{
	float		max_r2;
	t_triangle	*tri;
	float		r2;

	max_r2 = 0.0f;
	for (uint32_t i = 0; i < mesh->triangle_count; i++)
	{
		tri = &mesh->triangles[i];
		t_vec4 verts[3] = {tri->v0, tri->v1, tri->v2};
		for (int v = 0; v < 3; v++)
		{
			r2 = verts[v].x * verts[v].x + verts[v].y * verts[v].y + verts[v].z
				* verts[v].z;
			if (r2 > max_r2)
				max_r2 = r2;
		}
	}
	return (sqrtf(max_r2));
}

uint32_t	scene_add_material(t_scene *scene, t_material material)
{
	uint32_t	index;

	if (scene->material_count == scene->material_capacity)
	{
		scene->material_capacity *= 2;
		scene->materials = realloc(scene->materials, sizeof(t_material) * scene->material_capacity);
	}
	index = scene->material_count++;
	scene->materials[index] = material;
	scene->material_dirty = 1;
	return (index);
}

uint32_t	scene_add_mesh(t_scene *scene, t_mesh mesh, uint32_t material_index)
{
	uint32_t	index;

	if (scene->mesh_count == scene->mesh_capacity)
	{
		scene->mesh_capacity *= 2;
		scene->meshes = realloc(scene->meshes, sizeof(t_mesh) * scene->mesh_capacity);
		scene->descriptors = realloc(scene->descriptors, sizeof(t_mesh_descriptor) * scene->mesh_capacity);
		scene->bvhs = realloc(scene->bvhs, sizeof(t_bvh) * scene->mesh_capacity);
	}
	index = scene->mesh_count++;
	mesh.material_index = material_index;
	scene->meshes[index] = mesh;
	scene->descriptors[index] = (t_mesh_descriptor){.position = mesh.position,
		.tri_offset = 0, .tri_count = mesh.triangle_count, .bvh_root = 0,
		.smooth = mesh.smooth
	};
	scene->descriptors[index].position.w = compute_bounding_radius(&mesh);
	scene->bvhs[index] = bvh_build(mesh.triangles, mesh.triangle_count);
	scene->gpu_dirty = 1;
	scene->desc_dirty = 1;
	scene->bvh_dirty = 1;
	scene->tlas_dirty = 1;
	return (index);
}

void	scene_move_mesh(t_scene *scene, uint32_t index, t_vec4 position)
{
	t_mesh	*mesh;

	if (index >= scene->mesh_count)
		return ;
	position.w = scene->descriptors[index].position.w;
	mesh = &scene->meshes[index];
	mesh->position = position;
	scene->descriptors[index].position = position;
	scene->desc_dirty = 1;
	scene->tlas_dirty = 1;
}
