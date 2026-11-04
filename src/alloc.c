// I probably should analyse the branching and other optimisations that can be done, but thats for later

#include "malloc.h"

arena_lane  *arena;
int lane_count;

//---------------------------Metadata-Handling--------------------------------------------------

static int pool_grow(node_pool *p)
{
    node_chunk *c = mmap(NULL, CHUNK_BYTES, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (c == MAP_FAILED) return -1;

    c->next  = p->chunks;
    p->chunks = c;

    for (size_t i = 0; i < NODES_PER_CHUNK; i++) {
        NEXT_FREE(&c->nodes[i]) = p->free_nodes;
        p->free_nodes = &c->nodes[i];
    }
    return 0;
}

freeblock_node *node_get(node_pool *p)
{
    if (!p->free_nodes && pool_grow(p) == -1)
        return NULL;
    freeblock_node *n = p->free_nodes;
    p->free_nodes = NEXT_FREE(n);
    return n;
}

void node_put(node_pool *p, freeblock_node *n)
{
    NEXT_FREE(n)  = p->free_nodes;
    p->free_nodes = n;
}

void pool_destroy(node_pool *p)
{
    for (node_chunk *c = p->chunks, *next; c; c = next) {
        next = c->next;
        munmap(c, CHUNK_BYTES);
    }
    p->chunks = NULL;
    p->free_nodes = NULL;
}



//------------------slightly-old-version-------------------------------------------


// finds the index with enough freespace in a table of a particular lane.
int find_freeblock(int lane, uint32_t size)
{
	int freeblock_count = arena[lane].freeblock_count;
	for (int i = 0; i <= freeblock_count; i++;)
	{
		if(freeblock_table[lane][i].size >= size)
			return i;			
	}
	return -1;
}

// allocates memory for the freeblocks table (separate from the main memory)
int freeblock_table_alloc(int lane, uint32_t size)
{
	if ((arena[lane].freeblock_table = 
		mmap(NULL, (size_t)size , PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0)) == MAP_FAILED)
		return -1;
	return 0;
}

// changes/removes the freeblock as its allocated.
int write_freeblock(int lane, uint32_t size)
{
	int blk_idx = find_freeblock(lane, size);
	if (blk_idx == -1) return -1;

	
}



//------------Struck----Midway-------------------
//---------------Below-is-old-code---------------

void *arena_alloc(void)
{
    //increment the counter
    arena_global_idx++;
    // one arena gets 33 pages, arena_ is the array of arenas.
    if ((arena_[arena_global_idx] = 
        mmap(NULL,ARENA_SIZE*PAGE_SIZE,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS, -1, 0))
            == MAP_FAILED) 
    {
        arena_global_idx--; 
        return NULL;
    }

    /* Pardon the weird addressing, used lol,even I got confused rereading. 
     * arena_[arena_global_idx] contains MAX_ADDR_TOTAL = ARENA_SIZE*PAGE_SIZE/UINT_8T
     * number of addressable words. Initial idea of word addressing was bad,
     * so we are switching to byte addressing instead.
     */
    arena_[arena_global_idx][0] = 1;
    arena_[arena_global_idx][1] = (PAGE_SIZE<<16|PAGE_SIZE*32);

    return (void *)arena_[arena_global_idx];
}

// template for writing metadata, here idx is usually 0, 0 is metadata arena
void metadata_write(uint8_t idx, uint16_t addr, uint16_t size)
{
    uint32_t index_with_size = ((uint32_t)addr) << 16 | ((uint32_t)size) ;
    uint32_t total = arena_[idx][0]++;     
    arena_[idx][total] = index_with_size;
}

// probes over the arenas, this total <= page_size/size(uint32_t), since one page is used for metadata.
// so, 0 to max_total contains a 32 bit number, each having an address
uint16_t find_free_space(uint8_t idx, uint16_t size)
{
    for (uint32_t i = 1; i <= arena_[idx][0] ; i++)
    {
        if ( (arena_[idx][i] & 0xFFFF) >= size )
            return (uint16_t) (arena_[idx][i] >>16);
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

