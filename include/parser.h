#ifndef PARSER_H
# define PARSER_H

#include "scene.h"

t_scene	parse_scene(char *filename);
void	process_plane(t_scene *scene, char *line);
void	process_sphere(t_scene *scene, char *line);
void	process_camera(t_scene *scene, char *line);
void	process_cube(t_scene *scene, char *line);
void	process_ambient(t_scene *scene, char *line);

#endif
