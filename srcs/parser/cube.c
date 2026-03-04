#include "material.h"
#include "mesh.h"
#include "rt_math.h"
#include "scene.h"
#include <stdint.h>
#include <stdio.h>

void	process_cube(t_scene *scene, char *line)
{
	t_mesh		cube;
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
	float		diameter;
	float		roughness;
	float		metallic;
	uint32_t	material_idx;

	roughness = 0.6f;
	metallic = 0.0f;
	emission = vec4_create(0, 0, 0, 1);
	matched = sscanf(line, "cb %f,%f,%f %f,%f,%f %f %f,%f,%f %f %f %f,%f,%f",
		&px, &py, &pz,
		&nx, &ny, &nz,
		&diameter,
		&r, &g, &b,
		&roughness, &metallic,
		&emission.x, &emission.y, &emission.z);
	if (matched < 10)
	{
		printf("Error: invalid cube format .\n");
		return ;
	}
	cube = generate_cube(diameter);
	cube.position = vec4_create(px, py + diameter / 2.0f, pz, 1.0f);
	material.albedo    = vec4_create(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
	material.roughness = roughness;
	material.metallic  = metallic;
	emission.x /= 255.0f;
	emission.y /= 255.0f;
	emission.z /= 255.0f;
	material.emission  = emission;
	material.ior       = 1.5f;
	material_idx = scene_add_material(scene, material);
	scene_add_mesh(scene, cube, material_idx);
}
