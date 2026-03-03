#include "mesh.h"
#include <stdlib.h>
#include <math.h>

t_mesh	generate_torus(int stacks, int slices, float major_radius, float minor_radius)
{
	t_mesh	mesh;

	mesh.triangle_count = stacks * slices * 2;
	mesh.triangles = malloc(sizeof(t_triangle) * mesh.triangle_count);
	mesh.position = (t_vec4){0.0f, 0.0f, 0.0f, 0.0f};
	mesh.scale = (t_vec4){1.0f, 1.0f, 1.0f, 0.0f};
	mesh.smooth = 1;
	int index = 0;
	for (int i = 0; i < stacks; i++)
	{
		float theta0 = 2.0f * M_PI * ((float)i / stacks);
		float theta1 = 2.0f * M_PI * ((float)(i + 1) / stacks);
		for (int j = 0; j < slices; j++)
		{
			float phi0 = 2.0f * M_PI * ((float)j / slices);
			float phi1 = 2.0f * M_PI * ((float)(j + 1) / slices);

			// Torus parametric formula:
			// x = (R + r*cos(phi)) * cos(theta)
			// y =  r * sin(phi)
			// z = (R + r*cos(phi)) * sin(theta)
			t_vec4 p0 = {
				(major_radius + minor_radius * cosf(phi0)) * cosf(theta0),
				minor_radius * sinf(phi0),
				(major_radius + minor_radius * cosf(phi0)) * sinf(theta0),
				0.0f
			};
			t_vec4 p1 = {
				(major_radius + minor_radius * cosf(phi0)) * cosf(theta1),
				minor_radius * sinf(phi0),
				(major_radius + minor_radius * cosf(phi0)) * sinf(theta1),
				0.0f
			};
			t_vec4 p2 = {
				(major_radius + minor_radius * cosf(phi1)) * cosf(theta1),
				minor_radius * sinf(phi1),
				(major_radius + minor_radius * cosf(phi1)) * sinf(theta1),
				0.0f
			};
			t_vec4 p3 = {
				(major_radius + minor_radius * cosf(phi1)) * cosf(theta0),
				minor_radius * sinf(phi1),
				(major_radius + minor_radius * cosf(phi1)) * sinf(theta0),
				0.0f
			};

			// Normals point away from the tube center ring (not the origin)
			// Center of the tube at each point lies on the major circle
			t_vec4 c0 = { major_radius * cosf(theta0), 0.0f, major_radius * sinf(theta0), 0.0f };
			t_vec4 c1 = { major_radius * cosf(theta1), 0.0f, major_radius * sinf(theta1), 0.0f };
			t_vec4 n0 = { (p0.x - c0.x) / minor_radius, p0.y / minor_radius, (p0.z - c0.z) / minor_radius, 0.0f };
			t_vec4 n1 = { (p1.x - c1.x) / minor_radius, p1.y / minor_radius, (p1.z - c1.z) / minor_radius, 0.0f };
			t_vec4 n2 = { (p2.x - c1.x) / minor_radius, p2.y / minor_radius, (p2.z - c1.z) / minor_radius, 0.0f };
			t_vec4 n3 = { (p3.x - c0.x) / minor_radius, p3.y / minor_radius, (p3.z - c0.z) / minor_radius, 0.0f };
			mesh.triangles[index++] = (t_triangle){ p0, p1, p2, n0, n1, n2 };
			mesh.triangles[index++] = (t_triangle){ p0, p2, p3, n0, n2, n3 };
		}
	}
	return (mesh);
}
