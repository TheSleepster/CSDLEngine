/* ========================================================================
   $File: s_user_interface_widget.cpp $
   $Date: October 11 2025 12:08 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

// TODO(Sleepster): The adding of widgets to a hierarchy is broken. The parent is storing itself as a parent, causing an endless loop. 

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
        parent->first_attached_widget  = widget;
        parent->oldest_attached_widget = widget;
        parent->next_attached_widget   = null;
        parent->prev_attached_widget   = null;
    }
    else
    {
        // NOTE(Sleepster): Append 
        UI_widget_t *last_widget = parent->oldest_attached_widget;
        last_widget->next_attached_widget = widget;

        widget->prev_attached_widget      = last_widget;
        widget->next_attached_widget      = null;

        parent->oldest_attached_widget = widget;
    }
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
    result->hovering = rect2_vec2_test(widget->widget_rect, state->mouse_pos);
    if(result->hovering)
    {
        result->clicked        = left_mouse_state->is_down;
        result->right_clicked  = right_mouse_state->half_transition_counter >= 2;
        result->double_clicked = left_mouse_state->half_transition_counter  >= 4;
        result->pressed        = left_mouse_state->is_pressed;
        result->released       = left_mouse_state->is_released;
        result->dragging       = left_mouse_state->is_down && !left_mouse_state->is_released;
    }

    return(result);
}

internal UI_widget_t* 
ui_widget_create(UI_state_t *state, 
                 string_t    name, 
                 u32         widget_flags)
{
    UI_layout_t *layout = state->active_layout;
    Assert(layout);

    UI_widget_t *widget = (UI_widget_t *)c_hash_get_value(&layout->widget_hash, name);
    if(!widget)
    {
        widget = c_arena_push_struct(&state->arena, UI_widget_t);
        ZeroStruct(*widget);

        widget->widget_flags = widget_flags;
        widget->ID           = layout->widget_counter++;
        widget->name         = name;
        widget->font_size    = FONT_SIZE;

        widget->idle_color   = state->default_widget_idle_color;
        widget->hot_color    = state->default_widget_hot_color;
        widget->active_color = state->default_widget_active_color;
        widget->font_color   = state->default_widget_text_color;

        c_hash_insert_kv_pair(&layout->widget_hash, name, widget);
    }
    Assert(widget);
    widget->render_color = widget->idle_color;

    widget->next_attached_widget   = null;
    widget->prev_attached_widget   = null;
    widget->first_attached_widget  = null;
    widget->oldest_attached_widget = null;
    if((widget_flags & UIWF_DrawText) != 0)
    {
        widget->render_font = s_asset_font_get_at_size(state->asset_manager, 
                                                       state->DEBUG_font,
                                                       widget->font_size);
    }
    widget->panel_offset = vec2_subtract(layout->next_widget_cursor, layout->panel_position);
    layout->last_widget_height = widget->panel_size.y;

    if(!layout->row_pushed) layout->next_widget_cursor.y -= (widget->panel_size.y + (state->widget_padding_y * 2.0f));
    if(layout->active_parent_widget)
    {
        ui_widget_attach(layout, widget);
    }

    return(widget);
}

// TODO(Sleepster): How about we just make these get the interaction_info themselves...
internal void
ui_widget_do_interactable(UI_state_t *state, UI_widget_t *widget, UI_interaction_data_t *interaction_info)
{
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
}

internal bool8
ui_widget_labeled_button(UI_state_t *state, string_t name)
{
    bool8 result = false;

    UI_widget_t *widget = ui_widget_create(state, 
                                           name, 
                                           UIWF_Clickable|
                                           UIWF_DrawBorder|
                                           UIWF_DrawText|
                                           UIWF_FilledBox|
                                           UIWF_HotAnimation|
                                           UIWF_ActiveAnimation); 
    UI_interaction_data_t *interaction_info = ui_widget_get_interaction_data(state, widget);
    ui_widget_do_interactable(state, widget, interaction_info);

    result = interaction_info->pressed;
    return(result);
}

internal UI_widget_t*
ui_widget_titled_window(UI_state_t  *state, 
                        UI_layout_t *layout, 
                        string_t     title, 
                        vec2_t      *position, 
                        u32          layout_flags)
{
    // NOTE(Sleepster): UIWF_DrawInBackground doesn't mean "draw a rect" it just 
    // means that the layer is of that of the background 

    ui_widget_set_default_idle_color(state, {0.01, 0.01, 0.01, 0.03});
    UI_widget_t *window_pane = ui_widget_create(state, 
                                                STR("LAYOUT WINDOW"), 
                                                UIWF_DrawInBackground);
    if((layout_flags & UILF_Movable) != 0 && 
       s_input_manager_is_alt_key_down(s_input_manager_get_primary_controller(state->input_manager))) 
    {
        UI_interaction_data_t *interaction_info = ui_widget_get_interaction_data(state, window_pane);
        ui_widget_do_interactable(state, window_pane, interaction_info);

        if(interaction_info->pressed)
        {
            layout->drag_offset = vec2_subtract(*position, state->mouse_pos);
        }

        if (interaction_info->clicked)
        {
            *position = vec2_add(state->mouse_pos, layout->drag_offset);
        }
    }
    layout->panel_position = *position;

    // NOTE(Sleepster): This '5.0f' is just there to solidify alignment
    layout->next_widget_cursor.x  = layout->panel_position.x + state->widget_padding_x;
    layout->next_widget_cursor.y  = (layout->panel_position.y - (state->widget_padding_y * 5.0f)) + window_pane->panel_size.y;

    ui_widget_set_default_idle_color(state, COLOR_WHITE);
    ui_widget_set_default_text_color(state, COLOR_BLACK);

    ui_layout_row_push(state, layout);
    ui_widget_push_parent(layout, window_pane);
    if((layout_flags & UILF_Closeable) != 0)
    {
        string_t widget_icon = {};
        if(layout->layout_toggle)
        {
            widget_icon = STR("X");
            if(ui_widget_labeled_button(state, widget_icon))
            {
                layout->layout_toggle = false;
            }
        }
        else
        {
            widget_icon = STR("▼");
            if(ui_widget_labeled_button(state, widget_icon))
            {
                layout->layout_toggle = true;
            }
        }
    }

    ui_widget_set_default_text_color(state, COLOR_WHITE);
    if((layout_flags & UILF_HasTitlebar) != 0)
    {
        layout->next_widget_cursor.x += state->widget_padding_x * 4;
        UI_widget_t *title_text = ui_widget_text(state, title); 
        ui_widget_rect(state, title_text->panel_size, {0, 0, 0, 1});
    }
    layout->next_widget_cursor.x -= state->widget_padding_x * 4;

    ui_widget_pop_parent(layout);
    ui_layout_row_pop(state, layout);

    return(window_pane);
}

internal UI_widget_t*
ui_layout_get_widget(UI_layout_t *layout, string_t hash_id)
{
    UI_widget_t *result = null;
    result = (UI_widget_t*)c_hash_get_value(&layout->widget_hash, hash_id);

    return(result);
}

internal inline UI_widget_t*
ui_widget_pane(UI_state_t *state, string_t name)
{
    UI_widget_t *widget = ui_widget_create(state, 
                                           name, 
                                           UIWF_DrawInBackground); 
    return(widget);
}

internal inline UI_widget_t*
ui_widget_text(UI_state_t *state, string_t display_text)
{
    UI_widget_t *widget = ui_widget_create(state, 
                                           display_text,
                                           UIWF_DrawText);
    return(widget);
}

internal inline UI_widget_t*
ui_widget_rect(UI_state_t *state, vec2_t size, vec4_t color)
{
    UI_widget_t *widget = ui_widget_create(state,
                                           STR("WIDGET PANE TITLE PANEL"),
                                           UIWF_FilledBox);
    widget->panel_size   = size;
    widget->render_color = color;

    return(widget);
}

internal bool8
ui_widget_toggle_box(UI_state_t *state, string_t hash_name, vec2_t size, bool8 *condition)
{
    bool8 result = false;
    UI_widget_t *widget = ui_widget_create(state,
                                           hash_name,
                                           UIWF_FilledBox|
                                           UIWF_Clickable|
                                           UIWF_DrawBorder|
                                           UIWF_HotAnimation|
                                           UIWF_ActiveAnimation);
    widget->panel_size = size;

    UI_interaction_data_t *interaction_info = ui_widget_get_interaction_data(state, widget);
    ui_widget_do_interactable(state, widget, interaction_info);
    result = interaction_info->pressed;
    if(result)
    {
        bool8 prev_condition = *condition;
        *condition = !prev_condition;
    }

    if(*condition)
    {
        widget->is_active = true;
    }

    return(result);
}

internal void
ui_widget_float_slider(UI_state_t *state, float32 *value, float32 min, float32 max)
{
    UI_widget_t *widget = ;
}

internal true_inline void
ui_widget_set_pane_offset(UI_widget_t *widget, vec2_t pos)
{
    widget->panel_offset = pos;
}

internal true_inline void
ui_widget_set_size(UI_widget_t *widget, vec2_t size)
{
    widget->panel_size = size;
}

internal true_inline void
ui_widget_set_idle_color(UI_widget_t *widget, vec4_t color)
{
    widget->idle_color = color;
}

internal true_inline void
ui_widget_set_hot_color(UI_widget_t *widget, vec4_t color)
{
    widget->hot_color = color;
}

internal true_inline void
ui_widget_set_active_color(UI_widget_t *widget, vec4_t color)
{
    widget->active_color = color;
}

internal true_inline void
ui_widget_set_text_color(UI_widget_t *widget, vec4_t color)
{
    widget->font_color = color;
}

internal true_inline void
ui_widget_set_default_idle_color(UI_state_t *state, vec4_t color)
{
    state->default_widget_idle_color = color;
}

internal true_inline void
ui_widget_set_default_hot_color(UI_state_t *state, vec4_t color)
{
    state->default_widget_hot_color = color;
}

internal true_inline void
ui_widget_set_default_active_color(UI_state_t *state, vec4_t color)
{
    state->default_widget_active_color = color;
}

internal true_inline void
ui_widget_set_default_text_color(UI_state_t *state, vec4_t color)
{
    state->default_widget_text_color = color;
}

