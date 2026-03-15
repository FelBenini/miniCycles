#include "rt_math.h"
#include "scene.h"
#include "parser.h"
#include <stdio.h>

void	process_plane(t_scene *scene, char *line)
{
	t_mesh		plane;
	float		px, py, pz;
	float		nx, ny, nz;
	float		r, g, b;
	t_material	material;
	uint32_t	material_idx;
	float		roughness;
	float		metallic;
	t_vec4		emission;
	float		sizey;
	float		sizex;
	char		texture_idx[125] = "";
	char		disp_idx[125] = "";
	char		rough_idx[125] = "";

	roughness = 0.6f;
	metallic = 0.0f;
	emission = vec4_create(0, 0, 0, 1.0f);
	sizex = 1000;
	sizey = 1000;
	material.texture_idx = -1;
	material.displacement_tex_idx = -1;
	material.roughness_tex_idx = -1;
	material.normal_tex_idx = -1;
	material.texture_tile_size = 1.0;
	if (sscanf(line, "pl %f,%f,%f %f,%f,%f %f,%f,%f %f,%f %f %f %f,%f,%f %s %s %s %f",
		&px, &py, &pz,
		&nx, &ny, &nz,
		&r, &g, &b,
		&sizex, &sizey,
		&roughness, &metallic,
		&emission.x, &emission.y, &emission.z,
		texture_idx, disp_idx, rough_idx, &material.texture_tile_size) < 9)
	{
		printf("Error: invalid plane format.\n");
		return ;
	}
	emission.x /= 256.0f;
	emission.y /= 256.0f;
	emission.z /= 256.0f;
	plane = generate_plane(sizex, sizey);
	plane.direction = vec4_create(nx, ny, nz, 1.0f);
	plane.position = vec4_create(px, py, pz, 1.0f);
	material.albedo    = vec4_create(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
	material.roughness = roughness;
	material.metallic  = 0.0f;
	material.texture_idx = get_texture_if_valid(scene, texture_idx);
	material.roughness_tex_idx = get_texture_if_valid(scene, rough_idx);
	material.displacement_tex_idx = get_texture_if_valid(scene, disp_idx);
	material.emission  = emission;
	material.ior       = 0;
	material_idx = scene_add_material(scene, material);
	scene_add_mesh(scene, plane, material_idx);
}
