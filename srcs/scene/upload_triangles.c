#include "bvh.h"
#include "mesh.h"
#include "scene.h"
#include "triangle.h"
#include "cycles.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static uint32_t	count_triangles_in_meshes(t_scene *scene)
{
	uint32_t	i;
	uint32_t	res;

	i = 0;
	res = 0;
	while (i < scene->mesh_count)
	{
		res += scene->meshes[i].triangle_count;
		i++;
	}
	return (res);
}

static int	rebuild_flat_triangle_array(t_scene *scene)
{
	uint32_t	offset;
	uint32_t	i;
	t_mesh		*curr_mesh;
	t_bvh		*bvh;
	
	free(scene->triangles);
	scene->triangle_count = count_triangles_in_meshes(scene);
	scene->triangles = malloc(sizeof(t_triangle) * scene->triangle_count);
	if (!scene->triangles)
		return (fprintf(stderr, "scene: failed to allocate triangle array\n"), 0);
	offset = 0;
	i = 0;
	while (i < scene->mesh_count)
	{
		curr_mesh = &scene->meshes[i];
		bvh = &scene->bvhs[i];
		if (bvh->indices && bvh->index_count > 0)
		{
			for (uint32_t j = 0; j < bvh->index_count; j++)
				scene->triangles[offset + j] = curr_mesh->triangles[bvh->indices[j]];
		}
		else
			memcpy(scene->triangles + offset, curr_mesh->triangles, sizeof(t_triangle) * curr_mesh->triangle_count);
		scene->descriptors[i].tri_offset = offset;
		scene->descriptors[i].tri_count = curr_mesh->triangle_count;
		scene->descriptors[i].smooth = curr_mesh->smooth;
		scene->descriptors[i].material = curr_mesh->material_index;
		// Precompute rotation columns from mesh direction (mirrors mat_from_dir logic)
		float  dx = curr_mesh->direction.x;
		float  dy = curr_mesh->direction.y;
		float  dz = curr_mesh->direction.z;
		float  len = sqrtf(dx*dx + dy*dy + dz*dz);
		if (len < 1e-6f)
		{
			scene->descriptors[i].rot_col0 = (t_vec4){1,0,0,0};
			scene->descriptors[i].rot_col1 = (t_vec4){0,1,0,0};
			scene->descriptors[i].rot_col2 = (t_vec4){0,0,1,0};
		}
		else
		{
			t_vec4  fwd = {dx/len, dy/len, dz/len, 0};
			t_vec4  world_up = (fabsf(fwd.y) < 0.999f)
				? (t_vec4){0,1,0,0}
				: (t_vec4){1,0,0,0};
			t_vec4  right = vec4_normalize(vec4_cross(world_up, fwd));
			t_vec4  up = vec4_cross(fwd, right);
			scene->descriptors[i].rot_col0 = right;
			scene->descriptors[i].rot_col1 = up;
			scene->descriptors[i].rot_col2 = fwd;
		}
		offset += curr_mesh->triangle_count;
		i++;
	}
	return (1);
}

void	scene_upload_triangles(t_scene *scene)
{
	t_triangle_vertices	*verts;
	t_triangle_normals	*norms;
	t_triangle_texcoords	*texcoords;
	uint32_t			i;

	rebuild_flat_triangle_array(scene);
	verts = malloc(sizeof(t_triangle_vertices) * scene->triangle_count);
	norms = malloc(sizeof(t_triangle_normals) * scene->triangle_count);
	texcoords = malloc(sizeof(t_triangle_texcoords) * scene->triangle_count);
	if (!verts || !norms || !texcoords)
	{
		fprintf(stderr, "scene: failed to allocate vertex/normal/texcoord arrays\n");
		free(verts);
		free(norms);
		free(texcoords);
		return ;
	}
	for (i = 0; i < scene->triangle_count; i++)
	{
		verts[i].v0 = scene->triangles[i].v0;
		verts[i].v1 = scene->triangles[i].v1;
		verts[i].v2 = scene->triangles[i].v2;

		norms[i].n0 = scene->triangles[i].n0;
		norms[i].n1 = scene->triangles[i].n1;
		norms[i].n2 = scene->triangles[i].n2;

		texcoords[i].uv0 = scene->triangles[i].uv0;
		texcoords[i].uv1 = scene->triangles[i].uv1;
		texcoords[i].uv2 = scene->triangles[i].uv2;
	}

	// vertices
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, scene->ssbo_triangles);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(t_triangle_vertices) * scene->triangle_count,
			verts, GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, TRIANGLE_VERTS, scene->ssbo_triangles);

	// normals
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, scene->ssbo_normals);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(t_triangle_normals) * scene->triangle_count,
			norms, GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, TRIANGLE_NORMS, scene->ssbo_normals);

	// texcoords
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, scene->ssbo_texcoords);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(t_triangle_texcoords) * scene->triangle_count,
			texcoords, GL_STATIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, TEXCOORDS_SSBOS, scene->ssbo_texcoords);

	free(verts);
	free(norms);
	free(texcoords);
	scene->gpu_dirty = 0;
	scene_upload_descriptors(scene);
	scene_upload_bvh_nodes(scene);
}
