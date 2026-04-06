vec3 sample_hemisphere(vec3 N, inout uint seed)
{
    float r1 = rand(seed);
    float r2 = rand(seed);

    float phi       = 2.0 * 3.14159265 * r1;
    float cos_theta = sqrt(r2);
    float sin_theta = sqrt(1.0 - r2);

    vec3 T = normalize(abs(N.x) > 0.1
        ? cross(vec3(0,1,0), N)
        : cross(vec3(1,0,0), N));
    vec3 B = cross(N, T);

    return normalize(
        cos(phi) * sin_theta * T +
        sin(phi) * sin_theta * B +
        cos_theta * N);
}

void trace_textures(
    s_material mat,
    inout vec3 N,
    inout s_hit hit,
    inout vec3 albedo,
    inout float rough
)
{
    float uv_scale = mat.texture_tile_size;
    vec2 uv = fract(hit.uv * uv_scale);
    if (mat.texture_idx != -1)
    {
        vec4 tex_color = sample_image(uint(mat.texture_idx), uv);
        albedo = tex_color.rgb;
    }
    if (mat.texture_displacement_idx != -1)
    {
        uint disp_idx = uint(mat.texture_displacement_idx);
        s_image_meta meta = img_info[disp_idx];
        vec2 texel = vec2(1.0 / float(meta.width), 1.0 / float(meta.height));
        float hL = sample_image(disp_idx, uv - vec2(texel.x, 0.0)).r;
        float hR = sample_image(disp_idx, uv + vec2(texel.x, 0.0)).r;
        float hD = sample_image(disp_idx, uv - vec2(0.0, texel.y)).r;
        float hU = sample_image(disp_idx, uv + vec2(0.0, texel.y)).r;
        float dHdU = (hR - hL) * 0.5;
        float dHdV = (hU - hD) * 0.5;
        vec3 T = normalize(abs(N.x) > 0.1
            ? cross(vec3(0,1,0), N)
            : cross(vec3(1,0,0), N));
        vec3 B = normalize(cross(N, T));
        float scale = 0.02;
        N = normalize(N - scale * (dHdU * T + dHdV * B));
    }
    if (mat.normal_tex_idx != -1)
    {
        vec3 normal_sample = sample_image(uint(mat.normal_tex_idx), uv).rgb;
        vec3 tangent_normal = normalize(normal_sample * 2.0 - 1.0);
        vec3 T = normalize(abs(N.x) > 0.1
            ? cross(vec3(0,1,0), N)
            : cross(vec3(1,0,0), N));
        vec3 B = normalize(cross(N, T));
        N = normalize(T * tangent_normal.x + B * tangent_normal.y + N * tangent_normal.z);
    }
    if (mat.roughness_tex_idx != -1)
    {
        vec4 rough_tex = sample_image(uint(mat.roughness_tex_idx), uv);
        rough = rough_tex.r;
    }
}

vec3 trace_path(s_ray ray, inout uint seed)
{
    vec3 throughput    = vec3(1.0);
    vec3 radiance      = vec3(0.0);
    bool prev_used_nee = false;
    const int MAX_BOUNCES = 6;

    for (int bounce = 0; bounce < MAX_BOUNCES; bounce++)
    {
        s_hit hit;
        if (!scene_intersect(ray, hit))
        {
            radiance += throughput * sky_color(ray);
            break;
        }

        s_mesh_descriptor mesh = meshes[hit.mesh_index];
        s_material        mat  = materials[mesh.material];

        vec3  N             = hit.normal;
        vec3  albedo        = mat.albedo.rgb;
        vec3  emission      = mat.emission.rgb;
        float rough         = mat.roughness;
        float metallic      = mat.metallic;
        float adaptive_bias = max(1e-4, hit.t * 1e-4);

        trace_textures(mat, N, hit, albedo, rough);
        rough = clamp(rough, 0.001, 1.0);

        radiance += throughput * emission;

        if (length(emission) > 0.0)
            break;

        // --- Always do NEE (no specular check)
        vec3 direct = sample_lights(hit.pos, N, adaptive_bias);
        vec3 emissive_direct = sample_emissive_meshes(hit.pos, N, adaptive_bias, seed);

        emissive_direct = min(emissive_direct, vec3(10.0));
        direct = min(direct + emissive_direct, vec3(10.0));

        radiance += throughput * albedo * direct;
        prev_used_nee = true;

        // --- Next bounce ---
        vec3 diffuse_dir = sample_hemisphere(N, seed);
        vec3 R           = reflect(ray.dir, N);
        vec3 glossy_dir  = normalize(R + rough * sample_hemisphere(N, seed));

        if (dot(glossy_dir, N) < 0.0)
            glossy_dir = diffuse_dir;

        vec3 new_dir = normalize(mix(glossy_dir, diffuse_dir, rough));

        ray.origin  = hit.pos + N * adaptive_bias;
        ray.dir     = new_dir;
        ray.inv_dir = 1.0 / new_dir;

        // --- Energy conservation
        vec3 F0 = mix(vec3(0.04), albedo, metallic);
        float cosTheta = max(dot(N, new_dir), 0.0);
        vec3 F = F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);

        vec3 kd = (1.0 - F) * (1.0 - metallic);

        throughput *= (kd * albedo + F);

        throughput = min(throughput, vec3(1.0));

        if (max(throughput.r, max(throughput.g, throughput.b)) < 0.001)
            break;

        // Russian roulette
        if (bounce >= 1)
        {
            float p = clamp(max(throughput.r, max(throughput.g, throughput.b)), 0.05, 0.95);
            if (rand(seed) > p) break;
            throughput /= p;
        }
    }
    return radiance;
}

void main()
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (pixel.x >= int(u_resolution.x) ||
        pixel.y >= int(u_resolution.y))
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

    s_ray ray;
    ray.origin  = u_cam_pos;
    ray.dir     = ray_dir;
    ray.inv_dir = 1.0 / ray_dir;

    vec3 new_sample = trace_path(ray, seed);

    vec4  prev         = imageLoad(u_output, pixel);
    float sample_count = float(u_frame_index);

    if (u_reset_samples == 1u)
    {
        prev         = vec4(0.0);
        sample_count = 0.0;
    }

    prev.rgb += new_sample;
    sample_count += 1.0;

    imageStore(u_output, pixel, vec4(prev.rgb, sample_count));
}
