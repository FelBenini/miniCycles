#include "mesh.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

void	free_temp_buffers(FILE *file, t_obj_buffers *tmp)
{
	fclose(file);
	free(tmp->verts);
	free(tmp->norms);
	free(tmp->uvs);
}

FILE	*open_obj_file(const char *filepath, FILE **file)
{
	*file = fopen(filepath, "r");
	if (!*file)
		return (NULL);
	return (*file);
}

void	scan_obj_counts(t_obj_counts *counts, FILE *file)
{
	char	line[256];

	counts->vert_count = 0;
	counts->norm_count = 0;
	counts->uv_count = 0;
	counts->face_count = 0;
	while (fgets(line, sizeof(line), file))
	{
		if (line[0] == 'v' && line[1] == ' ')
			counts->vert_count++;
		else if (line[0] == 'v' && line[1] == 'n')
			counts->norm_count++;
		else if (line[0] == 'v' && line[1] == 't')
			counts->uv_count++;
		else if (line[0] == 'f' && line[1] == ' ')
		{
			int		verts_in_face = 0;
			char	*ptr = line + 2;
			char	*prev;
			while (*ptr)
			{
				while (*ptr == ' ' || *ptr == '\t') ptr++;
				if (!*ptr || *ptr == '\n' || *ptr == '\r')
					break;
				prev = ptr;
				while (*ptr && *ptr != ' ' && *ptr != '\t' && *ptr != '\n')
					ptr++;
				if (ptr == prev)
					break; // no progress, bail
				verts_in_face++;
			}
			if (verts_in_face >= 3)
				counts->face_count += verts_in_face - 2;
		}
	}
}

int	allocate_mesh_buffers(
	t_mesh *mesh,
	t_obj_buffers *tmp,
	t_obj_counts counts,
	float radius)
{
	tmp->verts = malloc(sizeof(t_vec4) * counts.vert_count);
	tmp->norms = malloc(sizeof(t_vec4) * counts.norm_count);
	tmp->uvs = malloc(sizeof(t_vec4) * counts.uv_count);
	mesh->triangles = malloc(sizeof(t_triangle) * counts.face_count);

	if (!tmp->verts || !tmp->norms || !tmp->uvs || !mesh->triangles)
	{
		free(tmp->verts);
		free(tmp->norms);
		free(tmp->uvs);
		free(mesh->triangles);
		return (0);
	}

	mesh->triangle_count = counts.face_count;
	mesh->position = (t_vec4){0.0f, 0.0f, 0.0f, radius};
	mesh->scale = (t_vec4){1.0f, 1.0f, 1.0f, 0.0f};
	mesh->smooth = 1;

	return (1);
}

static void	parse_vertex_line(char *line, t_vec4 *verts, int *vi)
{
	float	x, y, z;

	sscanf(line + 2, "%f %f %f", &x, &y, &z);
	verts[(*vi)++] = (t_vec4){x, y, z, 0.0f};
}

static void	parse_normal_line(char *line, t_vec4 *norms, int *ni)
{
	float	x, y, z;

	sscanf(line + 3, "%f %f %f", &x, &y, &z);
	norms[(*ni)++] = (t_vec4){x, y, z, 0.0f};
}

static void	parse_uv_line(char *line, t_vec4 *uvs, int *ui)
{
	float	u, v;

	sscanf(line + 3, "%f %f", &u, &v);
	uvs[(*ui)++] = (t_vec4){u, v, 0.0f, 0.0f};
}

static int	parse_face_tokens(char *ptr, int *face_v, int *face_vn, int *face_vt)
{
	int	count = 0;
	int	v, vn, vt, matched;

	while (*ptr && count < 64)
	{
		while (*ptr == ' ' || *ptr == '\t') ptr++;
		if (!*ptr || *ptr == '\n' || *ptr == '\r') break;
		v = 0; vn = 0; vt = 0; matched = 0;
		if (sscanf(ptr, "%d/%d/%d", &v, &vt, &vn) == 3)
			matched = 1;
		else if (sscanf(ptr, "%d//%d", &v, &vn) == 2)
			matched = 1;
		else if (sscanf(ptr, "%d/%d", &v, &vt) == 2)
			matched = 1;
		else if (sscanf(ptr, "%d", &v) == 1)
			matched = 1;
		if (!matched)
			break;
		face_v[count] = v;
		face_vn[count] = vn;
		face_vt[count] = vt;
		count++;
		while (*ptr && *ptr != ' ' && *ptr != '\t' && *ptr != '\n')
			ptr++;
	}
	return (count);
}

static void	triangulate_face(
	t_mesh *mesh,
	t_obj_buffers *tmp,
	t_obj_counts counts,
	int *face_v,
	int *face_vn,
	int *face_vt,
	int face_vert_count,
	float radius,
	int *index)
{
	for (int f = 1; f + 1 < face_vert_count; f++)
	{
		int		idx[3] = { 0, f, f + 1 };
		t_vec4	p[3], n[3], uv[3];

		for (int k = 0; k < 3; k++)
		{
			p[k] = tmp->verts[face_v[idx[k]] - 1];
			p[k].x *= radius;
			p[k].y *= radius;
			p[k].z *= radius;
		}

		if (counts.norm_count > 0 && face_vn[0] > 0)
		{
			for (int k = 0; k < 3; k++)
			{
				n[k] = tmp->norms[face_vn[idx[k]] - 1];
				n[k].x *= radius;
				n[k].y *= radius;
				n[k].z *= radius;
			}
		}
		else
		{
			t_vec4 e1 = {p[1].x-p[0].x, p[1].y-p[0].y, p[1].z-p[0].z, 0.0f};
			t_vec4 e2 = {p[2].x-p[0].x, p[2].y-p[0].y, p[2].z-p[0].z, 0.0f};
			t_vec4 fn = {
				e1.y*e2.z - e1.z*e2.y,
				e1.z*e2.x - e1.x*e2.z,
				e1.x*e2.y - e1.y*e2.x,
				0.0f
			};
			float len = sqrtf(fn.x*fn.x + fn.y*fn.y + fn.z*fn.z);
			fn.x /= len; fn.y /= len; fn.z /= len;
			n[0] = n[1] = n[2] = fn;
		}

		if (counts.uv_count > 0 && face_vt[0] > 0)
		{
			for (int k = 0; k < 3; k++)
				uv[k] = tmp->uvs[face_vt[idx[k]] - 1];
		}
		else
			uv[0] = uv[1] = uv[2] = (t_vec4){0.0f, 0.0f, 0.0f, 0.0f};

		mesh->triangles[(*index)++] = (t_triangle){p[0], p[1], p[2], n[0], n[1], n[2], uv[0], uv[1], uv[2]};
	}
}

static void	parse_face_line(
	char *line,
	t_mesh *mesh,
	t_obj_buffers *tmp,
	t_obj_counts counts,
	float radius,
	int *index)
{
	int	face_v[64];
	int	face_vn[64];
	int	face_vt[64];
	int	face_vert_count;

	face_vert_count = parse_face_tokens(line + 2, face_v, face_vn, face_vt);
	if (face_vert_count >= 3)
		triangulate_face(mesh, tmp, counts, face_v, face_vn, face_vt, face_vert_count, radius, index);
}

static void	fill_mesh_buffers(
	FILE *file,
	t_mesh *mesh,
	t_obj_buffers *tmp,
	t_obj_counts counts,
	float radius)
{
	char	line[256];
	int		vi = 0, ni = 0, ui = 0, index = 0;

	while (fgets(line, sizeof(line), file))
	{
		if (line[0] == 'v' && line[1] == ' ')
			parse_vertex_line(line, tmp->verts, &vi);
		else if (line[0] == 'v' && line[1] == 'n')
			parse_normal_line(line, tmp->norms, &ni);
		else if (line[0] == 'v' && line[1] == 't')
			parse_uv_line(line, tmp->uvs, &ui);
		else if (line[0] == 'f' && line[1] == ' ')
			parse_face_line(line, mesh, tmp, counts, radius, &index);
	}
}

t_mesh	load_mesh_from_obj(const char *filepath, float radius)
{
	t_mesh			mesh;
	FILE			*file;
	t_obj_counts	counts;
	t_obj_buffers	tmp;

	if (!open_obj_file(filepath, &file))
	{
		mesh.triangle_count = 0;
		mesh.triangles = NULL;
		return (mesh);
	}
	scan_obj_counts(&counts, file);

	if (!allocate_mesh_buffers(&mesh, &tmp, counts, radius))
		return (mesh);

	rewind(file);
	fill_mesh_buffers(file, &mesh, &tmp, counts, radius);
	free_temp_buffers(file, &tmp);
	printf("Obj loaded: %s\n", filepath);
	return (mesh);
}
