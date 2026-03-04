#ifndef RT_MATH_H
# define RT_MATH_H

# include <math.h>

typedef struct s_vec4
{
	float	x;
	float	y;
	float	z;
	float	w; // gpu padding
}	t_vec4;

typedef struct s_vec3
{
	float	x;
	float	y;
	float	z;
}	t_vec3;

typedef struct s_vertex
{
	t_vec4	pos;
	t_vec4	normal;
}	t_vertex;

t_vec4	vec4_from_vec3(t_vec3 v, float w);
t_vec3	vec3_from_vec4(t_vec4 v);

t_vec4	vec4_create(float x, float y, float z, float w);
t_vec4	vec4_normalize(t_vec4 v);
t_vec4	vec4_cross(t_vec4 a, t_vec4 b);

// vec3

t_vec3 vec3(float x, float y, float z);

t_vec3 vec3_add(t_vec3 a, t_vec3 b);
t_vec3 vec3_sub(t_vec3 a, t_vec3 b);
t_vec3 vec3_mul(t_vec3 v, float s);
t_vec3 vec3_div(t_vec3 v, float s);

float   vec3_dot(t_vec3 a, t_vec3 b);
t_vec3  vec3_cross(t_vec3 a, t_vec3 b);
float   vec3_length(t_vec3 v);
t_vec3  vec3_normalize(t_vec3 v);

t_vec3  vec3_reflect(t_vec3 v, t_vec3 n);

#endif
