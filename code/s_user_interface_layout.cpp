/* ========================================================================
   $File: s_user_interface_layout.cpp $
   $Date: October 11 2025 12:11 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

// TODO(Sleepster): Get rid of this
internal UI_layout_t *
UI_create_new_layout(UI_state_t *state)
{
    UI_layout_t *result = null;
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
        result->active_parent_widget  =  result->layout_pane;

        // NOTE(Sleepster): THIS MUST BE DONE LAST        
        result->layout_pane           =  ui_widget_pane(state, STR("DEFAULT WIDGET PANE"));

        UI_layout_t *last_layout = state->first_layout;
        state->first_layout      = result;
        result->next_layout      = last_layout;
    }
    Assert(result);

    return(result);
}

internal inline UI_layout_t* 
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

internal UI_layout_t*
ui_layout_create(UI_state_t *state, string_t layout_title, vec2_t position, vec2_t size, u32 layout_flags)
{
    UI_layout_t *result = null;
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
        result->active_parent_widget  =  result->layout_pane;

        result->is_valid = true;

        UI_layout_t *last_layout = state->first_layout;
        state->first_layout      = result;
        result->next_layout      = last_layout;
    }
    Assert(result);
    result->next_widget_cursor = vec2(position.x + state->widget_padding_x,
                                      position.y + state->widget_padding_y);
    result->layout_pane =  ui_widget_titled_window(state, result, layout_title, position, size, layout_flags);

    return(result);
}

internal UI_layout_t*
ui_layout_begin_titled(UI_state_t *state, string_t title, vec2_t position, vec2_t size, u32 layout_flags)
{
    UI_layout_t *result = ui_layout_create(state, title, position, size, layout_flags);
    if(result)
    {
        state->active_layout = result;
    }

    return(result);
}

// NOTE(Sleepster): Perhaps instead of getting the widget size and position data
// when we go to process the layout data, we instead just compute the size of the widget on creation
internal inline void
ui_layout_row_push(UI_layout_t *layout)
{
    layout->row_pushed = true;
    layout->this_row_y = layout->next_widget_cursor.y;
}

internal inline void
ui_layout_row_pop(UI_layout_t *layout)
{
    layout->row_pushed = true;
    layout->next_widget_cursor.y += layout->last_widget_height + 4;
}

