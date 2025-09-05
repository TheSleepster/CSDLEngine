/* ========================================================================
   $File: s_asset_manager.c $
   $Date: Fri, 01 Aug 25: 11:59PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include "s_asset_manager.h"
#include "c_hash_table.h"
#include "c_array.h"

#include "asset_builder/ab_packer_info.h"

internal void
s_asset_manager_read_asset_file_entries(asset_manager_t                *asset_manager,
                                        string_t                        entry_data,
                                        asset_file_table_of_contents_t *table_of_contents)
{
    byte *first_entry = entry_data.data;
    for(u32 entry_index = 0;
        entry_index < table_of_contents->entry_count;
        ++entry_index)
    {
        asset_package_entry_t entry = {};

        entry.name.count     = *first_entry;
        first_entry         += sizeof(u32);

        entry.name.data      = first_entry;
        first_entry         += entry.name.count + 1;

        entry.filepath.count = *first_entry;
        first_entry         += sizeof(u32);

        entry.filepath.data  = first_entry;
        first_entry         += entry.filepath.count + 1;

        entry.entry_data.count = *((u32*)first_entry);
        first_entry           += sizeof(u32);

        entry.ID      = *((u32*)first_entry);
        first_entry  += sizeof(u32);

        entry.file_ID = *((u32*)first_entry);
        first_entry  += sizeof(u32);

        entry.type    = *((asset_type_t*)first_entry);
        first_entry  += sizeof(asset_type_t);

        entry.data_offset_from_start_of_file = *((u64*)first_entry);
        first_entry  += sizeof(u64);

        asset_slot_t *slot_data    = c_arena_push_struct(&asset_manager->manager_arena, asset_slot_t);
        slot_data->asset_type      = entry.type;
        slot_data->slot_state      = ASS_UNLOADED;
        slot_data->asset_id        = entry.ID;
        slot_data->asset_file_id   = entry.file_ID;
        slot_data->filename        = c_string_make_copy(&asset_manager->manager_arena, entry.name);
        slot_data->system_filepath = c_string_make_copy(&asset_manager->manager_arena, entry.filepath);

        slot_data->asset_file_data_offset = entry.data_offset_from_start_of_file;
        slot_data->asset_file_data_length = entry.entry_data.count;
        switch(slot_data->asset_type)
        {
            case AT_BITMAP:
            {
                c_hash_insert_kv_pair(&asset_manager->texture_catalog.texture_hash, slot_data->filename, slot_data);
            }break;
            case AT_SHADER:
            {
                c_hash_insert_kv_pair(&asset_manager->shader_catalog.shader_hash,  slot_data->filename, slot_data);
            }break;
            case AT_FONT:
            {
                c_hash_insert_kv_pair(&asset_manager->font_catalog.font_hash,      slot_data->filename, slot_data);
            }break;
            case AT_SOUND:
            {
                c_hash_insert_kv_pair(&asset_manager->sound_catalog.sound_hash,    slot_data->filename, slot_data);
            }break;
            default:
            {
                InvalidCodePath;
            }break;
        }
    }
}

internal void
s_asset_manager_init(asset_manager_t *asset_manager, string_t packed_asset_filepath)
{
    asset_manager->texture_memory_capacity = MB(100);
    asset_manager->shader_memory_capacity  = MB(200);
    asset_manager->font_memory_capacity    = MB(500);
    asset_manager->sound_memory_capacity   = MB(500);

    asset_manager->manager_arena =  c_arena_create(MB(200));
    asset_manager->trash_arena   = &global_context.temporary_arena;
    
    void *memory              = c_arena_push_size(&asset_manager->manager_arena, sizeof(asset_slot_t)   * MANAGER_HASH_TABLE_SIZE);
    void *memory2             = c_arena_push_size(&asset_manager->manager_arena, sizeof(asset_slot_t)   * MANAGER_HASH_TABLE_SIZE);
    void *memory3             = c_arena_push_size(&asset_manager->manager_arena, sizeof(asset_slot_t)   * MANAGER_HASH_TABLE_SIZE);
    void *memory4             = c_arena_push_size(&asset_manager->manager_arena, sizeof(asset_slot_t)   * MANAGER_HASH_TABLE_SIZE);

    void *texture_view_memory = c_arena_push_size(&asset_manager->manager_arena, sizeof(texture_view_t) * MAX_TEXTURE_VIEWS);

    asset_manager->texture_catalog.texture_allocator = c_za_create(asset_manager->texture_memory_capacity);
    asset_manager->texture_catalog.texture_hash      = c_hash_table_create(memory, MANAGER_HASH_TABLE_SIZE, asset_slot_t*);
    asset_manager->texture_catalog.texture_views     = c_array_create_from_base(texture_view_memory, texture_view_t, MAX_TEXTURE_VIEWS);
    asset_manager->texture_catalog.primary_handler   = at_atlas_handler_create(asset_manager, asset_manager->texture_catalog.texture_allocator, ENGINE_ATLAS_SIZE, ENGINE_ATLAS_SIZE); 

    asset_manager->shader_catalog.shader_allocator   = c_za_create(asset_manager->shader_memory_capacity);
    asset_manager->shader_catalog.shader_hash        = c_hash_table_create(memory2, MANAGER_HASH_TABLE_SIZE, asset_slot_t*);

    FT_Error error = FT_Init_FreeType(&asset_manager->font_catalog.font_lib);
    if(error != 0)
    {
        Assert(false);
    }
    
    asset_manager->font_catalog.font_allocator = c_za_create(asset_manager->font_memory_capacity);
    asset_manager->font_catalog.font_hash      = c_hash_table_create(memory3, MANAGER_HASH_TABLE_SIZE, asset_slot_t*);

    asset_manager->sound_catalog.sound_allocator = c_za_create(asset_manager->sound_memory_capacity);
    asset_manager->sound_catalog.sound_hash      = c_hash_table_create(memory4, MANAGER_HASH_TABLE_SIZE, asset_slot_t*);

    // NOTE(Sleepster): Read asset file data
    {
        asset_manager->asset_file_handle = c_file_open(packed_asset_filepath, false);
        asset_manager->asset_file_data   = c_file_read(packed_asset_filepath, sizeof(asset_file_header_t), 0);

        asset_file_header_t *header = (asset_file_header_t *)asset_manager->asset_file_data.data;
        Assert(header->magic_value == ASSET_FILE_MAGIC_VALUE('W', 'A', 'D', ' '));

        string_t table_data = c_file_read(packed_asset_filepath, sizeof(asset_file_table_of_contents_t), header->offset_to_table_of_contents);
        asset_file_table_of_contents_t *table_of_contents = (asset_file_table_of_contents_t *)table_data.data; 
        Assert(table_of_contents->magic_value == ASSET_FILE_MAGIC_VALUE('t', 'o', 'c', 'd'));

        u32 first_entry_offset = header->offset_to_table_of_contents + sizeof(asset_file_table_of_contents_t);
        string_t entry_data    = c_file_read(packed_asset_filepath, READ_TO_END, first_entry_offset);

        s_asset_manager_read_asset_file_entries(asset_manager, entry_data, table_of_contents);
    }

    //s_asset_manager_generate_null_views(asset_manager);
}

typedef struct s_asset_system_work_data
{
    asset_manager_t    *asset_manager;
    zone_allocator_t   *zone;
    asset_slot_t       *slot_data;
    za_allocation_tag_t tag;
   
    bool8               is_reloading;
    string_t           *out_string;
}s_asset_system_work_data_t;

void
s_asset_load_data_async(void *user_data)
{
    s_asset_system_work_data_t *work_data = (s_asset_system_work_data_t*)user_data;
    
    string_t result = {};
    if(os_mutex_lock(&work_data->zone->mutex))
    {
        if(work_data->slot_data->asset_file_data_offset > 0 &&
           !work_data->is_reloading)
        {
            result = c_file_read_za(work_data->zone,
                                    work_data->asset_manager->asset_file_handle.filepath,
                                    work_data->slot_data->asset_file_data_length,
                                    work_data->slot_data->asset_file_data_offset,
                                    work_data->tag);
        }
        else if(work_data->is_reloading || work_data->slot_data->asset_file_data_offset == 0)
        {
            Assert(work_data->slot_data->filename.data != null);
            result = c_file_read_za(work_data->zone,
                                    work_data->slot_data->system_filepath,
                                    READ_ENTIRE_FILE,
                                    0,
                                    work_data->tag);
        }

        if(c_string_is_valid(result))
        {
            *work_data->out_string = result;
             work_data->slot_data->slot_state = ASS_LOADED;
        }
        else
        {
            log_warning("Asset data for file '%s' is not yet loaded...\n", work_data->slot_data->filename.data);
        }

        c_za_free(work_data->zone, work_data);
        os_mutex_unlock(&work_data->zone->mutex);
    }
}

// IMPORTANT(Sleepster): ALL ASSET LOADING SHOULD CALL BACK TO THIS FUNCTION 
internal void 
s_asset_load_data_from_asset_file_or_path(asset_manager_t    *asset_manager,
                                          string_t           *out_data,
                                          zone_allocator_t   *zone,
                                          asset_slot_t       *slot_data,
                                          za_allocation_tag_t tag,
                                          bool8               is_reloading = false)
{
    multithreading_work_queue_t *high_priority_queue = &asset_manager->queue_manager->high_priority_queue;
    slot_data->slot_state = ASS_QUEUED;

    s_asset_system_work_data_t *work_data = c_za_push_struct(zone, s_asset_system_work_data_t, tag);
    work_data->asset_manager =  asset_manager;
    work_data->zone          =  zone;
    work_data->slot_data     =  slot_data;
    work_data->tag           =  tag;
    work_data->out_string    =  out_data;
    work_data->is_reloading  =  is_reloading;
    s_work_queue_add_entry(high_priority_queue, &s_asset_load_data_async, work_data);
}

