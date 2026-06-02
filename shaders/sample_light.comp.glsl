void emit_shadow_ray(vec3 origin, vec3 dir, float max_t, vec3 contrib,
                     uint pixel_idx, uint exclude_mesh)
{
    uint slot = atomicAdd(shadow_count, 1u);
    shadow_queue[slot].origin       = origin;
    shadow_queue[slot].dir          = dir;
    shadow_queue[slot].inv_dir      = 1.0 / dir;
    shadow_queue[slot].max_t        = max_t;
    shadow_queue[slot].contrib      = contrib;
    shadow_queue[slot].pixel_idx    = pixel_idx;
    shadow_queue[slot].exclude_mesh = exclude_mesh;
}

vec3 sample_lights(vec3 pos, vec3 normal, float bias,
                   uint pixel_idx, vec3 throughput_albedo)
{
    for (uint i = 0u; i < u_light_count; i++)
    {
        s_light light = lights[i];

        if (light.type == LIGHT_SUN)
        {
            vec3 L = normalize(light.direction.xyz);
            float NdotL = max(dot(normal, L), 0.0);
            if (NdotL <= 0.0)
                continue;

            vec3 contrib = throughput_albedo * light.color.xyz * light.intensity * NdotL;
            emit_shadow_ray(pos + normal * bias, L, 1e30, contrib, pixel_idx, ~0u);
        }
        else if (light.type == LIGHT_POINT)
        {
            vec3  to_light    = light.position.xyz - pos;
            float dist        = length(to_light);
            vec3  L           = to_light / dist;
            float NdotL       = max(dot(normal, L), 0.0);
            if (NdotL <= 0.0)
                continue;

            float attenuation = 1.0 / (dist * dist);
            vec3 contrib = throughput_albedo * light.color.xyz * light.intensity * NdotL * attenuation;
            emit_shadow_ray(pos + normal * bias, L, dist, contrib, pixel_idx, ~0u);
        }
        else if (light.type == LIGHT_SPOT)
        {
            vec3  to_light    = light.position.xyz - pos;
            float dist        = length(to_light);
            vec3  L           = to_light / dist;
            float NdotL       = max(dot(normal, L), 0.0);
            if (NdotL <= 0.0)
                continue;

            vec3  spot_dir    = normalize(-light.direction.xyz);
            float cos_theta   = dot(L, spot_dir);
            float cos_inner   = light.cos_inner;
            float cos_outer   = light.cos_outer;

            if (cos_theta < cos_outer)
                continue;

            float spot_factor = clamp(
                (cos_theta - cos_outer) / (cos_inner - cos_outer),
                0.0, 1.0);
            spot_factor = spot_factor * spot_factor;

            float attenuation = 1.0 / (dist * dist);
            vec3 contrib = throughput_albedo * light.color.xyz * light.intensity * NdotL * attenuation * spot_factor;
            emit_shadow_ray(pos + normal * bias, L, dist, contrib, pixel_idx, ~0u);
        }
    }
    return vec3(0.0);
}

void sample_emissive_meshes(vec3 pos, vec3 normal, float bias, inout uint seed,
                            uint pixel_idx, vec3 throughput_albedo,
                            vec3 R_dir, float rough)
{
    for (uint i = 0u; i < u_mesh_count; i++)
    {
        uint mesh_idx = i;
        s_mesh_descriptor mesh = meshes[mesh_idx];
        s_material        mat  = materials[mesh.material];

        if (length(mat.emission.rgb) <= 0.0)
            continue;

        uint tri_count  = mesh.tri_count;
        uint tri_offset = mesh.tri_offset;
        if (tri_count == 0u)
            continue;

        mat3 R = mat_from_dir(mesh.direction.xyz);

        float r0      = rand(seed);
        uint  tri_idx = tri_offset + min(uint(r0 * float(tri_count)), tri_count - 1u);

        s_triangle tri = triangles[tri_idx];

        float r1      = rand(seed);
        float r2      = rand(seed);
        float sqrt_r1 = sqrt(r1);
        float u       = 1.0 - sqrt_r1;
        float v       = sqrt_r1 * (1.0 - r2);
        float w       = sqrt_r1 * r2;

        vec3 local_pos = tri.v0.xyz * u
                       + tri.v1.xyz * v
                       + tri.v2.xyz * w;
        vec3 light_pos = R * local_pos + mesh.position.xyz;

        vec3  e0       = tri.v1.xyz - tri.v0.xyz;
        vec3  e1       = tri.v2.xyz - tri.v0.xyz;
        vec3  cross_e  = cross(e0, e1);
        float tri_area = 0.5 * length(cross_e);
        if (tri_area <= 0.0)
            continue;

        vec3 light_n = normalize(R * normalize(cross_e));

        vec3  origin   = pos + normal * max(bias, 1e-3);
        vec3  to_light = light_pos - origin;
        float dist2    = dot(to_light, to_light);
        float dist     = sqrt(dist2);
        if (dist < 1e-6)
            continue;
        vec3 L = to_light / dist;

        float NdotL  = dot(normal,  L);
        float LdotLN = dot(light_n, -L);

        if (LdotLN < 0.0)
            LdotLN = dot(-light_n, -L);

        if (NdotL <= 0.0 || LdotLN <= 0.0)
            continue;

        // Cast shadow ray from light point surface to avoid self-intersection
        vec3 shadow_origin = light_pos + light_n * max(bias, 1e-4);
        vec3 to_shading = origin - shadow_origin;
        vec3 shadow_dir = normalize(to_shading);
        float shadow_dist = length(to_shading);

        float inv_pdf = (float(tri_count) * tri_area * LdotLN) / dist2;

        // MIS weight
        vec3 pdfs = eval_bsdf_pdf_dir(normal, L, R_dir, rough);
        float bsdf_pdf = pdfs.z;
        float nee_pdf = 1.0 / inv_pdf;
        float w_nee = balance_heuristic(nee_pdf, bsdf_pdf);

        vec3 contrib = throughput_albedo * mat.emission.rgb * NdotL * inv_pdf * w_nee;

        emit_shadow_ray(shadow_origin, shadow_dir, shadow_dist * 0.9999,
                        contrib, pixel_idx, mesh_idx);
    }
}

vec3 sample_emissive_mis(
    vec3 pos, vec3 N, vec3 R, float rough, float bias,
    inout uint seed, uint pixel_idx, vec3 throughput_albedo)
{
    sample_emissive_meshes(pos, N, bias, seed, pixel_idx, throughput_albedo, R, rough);
    return vec3(0.0);
}
