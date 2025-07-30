/* ========================================================================
   $File: c_file_api.c $
   $Date: Fri, 25 Jul 25: 01:25PM $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

internal string_t
c_read_entire_file(string_t filepath)
{
    string_t result = {};
    FILE    *file_handle = fopen(filepath.data, "rb");
    if(file_handle)
    {
        fseek(file_handle, 0, SEEK_END);
        result.count = ftell(file_handle);
        fseek(file_handle, 0, SEEK_SET);

        if(result.count > 0)
        {
            result.data = malloc(sizeof(u8) * result.count);
            Assert(result.data != null);
            memset(result.data, 0, sizeof(u8) * result.count);

            fread(result.data, sizeof(u8), result.count, file_handle);
            fclose(file_handle);
        }
    }
    else
    {
        log_fatal("Failure to read the file: '%s'\n", filepath.data);
        Assert(false);
    }

    return(result);
}
