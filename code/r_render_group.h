#if !defined(RENDER_GROUP_H)
/* ========================================================================
   $File: render_group.h $
   $Date: November 17 2025 02:41 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define RENDER_GROUP_H
#define MAX_RENDER_GROUPS_TO_OUTPUT 64

#include "r_render_material.h"

typedef struct render_state render_state_t;

typedef struct geometry_buffer
{
    bool8              is_valid;

    vertex_t          *quad_vertex_buffer;
    vertex_t          *line_vertex_buffer;

    render_quad_t     *quad_buffer;
    render_line_t     *line_buffer;

    u32                quad_count;
    u32                line_count;
    u32                quad_vertex_count;
    u32                line_vertex_count;

    render_camera_t   *render_camera;
    geometry_buffer   *next_buffer;
}geometry_buffer_t;

typedef struct render_group_desc
{
    render_material_t       render_material;
    render_phase_t          render_phase;
    render_pipeline_state_t pipeline_state;
}render_group_desc_t;

typedef struct render_group
{
    u32                 group_ID;
    u32                 phase_idx;

    render_group_desc_t render_desc;
    geometry_buffer_t   first_buffer;
}render_group_t;

internal        void               r_render_group_init_geometry_buffer(render_state_t *render_state, geometry_buffer_t *buffer);
internal        geometry_buffer_t* r_render_group_get_buffer(render_state_t *render_state, render_group_t *render_group, u32 primitive_type);
internal        void               r_render_group_process_quad_geometry(geometry_buffer_t *buffer);
internal        void               r_render_group_process_line_geometry(geometry_buffer_t *buffer);
internal        void               r_render_group_handle_geometry_buffers(multithreading_work_queue_t *queue, render_group_t *render_group);
internal        render_group_t*    r_render_group_create_new(render_state_t *render_state, hash_table_t *hash_table, u64 combo_id);
internal        void               r_renderpass_handle_data(render_state_t *render_state, asset_manager_t *asset_manager);
internal inline void               r_renderpass_begin(render_state_t *render_state);
internal inline void               r_renderpass_end(render_state_t *render_state);

#endif // RENDER_GROUP_H

