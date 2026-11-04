#ifndef MY_MALLOC_H
#define MY_MALLOC_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <bsd/sys/tree.h>

#ifndef __GNUC__
# define __attribute__(x)
#endif


//------------------------Unnecessary-Comments-Ignore-Please---------------------------------------

/*
 * we allocate an arena, initially to reduce
 * syscall overhead for every allocation, and manipulate
 * this arena and use this memory for malloc.
 *
 * there was two ways to get memory, brk/sbrk and mmap.
 * glibc uses both brk and mmap pathway to allocate arena
 * based on size. too reduce complexity we are going to use
 * only mmap.(thou its used for large size allocations,
 * slower syscall), which is the case with FreeBSD on newer
 * ports to aarch64 and risc-v
 *
 * refer : https://stackoverflow.com/questions/55768549/in-malloc-why-use-brk-at-all-why-not-just-use-mmap
 */

/*
 * so ya, now redesigning everything, changing from uint32 to uint8 addressing,
 * also separate and comprehensive metadata data management. for now, we'll be
 * using allocated table setup with free blocks listed.
 *
 * I should be using pretty and packed structs, so its more understandable to
 * my future self, not others, lol.
 */

//--------------------------Some-Pretty-Structs-----------------------------------------------

typedef struct
{
	uint32_t addres    : 16;     // 16,4096 byte sized, pages in one lane.
	uint32_t size_1    : 16;     // 16*4096 bytes is max size, we store size-1.

} __attribute__((packed)) freeblock; // sizeof(freeblock) == 4 bytes in atleast amd64 gcc/clang


typedef struct freeblock_node
{
	freeblock block;
	
	RB_ENTRY(freeblock_node) addr_entry; // tree initialized address-wise
	RB_ENTRY(freeblock_node) size_entry; // tree initialized freeblock-size-wise

} freeblock_node;


typedef struct node_chunk {
    struct node_chunk *next;
    freeblock_node     nodes[]; // flexible array member
} node_chunk;

typedef struct {
    node_chunk     *chunks;     // for munmap at lane teardown
    freeblock_node *free_nodes; // singly linked via rbe_left
} node_pool;


typedef struct 
{
    uint8_t *arena_memory;      // memory in one lane.

    node_pool pool;             // pool for storing all the metadata

    struct addr_tree addr_tree; // addressing the metadata according to address
    struct size_tree size_tree; // addressing the metadata according to size. 

} arena_lane;

//---------------------------------------------------------------------------------------------

#define PAGE_SIZE (1024*4)
#define MAX_SIZE (1024*1024*256)
#define ARENA_LANE_PAGES (16)

#define CHUNK_PAGES 1
#define CHUNK_BYTES (CHUNK_PAGES * PAGE_SIZE)
#define NODES_PER_CHUNK ((CHUNK_BYTES - sizeof(node_chunk)) / sizeof(freeblock_node))
#define NEXT_FREE(n)    RB_LEFT((n), addr_entry)   // free nodes aren't in any tree

//---------------------------------------------------------------------------------------------

#endif
