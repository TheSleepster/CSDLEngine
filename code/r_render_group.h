#if !defined(RENDER_GROUP_H)
/* ========================================================================
   $File: render_group.h $
   $Date: November 17 2025 02:41 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define RENDER_GROUP_H
#define MAX_RENDER_GROUPS_TO_OUTPUT 64

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

    geometry_buffer   *next_buffer;
}geometry_buffer_t;

typedef struct render_group_desc
{
    render_material_t       render_material;
    render_phase_t          render_phase;
    render_camera_t         camera;
    render_pipeline_state_t pipeline_state;
}render_group_desc_t;

typedef struct render_group
{
    u32                 group_ID;
    u32                 phase_idx;

    render_group_desc_t render_desc;
    geometry_buffer_t   first_buffer;
}render_group_t;

internal inline void r_renderpass_begin(render_state_t *render_state);
internal inline void r_renderpass_end(render_state_t *render_state);
internal void r_pipeline_state_reset(render_state_t *render_state);

#endif // RENDER_GROUP_H

