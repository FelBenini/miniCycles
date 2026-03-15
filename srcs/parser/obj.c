#include "material.h"
#include "mesh.h"
#include "rt_math.h"
#include "scene.h"
#include "parser.h"
#include <stdint.h>
#include <stdio.h>

void	process_obj(t_scene *scene, char *line)
{
	t_mesh		obj;
	char		path[250];
	float		r;
	float		g;
	float		b;
	float		roughness;
	float		metallic;
	float		er = 0;
	float		eg = 0;
	float		eb = 0;
	t_material	material;
	t_vec4		pos;
	t_vec4		dir;
	char		texture_idx[125] = "";
	char		rough_idx[125] = "";
	char		disp_idx[125] = "";
	char		normal_idx[125] = "";
	float		mesh_scale;

	roughness = 0.8f;
	metallic = 0;
	pos = (t_vec4){0.0f, 0.0f, 0.0f, 1.0f};
	dir = (t_vec4){0.0f, 0.0f, 0.0f, 1.0f};
	material.texture_idx = -1;
	material.displacement_tex_idx = -1;
	material.roughness_tex_idx = -1;
	material.normal_tex_idx = -1;
	material.texture_tile_size = 1.0;
	mesh_scale = 1.0f;
	if (sscanf(line, "obj %s %f %f,%f,%f %f,%f,%f %f,%f,%f %f %f %f,%f,%f %s %s %s %s %f",
				path, &mesh_scale,
				&pos.x, &pos.y, &pos.z,
				&dir.x, &dir.y, &dir.z,
				&r, &g, &b,
				&roughness, &metallic,
				&er, &eg, &eb,
				texture_idx, disp_idx, rough_idx, normal_idx, &material.texture_tile_size) < 11)
	{
		printf("Obj invalid format.\n");
		return ;
	}
	material.albedo = (t_vec4){r / 255, g / 255, b / 255, 1.0f};
	material.emission = (t_vec4){er / 255, eg / 255, eb / 255, 0.0f};
	material.roughness = roughness;
	material.metallic = metallic;
	material.texture_idx = get_texture_if_valid(scene, texture_idx);
	material.displacement_tex_idx = get_texture_if_valid(scene, disp_idx);
	material.roughness_tex_idx = get_texture_if_valid(scene, rough_idx);
	material.normal_tex_idx = get_texture_if_valid(scene, normal_idx);
	obj = load_mesh_from_obj(path, mesh_scale);
	obj.position = pos;
	obj.direction = dir;
	uint32_t material_idx = scene_add_material(scene, material);
	scene_add_mesh(scene, obj, material_idx);
}
