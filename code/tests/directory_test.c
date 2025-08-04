/* ========================================================================
   $File: directory_test.c $
   $Date: Sun, 03 Aug 25: 07:46PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */
#include "c_base.h"
#include "c_types.h"
#include "c_math.h"
#include "c_debug.h"
#include "c_memory.h"
#include "c_string.h"
#include "c_array.h"
#include "c_file_api.h"
#include "c_intrinsics.h"
#include "c_hash_table.h"

#include "os_platform_file.h"

#include "c_memory.c"
#include "c_string.c"
#include "c_array.c"
#include "c_file_api.c"

global string_t names[200];
global u32      name_counter;

VISIT_FILES(get_the_files)
{
    names[name_counter] = visit_file_data->full_name;
    name_counter++;
}

int
main()
{
    gc_setup();
    log_info("Testing Recursive Directory Visitor...\n");

    visit_file_data_t visit_info;
    visit_info.function  = get_the_files;
    visit_info.recursive = true;
    os_directory_visit(STR("../run_tree/res"), visit_info);

    getchar();

    return(0);
}
