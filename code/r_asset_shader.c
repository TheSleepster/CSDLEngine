/* ========================================================================
   $File: r_asset_shader.c $
   $Date: Sat, 02 Aug 25: 12:12AM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include "r_asset_shader.h"

internal void
r_update_shader_SSBO_data(GPU_shader_t *shader, string_t name, void *data, s64 size)
{
    shader_storage_buffer_t *buffer = null;
    for(u32 buffer_index = 0;
        buffer_index < shader->shader_storage_buffers.capacity;
        ++buffer_index)
    {
        shader_storage_buffer_t *found = (shader_storage_buffer_t *)shader->shader_storage_buffers.data + buffer_index;
        if(c_string_compare(found->name, name))
        {
            buffer = found;
            break;
        }
    }

    if(buffer)
    {
        buffer->data = data;
        buffer->size = size;
    }
    else
    {
        log_error("Attempted to pass data to shader buffer '%s', but could not find it...\n", name.data);
    }
}

internal void
r_update_shader_UBO_data(GPU_shader_t *shader, string_t name, void *data)
{
    shader_storage_buffer_t *buffer = null;
    for(u32 buffer_index = 0;
        buffer_index < shader->shader_uniform_buffers.capacity;
        ++buffer_index)
    {
        shader_storage_buffer_t *found = (shader_storage_buffer_t *)shader->shader_uniform_buffers.data + buffer_index;
        if(c_string_compare(found->name, name))
        {
            buffer = found;
            break;
        }
    }

    if(buffer)
    {
        buffer->data = data;
    }
    else
    {
        log_error("Attempted to update data for UBO '%s', but failed to find a UBO with that name...\n");
    }
}

internal void
r_update_shader_uniform_data(GPU_shader_t *shader, string_t name, void *data)
{
    shader_uniform_t *uniform = null;
    for(u32 uniform_index = 0;
        uniform_index < shader->shader_uniforms.capacity;
        ++uniform_index)
    {
        shader_uniform_t *found = (shader_uniform_t *)shader->shader_uniforms.data + uniform_index;
        if(c_string_compare(found->name, name))
        {
            uniform = found;
            break;
        }
    }

    if(uniform)
    {
        uniform->data = data;
    }
    else
    {
        log_error("Attempted to supply data to uniform with name '%s', but could not find it...\n");
    }
}

internal void
r_update_shader_gpu_data(GPU_shader_t *shader)
{
    for(u32 uniform_index = 0;
        uniform_index < shader->shader_uniforms.capacity;
        ++uniform_index)
    {
        shader_uniform_t *uniform_data = (shader_uniform_t *)shader->shader_uniforms.data + uniform_index;
        if((uniform_data->update != null) && (uniform_data->data != null))
        {
            uniform_data->update(uniform_data->location_id, uniform_data->data);
        }
        else
        {
            log_error("shader uniform '%s' has an 'update' pointer of '%ull' and a 'data' pointer of '%ull'... cannot update...\n",
                      uniform_data->name.data,
                      uniform_data->update,
                      uniform_data->data);
        }
    }

    for(u32 uniform_buffer_index = 0;
        uniform_buffer_index < shader->shader_uniform_buffers.capacity;
        ++uniform_buffer_index)
    {
        shader_storage_buffer_t *buffer = (shader_storage_buffer_t*)shader->shader_uniform_buffers.data + uniform_buffer_index;
        if((buffer->update != null) && (buffer->data != null))
        {
            buffer->update(buffer->location_id, buffer->size, buffer->data);
        }
        else
        {
            log_error("shader uniform buffer object '%s' has an 'update' pointer of '%ull' and a 'data' pointer of '%ull'... cannot update...\n",
                      buffer->name.data,
                      buffer->update,
                      buffer->data);
        }
    }

    for(u32 ssbo_index = 0;
        ssbo_index < shader->shader_storage_buffers.capacity;
        ++ssbo_index)
    {
        shader_storage_buffer_t *buffer = (shader_storage_buffer_t *)shader->shader_storage_buffers.data + ssbo_index;
        if((buffer->update != null) && (buffer->data != null))
        {
            if(buffer->old_size != buffer->size)
            {
                r_resize_SSBO(buffer->location_id, buffer->size);
                buffer->old_size = buffer->size;
            }
            buffer->update(buffer->location_id, buffer->size, buffer->data);
        }
        else
        {
            log_error("shader storage buffer object '%s' has an 'update' pointer of '%ull' and a 'data' pointer of '%ull'... cannot update...\n",
                      buffer->name.data,
                      buffer->update,
                      buffer->data);
        }
    }
}
