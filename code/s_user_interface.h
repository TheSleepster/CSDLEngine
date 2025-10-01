#if !defined(S_USER_INTERFACE_H)
/* ========================================================================
   $File: s_user_interface.h $
   $Date: September 29 2025 10:02 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#define S_USER_INTERFACE_H

typedef enum UI_widget_flags
{
    UIWF_Clickable       = (1 << 0),
    UIWF_ViewScroll      = (1 << 1),
    UIWF_DrawText        = (1 << 2),
    UIWF_DrawBorder      = (1 << 3),
    UIWF_DrawBackground  = (1 << 4),
    UIWF_DrawDropShadow  = (1 << 5),
    UIWF_Clip            = (1 << 6),
    UIWF_HotAnimation    = (1 << 7),
    UIWF_ActiveAnimation = (1 << 8),
}UI_widget_flags_t;

typedef struct UI_widget
{
    u32        ID;
    u32        widget_flags;
    u32        last_used_frame_index;

    bool8      is_hot;
    bool8      is_active;

    string_t   name;
    vec2_t     position;
    vec2_t     size;
    vec4_t     color;
    rectangle2 widget_rect;

    UI_widget *parent_widget;
    UI_widget *first_attached_widget;
    UI_widget *last_attached_widget;
    UI_widget *next_attached_widget;
    UI_widget *prev_attached_widget;
}UI_widget_t;

typedef struct UI_interaction_data
{
    UI_widget_t *widget;
    u64          last_interacted_frame;

    bool8        clicked;
    bool8        double_clicked;
    bool8        right_clicked;
    bool8        pressed;
    bool8        released;
    bool8        dragging;
    bool8        hovering;
}UI_interaction_data_t;

typedef struct UI_layout
{
    bool8            is_valid;
    memory_arena_t  *arena;

    input_manager_t *input_manager;

    hash_table_t     widget_hash;
    hash_table_t     interaction_hash;
    u32              widget_counter;
    UI_widget_t     *active_parent_widget;
    UI_widget_t     *first_attached_widget;

    UI_layout      *next_layout;
}UI_layout_t;

typedef struct UI_state
{
    bool8            is_valid;
    memory_arena_t   arena;
    input_manager_t *input_manager;
    vec2_t           mouse_pos;

    u32              current_frame;

    UI_layout_t     *first_layout;
    u32              layout_counter;

    UI_widget_t     *hot_widget;
    UI_widget_t     *active_widget;

    UI_layout_t     *active_layout;

    render_group_desc_t background_desc;
    render_group_desc_t widget_desc;
    render_group_desc_t text_desc;
}UI_state_t;

internal void                   ui_init_state(render_state_t *render_state, input_manager_t *input_manager, UI_state_t *state);
internal void                   ui_deinit_state(UI_state_t *state);

/*===========================================
  ================ WIDGET API ===============
  ===========================================*/
internal UI_layout_t*           UI_create_new_layout(UI_state_t *state);
internal void                   ui_layout_begin(UI_state_t *state);
internal void                   ui_layout_end(UI_state_t *state);

internal UI_widget_t*           ui_widget_create(UI_layout_t *layout, string_t name, u32 widget_flags);
internal void                   ui_widget_attach(UI_layout_t *layout, UI_widget_t *widget);
internal void                   ui_widget_push_parent(UI_layout_t *layout, UI_widget_t *widget);
internal void                   ui_widget_pop_parent(UI_layout_t *layout);
internal UI_interaction_data_t* ui_widget_get_interaction_data(UI_layout_t *layout, UI_widget_t *widget);

internal bool8                  ui_widget_button(UI_state_t *state, string_t name);
internal UI_widget_t*           ui_widget_pane(UI_state_t *state, string_t name);

internal void                   ui_render_all_widgets(render_state_t *render_state, UI_state_t *state);
internal void                   ui_resolve_layouts(UI_state_t *state);

#endif // S_USER_INTERFACE_H

