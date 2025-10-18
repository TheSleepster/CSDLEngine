/* ========================================================================
   $File: s_user_interface_core.cpp $
   $Date: September 29 2025 02:50 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include "s_user_interface.h"

#include "s_user_interface_layout.cpp"
#include "s_user_interface_widget.cpp"

/*===========================================
  ================== CORE API ===============
  ===========================================*/

internal void
ui_init_state(render_state_t  *render_state, 
              input_manager_t *input_manager, 
              asset_manager_t *asset_manager, 
              UI_state_t      *state)
{
    Assert(state->is_valid == false);

    state->arena          = c_arena_create(MB(20));
    state->first_layout   = c_arena_push_struct(&state->arena, UI_layout_t);
    ZeroStruct(*state->first_layout);

    state->active_layout  = state->first_layout;

    state->first_layout->arena                = &state->arena;
    state->first_layout->widget_hash          =  c_hash_table_create_ma(&state->arena, 2048, sizeof(UI_widget_t));
    state->first_layout->interaction_hash     =  c_hash_table_create_ma(&state->arena, 2048, sizeof(UI_interaction_data_t));
    state->first_layout->layout_pane          =  ui_widget_pane(state, STR("DEFAULT WIDGET PANE")); 
    state->first_layout->active_parent_widget =  state->first_layout->layout_pane;
    state->first_layout->is_valid             =  true;

    state->widget_padding_x      = 12;
    state->widget_padding_y      = 6;
    state->default_widget_width  = 60;
    state->default_widget_height = 30;

    state->default_widget_idle_color   = COLOR_WHITE;
    state->default_widget_hot_color    = COLOR_BLUE;
    state->default_widget_active_color = COLOR_GREEN;
    state->default_widget_text_color   = COLOR_BLACK;

    state->layout_counter = 0;
    state->input_manager  = input_manager;
    state->asset_manager  = asset_manager;

    mat4_t font_projection_matrix = mat4_RHGL_ortho(-960, 960, -540, 540, -1, 1);
    mat4_t font_view_matrix       = mat4_identity();

    r_reset_draw_frame_pipeline_state(render_state);
    r_set_active_blending_state(render_state, false);
    state->widget_desc = r_build_renderpass_desc(render_state,
                                                &render_state->font_shader,
                                                 1,
                                                 font_view_matrix,
                                                 font_projection_matrix,
                                                 RGE_None,
                                                 RGP_PostBlitPass,
                                                 RGPT_Quads);

    state->text_desc = r_build_renderpass_desc(render_state,
                                               &render_state->font_shader,
                                               0,
                                               font_view_matrix,
                                               font_projection_matrix,
                                               RGE_None,
                                               RGP_PostBlitPass,
                                               RGPT_Quads);

    state->background_desc = r_build_renderpass_desc(render_state,
                                                    &render_state->font_shader,
                                                     2,
                                                     font_view_matrix,
                                                     font_projection_matrix,
                                                     RGE_None,
                                                     RGP_PostBlitPass,
                                                     RGPT_Quads);
    r_reset_draw_frame_pipeline_state(render_state);

    state->DEBUG_font = s_asset_font_get(asset_manager, STR("LiberationMono_Regular"));
    state->is_valid   = true;
}

// TODO(Sleepster): This... 
internal void
ui_deinit_state(UI_state_t *state)
{
}

internal void
ui_resolve_layouts(asset_manager_t *asset_manager, UI_state_t *state)
{
    for(UI_layout_t *this_layout = state->first_layout;
        this_layout;
        this_layout = this_layout->next_layout)
    {
        UI_widget_t *root_widget = this_layout->layout_pane;
        root_widget->panel_size  = vec2(0, 0);

        this_layout->next_widget_cursor = this_layout->panel_position;
        for(UI_widget_t *child = root_widget->oldest_attached_widget;
            child;
            child = child->prev_attached_widget)
        {
            dynamic_render_font_varient_t *child_font = s_asset_font_get_at_size(asset_manager, 
                                                                                 state->DEBUG_font,
                                                                                 child->font_size);

            vec2_t child_panel_position = vec2_add(this_layout->panel_position, child->panel_offset);
            if((child->widget_flags & UIWF_DrawText) != 0)
            {
                child->string_size = r_prepare_string_for_rendering(asset_manager, child_font, child->name);

                child->panel_size.x = child->string_size.x + state->widget_padding_x * 2.0f;
                child->panel_size.y = child->string_size.y + state->widget_padding_y * 2.0f;
            }

            child->widget_rect = rect2_create(child_panel_position, child->panel_size);

            child->string_offset = { 
                child_panel_position.x + (child->panel_size.x - child->string_size.x) * 0.5f,
                child_panel_position.y + (child->panel_size.y - child->string_size.y) * 0.5f
            };

            root_widget->panel_size.y += child->panel_size.y + state->widget_padding_y;
            if(child->panel_size.x > root_widget->panel_size.x) 
            {
                root_widget->panel_size.x = child->panel_size.x + (state->widget_padding_x * 2);
            }
        }
        root_widget->widget_rect = rect2_create(this_layout->panel_position, vec2_add(root_widget->panel_size, vec2(state->widget_padding_x, state->widget_padding_y)));
    }
}

internal void
ui_render_widget(render_state_t *render_state, asset_manager_t *asset_manager, UI_state_t *state, UI_widget_t *widget)
{
    vec2_t render_pos = widget->widget_rect.min;
    if((widget->widget_flags & UIWF_DrawInBackground) != 0)
    {
        r_begin_renderpass(render_state, &state->background_desc);
        r_draw_rect(render_state, render_pos, widget->panel_size, widget->render_color, 0, RQO_NONE);
        r_end_renderpass(render_state);
    }

    if((widget->widget_flags & UIWF_HotAnimation) != 0)
    {
        if(widget->is_hot)
        {
            widget->render_color = widget->hot_color;
        }
    }
    if((widget->widget_flags & UIWF_ActiveAnimation) != 0)
    {
        if(widget->is_active)
        {
            widget->render_color = widget->active_color;
        }
    }
    if((widget->widget_flags & UIWF_Clickable) != 0)
    {
    }

    r_begin_renderpass(render_state, &state->widget_desc);
    if((widget->widget_flags & UIWF_FilledBox) != 0)
    {
        r_draw_rect(render_state, render_pos, widget->panel_size, widget->render_color, 0, RQO_NONE);
    }
    r_end_renderpass(render_state);

    r_begin_renderpass(render_state, &state->text_desc);
    if((widget->widget_flags & UIWF_DrawText) != 0)
    {
        r_draw_string(asset_manager, 
                      render_state, 
                      widget->name, 
                      state->DEBUG_font, 
                      FONT_SIZE, 
                      widget->string_offset,
                      widget->font_color,
                      RQO_NONE);
    }
    r_end_renderpass(render_state);

    // NOTE(Sleepster): recurse for children 
    for(UI_widget_t *child = widget->oldest_attached_widget;
        child;
        child = child->prev_attached_widget)
    {
        child->color = COLOR_BLUE;
        ui_render_widget(render_state, asset_manager, state, child);
    }
}

internal void
ui_render_all_widgets(render_state_t *render_state, asset_manager_t *asset_manager, UI_state_t *state)
{
    input_controller_t *primary_controller = s_input_manager_get_primary_controller(state->input_manager);
    state->mouse_pos = s_input_manager_transform_mouse_data(primary_controller, 
                                                            state->widget_desc.view_matrix, 
                                                            state->widget_desc.projection_matrix); 
    for(UI_layout_t *this_layout = state->first_layout;
        this_layout;
        this_layout = this_layout->next_layout)
    {
        UI_widget_t *root_widget = this_layout->layout_pane;
        root_widget->color       = {0.03, 0.03, 0.03, 0.05};

        ui_render_widget(render_state, asset_manager, state, root_widget);

        this_layout->layout_pane->next_attached_widget   = null;
        this_layout->layout_pane->oldest_attached_widget = null;
        this_layout->layout_pane->first_attached_widget  = null;
        this_layout->layout_pane->prev_attached_widget   = null;
    }

    state->current_frame++;
}

