void trace_textures(
    s_material mat,
    inout vec3 N,
    inout s_hit hit,
    inout vec3 albedo,
	inout float alpha,
    inout float rough
)
{
    float uv_scale = mat.texture_tile_size;
    vec2 uv = fract(hit.uv * uv_scale);
    if (mat.texture_idx != -1)
    {
        vec4 tex_color = sample_image(uint(mat.texture_idx), uv);
        albedo = tex_color.rgb;
		alpha = tex_color.a;
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
