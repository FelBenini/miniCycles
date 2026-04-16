void blas_intersect(s_ray ray, uint mesh_idx, inout s_hit hit)
{
    uint stack[96];
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
                    hit.tri_area = 0.5 * length(cross(e1, e2));

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

                    vec2 uv_interp =
                        bary.x * triangle_texcoords[tri_idx].uv0.xy +
                        bary.y * triangle_texcoords[tri_idx].uv1.xy +
                        bary.z * triangle_texcoords[tri_idx].uv2.xy;
                    hit.uv = uv_interp;
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
        else if (hit_left)
          stack[ptr++] = node.left_child;
        else if (hit_right)
          stack[ptr++] = node.right_child;
    }
}

