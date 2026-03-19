#ifndef LUT_H
# define LUT_H

#include <stdint.h>
#include <stddef.h>

typedef struct s_lut
{
	float	*data;
	int		size;
}	t_lut;

t_lut	load_lut(const char *path);
void	destroy_lut(t_lut *lut);

#endif
