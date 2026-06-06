#include "bvh.h"
#include "scene.h"
#include <stdint.h>
#include <stdlib.h>

static void	compute_mesh_bbox(t_mesh_descriptor *d,
								t_vec4 *out_min, t_vec4 *out_max)
{
	t_vec4	mn = d->local_bbox_min;
	t_vec4	mx = d->local_bbox_max;
	float	cx[8] = {mn.x, mx.x, mn.x, mx.x, mn.x, mx.x, mn.x, mx.x};
	float	cy[8] = {mn.y, mn.y, mx.y, mx.y, mn.y, mn.y, mx.y, mx.y};
	float	cz[8] = {mn.z, mn.z, mn.z, mn.z, mx.z, mx.z, mx.z, mx.z};
	t_vec4	r0 = d->rot_col0;
	t_vec4	r1 = d->rot_col1;
	t_vec4	r2 = d->rot_col2;
	float	px = d->position.x;
	float	py = d->position.y;
	float	pz = d->position.z;
	float	wx, wy, wz;
	int		i;

	wx = r0.x*cx[0] + r1.x*cy[0] + r2.x*cz[0] + px;
	wy = r0.y*cx[0] + r1.y*cy[0] + r2.y*cz[0] + py;
	wz = r0.z*cx[0] + r1.z*cy[0] + r2.z*cz[0] + pz;
	out_min->x = wx; out_max->x = wx;
	out_min->y = wy; out_max->y = wy;
	out_min->z = wz; out_max->z = wz;
	out_min->w = 0.0f; out_max->w = 0.0f;

	i = 1;
	while (i < 8)
	{
		wx = r0.x*cx[i] + r1.x*cy[i] + r2.x*cz[i] + px;
		wy = r0.y*cx[i] + r1.y*cy[i] + r2.y*cz[i] + py;
		wz = r0.z*cx[i] + r1.z*cy[i] + r2.z*cz[i] + pz;
		if (wx < out_min->x) out_min->x = wx;
		if (wy < out_min->y) out_min->y = wy;
		if (wz < out_min->z) out_min->z = wz;
		if (wx > out_max->x) out_max->x = wx;
		if (wy > out_max->y) out_max->y = wy;
		if (wz > out_max->z) out_max->z = wz;
		i++;
	}
}

static void	compute_meshes_bbox(t_mesh_descriptor *descriptors, uint32_t *indices,
								uint32_t start, uint32_t count,
								t_vec4 *out_min, t_vec4 *out_max)
{
	uint32_t	i;
	t_vec4		mesh_min;
	t_vec4		mesh_max;

	if (count == 0)
		return ;

	compute_mesh_bbox(&descriptors[indices[start]], out_min, out_max);
	i = start + 1;
	while (i < start + count)
	{
		compute_mesh_bbox(&descriptors[indices[i]], &mesh_min, &mesh_max);

		if (mesh_min.x < out_min->x) out_min->x = mesh_min.x;
		if (mesh_min.y < out_min->y) out_min->y = mesh_min.y;
		if (mesh_min.z < out_min->z) out_min->z = mesh_min.z;

		if (mesh_max.x > out_max->x) out_max->x = mesh_max.x;
		if (mesh_max.y > out_max->y) out_max->y = mesh_max.y;
		if (mesh_max.z > out_max->z) out_max->z = mesh_max.z;
		i++;
	}
}

static float	sah_surface_area(t_vec4 mn, t_vec4 mx)
{
	float	ex;
	float	ey;
	float	ez;

	ex = mx.x - mn.x;
	ey = mx.y - mn.y;
	ez = mx.z - mn.z;
	return (2.0f * (ex * ey + ey * ez + ez * ex));
}

static uint32_t	find_split_index(t_mesh_descriptor *descriptors, uint32_t *indices,
								uint32_t start, uint32_t count,
								t_vec4 bbox_min, t_vec4 bbox_max)
{
	float		best_cost;
	uint32_t	best_split;
	uint32_t	best_axis;
	uint32_t	axis;
	uint32_t	i;
	float		parent_sa;
	float		cost;
	t_vec4		lmin, lmax, rmin, rmax;
	t_vec4		mesh_min, mesh_max;
	t_vec4  *prefix_min = malloc(count * sizeof(t_vec4));
	t_vec4  *prefix_max = malloc(count * sizeof(t_vec4));

	if (!prefix_min || !prefix_max)
	{
    	free(prefix_min);
	    free(prefix_max);
	    return (start + 1);
	}
	parent_sa  = sah_surface_area(bbox_min, bbox_max);
	best_cost  = 1e30f;
	best_split = start + 1;
	best_axis  = 0;

	axis = 0;
	while (axis < 3)
	{
		uint32_t j = start + 1;
		while (j < start + count)
		{
			uint32_t key = indices[j];
			float key_c = (&descriptors[key].position.x)[axis];
			int32_t k = (int32_t)j - 1;
			while (k >= (int32_t)start &&
				   (&descriptors[indices[k]].position.x)[axis] > key_c)
			{
				indices[k + 1] = indices[k];
				k--;
			}
			indices[k + 1] = key;
			j++;
		}
		compute_mesh_bbox(&descriptors[indices[start]], &lmin, &lmax);
		prefix_min[0] = lmin;
		prefix_max[0] = lmax;
		i = 1;
		while (i < count)
		{
			compute_mesh_bbox(&descriptors[indices[start + i]], &mesh_min, &mesh_max);
			prefix_min[i].x = mesh_min.x < prefix_min[i-1].x ? mesh_min.x : prefix_min[i-1].x;
			prefix_min[i].y = mesh_min.y < prefix_min[i-1].y ? mesh_min.y : prefix_min[i-1].y;
			prefix_min[i].z = mesh_min.z < prefix_min[i-1].z ? mesh_min.z : prefix_min[i-1].z;
			prefix_min[i].w = 0.0f;
			prefix_max[i].x = mesh_max.x > prefix_max[i-1].x ? mesh_max.x : prefix_max[i-1].x;
			prefix_max[i].y = mesh_max.y > prefix_max[i-1].y ? mesh_max.y : prefix_max[i-1].y;
			prefix_max[i].z = mesh_max.z > prefix_max[i-1].z ? mesh_max.z : prefix_max[i-1].z;
			prefix_max[i].w = 0.0f;
			i++;
		}
		compute_mesh_bbox(&descriptors[indices[start + count - 1]], &rmin, &rmax);
		i = count - 1;
		while (i > 0)
		{
			float sa_left  = sah_surface_area(prefix_min[i - 1], prefix_max[i - 1]);
			float sa_right = sah_surface_area(rmin, rmax);
			cost = (sa_left * (float)i + sa_right * (float)(count - i)) / parent_sa;
			if (cost < best_cost)
			{
				best_cost  = cost;
				best_split = start + i;
				best_axis  = axis;
			}
			compute_mesh_bbox(&descriptors[indices[start + i - 1]], &mesh_min, &mesh_max);
			if (mesh_min.x < rmin.x) rmin.x = mesh_min.x;
			if (mesh_min.y < rmin.y) rmin.y = mesh_min.y;
			if (mesh_min.z < rmin.z) rmin.z = mesh_min.z;
			if (mesh_max.x > rmax.x) rmax.x = mesh_max.x;
			if (mesh_max.y > rmax.y) rmax.y = mesh_max.y;
			if (mesh_max.z > rmax.z) rmax.z = mesh_max.z;
			i--;
		}
		axis++;
	}
	uint32_t j = start + 1;
	while (j < start + count)
	{
		uint32_t key = indices[j];
		float key_c = (&descriptors[key].position.x)[best_axis];
		int32_t k = (int32_t)j - 1;
		while (k >= (int32_t)start && (&descriptors[indices[k]].position.x)[best_axis] > key_c)
		{
			indices[k + 1] = indices[k];
			k--;
		}
		indices[k + 1] = key;
		j++;
	}
    free(prefix_min);
	free(prefix_max);
	return (best_split);
}

static uint32_t	filter_invalid_indices(uint32_t *indices,
										uint32_t start, uint32_t count,
										uint32_t max_descriptors)
{
	uint32_t	read;
	uint32_t	write;

	write = start;
	read  = start;
	while (read < start + count)
	{
		if (indices[read] < max_descriptors)
		{
			indices[write] = indices[read];
			write++;
		}
		read++;
	}
	return (write - start);
}

uint32_t	tlas_build_recursive(t_tlas_builder_ctx *ctx, uint32_t *indices,
								uint32_t start, uint32_t count)
{
	uint32_t	node_idx;
	t_tlas_node	*node;
	t_vec4		bbox_min;
	t_vec4		bbox_max;
	uint32_t	split_idx;
	uint32_t	left_count;
	uint32_t	right_count;

	count = filter_invalid_indices(indices, start, count, ctx->mesh_count);
	if (count == 0)
		return (0);
	node_idx = *ctx->node_idx;
	if (node_idx >= 2 * ctx->mesh_count - 1)
		return (0);
	(*ctx->node_idx)++;
	node = &ctx->nodes[node_idx];
	compute_meshes_bbox(ctx->descriptors, indices, start, count, &bbox_min, &bbox_max);
	node->bbox_min = bbox_min;
	node->bbox_max = bbox_max;
	if (count == 1)
	{
		node->left_child  = 0;
		node->right_child = 0;
		node->mesh_index  = indices[start];
		node->_padding    = 0;
		return (node_idx);
	}
	split_idx   = find_split_index(ctx->descriptors, indices, start, count,
									bbox_min, bbox_max);
	left_count  = split_idx - start;
	right_count = start + count - split_idx;
	node->left_child  = tlas_build_recursive(ctx, indices, start,      left_count);
	node->right_child = tlas_build_recursive(ctx, indices, split_idx,  right_count);
	node->mesh_index  = 0;
	node->_padding    = 0;
	return (node_idx);
}
