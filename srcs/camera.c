#include "camera.h"
#include "math.h"

t_camera	camera_create(float x, float y, float z, float fov_deg)
{
	t_camera	cam;

	cam.pos = (t_vec4){x, y, z, 0.0f};
	cam.yaw = 0.0f;
	cam.pitch = 0.0f;
	cam.fov = fov_deg * (3.14159265f / 180.0f);
	cam.lens_radius = 0.0f;
	cam.focal_distance = 0.0f;
	cam.is_active = 0;
	cam.dirty = 0;
	return (cam);
}

void	camera_update_basis(t_camera *cam)
{
	float	cp = cosf(cam->pitch);
	float	sp = sinf(cam->pitch);
	float	cy = cosf(cam->yaw);
	float	sy = sinf(cam->yaw);

	cam->forward = (t_vec4){ cy * cp,  sp, -sy * cp, 0.0f };
	cam->right = (t_vec4){ sy, 0.0f, cy, 0.0f };
	cam->up = (t_vec4){
		cam->right.y * cam->forward.z - cam->right.z * cam->forward.y,
		-cam->right.x * cam->forward.z + cam->right.z * cam->forward.x,
		cam->right.x * cam->forward.y - cam->right.y * cam->forward.x,
		0.0f
	};
}


