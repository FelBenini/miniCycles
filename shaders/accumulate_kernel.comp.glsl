layout(local_size_x = 64) in;

void main()
{
    uint idx = gl_GlobalInvocationID.x;
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

    // Indirect radiance from path tracing (emissive hits, sky, future bounces)
    prev.rgb += state.radiance;

    // Direct lighting from shadow kernel (accumulated via atomic float add)
    uint base = pid * 3u;
    prev.r += uintBitsToFloat(shadow_accum[base]);
    prev.g += uintBitsToFloat(shadow_accum[base + 1u]);
    prev.b += uintBitsToFloat(shadow_accum[base + 2u]);

    // Clear shadow accum for this pixel (ready for next frame)
    shadow_accum[base]     = 0u;
    shadow_accum[base + 1u] = 0u;
    shadow_accum[base + 2u] = 0u;

    sample_count += 1.0;

    imageStore(u_output, pixel, vec4(prev.rgb, sample_count));
}
