#ifndef PARSER_H
# define PARSER_H

#include "scene.h"

t_scene	parse_scene(char *filename);
void	process_plane(t_scene *scene, char *line);
void	process_sphere(t_scene *scene, char *line);
void	process_camera(t_scene *scene, char *line);
void	process_cube(t_scene *scene, char *line);
void	process_ambient(t_scene *scene, char *line);
void	process_torus(t_scene *scene, char *line);
void	process_obj(t_scene *scene, char *line);
void	process_light(t_scene *scene, char *line);
void	process_cone(t_scene *scene, char *line);
void	process_cylinder(t_scene *scene, char *line);
int		get_texture_if_valid(t_scene *scene, char *path);

#endif
