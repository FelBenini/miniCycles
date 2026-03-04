#include "rt_math.h"

t_vec4	vec4_create(float x, float y, float z, float w)
{
	t_vec4	v;

	v.x = x;
	v.y = y;
	v.z = z;
	v.w = w;
	return (v);
}

t_vec4	vec4_normalize(t_vec4 v)
{
	float	len;

	len = sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
	if (len == 0)
		return (v);
	v.x /= len;
	v.y /= len;
	v.z /= len;
	v.w = 0;
	return (v);
}

t_vec4	vec4_cross(t_vec4 a, t_vec4 b)
{
	t_vec4	v;

	v.x = a.y * b.z - a.z * b.y;
	v.y = a.z * b.x - a.x * b.z;
	v.z = a.x * b.y - a.y * b.x;
	v.w = 0;
	return (v);
}
