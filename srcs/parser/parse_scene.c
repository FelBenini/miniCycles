#include "camera.h"
#include "parser.h"
#include "rt_math.h"
#include "scene.h"
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

static char	*read_line(FILE *file)
{
	char	*line;
	char	*tmp;
	char	buf[1024];
	size_t	len;

	line = NULL;
	len = 0;
	while (fgets(buf, sizeof(buf), file) != NULL)
	{
		tmp = realloc(line, len + strlen(buf) + 1);
		if (!tmp)
		{
			free(line);
			return (NULL);
		}
		line = tmp;
		strcpy(line + len, buf);
		len += strlen(buf);
		if (line[len - 1] == '\n')
			break ;
	}
	return (line);
}

void	process_line(t_scene *scene, char *line)
{
	size_t	i;

	i = 0;
	while(line[i] && isspace(line[i]))
		i++;
	if (strncmp(&line[i], "C", 1) == 0)
		process_camera(scene, line);
	if (strncmp(&line[i], "pl", 2) == 0)
		process_plane(scene, line);
	if (strncmp(&line[i], "sp", 2) == 0)
		process_sphere(scene, line);
	if (strncmp(&line[i], "cb", 2) == 0)
		process_cube(scene, line);
	if (strncmp(&line[i], "A", 1) == 0)
		process_ambient(scene, line);
}

t_scene	parse_scene(char *filename)
{
	t_scene	scene;
	FILE	*file;
	char	*line;

	scene = scene_create(8);
	file = fopen(filename, "r");
	if (!file)
	{
		printf("Could not open the file '%s'.\n", filename);
		exit(1);
	}
	while ((line = read_line(file)) != NULL)
	{
		process_line(&scene, line);
		free(line);
	}
	fclose(file);
	return (scene);
}
