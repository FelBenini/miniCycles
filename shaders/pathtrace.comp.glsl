uint get_linear_idx()
{
    return gl_GlobalInvocationID.y * (gl_NumWorkGroups.x * 8u) + gl_GlobalInvocationID.x;
}

void gen_primary_rays()
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy) + ivec2(u_tile_offset);
    if (pixel.x >= int(u_resolution.x) || pixel.y >= int(u_resolution.y))
        return;

    uint seed = wang_hash(uint(pixel.x) ^ wang_hash(uint(pixel.y) ^ wang_hash(uint(u_frame_index))));

    vec2 jitter = vec2(rand(seed), rand(seed));

    vec2 uv = (vec2(pixel) + jitter) / u_resolution * 2.0 - 1.0;
    uv.x *= u_resolution.x / u_resolution.y;

    float half_fov = tan(u_cam_fov * 0.5);

    vec3 ray_dir = normalize(
        u_cam_forward +
        uv.x * half_fov * u_cam_right +
        uv.y * half_fov * u_cam_up);

    s_path_state state;
    state.origin      = u_cam_pos;
    state.dir         = ray_dir;
    state.inv_dir     = 1.0 / ray_dir;
    state.throughput  = vec3(1.0);
    state.radiance    = vec3(0.0);
    state.prev_bsdf_pdf = 0.0;
    state.prev_origin   = u_cam_pos;
    state.prev_specular = 1u;
    state.pixel_idx     = uint(pixel.y * int(u_resolution.x) + pixel.x);
    state.bounce        = 0;
    state.seed          = seed;
    state.alive         = 1u;

    uint slot = atomicAdd(active_count, 1u);
    ray_queue[slot] = state;
}

void process_one_bounce()
{
    uint idx = get_linear_idx();
    if (idx >= active_count) return;

    s_path_state state = ray_queue[idx];

    if (state.alive == 0u)
    {
        next_ray_queue[idx] = state;
        return;
    }

    if (state.bounce >= u_max_bounces)
    {
        state.alive = 0u;
        next_ray_queue[idx] = state;
        return;
    }

    s_ray ray;
    ray.origin  = state.origin;
    ray.dir     = state.dir;
    ray.inv_dir = state.inv_dir;

    s_hit hit;

    if (!scene_intersect(ray, hit))
    {
        state.radiance += state.throughput * sky_color(ray);
        state.alive = 0u;
    }
    else
    {
        float adaptive_bias = max(1e-4, hit.t * 1e-4);

        s_shade_result res = shade_hit(
            ray, hit, state.throughput,
            bool(state.prev_specular), state.prev_bsdf_pdf,
            state.prev_origin, state.seed
        );

        state.radiance += res.direct_radiance;

        if (res.alpha < 1.0 && rand(state.seed) > res.alpha)
        {
            state.origin = hit.pos + ray.dir * adaptive_bias;
            state.bounce--;
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

                if (state.bounce >= 1)
                {
                    float p = clamp(max(state.throughput.r, max(state.throughput.g, state.throughput.b)), 0.05, 0.95);
                    if (rand(state.seed) > p)
                        state.alive = 0u;
                    else
                        state.throughput /= p;
                }
            }
        }
    }

    state.bounce++;
    next_ray_queue[idx] = state;
}

void accumulate_radiance()
{
    uint idx = get_linear_idx();
    if (idx >= active_count) return;

    s_path_state state = ray_queue[idx];
    uint pid = state.pixel_idx;
    ivec2 pixel = ivec2(int(pid % uint(u_resolution.x)), int(pid / uint(u_resolution.x)));

    vec4  prev         = imageLoad(u_output, pixel);
    float sample_count = float(u_frame_index);

    if (u_reset_samples == 1u)
    {
        prev         = vec4(0.0);
        sample_count = 0.0;
    }

    prev.rgb += state.radiance;
    sample_count += 1.0;

    imageStore(u_output, pixel, vec4(prev.rgb, sample_count));
}

void main()
{
    if (u_pass_type == 0u)
        gen_primary_rays();
    else if (u_pass_type == 1u)
        process_one_bounce();
    else if (u_pass_type == 2u)
        accumulate_radiance();
}
