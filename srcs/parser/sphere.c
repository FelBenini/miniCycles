#include "rt_math.h"
#include "scene.h"
#include "parser.h"
#include <stdio.h>

void	process_sphere(t_scene *scene, char *line)
{
	t_mesh		sphere;
	t_material	material;
	uint32_t	material_idx;
	float		px, py, pz;
	float		diameter;
	float		r, g, b;
	int			matched;
	float		roughness = 0.7f;
	float		metallic = 0.0f;
	t_vec4		emission;
	char		texture_idx[125] = "";
	char		rough_idx[125] = "";
	char		disp_idx[125] = "";
	char		normal_idx[125] = "";

	emission = vec4_create(0, 0, 0, 1);
	material.texture_idx = -1;
	material.displacement_tex_idx = -1;
	material.roughness_tex_idx = -1;
	material.normal_tex_idx = -1;
	material.texture_tile_size = 1.0;
	matched = sscanf(line, "sp %f,%f,%f %f %f,%f,%f %f %f %f,%f,%f %s %s %s %s %f",
		&px, &py, &pz,
		&diameter,
		&r, &g, &b,
		&roughness, &metallic,
		&emission.x, &emission.y, &emission.z,
		texture_idx, disp_idx, rough_idx, normal_idx, &material.texture_tile_size);
	if (matched < 7)
	{
		printf("Error: invalid sphere format.\n");
		return ;
	}
	sphere = generate_uv_sphere(40, 40, diameter / 2.0f);
	sphere.position = vec4_create(px, py + diameter / 2.0f, pz, 1.0f);
	sphere.direction = vec4_create(0.0f, 0.0f, 0.0f, 1.0f);
	material.albedo    = vec4_create(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
	material.roughness = roughness;
	material.metallic  = metallic;
	material.texture_idx = get_texture_if_valid(scene, texture_idx);
	material.roughness_tex_idx = get_texture_if_valid(scene, rough_idx);
	material.displacement_tex_idx = get_texture_if_valid(scene, disp_idx);
	material.normal_tex_idx = get_texture_if_valid(scene, normal_idx);
	emission.x /= 255.0f;
	emission.y /= 255.0f;
	emission.z /= 255.0f;
	material.emission  = emission;
	material.ior       = 1.5f;
	material_idx = scene_add_material(scene, material);
	scene_add_mesh(scene, sphere, material_idx);
}
