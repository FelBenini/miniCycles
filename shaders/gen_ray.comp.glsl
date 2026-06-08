layout(local_size_x = 8, local_size_y = 8) in;

vec2 concentric_sample_disk(vec2 u)
{
    float sx = 2.0 * u.x - 1.0;
    float sy = 2.0 * u.y - 1.0;

    if (sx == 0.0 && sy == 0.0)
        return vec2(0.0);

    float r, theta;

    if (abs(sx) > abs(sy))
    {
        r = sx;
        theta = 0.78539816339 * (sy / sx);
    }
    else
    {
        r = sy;
        theta = 1.57079632679 - 0.78539816339 * (sx / sy);
    }

    return r * vec2(cos(theta), sin(theta));
}

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

    vec3 ray_origin = u_cam_pos;

    if (u_lens_radius > 0.0 && u_focal_distance > 0.0)
    {
        vec2 lens_uv = concentric_sample_disk(vec2(rand(seed), rand(seed))) * u_lens_radius;
        vec3 lens_pos = u_cam_pos + u_cam_right * lens_uv.x + u_cam_up * lens_uv.y;

        float t_focus = u_focal_distance / dot(ray_dir, u_cam_forward);
        vec3 focus_point = u_cam_pos + ray_dir * t_focus;

        ray_origin = lens_pos;
        ray_dir = normalize(focus_point - lens_pos);
    }

    s_path_state state;
    state.origin      = ray_origin;
    state.dir         = ray_dir;
    state.inv_dir     = 1.0 / ray_dir;
    state.throughput  = vec3(1.0);
    state.radiance    = vec3(0.0);
    state.prev_bsdf_pdf = 0.0;
    state.prev_origin   = ray_origin;
    state.prev_specular = 1u;
    state.pixel_idx     = uint(pixel.y * int(u_resolution.x) + pixel.x);
    state.bounce        = 0;
    state.seed          = seed;
    state.alive         = 1u;

    uint slot = atomicAdd(active_count, 1u);
    ray_queue[slot] = state;
}
