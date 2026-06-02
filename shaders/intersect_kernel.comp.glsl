layout(local_size_x = 64) in;

void main()
{
    uint idx = gl_GlobalInvocationID.x;
    if (idx >= active_count) return;

    s_path_state state = ray_queue[idx];
    if (state.alive == 0u)
    {
        ray_queue[idx] = state;
        return;
    }
    if (state.bounce >= u_max_bounces)
    {
        state.alive = 0u;
        ray_queue[idx] = state;
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
        hit_queue[idx] = hit;
    }

    ray_queue[idx] = state;
}
