#include "config.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

uint32_t ** arena_;
uint8_t arena_global_idx;

void *arena_alloc(void)
{
    arena_global_idx++;
    if ((arena_[arena_global_idx] = 
        mmap(NULL,ARENA_SIZE*PAGE_SIZE,PROT_READ|PROT_WRITE,MAP_FIXED_NOREPLACE|MAP_PRIVATE|MAP_ANONYMOUS, -1, 0))
            == MAP_FAILED) 
    {
        arena_global_idx--; 
        return NULL;
    }

    arena_[arena_global_idx][0] = 1;
    arena_[arena_global_idx][1] = (PAGE_SIZE<<16|PAGE_SIZE*32);
    return (void *)arena_[arena_global_idx];
}

void metadata_write(uint8_t idx, uint16_t addr, uint16_t size)
{
    uint32_t index_with_size = ((uint32_t)addr) << 16 | ((uint32_t)size) ;
    uint32_t total = arena_[idx][0];
    arena_[idx][total] = index_with_size;
}

uint16_t find_free_space(uint8_t idx, uint16_t size)
{
    for (uint32_t i = 1; i <= arena_[idx][0] ; i++)
    {
        if ((arena_[idx][i] & 0xFFFFFFFF) >= size)
            return ((uint16_t)((arena_[idx][i] & 0xFFFFFFFF<<16)>>16));
    }
    return 0;
}

void metadata_update(uint8_t idx, uint16_t alloc_addr, uint16_t alloc_size)
{
    uint16_t curr_addr;
    uint16_t curr_size;
    uint32_t total = arena_[idx][0];
    for (int i = 0; i <= total; i++) 
    {
        
        curr_addr = ((arena_[idx][i] & 0xFFFFFFFF<<16)>>16);
        curr_size = (arena_[idx][i] & 0xFFFFFFFF);
        if (alloc_addr < (curr_addr+curr_size) && alloc_addr >= curr_addr)
        {
            arena_[idx][i] = curr_addr << 16 | (alloc_addr - curr_addr); 
            metadata_write(idx, curr_addr+curr_size, curr_size+curr_addr - (alloc_addr+alloc_size));
        }
    }
}

uint16_t mem_allocate(uint8_t *data, uint16_t size)
{
    
    uint16_t nearest_free_address;
    uint8_t arena_index = 0;
    while (arena_index <= arena_global_idx && !(nearest_free_address = find_free_space(arena_index, size)))
        arena_index++;
    if (nearest_free_address == 0) 
    {    
        arena_alloc();
        arena_index = arena_global_idx;
        nearest_free_address = ((sizeof(arena_))*arena_global_idx + PAGE_SIZE);
    }
    

    /*
     * Check if the data is 8-bit aligned,
     * Then search through the table or loop through shii
     * then find the next place with appropriate location
     * then mmap the memory through syscall or something
     * then return the memory address, return 0 if failed. 
     * the first 8 bits is always allocated for storing 
     * page_no, so a node contains only size and data, 
     * I can do a hashmap to denote that these places are 
     * filled but rather a table seems like a better deal
     * table with start address and size (in bytes), seems
     * interesting letsee, lol. 
     *
     * it would be interesting to allocate fragmented parts
     * into single part through inbetween addressing 
     * it would be slow though but better asking for new
     * allocation everytime : Probably TO DO later
    */
    if (size > MAX_SIZE)            
    {
        fprintf(stderr,"Error: Too large allocation, size limit exceed");
        exit(1);
    }
    
    

    return 0;
}

void update_table(void)
{
    /* 
     * this function updates the table value, after each
     * allocation, so that the table is consistent and
     * ready for the next allocation
    */
}

