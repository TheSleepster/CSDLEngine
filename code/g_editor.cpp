/* ========================================================================
   $File: g_editor.cpp $
   $Date: November 13 2025 05:41 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
struct game_map_editor_t
{
    render_group_t *editor_grid;
};

#if 0
internal void
game_editor_init(render_state_t *render_state, game_map_editor_t *editor)
{
    mat4_t projection_matrix = mat4_RHGL_ortho(-160, 160, -90, 90, -1, 1);
    mat4_t view_matrix       = mat4_identity();
    render_group_desc_t editor_group_desc = r_renderpass_build_pass_desc(render_state,
                                                                        &render_state->test_shader,
                                                                         18,
                                                                         view_matrix,
                                                                         projection_matrix,
                                                                         RGE_None,
                                                                         RGP_MainGamePass,
                                                                         RGPT_Lines);
    editor->editor_grid = r_renderpass_get_or_create(render_state, &editor_group_desc);
}

internal void
game_editor_update(render_state_t *render_state, game_map_editor_t *editor)
{
}
#endif
