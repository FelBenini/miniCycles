#include "rt_math.h"
#include "scene.h"
#include <stdio.h>

void	process_ambient(t_scene *scene, char *line)
{
	t_vec4	color;
	float	intensity;
	char	path[256];

	intensity = 1.0f;
	if (sscanf(line, "A %f %255s", &intensity, path) == 2 && path[0] != '-')
	{
		scene->sky_tex = scene_load_image(scene, path);
		scene->sky_intensity = intensity;
		if (scene->sky_tex != -1)
			return ;
	}
	color = vec4_create(125.0f / 255.0f, 125.0f / 255.0f, 125.0f / 255.0f, 1.0f);
	intensity = 1.0f;
	if (sscanf(line, "A %f %f,%f,%f", &intensity,
			&color.x, &color.y, &color.z) != 4)
	{
		printf("Error: Invalid ambient format. Expected 'A <intensity> <r,g,b>' or 'A <path>'\n");
		return ;
	}
	intensity = fmaxf(0.0f, fminf(1.0f, intensity));
	color.x = (color.x / 255.0f) * intensity;
	color.y = (color.y / 255.0f) * intensity;
	color.z = (color.z / 255.0f) * intensity;
	scene->ambient = color;
}
