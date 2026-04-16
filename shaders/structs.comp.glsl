layout (local_size_x = 8, local_size_y = 8) in;
layout (binding = 0, rgba32f) uniform image2D u_output;

// Camera

uniform vec2  u_resolution;
uniform vec3  u_cam_pos;
uniform vec3  u_cam_forward;
uniform vec3  u_cam_right;
uniform vec3  u_cam_up;
uniform float u_cam_fov;

uniform vec4  u_ambient_color;
uniform int   u_sky_tex;
uniform float u_sky_intensity;
uniform uint  u_light_count;
uniform uint  u_emissive_mesh_count;
// Progressive sampling control
uniform uint u_frame_index;   // increment every frame
uniform uint u_reset_samples; // set to 1 when camera moves

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
layout(std430, binding = 11) readonly buffer EmissiveMeshIndices {
    uint emissive_mesh_indices[];
};
