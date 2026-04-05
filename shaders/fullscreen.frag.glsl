#version 430 core

// UV coordinates interpolated across the screen quad (0,0) to (1,1)
in vec2 v_uv;

// Final output color written to the framebuffer
out vec4 fragColor;

// Accumulation texture: holds the SUM of all light samples (RGB) and count (A)
uniform sampler2D u_accumulation_tex;

uniform uint u_tonemap;

uniform sampler3D u_lut_tex;
uniform int u_lut_size;

const mat3 AGX_INSET = mat3(
    0.842479062253094,   0.0423282422610123, 0.0423756549057051,
    0.0784335999999992,  0.878468636469772,  0.0784336,
    0.0792237451477643,  0.0791661274605434, 0.879142973793104
);

const mat3 AGX_OUTSET = mat3(
     1.19687900512017,   -0.0528968517574562, -0.0529716355144438,
    -0.0980208811401368,  1.15190312990417,   -0.0980434501171241,
    -0.0990297440797205, -0.0989611768448433,  1.15107367264116
);

const float AGX_MIN_EV = -12.47393;
const float AGX_MAX_EV =   4.026069;

vec3 agxSigmoid(vec3 x) {
    vec3 x2 = x * x;
    vec3 x4 = x2 * x2;
    return  15.5     * x4 * x2
          - 40.14    * x4 * x
          + 31.96    * x4
          -  6.868   * x2 * x
          +  0.4298  * x2
          +  0.1191  * x
          -  0.00232;
}

vec3 agxSaturation(vec3 color, float factor) {
    const vec3 LUMA = vec3(0.2126729, 0.7151522, 0.0721750);
    float luma = dot(color, LUMA);
    return luma + factor * (color - luma);
}

vec3 agxLookPunchy(vec3 color) {
    color = agxSaturation(color, 1.05);
    color = pow(max(color, 0.0), vec3(1.15));
    return color;
}

vec3 agx(vec3 color) {
    color = max(color, vec3(0.0));

    // 1. Inset — pull out-of-gamut values back inside the working space.
    color = AGX_INSET * color;

    // 2. Log2 encode — map the HDR exposure window to [0, 1].
    color = clamp(log2(max(color, vec3(1e-10))), AGX_MIN_EV, AGX_MAX_EV);
    color = (color - AGX_MIN_EV) / (AGX_MAX_EV - AGX_MIN_EV);

    // 3. Sigmoid — smooth S-curve tone compression.
    color = agxSigmoid(color);

    // 4. Look — Punchy saturation + contrast.
    color = agxLookPunchy(color);

    // 5. Outset — back to display-referred linear sRGB.
    color = AGX_OUTSET * color;

    return color;
}

vec3 linearToSRGB(vec3 x) {
    x = clamp(x, 0.0, 1.0);
    return mix(
        12.92 * x,
        1.055 * pow(x, vec3(1.0 / 2.4)) - 0.055,
        step(vec3(0.0031308), x)
    );
}

vec3 applyLUT(vec3 color) {
    if (u_lut_size == 0)
        return color;
    float scale = float(u_lut_size - 1) / float(u_lut_size);
    float l_offset = 0.5 / float(u_lut_size);
    color = max(color, vec3(0.0));
    vec3 uvw = color * scale + l_offset;
    return texture(u_lut_tex, uvw).rgb;
}

void main()
{
    // Raw sum of all color samples accumulated so far at this pixel
    vec4 accum = texture(u_accumulation_tex, v_uv);

    // Clamp to 1.0 minimum to avoid division by zero on the first frame
    float sample_count = max(accum.a, 1.0);
    // Divide the accumulated sum by the frame count to get the running average.
    // The more frames accumulate, the more noise cancels out and the image converges.
    vec3 color = accum.rgb / sample_count;

    if (u_tonemap == 1)
        color = linearToSRGB(agx(color));
	else if (u_tonemap == 2 && u_lut_size > 0)
	  {
		    color = linearToSRGB(color);
	      color = applyLUT(color);
	}
	  else
    	  color = linearToSRGB(color);

    fragColor = vec4(color, 1.0);
}
