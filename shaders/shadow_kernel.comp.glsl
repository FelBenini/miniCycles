layout(local_size_x = 64) in;

void main()
{
    uint idx = gl_GlobalInvocationID.x;
    if (idx >= shadow_count) return;

    s_shadow_ray sr = shadow_queue[idx];

    s_ray ray;
    ray.origin  = sr.origin;
    ray.dir     = sr.dir;
    ray.inv_dir = sr.inv_dir;

    bool occluded;
    if (sr.exclude_mesh != ~0u)
        occluded = scene_intersect_shadow_exclude(ray, sr.max_t, sr.exclude_mesh);
    else
        occluded = scene_intersect_shadow(ray, sr.max_t);

    if (!occluded)
    {
        uint base = sr.pixel_idx * 3u;

        // Atomic float add using uint CAS on buffer variables directly
        uint expected_r = shadow_accum[base];
        uint desired_r;
        do {
            desired_r = floatBitsToUint(uintBitsToFloat(expected_r) + sr.contrib.r);
            expected_r = atomicCompSwap(shadow_accum[base], expected_r, desired_r);
        } while (expected_r != desired_r);

        uint expected_g = shadow_accum[base + 1u];
        uint desired_g;
        do {
            desired_g = floatBitsToUint(uintBitsToFloat(expected_g) + sr.contrib.g);
            expected_g = atomicCompSwap(shadow_accum[base + 1u], expected_g, desired_g);
        } while (expected_g != desired_g);

        uint expected_b = shadow_accum[base + 2u];
        uint desired_b;
        do {
            desired_b = floatBitsToUint(uintBitsToFloat(expected_b) + sr.contrib.b);
            expected_b = atomicCompSwap(shadow_accum[base + 2u], expected_b, desired_b);
        } while (expected_b != desired_b);
    }
}
