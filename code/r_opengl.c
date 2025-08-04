/* ========================================================================
   $File: r_opengl.c $
   $Date: Wed, 23 Jul 25: 01:39PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include "r_renderer_data.h"

UPDATE_GPU_SHADER_STORAGE_BUFFER_OBJECT(gl_update_ssbo)
{
    Assert(data != null);
    if(size)
    {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, location_id);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER,  0, size, data);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }
    else
    {
        log_warning("We are skipping the update of this buffer since the size is 0...\n");
    }
}

UPDATE_GPU_UNIFORM_BUFFER_OBJECT(gl_update_ubo)
{
    Assert(data != null);
    Assert(size != 0);

    glBindBufferBase(GL_UNIFORM_BUFFER, 0, location_id);
    glBufferSubData(GL_UNIFORM_BUFFER,  0, size, data);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

UPDATE_SHADER_UNIFORM(gl_update_integer_uniform)
{
    Assert(data != null);

    s32 *value = (s32*)data;
    glUniform1i(location_id, *value);
}

UPDATE_SHADER_UNIFORM(gl_update_unsigned_integer_uniform)
{
    Assert(data != null);

    u32 *value = (u32*)data;
    glUniform1ui(location_id, *value);
}

UPDATE_SHADER_UNIFORM(gl_update_float1_uniform)
{
    Assert(data != null);

    float32 *value = (float32*)data;
    glUniform1f(location_id, *value);
}

UPDATE_SHADER_UNIFORM(gl_update_float_vector2_uniform)
{
    Assert(data != null);

    vec2_t *value = (vec2_t*)data;
    glUniform2f(location_id, value->x, value->y);
}

UPDATE_SHADER_UNIFORM(gl_update_float_vector3_uniform)
{
    Assert(data != null);

    vec3_t *value = (vec3_t*)data;
    glUniform3f(location_id, value->x, value->y, value->z);
}

UPDATE_SHADER_UNIFORM(gl_update_float_vector4_uniform)
{
    Assert(data != null);

    vec4_t *value = (vec4_t*)data;
    glUniform4f(location_id, value->x, value->y, value->z, value->w);
}

UPDATE_SHADER_UNIFORM(gl_update_float_mat3_uniform)
{
    Assert(data != null);

    mat3_t *value = (mat3_t*)data;
    glUniformMatrix3fv(location_id, 1, false, value->values);
}

UPDATE_SHADER_UNIFORM(gl_update_float_mat4_uniform)
{
    Assert(data != null);

    mat4_t *value = (mat4_t*)data;
    glUniformMatrix4fv(location_id, 1, false, value->values);
}

internal void
r_resize_SSBO(s32 location_id, s64 new_size)
{
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, location_id);
    glBufferData(GL_SHADER_STORAGE_BUFFER, new_size, null, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

internal void
r_assign_uniform_type(shader_uniform_t *uniform, GLenum type)
{
    switch(type)
    {
        case GL_INT:
        {
            uniform->type   = SUT_INTEGER;
            uniform->update = gl_update_integer_uniform;
        }break;
        case GL_UNSIGNED_INT:
        {
            uniform->type   = SUT_UNSIGNED_INTEGER;
            uniform->update = gl_update_unsigned_integer_uniform;
        }break;
        case GL_FLOAT:
        {
            uniform->type   = SUT_FLOAT;
            uniform->update = gl_update_float1_uniform;
        }break;
        case GL_FLOAT_VEC2:
        {
            uniform->type   = SUT_FLOAT_VECTOR2;
            uniform->update = gl_update_float_vector2_uniform;
        }break;
        case GL_FLOAT_VEC3:
        {
            uniform->type   = SUT_FLOAT_VECTOR3;
            uniform->update = gl_update_float_vector3_uniform;
        }break;
        case GL_FLOAT_VEC4:
        {
            uniform->type   = SUT_FLOAT_VECTOR4;
            uniform->update = gl_update_float_vector4_uniform;
        }break;
        case GL_FLOAT_MAT3:
        {
            uniform->type   = SUT_FLOAT_MATRIX3;
            uniform->update = gl_update_float_mat3_uniform;
        }break;
        case GL_FLOAT_MAT4:
        {
            uniform->type   = SUT_FLOAT_MATRIX4;
            uniform->update = gl_update_float_mat4_uniform;
        }break;
        case GL_SAMPLER_2D:
        {
            uniform->type = SUT_TEXTURE_BINDING;
        }break;
        case GL_SAMPLER_2D_ARRAY:
        {
            uniform->type = SUT_TEXTURE_ARRAY;
        }break;
        default:
        {
            uniform->type   = SUT_INVALID;
            uniform->update = null;
        }break;
    }
}

internal GLuint
r_make_shader_object(string_t shader_source, string_t shader_prefix, GLenum shader_type)
{
    u32 shader_object = glCreateShader(shader_type);
    
    char *shaders[2];
    s32   lengths[2];

    shaders[0] = C_STR(shader_prefix);
    shaders[1] = C_STR(shader_source);
    lengths[0] = shader_prefix.count;
    lengths[1] = shader_source.count;

    glShaderSource(shader_object, 2, shaders, lengths);
    glCompileShader(shader_object);

    u32 success = 0;
    glGetShaderiv(shader_object, GL_COMPILE_STATUS, &success);
    if(!success)
    {
        char log_data[512];
        glGetShaderInfoLog(shader_object, 512, null, log_data);
        log_error("[SHADER COMP ERROR]: %s\n", log_data);

        glDeleteShader(shader_object);
        return(0);
    }

    return(shader_object);
}

internal void
r_compile_pixel_shader(GPU_shader_t *shader, string_t shader_source)
{
    string_t vertex_shader_prefix   = STR("#version 430 core\n#define VERTEX_SHADER\n");
    string_t fragment_shader_prefix = STR("#version 430 core\n#define FRAGMENT_SHADER\n");

    u32 vertex_shader   = r_make_shader_object(shader_source, vertex_shader_prefix,   GL_VERTEX_SHADER);
    u32 fragment_shader = r_make_shader_object(shader_source, fragment_shader_prefix, GL_FRAGMENT_SHADER);
    shader->program_id  = glCreateProgram();

    glAttachShader(shader->program_id, vertex_shader);
    glAttachShader(shader->program_id, fragment_shader);
    glLinkProgram(shader->program_id);

    u32 success = 0;
    glGetProgramiv(shader->program_id, GL_LINK_STATUS, &success);
    if(!success)
    {
        char log_data[512];
        glGetProgramInfoLog(shader->program_id, 512, null, log_data);
        log_fatal("\n[SHADER COMP ERROR]: %s\n\n", log_data);

        glDeleteProgram(shader->program_id);
        shader->program_id = 0;
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
}

internal GPU_shader_t
r_create_shader_program(string_t shader_source, gpu_shader_type_t shader_type)
{
    GPU_shader_t result;
    result.shader_source = shader_source;
    
    switch(shader_type)
    {
        case ST_PIXEL_SHADER:
        {
            r_compile_pixel_shader(&result, shader_source);
        }break;
        case ST_COMPUTE_SHADER:
        {
            ZeroStruct(result);
            log_fatal("Implement compute shader handling");
        }break;
    }

    if(result.program_id != 0)
    {
        // SHADER UNIFORM BUFFER OBJECTS
        {
            s32 UBO_count = 0;
            glGetProgramiv(result.program_id, GL_ACTIVE_UNIFORM_BLOCKS, &UBO_count);
            if(UBO_count != 0)
            {
                result.shader_uniform_buffers = c_array_create(shader_storage_buffer_t, UBO_count);
                for(s32 ubo_index = 0;
                    ubo_index < UBO_count;
                    ++ubo_index)
                {
                    shader_storage_buffer_t *ubo_data = (shader_storage_buffer_t*)result.shader_uniform_buffers.data + ubo_index;
                    glGetActiveUniformBlockiv(result.program_id, ubo_index, GL_UNIFORM_BLOCK_NAME_LENGTH, &ubo_data->name.count);

                    ubo_data->name.count -= 1;
                    ubo_data->name.data   = malloc(sizeof(u8) * ubo_data->name.count);
                    ubo_data->data        = null;
                    glGetActiveUniformBlockName(result.program_id, ubo_index, ubo_data->name.count + 1, null, ubo_data->name.data);

                    glGetActiveUniformBlockiv(result.program_id, ubo_index, GL_UNIFORM_BLOCK_DATA_SIZE, &ubo_data->size);
                    glGetActiveUniformBlockiv(result.program_id, ubo_index, GL_UNIFORM_BLOCK_BINDING,   &ubo_data->binding);

                    glGenBuffers(1, &ubo_data->location_id);
                    glBindBuffer(GL_UNIFORM_BUFFER, ubo_data->location_id);
                    glBufferData(GL_UNIFORM_BUFFER, ubo_data->size, null, GL_DYNAMIC_DRAW);
                    glBindBuffer(GL_UNIFORM_BUFFER, 0);

                    ubo_data->update = gl_update_ubo;
                }
            }
            else
            {
                ZeroStruct(result.shader_uniform_buffers);
            }
        }

        // SHADER UNIFORMS
        {
            s32 shader_uniform_count = 0;
            glGetProgramiv(result.program_id, GL_ACTIVE_UNIFORMS, &shader_uniform_count);
            if(shader_uniform_count != 0)
            {
                result.shader_uniforms = c_array_create(shader_uniform_t, shader_uniform_count);

                s32 true_uniform_index = 0;
                for(s32 uniform_index = 0;
                    uniform_index < shader_uniform_count;
                    ++uniform_index)
                {
                    shader_uniform_t *uniform_data = (shader_uniform_t *)result.shader_uniforms.data + true_uniform_index;

                    char   buffer[256];
                    memset(buffer, 0, sizeof(u8) * 256);

                    s32    name_length;
                    s32    uniform_size;
                    GLenum uniform_type;
                    glGetActiveUniform(result.program_id, uniform_index, 256, &name_length, &uniform_size, &uniform_type, buffer);
                    r_assign_uniform_type(uniform_data, uniform_type);

                    uniform_data->name.data  = malloc(sizeof(u8) * name_length);
                    uniform_data->name.count = name_length;
                    uniform_data->size = uniform_size;
                    uniform_data->data = null;
                    memcpy(uniform_data->name.data, buffer, name_length * sizeof(u8));

                    uniform_data->name.data[uniform_data->name.count] = '\0';

                    uniform_data->location_id = glGetUniformLocation(result.program_id, uniform_data->name.data);
                    if(uniform_data->location_id != -1)
                    {
                        true_uniform_index += 1;
                    }
                    else
                    {
                        log_info("Skipping uniform '%s' as it is a member of a UBO...\n", uniform_data->name.data);
                        ZeroStruct(*uniform_data);
                    }
                }
            }
            else
            {
                ZeroStruct(result.shader_uniforms);
            }
        }

        // SHADER STORAGE BUFFER OBJECTS
        {
            s32 ssbo_count = 0;
            glGetProgramInterfaceiv(result.program_id, GL_SHADER_STORAGE_BLOCK, GL_ACTIVE_RESOURCES, &ssbo_count);
            if(ssbo_count != 0)
            {
                result.shader_storage_buffers = c_array_create(shader_storage_buffer_t, ssbo_count);
                for(s32 buffer_index = 0;
                    buffer_index < ssbo_count;
                    ++buffer_index)
                {
                    shader_storage_buffer_t *working_buffer = (shader_storage_buffer_t*)result.shader_storage_buffers.data + buffer_index;
                    char buffer[256];
                    s32  name_length;
                    glGetProgramResourceName(result.program_id, GL_SHADER_STORAGE_BLOCK, buffer_index, 256, &name_length, buffer);

                    working_buffer->name.data  = malloc(sizeof(u8) * name_length);
                    working_buffer->name.count = name_length;
                    working_buffer->data       = null;
                    memset(working_buffer->name.data, 0, sizeof(u8) * name_length);
                    memcpy(working_buffer->name.data, buffer, sizeof(u8) * name_length);

                    GLenum property = GL_BUFFER_BINDING;
                    glGetProgramResourceiv(result.program_id, GL_SHADER_STORAGE_BLOCK, buffer_index, 1, &property, 1, null, &working_buffer->binding);

                    working_buffer->update   = gl_update_ssbo;
                    working_buffer->old_size = 0;

                    glGenBuffers(1, &working_buffer->location_id);
                }
            }
            else
            {
                ZeroStruct(result.shader_storage_buffers);
            }
        }
    }
    else
    {
        log_error("shader program_id is invalid...\n");
    }

    return(result);
}

internal void
r_init_renderer_data(SDL_Window *window, render_state_t *render_state)
{
    render_state->draw_frame_arena = c_arena_create(MB(100));
    Assert(render_state->draw_frame_arena.base != null);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,  1);
    SDL_GLContext context = SDL_GL_CreateContext(window);

    bool8 success = SDL_GL_MakeCurrent(window, context);
    if(success)
    {
        gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);
        log_info("GL Context created... Loading GL functions...\n");
    }
    else
    {
        log_fatal("Failed to apply the GL context...\n");
    }

    glEnable(GL_FRAMEBUFFER_SRGB);
    SDL_GL_SetSwapInterval(0);

    u32 index_buffer[MAX_INDICES];
    u32 index_offset = 0;

    for(u32 index = 0;
        index < MAX_INDICES;
        index += 6)
    {
        index_buffer[index + 0] = index_offset + 0;
        index_buffer[index + 1] = index_offset + 1;
        index_buffer[index + 2] = index_offset + 2;
        index_buffer[index + 3] = index_offset + 0;
        index_buffer[index + 4] = index_offset + 2;
        index_buffer[index + 5] = index_offset + 3;

        index_offset += 4;
    }

    glGenVertexArrays(1, &render_state->primary_vao_id);
    glBindVertexArray(render_state->primary_vao_id);

    glGenBuffers(1, &render_state->primary_vbo_id);
    glBindBuffer(GL_ARRAY_BUFFER, render_state->primary_vbo_id);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_t) * MAX_VERTICES, null, GL_STREAM_DRAW);

    glGenBuffers(1, &render_state->primary_ebo_id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, render_state->primary_ebo_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(u32) * MAX_INDICES, index_buffer, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)OffsetOf(vertex_t, vPosition));
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)OffsetOf(vertex_t, vUVData));
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)OffsetOf(vertex_t, vColor));
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(vertex_t), (void*)OffsetOf(vertex_t, vVSNormals));

    glVertexAttribIPointer(4, 1, GL_UNSIGNED_INT,   sizeof(vertex_t), (void*)OffsetOf(vertex_t, vTextureIndex));

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    string_t lighting_shader  = c_file_read(STR("../code/shaders/lighting.glsl"));
    render_state->lighting_data.lighting_shader = r_create_shader_program(lighting_shader, ST_PIXEL_SHADER);

    string_t shader_source    = c_file_read(STR("../code/shaders/basic.glsl"));
    render_state->test_shader = r_create_shader_program(shader_source, ST_PIXEL_SHADER);

    // LIGHTING FRAMEBUFFER SETUP
    {
        glGenFramebuffers(1, &render_state->lighting_data.framebuffer_id);
        glBindFramebuffer(GL_FRAMEBUFFER, render_state->lighting_data.framebuffer_id);

        glGenTextures(1, &render_state->lighting_data.color_attachment_0);
        glBindTexture(GL_TEXTURE_2D, render_state->lighting_data.color_attachment_0);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 
                     LIGHTMAP_SIZE, LIGHTMAP_SIZE, 0, 
                     GL_RGBA, GL_UNSIGNED_BYTE, null);


        glFramebufferTexture2D(GL_FRAMEBUFFER,
                               GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_2D,
                               render_state->lighting_data.color_attachment_0,
                               0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
} 

internal void
r_render_single_frame(render_state_t *render_state)
{
    glEnable(GL_BLEND);

    r_handle_lighting_data(render_state);
    // LIGHTING
    {
        glBindFramebuffer(GL_FRAMEBUFFER, render_state->lighting_data.framebuffer_id);
        glDisable(GL_DEPTH_TEST);

        glBlendFunc(GL_ONE, GL_ONE);
        glBlendEquation(GL_FUNC_ADD);
    
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    
        glUseProgram(render_state->lighting_data.lighting_shader.program_id);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // SHADOWS
    {
        glBlendFunc(GL_ONE, GL_ONE);
        glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
    }

    glEnable(GL_DEPTH_TEST);
    glClearDepth(0.0f);
    glDepthFunc(GL_GREATER);
    glDepthMask(GL_TRUE);

    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);
    
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    r_handle_renderpass_data(render_state);

    mat4_t projection_matrix = mat4_RHGL_ortho(-160, 160, -90, 90, -1, 1);
    mat4_t view_matrix       = mat4_identity();

    r_update_shader_uniform_data(&render_state->test_shader, STR("uProjectionMatrix"), &projection_matrix.values);
    r_update_shader_uniform_data(&render_state->test_shader, STR("uViewMatrix"), &view_matrix.values);

    glBindVertexArray(render_state->primary_vao_id);
    for(u32 group_index = 0;
        group_index < render_state->draw_frame.render_group_counter;
        ++group_index)
    {
        render_group_t *group = render_state->draw_frame.render_groups[group_index];
        Assert(group);
        
        glUseProgram(group->render_desc.shader->program_id);
        r_update_shader_gpu_data(group->render_desc.shader);

        glBindBuffer(GL_ARRAY_BUFFER, render_state->primary_vbo_id);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertex_t) * group->vertex_count, group->vertex_buffer);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, render_state->primary_ebo_id);
        glDrawElements(GL_TRIANGLES, group->quad_count * 6, GL_UNSIGNED_INT, null);
    }

}
