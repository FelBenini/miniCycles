layout(local_size_x = 8, local_size_y = 8) in;

void main()
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
