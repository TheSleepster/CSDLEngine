#if !defined(R_OPENGL_H)
/* ========================================================================
   $File: r_opengl.h $
   $Date: November 17 2025 03:41 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define R_OPENGL_H

typedef struct render_backend_data 
{
    u32 primary_vao_id;
    u32 primary_vbo_id;
    u32 primary_ebo_id;

    struct  
    {
        u32 ID;
        u32 color_attachment0;
        u32 color_attachment1;
        u32 depth_buffer;
    }primary_framebuffer;
}render_backend_data_t;

internal void r_renderer_check_error_(const char *file, s32 line_number);

#if ASSERT_ENABLED 
    #define r_renderer_check_error() r_renderer_check_error_(__FILE__, __LINE__)
#else
    #define r_renderer_check_error()
#endif

#endif // R_OPENGL_H

