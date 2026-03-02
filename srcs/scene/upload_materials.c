#include "scene.h"
#include "cycles.h"

void	scene_upload_materials(t_scene *scene)
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, scene->ssbo_materials);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(t_material) * scene->material_count,
			scene->materials, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, MATERIALS_SSBOS, scene->ssbo_materials);
	scene->material_dirty = 0;
}
