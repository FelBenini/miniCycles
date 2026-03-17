vec4 sample_image(uint img_idx, vec2 uv) {
    s_image_meta meta = img_info[img_idx];
    uint x      = uint(clamp(uv.x, 0.0, 1.0) * float(meta.width  - 1u));
    uint y      = uint(clamp(uv.y, 0.0, 1.0) * float(meta.height - 1u));
    uint offset = meta.pixel_offset + y * meta.width + x;
    uint texel  = pixels[offset];
    return vec4(
        float((texel      ) & 0xFF) / 255.0,
        float((texel >>  8) & 0xFF) / 255.0,
        float((texel >> 16) & 0xFF) / 255.0,
        float((texel >> 24) & 0xFF) / 255.0
    );
}

vec3 sky_color(s_ray ray)
{
    if (u_sky_tex == -1)
        return vec3(u_ambient_color.xyz);

    vec3 d  = normalize(ray.dir);
    vec2 uv = vec2(
        0.5 + atan(d.z, d.x) / (2.0 * 3.14159265358979),
        0.5 + asin(clamp(d.y, -1.0, 1.0)) / 3.14159265358979
    );
	vec3 res = sample_image(u_sky_tex, uv).rgb;
    return res * u_sky_intensity;
}
