#ifndef FRAME_H
# define FRAME_H

#include "cycles.h"
#include "scene.h"
#include <stdint.h>

void	render_frame(
			t_cycles cycles,
			t_scene scene,
			uint32_t frame_index,
			uint32_t reset_samples,
			int preview);

#endif
