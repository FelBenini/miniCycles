#include "scene.h"
#include <string.h>
#include <stdio.h>

void	process_camera(t_scene *scene, char *line)
{
	t_camera	camera;
	t_vec4		world_up;
	float		fov_deg;

	memset(&camera, 0, sizeof(t_camera));
	if (sscanf(line, "C %f,%f,%f %f,%f,%f %f",
		&camera.pos.x, &camera.pos.y, &camera.pos.z,
		&camera.forward.x, &camera.forward.y, &camera.forward.z,
		&fov_deg) != 7)
	{
		printf("Error: invalid camera format.\n");
		return ;
	}
	if (fov_deg <= 0 || fov_deg >= 180)
	{
		printf("Error: FOV must be between 0 and 180.\n");
		return ;
	}
	camera.fov = fov_deg * (3.14159265f / 180.0f);
	camera.forward = vec4_normalize(camera.forward);
	camera.yaw   = atan2f(camera.forward.x, camera.forward.z) - 1.55f;
	camera.pitch = asinf(camera.forward.y) + 0.15f;
	world_up = vec4_create(0, 1, 0, 0);
	camera.right = vec4_normalize(vec4_cross(camera.forward, world_up));
	camera.up    = vec4_normalize(vec4_cross(camera.right, camera.forward));
	camera.is_active = 1;
	camera.dirty     = 1;
	scene->camera    = camera;
}

