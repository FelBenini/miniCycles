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

s_shade_result shade_hit(
    s_ray   ray,
    s_hit   hit,
    vec3    throughput,
    bool    prev_specular,
    float   prev_bsdf_pdf,
    vec3    prev_origin,
    inout uint seed,
    uint    pixel_idx
)
{
    s_shade_result res;
    res.terminate       = false;
    res.direct_radiance = vec3(0.0);

    s_mesh_descriptor mesh = meshes[hit.mesh_index];
    s_material        mat  = materials[mesh.material];
    float adaptive_bias    = max(1e-4, hit.t * 1e-4);

    vec3  N      = hit.normal;
    vec3  albedo = mat.albedo.rgb;
    float alpha  = 1.0;
    float rough  = mat.roughness;
    trace_textures(mat, N, hit, albedo, alpha, rough);

    res.alpha = alpha;
    res.N     = N;
    res.rough = rough;

    vec3 emission = mat.emission.rgb;
    float metallic = mat.metallic;

    if (length(emission) > 0.0)
    {
        float mis_weight = mis_emission_weight(prev_specular, prev_bsdf_pdf, prev_origin, hit);
        res.direct_radiance = throughput * emission * mis_weight;
        res.terminate = true;
        return res;
    }

    vec3 throughput_albedo = throughput * albedo;
    vec3 R = reflect(ray.dir, N);

    // Emit shadow rays for direct lighting — shadow kernel handles occlusion
    sample_lights(hit.pos, N, adaptive_bias, pixel_idx, throughput_albedo);
    sample_emissive_mis(hit.pos, N, R, rough, adaptive_bias, seed, pixel_idx, throughput_albedo);

    // BSDF sampling — unchanged
    vec3 diffuse_dir = sample_hemisphere(N, seed);
    vec3 glossy_dir  = normalize(R + rough * sample_hemisphere(N, seed));
    if (dot(glossy_dir, N) < 0.0) glossy_dir = diffuse_dir;
    vec3 new_dir = normalize(mix(glossy_dir, diffuse_dir, rough));

    vec3  F0       = mix(vec3(0.04), albedo, metallic);
    float cosTheta = max(dot(N, new_dir), 0.0);
    vec3  F        = F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
    vec3  kd       = (1.0 - F) * (1.0 - metallic);

    res.new_throughput = min(throughput * (kd * albedo + F), vec3(1.0));
    res.new_dir        = new_dir;
    res.new_bsdf_pdf   = eval_bsdf_pdf(N, new_dir, R, rough);
    res.is_specular    = (rough < 0.05);

    return res;
}
