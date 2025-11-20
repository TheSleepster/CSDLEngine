#if !defined(S_USER_INTERFACE_H)
/* ========================================================================
   $File: s_user_interface.h $
   $Date: September 29 2025 10:02 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#define S_USER_INTERFACE_H

 /* TODO: What widgets do we want?
  *
  *  - [x] UI Text. No box.
  *  - [x] Add titled windows.
  *  - [x] Different button types.
  *  - [x] Support dropdown menus
  *  - [x] Checkboxes
  *  - [ ] Value sliders
  *  - [ ] Scroll bars
  *  - [ ] Resizable widgets
  *  - [ ] Graph boxes
  *  - [ ] Bitmap Containers
  *  - [ ] Seperators
  *  - [ ] Text Seperators
  */

#define FONT_SIZE 16 
#define COLOR_WHITE  ((vec4_t){1.0, 1.0, 1.0, 1.0})
#define COLOR_RED    ((vec4_t){1.0, 0.0, 0.0, 1.0})
#define COLOR_GREEN  ((vec4_t){0.0, 1.0, 0.0, 1.0})
#define COLOR_BLUE   ((vec4_t){0.0, 0.0, 1.0, 1.0})
#define COLOR_BLACK  ((vec4_t){0.0, 0.0, 0.0, 1.0})

typedef enum UI_widget_flags 
{
    UIWF_Clickable        = (1 <<  0),
    UIWF_ViewScroll       = (1 <<  1),
    UIWF_DrawText         = (1 <<  2),
    UIWF_DrawBorder       = (1 <<  3),
    UIWF_DrawInBackground = (1 <<  4),
    UIWF_FilledBox        = (1 <<  5),
    UIWF_DrawDropShadow   = (1 <<  6),
    UIWF_Clip             = (1 <<  7),
    UIWF_HotAnimation     = (1 <<  8),
    UIWF_ActiveAnimation  = (1 <<  9),
}UI_widget_flags_t;

typedef enum UI_layout_flags
{
    UILF_HasTitlebar = (1 << 0),
    UILF_Movable     = (1 << 1),
    UILF_Closeable   = (1 << 2),
}UI_layout_flags_t;

typedef enum UI_slider_value_type
{
    UISV_Integer32,
    UISV_Integer64,
    UISV_Float32,
    UISV_Float64,
}UI_slider_value_type_t;

typedef struct UI_slider_value
{
    u32     value_type;
    void   *value_ptr;

    float64 float_min;
    float64 float_max;

    s64     integer_min;
    s64     integer_max;
}UI_slider_value_t;

// TODO(Sleepster): Allow the user to control the widget depth
typedef struct UI_widget
{
    u32        ID;
    u32        widget_flags;

    bool8      is_hot;
    bool8      is_active;

    bool8      started_inside;
    bool8      is_expanded;

    vec2_t     panel_offset;
    vec2_t     panel_size;
    rectangle2 widget_rect;

    string_t   name;
    vec2_t     string_offset;
    vec2_t     string_size; 
    vec4_t     color;

    u32        font_size;
    u32        widget_icon;

    vec4_t     render_color;
    vec4_t     font_color;
    vec4_t     idle_color;
    vec4_t     hot_color;
    vec4_t     active_color;

    UI_widget *parent_widget;
    UI_widget *first_attached_widget;
    UI_widget *oldest_attached_widget;
    UI_widget *next_attached_widget;
    UI_widget *prev_attached_widget;

    dynamic_render_font_varient_t *render_font;
}UI_widget_t;

typedef struct UI_interaction_data 
{
    UI_widget_t *widget;
    u64          last_hot_frame; 
    u64          last_active_frame;

    bool8        clicked;
    bool8        double_clicked;
    bool8        right_clicked;
    bool8        pressed;
    bool8        released;
    bool8        dragging;
    bool8        hovering;
}UI_interaction_data_t;

/* NOTE(Sleepster): Layouts and standard widgets are different. 
 * A layout's purpose is to store the widgets contained within itself. 
 * A layout is a special widget.
 */
typedef struct UI_layout
{
    bool8            is_valid;
    bool8            layout_toggle;
    bool8            row_pushed; 

    memory_arena_t  *arena;
    input_manager_t *input_manager;

    vec2_t           next_widget_cursor;
    vec2_t           panel_position;
    vec2_t           drag_offset;

    float32          this_row_y;

    float32          largest_widget_width;
    float32          largest_widget_height;

    float32          last_widget_width;
    float32          last_widget_height;

    hash_table_t     widget_hash;
    hash_table_t     interaction_hash;
    u32              widget_counter;

    float32          ui_scale;

    UI_widget_t     *active_parent_widget;
    UI_widget_t     *layout_pane;

    UI_layout       *next_layout;
}UI_layout_t;

// NOTE(Sleepster): UI_manager_t?
typedef struct UI_state
{
    bool8            is_valid;
    bool8            is_interacting;

    memory_arena_t   arena;
    input_manager_t *input_manager;
    asset_manager_t *asset_manager;

    render_camera_t  camera;

    vec2_t           mouse_pos;
    u32              current_frame;

    UI_layout_t     *first_layout;
    u32              layout_counter;

    UI_widget_t     *hot_widget;
    UI_widget_t     *active_widget;

    UI_layout_t     *active_layout;
    asset_handle_t   DEBUG_font;  
    u32              default_font_size;

    float32          widget_padding_x;
    float32          widget_padding_y;
    float32          default_widget_width;
    float32          default_widget_height;

    vec4_t           default_widget_idle_color;
    vec4_t           default_widget_hot_color;
    vec4_t           default_widget_active_color;
    vec4_t           default_widget_text_color;
    vec4_t           default_widget_background_color;
    vec4_t           default_widget_tilebar_color;
    vec4_t           default_widget_border_color;

    render_group_desc_t   background_desc;
    render_group_desc_t   widget_desc;
    render_group_desc_t   text_desc;

    render_group_t       *background_layer;
    render_group_t       *widget_layer;
    render_group_t       *text_layer;
}UI_state_t;

/*===========================================
  ================== CORE API ===============
  ===========================================*/
internal void                   ui_init_state(render_state_t *render_state, input_manager_t *input_manager, asset_manager_t *asset_manager, UI_state_t *state);
internal void                   ui_deinit_state(UI_state_t *state);
internal void                   ui_render_widget(render_state_t *render_state, asset_manager_t *asset_manager, UI_state_t *state, UI_widget_t *widget);
internal void                   ui_render_all_widgets(render_state_t *render_state, asset_manager_t *asset_manager, UI_state_t *state);
internal void                   ui_resolve_layouts(asset_manager_t *asset_manager, UI_state_t *state);

/*===========================================
  ================ LAYOUT API ===============
  ===========================================*/
internal        UI_layout_t*    UI_create_new_layout(UI_state_t *state);
internal        UI_layout_t*    ui_layout_create(UI_state_t *state, string_t layout_title, u32 layout_flags);

internal        UI_layout_t*    ui_layout_begin(UI_state_t *state);
internal        UI_layout_t*    ui_layout_begin_titled(UI_state_t *state, string_t title, vec2_t *position, u32 layout_flags);
internal        void            ui_layout_end(UI_state_t *state);

internal inline void            ui_layout_row_pop(UI_state_t *state, UI_layout_t *layout);
internal inline void            ui_layout_row_push(UI_state_t *state, UI_layout_t *layout);

internal inline UI_widget_t*    ui_layout_get_widget(UI_layout_t *layout, string_t widget_name);

/*===========================================
  ================ WIDGET API ===============
  ===========================================*/
internal UI_widget_t*           ui_widget_create(UI_state_t *state, string_t name, u32 widget_flags);
internal void                   ui_widget_attach(UI_layout_t *layout, UI_widget_t *widget);
internal void                   ui_widget_push_parent(UI_layout_t *layout, UI_widget_t *widget);
internal void                   ui_widget_pop_parent(UI_layout_t *layout);
internal UI_interaction_data_t* ui_widget_get_interaction_data(UI_layout_t *layout, UI_widget_t *widget);

internal true_inline void       ui_widget_set_pane_offset(UI_widget_t *widget, vec2_t pos);
internal true_inline void       ui_widget_set_size(UI_widget_t *widget, vec2_t size);
internal true_inline void       ui_widget_set_idle_color(UI_widget_t *widget, vec4_t color);
internal true_inline void       ui_widget_set_hot_color(UI_widget_t *widget, vec4_t color);
internal true_inline void       ui_widget_set_active_color(UI_widget_t *widget, vec4_t color);

internal true_inline void       ui_widget_set_default_idle_color(UI_state_t *state, vec4_t color);
internal true_inline void       ui_widget_set_default_hot_color(UI_state_t *state, vec4_t color);
internal true_inline void       ui_widget_set_default_active_color(UI_state_t *state, vec4_t color);
internal true_inline void       ui_widget_set_default_text_color(UI_state_t *state, vec4_t color);

// I.M WIDGETS
internal        bool8           ui_widget_button(UI_state_t *state, string_t name, bool8 has_label);
internal        bool8           ui_widget_toggle_box(UI_state_t *state, string_t hash_name, vec2_t size, bool8 *condition);
internal inline UI_widget_t*    ui_widget_pane(UI_state_t *state, string_t name);
internal inline UI_widget_t*    ui_widget_text(UI_state_t *state, string_t display_text);
internal inline UI_widget_t*    ui_widget_rect(UI_state_t *state, string_t hash_name, vec2_t size, vec4_t color);
internal        UI_widget_t*    ui_widget_titled_window(UI_state_t *state, UI_layout_t *layout, string_t title, vec2_t *position, u32 layout_flags);
internal        void            ui_widget_float_slider(UI_state_t *state, string_t slider_name, float32 *value_ptr, float32 min, float32 max);

internal        void            ui_widget_do_interactable(UI_state_t *state, UI_widget_t *widget, UI_interaction_data_t *interaction_info);

#endif // S_USER_INTERFACE_H

