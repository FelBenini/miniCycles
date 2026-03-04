#include "rt_math.h"
#include "scene.h"
#include <stdio.h>

void	process_ambient(t_scene *scene, char *line)
{
	t_vec4	color;
	float	intensity;

	color = vec4_create(125.0f, 125.0f, 125.0f, 1.0f);
	intensity = 1;
	if (sscanf(line, "A %f %f,%f,%f", &intensity,
			&color.x, &color.y, &color.z) != 4)
	{
		printf("Error: Invalid ambient format.\n");
		return ;
	}
	if (intensity > 1.0)
		intensity = 1.0;
	if (intensity < 0.0)
		intensity = 0.0;
	color.x /= 255;
	color.y /= 255;
	color.z /= 255;
	color.x *= intensity;
	color.y *= intensity;
	color.z *= intensity;
	scene->ambient = color;
}
