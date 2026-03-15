#ifndef MATERIAL_H
# define MATERIAL_H

# include "rt_math.h"
# include <stdint.h>

typedef struct s_material
{
	t_vec4		albedo;
	t_vec4		emission;

	float		roughness;
	float		metallic;
	float		ior; // index of refraction
	uint32_t	type;
	int			texture_idx;
	int			displacement_tex_idx;
	int			roughness_tex_idx;
	int			normal_tex_idx;
	float		texture_tile_size;
	float		pad[3];
}	t_material;

#endif
