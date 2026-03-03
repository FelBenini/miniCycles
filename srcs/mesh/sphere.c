#include <math.h>
#include <stdlib.h>
#include "triangle.h"
#include <math.h>
#include <stdlib.h>
#include "mesh.h"

t_mesh	generate_uv_sphere(int stacks, int slices, float radius)
{
	t_mesh mesh;

	mesh.triangle_count = stacks * slices * 2;
	mesh.triangles = malloc(sizeof(t_triangle) * mesh.triangle_count);

	mesh.position = (t_vec4){0.0f, 0.0f, 0.0f, radius};
	mesh.scale = (t_vec4){1.0f, 1.0f, 1.0f, 0.0f};
	mesh.smooth = 1;

	int index = 0;

	for (int i = 0; i < stacks; i++)
	{
		float phi0 = M_PI * ((float)i / stacks);
		float phi1 = M_PI * ((float)(i + 1) / stacks);
		for (int j = 0; j < slices; j++)
		{
			float theta0 = 2.0f * M_PI * ((float)j / slices);
			float theta1 = 2.0f * M_PI * ((float)(j + 1) / slices);
			t_vec4 p0 = {
				radius * sinf(phi0) * cosf(theta0),
				radius * cosf(phi0),
				radius * sinf(phi0) * sinf(theta0),
				0.0f
			};
			t_vec4 p1 = {
				radius * sinf(phi1) * cosf(theta0),
				radius * cosf(phi1),
				radius * sinf(phi1) * sinf(theta0),
				0.0f
			};
			t_vec4 p2 = {
				radius * sinf(phi1) * cosf(theta1),
				radius * cosf(phi1),
				radius * sinf(phi1) * sinf(theta1),
				0.0f
			};
			t_vec4 p3 = {
				radius * sinf(phi0) * cosf(theta1),
				radius * cosf(phi0),
				radius * sinf(phi0) * sinf(theta1),
				0.0f
			};
            t_vec4 n0 = { p0.x / radius, p0.y / radius, p0.z / radius, 0.0f };
            t_vec4 n1 = { p1.x / radius, p1.y / radius, p1.z / radius, 0.0f };
            t_vec4 n2 = { p2.x / radius, p2.y / radius, p2.z / radius, 0.0f };
            t_vec4 n3 = { p3.x / radius, p3.y / radius, p3.z / radius, 0.0f };
			mesh.triangles[index++] = (t_triangle){ p0, p1, p2, n0, n1, n2 };
			mesh.triangles[index++] = (t_triangle){ p0, p2, p3, n0, n2, n3 };
		}
	}
	return (mesh);
}
