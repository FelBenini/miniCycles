#include "screenshot.h"
#include "cycles.h"
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

static float clampf(float x, float min_val, float max_val)
{
	if (x < min_val)
		return min_val;
	if (x > max_val)
		return max_val;
	return x;
}

void save_screenshot(int width, int height, int *screenshot_index)
{
	GLfloat *pixels;
	GLubyte *output;
	char filename[256];
	int i;
	FILE *f;

	pixels = (GLfloat *)malloc(width * height * 4 * sizeof(GLfloat));
	if (!pixels)
		return ;

	output = (GLubyte *)malloc(width * height * 3);
	if (!output)
	{
		free(pixels);
		return ;
	}

	glReadPixels(0, 0, width, height, GL_RGBA, GL_FLOAT, pixels);

	for (i = 0; i < width * height; i++)
	{
		int flipped_y;
		float r, g, b;

		flipped_y = (height - 1 - (i / width));
		r = pixels[flipped_y * width * 4 + (i % width) * 4];
		g = pixels[flipped_y * width * 4 + (i % width) * 4 + 1];
		b = pixels[flipped_y * width * 4 + (i % width) * 4 + 2];

		r = clampf(r, 0.0f, 1.0f);
		g = clampf(g, 0.0f, 1.0f);
		b = clampf(b, 0.0f, 1.0f);

		output[i * 3] = (GLubyte)(r * 255.0f);
		output[i * 3 + 1] = (GLubyte)(g * 255.0f);
		output[i * 3 + 2] = (GLubyte)(b * 255.0f);
	}

	mkdir("output", 0755);
	snprintf(filename, sizeof(filename), "output/%d.png", *screenshot_index);
	(*screenshot_index)++;

	f = fopen(filename, "rb");
	if (f)
	{
		fclose(f);
		snprintf(filename, sizeof(filename), "output/%d.png", *screenshot_index);
		(*screenshot_index)++;
	}

	if (stbi_write_png(filename, width, height, 3, output, width * 3))
		printf("Screenshot saved to %s\n", filename);
	else
		fprintf(stderr, "Failed to write screenshot to %s\n", filename);
	free(pixels);
	free(output);
}
