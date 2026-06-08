layout (binding = 0, rgba32f) uniform image2D u_output;

// Scene UBO — uploaded once per frame, shared by all compute shaders
layout(std140, binding = 0) uniform SceneParams {
    vec2  u_resolution;
    vec2  _pad_00;
    vec3  u_cam_pos;
    float _pad_cam_pos;
    vec3  u_cam_forward;
    float _pad_cam_forward;
    vec3  u_cam_right;
    float _pad_cam_right;
    vec3  u_cam_up;
    float _pad_cam_up;
    float u_cam_fov;
    float u_lens_radius;
    float u_focal_distance;
    float _pad_fov;
    vec4  u_ambient_color;
    int   u_sky_tex;
    float u_sky_intensity;
    uint  u_mesh_count;
    uint  u_light_count;
    uint  u_emissive_mesh_count;
    uint  u_frame_index;
    uint  u_reset_samples;
    int   u_max_bounces;
};

uniform vec2 u_tile_offset;

// Structs

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

struct s_triangle_texcoords {
    vec4 uv0;
    vec4 uv1;
    vec4 uv2;
};

struct s_mesh_descriptor {
    vec4  position;
    vec4  rot_col0;
    vec4  rot_col1;
    vec4  rot_col2;
    vec4  local_bbox_min;
    vec4  local_bbox_max;
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
	int   texture_idx;
	int   texture_displacement_idx;
	int   roughness_tex_idx;
	int   normal_tex_idx;
	float texture_tile_size;
	float pad[3];
};

struct s_hit {
    float t;
    vec3  pos;
    vec3  normal;
    vec3  geo_normal;
    uint  mesh_index;
    vec2  uv;
    float tri_area;
};

struct s_image_meta {
    uint width;
    uint height;
    uint channels;
    uint pixel_offset;
};

struct s_path_state {
    vec3  origin;
    vec3  dir;
    vec3  inv_dir;
    vec3  throughput;
    vec3  radiance;
    float prev_bsdf_pdf;
    vec3  prev_origin;
    uint  prev_specular;
    uint  pixel_idx;
    int   bounce;
    uint  seed;
    uint  alive;
};

struct s_shade_result {
    vec3  direct_radiance;
    vec3  new_throughput;
    vec3  new_dir;
    float new_bsdf_pdf;
    bool  is_specular;
    bool  terminate;
    float alpha;
    vec3  N;
    float rough;
};

#define LIGHT_SUN   0u
#define LIGHT_POINT 1u
#define LIGHT_SPOT  2u

struct s_light
{
    vec3  position;
    float pad0;
    vec3  direction;
    float pad1;
    vec3  color;
    float intensity;
    uint  type;
    float cos_inner;
    float cos_outer;
    float pad;
};

// SSBOs

layout(std430, binding = 1) readonly buffer Triangles        { s_triangle          triangles[];        };
layout(std430, binding = 2) readonly buffer TriangleNormals  { s_triangle_normals  triangle_normals[]; };
layout(std430, binding = 3) readonly buffer TriangleTexcoords { s_triangle_texcoords triangle_texcoords[]; };
layout(std430, binding = 4) readonly buffer Meshes           { s_mesh_descriptor   meshes[];           };
layout(std430, binding = 5) readonly buffer BVHNodes         { s_bvh_node          bvh_nodes[];        };
layout(std430, binding = 6) readonly buffer TLASNodes        { s_tlas_node         tlas_nodes[];       };
layout(std430, binding = 7) readonly buffer Materials        { s_material          materials[];        };
layout(std430, binding = 8) readonly buffer LightBuffer
{
    s_light lights[];
};
layout(std430, binding = 9) readonly buffer ImageMeta {
    uint          img_count;
	uint          _pad[3];
    s_image_meta  img_info[];
};
layout(std430, binding = 10) readonly buffer ImagePixels {
    uint pixels[];
};
layout(std430, binding = 11) buffer RayQueue {
    s_path_state ray_queue[];
};
layout(std430, binding = 12) buffer NextRayQueue {
    s_path_state next_ray_queue[];
};
layout(std430, binding = 13) buffer Counters {
    uint active_count;
    uint shadow_count;
    uint next_count;
	uint _padding;
};
layout(std430, binding = 14) buffer HitQueue {
    s_hit hit_queue[];
};

struct s_shadow_ray {
    vec3  origin;
    vec3  dir;
    vec3  inv_dir;
    float max_t;
    vec3  contrib;
    uint  pixel_idx;
    uint  exclude_mesh;
};

layout(std430, binding = 15) buffer ShadowQueue {
    s_shadow_ray shadow_queue[];
};
layout(std430, binding = 16) buffer ShadowAccum {
    uint shadow_accum[];
};
