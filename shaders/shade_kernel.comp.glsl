layout(local_size_x = 64) in;

void main()
{
    uint idx = gl_GlobalInvocationID.x;
    if (idx >= active_count) return;

    s_path_state state = ray_queue[idx];
	if (state.alive == 0u)
	{
		next_ray_queue[idx] = state;
    	return;
	}

    s_ray ray;
    ray.origin  = state.origin;
    ray.dir     = state.dir;
    ray.inv_dir = state.inv_dir;

    s_hit hit = hit_queue[idx];
    float adaptive_bias = max(1e-4, hit.t * 1e-4);

    s_shade_result res = shade_hit(
        ray, hit, state.throughput,
        bool(state.prev_specular), state.prev_bsdf_pdf,
        state.prev_origin, state.seed, state.pixel_idx
    );

    state.radiance += res.direct_radiance;

    if (res.alpha < 1.0 && rand(state.seed) > res.alpha)
    {
        state.origin = hit.pos + ray.dir * adaptive_bias;
    }
    else if (res.terminate)
    {
        state.alive = 0u;
    }
    else
    {
        state.throughput = res.new_throughput;
        if (max(state.throughput.r, max(state.throughput.g, state.throughput.b)) < 0.001)
        {
            state.alive = 0u;
        }
        else
        {
            state.prev_bsdf_pdf = res.new_bsdf_pdf;
            state.prev_specular = uint(res.is_specular);
            state.prev_origin   = ray.origin;

            state.origin  = hit.pos + res.N * adaptive_bias;
            state.dir     = res.new_dir;
            state.inv_dir = 1.0 / res.new_dir;
        }
    }

    state.bounce++;
	if (state.bounce >= 3)
	{
    	float lum = max(state.throughput.r,
        	        max(state.throughput.g, state.throughput.b));
    	float p = clamp(lum, 0.05, 0.95);
    	if (rand(state.seed) > p)
    	{
    		state.alive = 0u;
    	}
    	else
        	state.throughput /= p;
	}
    next_ray_queue[idx] = state;
}
