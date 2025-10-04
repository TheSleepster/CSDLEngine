/* ========================================================================
   $File: s_user_interface.cpp $
   $Date: September 29 2025 02:50 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include "s_user_interface.h"

#define FONT_SIZE 16 

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

    state->layout_counter = 0;
    state->input_manager  = input_manager;

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

/*===========================================
  ================ WIDGET API ===============
  ===========================================*/

internal UI_layout_t*
UI_create_new_layout(UI_state_t *state)
{
    UI_layout_t *result      = null;
    for(UI_layout_t *this_layout = state->first_layout;
        this_layout;
        this_layout = this_layout->next_layout)
    {
        if(this_layout->is_valid)
        {
            result = this_layout;
            break;
        }
    }

    if(!result)
    {
        result =  c_arena_push_struct(&state->arena, UI_layout_t);
        ZeroStruct(*result);

        result->widget_hash           =  c_hash_table_create_ma(&state->arena, 2048, sizeof(UI_widget_t));
        result->interaction_hash      =  c_hash_table_create_ma(&state->arena, 2048, sizeof(UI_interaction_data_t));
        result->arena                 = &state->arena;
        result->input_manager         =  state->input_manager;
        result->layout_pane           =  ui_widget_pane(state, STR("DEFAULT WIDGET PANE"));
        result->active_parent_widget  =  result->layout_pane;

        UI_layout_t *last_layout = state->first_layout;
        state->first_layout      = result;
        result->next_layout      = last_layout;
    }
    Assert(result);

    result->is_valid = true;
    return(result);
}

internal inline UI_layout_t * 
ui_layout_begin(UI_state_t *state)
{
    UI_layout_t *layout = UI_create_new_layout(state);
    if(layout)
    {
        state->active_layout = layout;
    }

    return(layout);
}

internal inline void
ui_layout_end(UI_state_t *state)
{
    state->active_layout = null;
}

internal void
ui_widget_attach(UI_layout_t *layout, UI_widget_t *widget)
{
    Assert(layout);
    Assert(layout->active_parent_widget);

    UI_widget_t *parent = layout->active_parent_widget;
    widget->parent_widget = parent;
    if(!parent->first_attached_widget)
    {
        // NOTE(Sleepster): First 
        parent->first_attached_widget = widget;
        parent->last_attached_widget  = widget;
        parent->next_attached_widget  = null;
        parent->prev_attached_widget  = null;
    }
    else
    {
        // NOTE(Sleepster): Append 
        UI_widget_t *last_widget = parent->last_attached_widget;
        last_widget->next_attached_widget = widget;

        widget->prev_attached_widget      = last_widget;
        widget->next_attached_widget      = null;

        parent->last_attached_widget = widget;
    }
}

internal UI_widget_t* 
ui_widget_create(UI_layout_t *layout, string_t name, u32 widget_flags)
{
    Assert(layout);

    UI_widget_t *widget = (UI_widget_t *)c_hash_get_value(&layout->widget_hash, name);
    if(!widget)
    {
        widget = c_arena_push_struct(layout->arena, UI_widget_t);
        ZeroStruct(*widget);

        widget->widget_flags = widget_flags;
        widget->ID           = layout->widget_counter++;
        widget->name         = name;
        widget->font_size    = FONT_SIZE;
        c_hash_insert_kv_pair(&layout->widget_hash, name, widget);
    }

    widget->next_attached_widget  = null;
    widget->prev_attached_widget  = null;
    widget->first_attached_widget = null;
    widget->last_attached_widget  = null;

    if(layout->active_parent_widget)
    {
        ui_widget_attach(layout, widget);
    }
    return(widget);
}

internal inline void
ui_widget_push_parent(UI_layout_t *layout, UI_widget *widget)
{
    layout->active_parent_widget = widget;
}

internal inline void
ui_widget_pop_parent(UI_layout_t *layout)
{
    layout->active_parent_widget = layout->layout_pane;
}

internal UI_interaction_data_t*
ui_widget_get_interaction_data(UI_state_t *state, UI_widget_t *widget)
{
    UI_interaction_data_t *result = (UI_interaction_data_t *)c_hash_get_value(&state->active_layout->interaction_hash, widget->name);
    if(result == null)
    {
        result = c_arena_push_struct(&state->arena, UI_interaction_data_t);
    }
    Assert(result);

    input_controller_t *controller        = s_input_manager_get_primary_controller(state->input_manager);
    action_button_t    *left_mouse_state  = s_input_manager_get_key_state(controller, SDL_LEFT_MOUSE);
    action_button_t    *right_mouse_state = s_input_manager_get_key_state(controller, SDL_RIGHT_MOUSE);

    result->widget   = widget;
    result->hovering = rect_vec2_test(widget->widget_rect, state->mouse_pos);

    result->clicked        = left_mouse_state->is_down;
    result->right_clicked  = right_mouse_state->half_transition_counter >= 2;
    result->double_clicked = left_mouse_state->half_transition_counter  >= 4;
    result->pressed        = left_mouse_state->is_pressed;
    result->released       = left_mouse_state->is_released;
    result->dragging       = left_mouse_state->is_down && !left_mouse_state->is_released;

    return(result);
}

internal bool8
ui_widget_button(UI_state_t *state, string_t name)
{
    bool8 result = false;

    UI_widget_t *widget = ui_widget_create(state->active_layout, 
                                           name, 
                                           UIWF_Clickable|
                                           UIWF_DrawBorder|
                                           UIWF_DrawText|
                                           UIWF_FilledBox|
                                           UIWF_HotAnimation|
                                           UIWF_ActiveAnimation); 
    UI_interaction_data_t *interaction_info = ui_widget_get_interaction_data(state, widget);
    if(interaction_info->hovering)
    {
        widget->is_hot = true;
        interaction_info->last_hot_frame = state->current_frame;

        bool8 down_this_frame = interaction_info->pressed;
        if(down_this_frame)
        {
            widget->started_inside = true;
        }

        if(widget->started_inside)
        {
            widget->is_active = interaction_info->clicked;
            interaction_info->last_active_frame = state->current_frame;
        }
    }
    else if(interaction_info->last_hot_frame != state->current_frame)
    {
        widget->is_hot    = false;
        widget->is_active = false;
    }

    if(!interaction_info->dragging)
    {
        widget->started_inside = false;
    }

    result = interaction_info->pressed;
    return(result);
}

internal inline UI_widget_t*
ui_widget_pane(UI_state_t *state, string_t name)
{
    UI_widget_t *widget = ui_widget_create(state->active_layout, 
                                           name, 
                                           UIWF_DrawInBackground|
                                           UIWF_DrawBorder|
                                           UIWF_Clip); 
    return(widget);
}

internal inline void
ui_widget_set_position(UI_widget_t *widget, vec2_t pos)
{
    widget->position = pos;
}

internal inline void
ui_widget_set_size(UI_widget_t *widget, vec2_t size)
{
    widget->size = size;
}

// TODO(Sleepster): Current all of these do the same thing. Change that. 
//
// TODO(Sleepster): Maybe just make these change state->default_*_color... 
internal inline void
ui_widget_set_idle_color(UI_widget_t *widget, vec4_t color)
{
    widget->color = color;
}

internal inline void
ui_widget_set_hot_color(UI_widget_t *widget, vec4_t color)
{
    widget->color = color;
}

internal inline void
ui_widget_set_active_color(UI_widget_t *widget, vec4_t color)
{
    widget->color = color;
}

/*===========================================
  ================== CORE API ===============
  ===========================================*/

internal void
ui_resolve_layouts(asset_manager_t *asset_manager, UI_state_t *state)
{
    for(UI_layout_t *this_layout = state->first_layout;
        this_layout;
        this_layout = this_layout->next_layout)
    {
        for(UI_widget_t *widget = this_layout->layout_pane;
            widget;
            widget = widget->next_attached_widget)
        {
            widget->position    = {0, 0};
            widget->size        = {0, 0};
            widget->size.x     += state->widget_padding_x;
            widget->size.y     += state->widget_padding_y;
            float32 next_widget_offset = 0.0f;
            for(UI_widget_t *child = widget->last_attached_widget;
                child;
                child = child->prev_attached_widget)
            {
                dynamic_render_font_varient_t *child_font = s_asset_font_get_at_size(asset_manager, 
                                                                                     state->DEBUG_font,
                                                                                     child->font_size);
                child->position = {widget->position.x + (state->widget_padding_x * 0.5f), 
                                  (widget->position.y + next_widget_offset) + (state->widget_padding_y * 0.5f)};

                child->size        = r_prepare_string_for_rendering(asset_manager, child_font, child->name);
                child->size.x     += state->widget_padding_x;
                child->size.y     += state->widget_padding_y;
                child->widget_rect = rect_create(child->position, vec2_add(child->position, child->size));
                child->string_position = {child->position.x + (child->size.x * 0.20f) - (state->widget_padding_x * 0.5f), 
                                          child->position.y + (child->size.y * 0.25f)};

                next_widget_offset += child->size.y;
                widget->size.y     += child->size.y;
                if(child->size.x > widget->size.x) 
                {
                    widget->size.x = child->size.x + state->widget_padding_x;
                }
            }
            widget->widget_rect = rect_create(widget->position, vec2_add(widget->position, widget->size));

        } 
    }
}

#define COLOR_WHITE  ((vec4_t){1.0, 1.0, 1.0, 1.0})
#define COLOR_RED    ((vec4_t){1.0, 0.0, 0.0, 1.0})
#define COLOR_GREEN  ((vec4_t){0.0, 1.0, 0.0, 1.0})
#define COLOR_BLUE   ((vec4_t){0.0, 0.0, 1.0, 1.0})
#define COLOR_BLACK  ((vec4_t){0.0, 0.0, 0.0, 1.0})

internal void
ui_render_widget(render_state_t *render_state, asset_manager_t *asset_manager, UI_state_t *state, UI_widget_t *widget)
{
    if((widget->widget_flags & UIWF_HotAnimation) != 0)
    {
        if(widget->is_hot)
        {
            widget->color = COLOR_RED;
        }
    }
    if((widget->widget_flags & UIWF_ActiveAnimation) != 0)
    {
        if(widget->is_active)
        {
            widget->color = COLOR_GREEN;
        }
    }
    if((widget->widget_flags & UIWF_Clickable) != 0)
    {
    }

    r_begin_renderpass(render_state, &state->widget_desc);
    if((widget->widget_flags & UIWF_FilledBox) != 0)
    {
        r_draw_rect(render_state, widget->position, widget->size, widget->color, 0, RQO_NONE);
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
                      widget->string_position,
                      COLOR_BLACK,
                      RQO_NONE);
    }
    r_end_renderpass(render_state);

    r_begin_renderpass(render_state, &state->background_desc);
    if((widget->widget_flags & UIWF_DrawInBackground) != 0)
    {
        r_draw_rect(render_state, widget->position, widget->size, widget->color, 0, RQO_NONE);
    }
    r_end_renderpass(render_state);

    // NOTE(Sleepster): recurse for children 
    for(UI_widget_t *child = widget->last_attached_widget;
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
        for(UI_widget_t *widget = this_layout->layout_pane;
            widget;
            widget = widget->next_attached_widget)
        {
            widget->color = {0.03, 0.03, 0.03, 0.05};
            ui_render_widget(render_state, asset_manager, state, widget);
        }

        this_layout->layout_pane->next_attached_widget  = null;
        this_layout->layout_pane->last_attached_widget  = null;
        this_layout->layout_pane->first_attached_widget = null;
        this_layout->layout_pane->prev_attached_widget  = null;
    }

    state->current_frame++;
}

// TODO(Sleepster): Update UI data
// TODO(Sleepster): Render the UI Data
// TODO(Sleepster): Reset UI Data
