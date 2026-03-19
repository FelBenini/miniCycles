#include "lut.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int	parse_lut_size(const char *line)
{
	int	size;

	if (sscanf(line, "LUT_3D_SIZE %d", &size) == 1)
		return (size);
	return (0);
}

static int	is_data_line(const char *line)
{
	float	r;
	float	g;
	float	b;

	if (line[0] == '#')
		return (0);
	if (sscanf(line, "%f %f %f", &r, &g, &b) == 3)
		return (1);
	return (0);
}

static void	parse_lut_data(FILE *file, t_lut *lut)
{
	char		line[256];
	float		*r;
	int			count;

	r = lut->data;
	count = 0;
	while (fgets(line, sizeof(line), file) && count < lut->size * lut->size * lut->size)
	{
		if (is_data_line(line))
		{
			sscanf(line, "%f %f %f", r, r + 1, r + 2);
			r += 3;
			count++;
		}
	}
}

t_lut	load_lut(const char *path)
{
	t_lut	lut;
	FILE	*file;
	char	line[256];
	int		size_found;

	lut.data = NULL;
	lut.size = 0;
	file = fopen(path, "r");
	if (!file)
	{
		printf("Failed to open LUT file: %s\n", path);
		return (lut);
	}
	size_found = 0;
	while (fgets(line, sizeof(line), file))
	{
		if (!size_found)
		{
			lut.size = parse_lut_size(line);
			if (lut.size > 0)
			{
				size_found = 1;
				lut.data = malloc(lut.size * lut.size * lut.size * 3 * sizeof(float));
				if (!lut.data)
				{
					fclose(file);
					printf("Failed to allocate LUT data\n");
					return (lut);
				}
				parse_lut_data(file, &lut);
				break ;
			}
		}
	}
	fclose(file);
	if (!size_found)
	{
		lut.size = 33;
		lut.data = malloc(lut.size * lut.size * lut.size * 3 * sizeof(float));
		if (lut.data)
		{
			float	*f = lut.data;
			int		i, j, k;
			for (k = 0; k < lut.size; k++)
			{
				for (j = 0; j < lut.size; j++)
				{
					for (i = 0; i < lut.size; i++)
					{
						*f++ = (float)i / (lut.size - 1);
						*f++ = (float)j / (lut.size - 1);
						*f++ = (float)k / (lut.size - 1);
					}
				}
			}
			size_found = 1;
		}
	}
	return (lut);
}

void	destroy_lut(t_lut *lut)
{
	if (lut->data)
	{
		free(lut->data);
		lut->data = NULL;
	}
	lut->size = 0;
}
