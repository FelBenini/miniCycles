float eval_bsdf_pdf(vec3 N, vec3 dir, vec3 R, float rough)
{
    // Cosine-weighted diffuse PDF (Lambertian)
    float pdf_diffuse = max(dot(N, dir), 0.0) / 3.14159265;

    // Glossy lobe (Phong-like) PDF around reflection direction R
    float cos_a = max(dot(dir, R), 0.0);
    float shininess = 2.0 / max(rough * rough, 1e-4) - 2.0;
    float pdf_glossy = (shininess + 1.0) / (2.0 * 3.14159265) * pow(cos_a, shininess);

    // Blend between glossy and diffuse PDFs based on roughness
    return max(mix(pdf_glossy, pdf_diffuse, rough), 1e-6);
}

vec3 eval_bsdf_pdf_dir(vec3 N, vec3 L, vec3 R, float rough)
{
    // Angle between sampled direction and reflection direction
    float cos_a = max(dot(L, R), 0.0);

    // Diffuse PDF
    float pdf_diff = max(dot(N, L), 0.0) / 3.14159265;

    // Glossy PDF (Phong-like)
    float shininess = 2.0 / max(rough * rough, 1e-4) - 2.0;
    float pdf_glos = (shininess + 1.0) / (2.0 * 3.14159265) * pow(cos_a, shininess);

    // Return: diffuse, glossy, and mixed PDF
    return vec3(pdf_diff, pdf_glos, max(mix(pdf_glos, pdf_diff, rough), 1e-6));
}

float eval_nee_pdf(s_hit hit, vec3 origin)
{
    // Squared distance from shading point to light sample
    float dist2 = dot(hit.pos - origin, hit.pos - origin);

    // Cosine term on light surface (geometry normal)
    float LdotLN = abs(dot(hit.geo_normal, -normalize(hit.pos - origin)));

    // Number of triangles in the emissive mesh
    float tri_count = float(meshes[hit.mesh_index].tri_count);

    // Convert area-based PDF to solid angle PDF
    return (LdotLN > 0.0 && hit.tri_area > 0.0)
        ? dist2 / (LdotLN * tri_count * hit.tri_area)
        : 0.0;
}

float balance_heuristic(float pdf1, float pdf2)
{
    // Standard balance heuristic for MIS
    float sum = pdf1 + pdf2;
    return (sum > 0.0) ? pdf1 / sum : 1.0;
}

float mis_emission_weight(bool prev_specular, float prev_bsdf_pdf, vec3 prev_origin, s_hit hit)
{
    // If previous bounce was specular or invalid PDF, skip MIS
    if (prev_specular || prev_bsdf_pdf <= 0.0)
        return 1.0;

    // Compute NEE PDF for this light hit
    float nee_pdf = eval_nee_pdf(hit, prev_origin);

    // Combine BSDF and NEE PDFs using balance heuristic
    return balance_heuristic(prev_bsdf_pdf, nee_pdf);
}

vec3 sample_emissive_mis(
    vec3 pos,
    vec3 N,
    vec3 R,
    float rough,
    float bias,
    inout uint seed
)
{
    vec3 result = vec3(0.0);

    // Sample emissive geometry: returns radiance contribution (L) and inverse PDF
    vec3 L;
    float inv_pdf;
    sample_emissive_meshes(pos, N, bias, seed, L, inv_pdf);

    // Validate sample
    if (dot(L, L) > 1e-8 && inv_pdf > 0.0)
    {
        // Evaluate BSDF PDFs for this sampled direction
        vec3 pdfs = eval_bsdf_pdf_dir(N, L, R, rough);
        float bsdf_pdf = pdfs.z;

        // Recover NEE PDF from inverse
        float nee_pdf = 1.0 / inv_pdf;

        // Compute MIS weight for NEE sample
        float w_nee = balance_heuristic(nee_pdf, bsdf_pdf);

        // Accumulate contribution with MIS weight
        result += L * w_nee;
    }
    // Clamp to reduce fireflies
    return min(result, vec3(10.0));
}
