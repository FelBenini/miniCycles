#version 430 core

layout (local_size_x = 8, local_size_y = 8) in;
layout (binding = 0, rgba32f) uniform image2D u_output;

// ------------------------------------------------ Camera

uniform vec2  u_resolution;
uniform vec3  u_cam_pos;
uniform vec3  u_cam_forward;
uniform vec3  u_cam_right;
uniform vec3  u_cam_up;
uniform float u_cam_fov;
uniform vec4  u_ambient_color;

// Progressive sampling control
uniform uint u_frame_index;   // increment every frame
uniform uint u_reset_samples; // set to 1 when camera moves

// ------------------------------------------------ Structs

struct s_ray {
    vec3 origin;
    vec3 dir;
    vec3 inv_dir;
};

struct s_triangle {
    vec4 v0;
    vec4 v1;
    vec4 v2;
};

struct s_triangle_normals {
    vec4 n0;
    vec4 n1;
    vec4 n2;
};

struct s_mesh_descriptor {
    vec4  position;
    vec4  direction;
    uint  tri_offset;
    uint  tri_count;
    uint  smooth_shade;
    uint  bvh_root;
    uint  material;
    uint  pad[3];
};

struct s_bvh_node {
    vec4  bbox_min;
    vec4  bbox_max;
    uint  left_child;
    uint  right_child;
    uint  tri_offset;
    uint  tri_count;
};

struct s_tlas_node {
    vec4  bbox_min;
    vec4  bbox_max;
    uint  left_child;
    uint  right_child;
    uint  mesh_index;
    uint  _padding;
};

struct s_material {
    vec4  albedo;
    vec4  emission;
    float roughness;
    float metallic;
    float ior;
    uint  type;
};

struct s_hit {
    float t;
    vec3  pos;
    vec3  normal;
    vec3  geo_normal;
    uint  mesh_index;
};

// ------------------------------------------------ SSBOs

layout(std430, binding = 1) readonly buffer Triangles        { s_triangle          triangles[];        };
layout(std430, binding = 2) readonly buffer TriangleNormals  { s_triangle_normals  triangle_normals[]; };
layout(std430, binding = 3) readonly buffer Meshes           { s_mesh_descriptor   meshes[];           };
layout(std430, binding = 4) readonly buffer BVHNodes         { s_bvh_node          bvh_nodes[];        };
layout(std430, binding = 5) readonly buffer TLASNodes        { s_tlas_node         tlas_nodes[];       };
layout(std430, binding = 6) readonly buffer Materials        { s_material          materials[];        };

// ------------------------------------------------ RNG

uint wang_hash(uint seed)
{
    seed = (seed ^ 61u) ^ (seed >> 16u);
    seed *= 9u;
    seed = seed ^ (seed >> 4u);
    seed *= 0x27d4eb2du;
    seed = seed ^ (seed >> 15u);
    return seed;
}

float rand(inout uint seed)
{
    seed = wang_hash(seed);
    return float(seed) / 4294967296.0;
}

// ------------------------------------------------ Rotation helpers

mat3 mat_from_dir(vec3 up)
{
    float len = length(up);
    if (len < 1e-6)
        return mat3(1.0);

    up = up / len;
    vec3 world_fwd = abs(up.z) < 0.999 ? vec3(0.0, 0.0, 1.0)
                                        : vec3(0.0, 1.0, 0.0);
    vec3 right   = normalize(cross(world_fwd, up));
    vec3 forward = cross(up, right);
    return mat3(right, up, forward);
}

// ------------------------------------------------ AABB

bool intersect_aabb(
    s_ray ray,
    vec3  bmin,
    vec3  bmax,
    float max_t,
    out float t_near)
{
    vec3 t0 = (bmin - ray.origin) * ray.inv_dir;
    vec3 t1 = (bmax - ray.origin) * ray.inv_dir;

    vec3 tmin_v = min(t0, t1);
    vec3 tmax_v = max(t0, t1);

    float t0_max = max(max(tmin_v.x, tmin_v.y), tmin_v.z);
    float t1_min = min(min(tmax_v.x, tmax_v.y), tmax_v.z);

    t_near = t0_max;

    return (t1_min >= t0_max) &&
           (t1_min >  1e-4)   &&
           (t0_max <  max_t);
}

// ------------------------------------------------ Triangle

bool intersect_triangle(
    s_ray      ray,
    s_triangle tri,
    out float  t,
    out vec3   bary)
{
    const float EPS = 1e-6;

    vec3 v0 = tri.v0.xyz;
    vec3 e1 = tri.v1.xyz - v0;
    vec3 e2 = tri.v2.xyz - v0;

    vec3  p   = cross(ray.dir, e2);
    float det = dot(e1, p);
    if (abs(det) < EPS) return false;

    float inv = 1.0 / det;
    vec3  s   = ray.origin - v0;

    float u = dot(s, p) * inv;
    if (u < 0.0 || u > 1.0) return false;

    vec3  q = cross(s, e1);
    float v = dot(ray.dir, q) * inv;
    if (v < 0.0 || u + v > 1.0) return false;

    t = dot(e2, q) * inv;
    if (t < EPS) return false;

    bary = vec3(1.0 - u - v, u, v);
    return true;
}

// ------------------------------------------------ BLAS closest

void blas_intersect(s_ray ray, uint mesh_idx, inout s_hit hit)
{
    uint stack[48];
    uint ptr = 0;
    stack[ptr++] = meshes[mesh_idx].bvh_root;

    while (ptr > 0)
    {
        uint       node_idx = stack[--ptr];
        s_bvh_node node     = bvh_nodes[node_idx];

        // Leaf node — test triangles
        if (node.tri_count > 0)
        {
            uint base = meshes[mesh_idx].tri_offset + node.tri_offset;
            for (uint i = 0; i < node.tri_count; i++)
            {
                float t;
                vec3  bary;
                uint  tri_idx = base + i;

                if (intersect_triangle(ray, triangles[tri_idx], t, bary) && t < hit.t)
                {
                    hit.t          = t;
                    hit.mesh_index = mesh_idx;

                    vec3 e1    = triangles[tri_idx].v1.xyz - triangles[tri_idx].v0.xyz;
                    vec3 e2    = triangles[tri_idx].v2.xyz - triangles[tri_idx].v0.xyz;
                    vec3 geo_n = normalize(cross(e1, e2));
                    if (dot(geo_n, ray.dir) > 0.0) geo_n = -geo_n;
                    hit.geo_normal = geo_n;

                    if (meshes[mesh_idx].smooth_shade == 1u)
                    {
                        vec3 interp =
                            bary.x * triangle_normals[tri_idx].n0.xyz +
                            bary.y * triangle_normals[tri_idx].n1.xyz +
                            bary.z * triangle_normals[tri_idx].n2.xyz;
                        hit.normal = normalize(interp);
                        if (dot(hit.normal, geo_n) < 0.0) hit.normal = -hit.normal;
                    }
                    else
                        hit.normal = geo_n;
                }
            }
            continue;
        }

        // ---- Ordered child traversal ----
        float t_left  = 1e30;
        float t_right = 1e30;

        bool hit_left  = (node.left_child  != 0) && intersect_aabb(ray,
                            bvh_nodes[node.left_child].bbox_min.xyz,
                            bvh_nodes[node.left_child].bbox_max.xyz,
                            hit.t, t_left);

        bool hit_right = (node.right_child != 0) && intersect_aabb(ray,
                            bvh_nodes[node.right_child].bbox_min.xyz,
                            bvh_nodes[node.right_child].bbox_max.xyz,
                            hit.t, t_right);

        if (hit_left && hit_right)
        {
            if (t_left < t_right) {
                stack[ptr++] = node.right_child;
                stack[ptr++] = node.left_child;
            } else {
                stack[ptr++] = node.left_child;
                stack[ptr++] = node.right_child;
            }
        }
        else if (hit_left)  stack[ptr++] = node.left_child;
        else if (hit_right) stack[ptr++] = node.right_child;
    }
}

// ------------------------------------------------ TLAS

bool scene_intersect(s_ray ray_world, out s_hit hit)
{
    hit.t  = 1e30;
    bool found = false;

    uint stack[48];
    uint ptr = 0;
    stack[ptr++] = 0;

    while (ptr > 0)
    {
        uint        idx  = stack[--ptr];
        s_tlas_node node = tlas_nodes[idx];

        float tnear;
        if (!intersect_aabb(ray_world,
                            node.bbox_min.xyz,
                            node.bbox_max.xyz,
                            hit.t,
                            tnear))
            continue;

        // Leaf — intersect the mesh in its local space
        if (node.left_child == 0 && node.right_child == 0)
        {
            uint mesh_idx  = node.mesh_index;
            vec3 mesh_pos  = meshes[mesh_idx].position.xyz;
            vec3 mesh_fwd  = meshes[mesh_idx].direction.xyz;

            mat3 R     = mat_from_dir(mesh_fwd);
            mat3 R_inv = transpose(R);

            s_ray ray;
            ray.origin  = R_inv * (ray_world.origin - mesh_pos);
            ray.dir     = R_inv * ray_world.dir;
            ray.inv_dir = 1.0 / ray.dir;

            float t_before = hit.t;
            blas_intersect(ray, mesh_idx, hit);

            if (hit.t < t_before)
            {
                hit.normal     = normalize(R * hit.normal);
                hit.geo_normal = normalize(R * hit.geo_normal);
                found = true;
            }

            continue;
        }

        // ---- Ordered child traversal ----
        float t_left  = 1e30;
        float t_right = 1e30;

        bool hit_left  = (node.left_child  != 0) && intersect_aabb(ray_world,
                            tlas_nodes[node.left_child].bbox_min.xyz,
                            tlas_nodes[node.left_child].bbox_max.xyz,
                            hit.t, t_left);

        bool hit_right = (node.right_child != 0) && intersect_aabb(ray_world,
                            tlas_nodes[node.right_child].bbox_min.xyz,
                            tlas_nodes[node.right_child].bbox_max.xyz,
                            hit.t, t_right);

        if (hit_left && hit_right)
        {
            if (t_left < t_right) {
                stack[ptr++] = node.right_child;
                stack[ptr++] = node.left_child;
            } else {
                stack[ptr++] = node.left_child;
                stack[ptr++] = node.right_child;
            }
        }
        else if (hit_left)  stack[ptr++] = node.left_child;
        else if (hit_right) stack[ptr++] = node.right_child;
    }

    if (found)
        hit.pos = ray_world.origin + ray_world.dir * hit.t;

    return found;
}

// ------------------------------------------------ Shadow any-hit

bool scene_intersect_shadow(s_ray ray_world, float max_t)
{
    uint stack[48];
    uint ptr = 0;
    stack[ptr++] = 0;

    while (ptr > 0)
    {
        uint        idx  = stack[--ptr];
        s_tlas_node node = tlas_nodes[idx];

        float tnear;
        if (!intersect_aabb(ray_world, node.bbox_min.xyz,
                            node.bbox_max.xyz, max_t, tnear))
            continue;

        if (node.left_child == 0 && node.right_child == 0)
        {
            uint mesh_idx = node.mesh_index;
            vec3 mesh_pos = meshes[mesh_idx].position.xyz;
            mat3 R_inv    = transpose(mat_from_dir(meshes[mesh_idx].direction.xyz));

            s_ray ray;
            ray.origin  = R_inv * (ray_world.origin - mesh_pos);
            ray.dir     = R_inv * ray_world.dir;
            ray.inv_dir = 1.0 / ray.dir;

            uint bstack[48];
            uint bptr = 0;
            bstack[bptr++] = meshes[mesh_idx].bvh_root;

            while (bptr > 0)
            {
                s_bvh_node bnode = bvh_nodes[bstack[--bptr]];

                if (bnode.tri_count > 0)
                {
                    uint base = meshes[mesh_idx].tri_offset + bnode.tri_offset;
                    for (uint i = 0; i < bnode.tri_count; i++)
                    {
                        float t; vec3 bary;
                        if (intersect_triangle(ray, triangles[base + i], t, bary)
                            && t < max_t)
                            return true;
                    }
                    continue;
                }

                float tl, tr;
                bool hl = (bnode.left_child  != 0) && intersect_aabb(ray,
                    bvh_nodes[bnode.left_child].bbox_min.xyz,
                    bvh_nodes[bnode.left_child].bbox_max.xyz, max_t, tl);
                bool hr = (bnode.right_child != 0) && intersect_aabb(ray,
                    bvh_nodes[bnode.right_child].bbox_min.xyz,
                    bvh_nodes[bnode.right_child].bbox_max.xyz, max_t, tr);
                if (hl) bstack[bptr++] = bnode.left_child;
                if (hr) bstack[bptr++] = bnode.right_child;
            }
            continue;
        }

        float tl, tr;
        bool hl = (node.left_child  != 0) && intersect_aabb(ray_world,
            tlas_nodes[node.left_child].bbox_min.xyz,
            tlas_nodes[node.left_child].bbox_max.xyz, max_t, tl);
        bool hr = (node.right_child != 0) && intersect_aabb(ray_world,
            tlas_nodes[node.right_child].bbox_min.xyz,
            tlas_nodes[node.right_child].bbox_max.xyz, max_t, tr);
        if (hl) stack[ptr++] = node.left_child;
        if (hr) stack[ptr++] = node.right_child;
    }
    return false;
}

// ------------------------------------------------ Sky

vec3 sky_color()
{
    return vec3(u_ambient_color.xyz);
}

// ------------------------------------------------ Hemisphere sampling

vec3 sample_hemisphere(vec3 N, inout uint seed)
{
    float r1 = rand(seed);
    float r2 = rand(seed);

    float phi       = 2.0 * 3.14159265 * r1;
    float cos_theta = sqrt(1.0 - r2);
    float sin_theta = sqrt(r2);

    vec3 T = normalize(abs(N.x) > 0.1
        ? cross(vec3(0,1,0), N)
        : cross(vec3(1,0,0), N));
    vec3 B = cross(N, T);

    return normalize(
        cos(phi) * sin_theta * T +
        sin(phi) * sin_theta * B +
        cos_theta * N);
}

// ------------------------------------------------ Path tracing core

const vec3  SUN_DIR   = normalize(vec3(-0.6, 1.0, 0.4));
const vec3  SUN_COLOR = vec3(1.0);

vec3 trace_path(s_ray ray, inout uint seed)
{
    vec3 throughput = vec3(1.0);
    vec3 radiance   = vec3(0.0);

    const int MAX_BOUNCES = 8;

    for (int bounce = 0; bounce < MAX_BOUNCES; bounce++)
    {
        s_hit hit;

        if (!scene_intersect(ray, hit))
        {
            radiance += throughput * sky_color();
            break;
        }

        s_mesh_descriptor mesh = meshes[hit.mesh_index];
        s_material        mat  = materials[mesh.material];

        vec3 N        = hit.normal;
        vec3 albedo   = mat.albedo.rgb;
        vec3 emission = mat.emission.rgb;

        radiance += throughput * emission;

        if (length(emission) > 0.0)
            break;

        float adaptive_bias = max(1e-4, hit.t * 1e-4);

        float sun_dot = max(dot(N, SUN_DIR), 0.0);
        if (sun_dot > 0.0)
        {
            s_ray shadow;
            shadow.origin  = hit.pos + hit.geo_normal * adaptive_bias;
            shadow.dir     = SUN_DIR;
            shadow.inv_dir = 1.0 / SUN_DIR;

            if (!scene_intersect_shadow(shadow, 1e30))
                radiance += throughput * SUN_COLOR * sun_dot * albedo;
        }

        float rough       = clamp(mat.roughness, 0.001, 1.0);
        vec3  R           = reflect(ray.dir, N);
        vec3  glossy_dir  = normalize(R + rough * sample_hemisphere(N, seed));
        vec3  diffuse_dir = sample_hemisphere(N, seed);
        vec3  new_dir     = normalize(mix(glossy_dir, diffuse_dir, rough));

        ray.origin  = hit.pos + hit.geo_normal * adaptive_bias;
        ray.dir     = new_dir;
        ray.inv_dir = 1.0 / new_dir;

        vec3 specular_color = mix(vec3(1.0), albedo, mat.metallic);
        vec3 diffuse_color  = albedo * (1.0 - mat.metallic);
        throughput *= mix(specular_color, diffuse_color, rough);

        float p = max(throughput.r, max(throughput.g, throughput.b));
        if (bounce > 0)
        {
            if (rand(seed) > p) break;
            throughput /= p;
        }
    }

    return radiance;
}

// ------------------------------------------------ Main

void main()
{
    ivec2 pixel = ivec2(gl_GlobalInvocationID.xy);
    if (pixel.x >= int(u_resolution.x) ||
        pixel.y >= int(u_resolution.y))
        return;

    uint seed = wang_hash(uint(pixel.x) ^ wang_hash(uint(pixel.y) ^ wang_hash(u_frame_index)));

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

    vec3 accum = (prev.rgb * sample_count + new_sample)
               / (sample_count + 1.0);

    imageStore(u_output, pixel, vec4(accum, 1.0));
}
