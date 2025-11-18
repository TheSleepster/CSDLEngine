#if !defined(C_SORTING_H)
/* ========================================================================
   $File: c_sorting.h $
   $Date: November 17 2025 02:35 pm $
   $Revision: $
   $Creator: Justin Lewis $
   ======================================================================== */

#define C_SORTING_H

internal inline void
c_radix_sort(void  *primary_buffer,
             void  *sorting_buffer,
             s32    item_count,
             usize  item_size,
             s32    value_offset,
             s32    bits_to_search)
{
    const s32 radix         = 256;
    const s32 bits_per_pass = 8;

    const s32 pass_count = (bits_to_search + bits_per_pass - 1) / bits_per_pass;
    const s64 half_range = 1ULL << (bits_to_search - 1);

    s64 count[radix];
    s64 digit_sum[radix];

    for(s32 pass_index = 0;
        pass_index < pass_count;
        ++pass_index)
    {
        s32 bit_shift = pass_index * bits_per_pass;
        
        memset(count, 0, sizeof(count));
        for(s32 item_index = 0;
            item_index < item_count;
            ++item_index)
        {
            u8 *item = (u8 *)primary_buffer + (item_index * item_size);
            u64 value_to_sort = *(u64 *)(item + value_offset);
            value_to_sort += half_range;

            u32 digit = (value_to_sort >> bit_shift) & (radix - 1);
            ++count[digit];
        }

        digit_sum[0] = 0;
        for(s32 sum_index = 1;
            sum_index < radix;
            ++sum_index)
        {
            digit_sum[sum_index] = digit_sum[sum_index - 1] + count[sum_index - 1];
        }

        for(s32 item_index = 0;
            item_index < item_count;
            ++item_index)
        {
            u8 *item = (u8 *)primary_buffer + (item_index * item_size);
            u64 value_to_sort = *(u64 *)(item + value_offset);
            value_to_sort += half_range;

            u32 digit = (value_to_sort >> bit_shift) & (radix - 1);
            memcpy((u8 *)sorting_buffer + (digit_sum[digit] * item_size), item, item_size);

            ++digit_sum[digit];
        }
    }

    memcpy(primary_buffer, sorting_buffer, item_count * item_size);
}

#endif // C_SORTING_H

