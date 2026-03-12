#include "rt_math.h"
#include "scene.h"
#include <stdio.h>

void	process_light(t_scene *scene, char *line)
{
	float	intensity;
	t_vec3	color;
	char	type[10];
	t_light	light;

	if (sscanf(line, "L %f %f,%f,%f %s",
				&intensity,
				&color.x, &color.y, &color.z,
				type) < 4)
	{
		printf("Error: Invalid light format.\n");
		return ;
	}
	color.x /= 255 * intensity;
	color.y /= 255 * intensity;
	color.z /= 255 * intensity;
	light.color = color;
	light.type = 0;
	scene_add_light(scene, light);
}
