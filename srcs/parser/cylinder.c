#include "mesh.h"
#include "parser.h"
#include <stdio.h>

void	process_cylinder(t_scene *scene, char *line)
{
	t_mesh		cylinder;
	t_material	material;
	int			matched;
	t_vec4		emission;
	float		px;
	float		py;
	float		pz;
	float		nx;
	float		ny;
	float		nz;
	float		r;
	float		g;
	float		b;
	float		height;
	float		radius;
	float		roughness;
	float		metallic;
	uint32_t	material_idx;
	char		texture_idx[125] = "";
	char		rough_idx[125] = "";
	char		disp_idx[125] = "";

	roughness = 0.6f;
	metallic = 0.0f;
	emission = vec4_create(0, 0, 0, 1);
	material.texture_idx = -1;
	material.displacement_tex_idx = -1;
	material.roughness_tex_idx = -1;
	material.normal_tex_idx = -1;
	material.texture_tile_size = 1.0;
	matched = sscanf(line, "cy %f,%f,%f %f,%f,%f %f,%f %f,%f,%f %f %f %f,%f,%f %s %s %s %f",
		&px, &py, &pz,
		&nx, &ny, &nz,
		&radius, &height,
		&r, &g, &b,
		&roughness, &metallic,
		&emission.x, &emission.y, &emission.z,
		texture_idx, disp_idx, rough_idx, &material.texture_tile_size);
	if (matched < 11)
	{
		printf("Error: invalid cylinder format .\n");
		return ;
	}
	cylinder = generate_cylinder(32, 32, radius, height);
	cylinder.position = vec4_create(px, py + (height / 2), pz, 1.0f);
	cylinder.direction = vec4_create(nx, ny, nz, 1.0f);
	material.albedo    = vec4_create(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
	material.roughness = roughness;
	material.metallic  = metallic;
	material.texture_idx = get_texture_if_valid(scene, texture_idx);
	material.roughness_tex_idx = get_texture_if_valid(scene, rough_idx);
	material.displacement_tex_idx = get_texture_if_valid(scene, disp_idx);
	emission.x /= 255.0f;
	emission.y /= 255.0f;
	emission.z /= 255.0f;
	material.emission  = emission;
	material.ior       = 1.5f;
	material_idx = scene_add_material(scene, material);
	scene_add_mesh(scene, cylinder, material_idx);
}
