// I probably should analyse the branching and other optimisations that can be done, but thats for later

#include "malloc.h"

arena_lane  *arena;
int lane_count;

//---------------------------Tree-Comparators---------------------------------------------------

int addr_cmp(freeblock_node *a, freeblock_node *b)
{
    return (a->block.addres > b->block.addres) - (a->block.addres < b->block.addres);
}

// size first, address as tie-break so equal-size blocks stay distinct
int size_cmp(freeblock_node *a, freeblock_node *b)
{
    if (a->block.size_1 != b->block.size_1)
        return a->block.size_1 > b->block.size_1 ? 1 : -1;
    return addr_cmp(a, b);
}

RB_GENERATE(addr_tree, freeblock_node, addr_entry, addr_cmp)
RB_GENERATE(size_tree, freeblock_node, size_entry, size_cmp)

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

