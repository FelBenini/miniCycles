#define STB_IMAGE_IMPLEMENTATION
#include "cycles.h"
#include "scene.h"
#include "stb_image.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned char	**s_pixel_cache = NULL;
static int				s_cache_count = 0;
static int				s_cache_cap = 0;

int	scene_load_image(t_scene *scene, const char *path)
{
	t_image_ssbo	*iss;
	t_image_data	*new_meta;
	t_image_data	*meta;
	unsigned char	*pixels;
	int				new_cap;
	unsigned char	**new_cache;
	int				index;

	if (!scene->images)
	{
		scene->images = calloc(1, sizeof(t_image_ssbo));
		if (!scene->images)
			return (-1);
		scene->images->count = 0;
		scene->images->metadata = NULL;
		scene->images->ssbo = 0;
	}
	iss = scene->images;
	new_meta = realloc(iss->metadata, sizeof(t_image_data) * (iss->count + 1));
	if (!new_meta)
		return (-1);
	iss->metadata = new_meta;
	stbi_set_flip_vertically_on_load(1);
	meta = &iss->metadata[iss->count];
	pixels = stbi_load(path, &meta->width, &meta->height, &meta->channels, 4);
	if (!pixels)
		return (-1);
	meta->channels = 4;
	if (s_cache_count >= s_cache_cap)
	{
		new_cap = s_cache_cap == 0 ? 8 : s_cache_cap * 2;
		new_cache = realloc(s_pixel_cache, sizeof(unsigned char *) * new_cap);
		if (!new_cache)
		{
			stbi_image_free(pixels);
			return (-1);
		}
		s_pixel_cache = new_cache;
		s_cache_cap = new_cap;
	}
	printf("Image loaded: %s\n", path);
	s_pixel_cache[s_cache_count] = pixels;
	s_cache_count++;
	index = iss->count;
	iss->count++;
	return (index);
}

void	scene_upload_images(t_scene *scene)
{
	t_image_ssbo	*iss;
	int				count;
	size_t			*offsets;
	size_t			total_pixels;
	size_t			meta_buf_size;
	uint32_t		*meta_buf;
	uint32_t		*block;
	size_t			pixel_buf_size;
	uint8_t			*pixel_buf;
	size_t			img_bytes;

	if (!scene->images || scene->images->count == 0)
		return ;
	iss = scene->images;
	count = iss->count;
	offsets = malloc(sizeof(size_t) * count);
	total_pixels = 0;
	for (int i = 0; i < count; i++)
	{
		offsets[i] = total_pixels;
		total_pixels += (size_t)iss->metadata[i].width
			* (size_t)iss->metadata[i].height;
	}
	iss->total_bytes = total_pixels * 4;
	meta_buf_size = 16 + (size_t)count * 16;
	meta_buf = calloc(1, meta_buf_size);
	if (!meta_buf)
	{
		free(offsets);
		return ;
	}
	meta_buf[0] = (uint32_t)count;
	for (int i = 0; i < count; i++)
	{
		block = meta_buf + 4 + i * 4;
		block[0] = (uint32_t)iss->metadata[i].width;
		block[1] = (uint32_t)iss->metadata[i].height;
		block[2] = (uint32_t)iss->metadata[i].channels;
		block[3] = (uint32_t)offsets[i];
	}
	if (iss->ssbo)
		glDeleteBuffers(1, &iss->ssbo);
	glGenBuffers(1, &iss->ssbo);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, iss->ssbo);
	glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)meta_buf_size, meta_buf,
		GL_STATIC_READ);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, IMAGES_SSBOS, iss->ssbo);
	free(meta_buf);
	pixel_buf_size = total_pixels * 4;
	pixel_buf = malloc(pixel_buf_size);
	if (!pixel_buf)
	{
		free(offsets);
		return ;
	}
	for (int i = 0; i < count; i++)
	{
		img_bytes = (size_t)iss->metadata[i].width * iss->metadata[i].height
			* 4;
		memcpy(pixel_buf + offsets[i] * 4, s_pixel_cache[i], img_bytes);
	}
	free(offsets);
	if (iss->ssbo_pixels)
		glDeleteBuffers(1, &iss->ssbo_pixels);
	glGenBuffers(1, &iss->ssbo_pixels);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, iss->ssbo_pixels);
	glBufferData(GL_SHADER_STORAGE_BUFFER, (GLsizeiptr)pixel_buf_size,
		pixel_buf, GL_STATIC_READ);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, PIXELS_SSBOS, iss->ssbo_pixels);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	free(pixel_buf);
	for (int i = 0; i < s_cache_count; i++)
		stbi_image_free(s_pixel_cache[i]);
	free(s_pixel_cache);
	s_pixel_cache = NULL;
	s_cache_count = 0;
	s_cache_cap = 0;
}

void	scene_destroy_images(t_scene *scene)
{
	if (!scene->images)
		return ;
	for (int i = 0; i < s_cache_count; i++)
		stbi_image_free(s_pixel_cache[i]);
	free(s_pixel_cache);
	s_pixel_cache = NULL;
	s_cache_count = 0;
	s_cache_cap = 0;
	if (scene->images->ssbo)
		glDeleteBuffers(1, &scene->images->ssbo);
	free(scene->images->metadata);
	free(scene->images);
	scene->images = NULL;
}
