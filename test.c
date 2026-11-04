#include <stdio.h>
#include <stdint.h>
// Source - https://stackoverflow.com/a/58010396
// Posted by S.S. Anne, modified by community. See post 'Timeline' for change history
// Retrieved 2026-11-03, License - CC BY-SA 4.0

#ifndef __GNUC__
# define __attribute__(x)
#endif
//struct uint24_t { unsigned long v:24; };
//typedef struct uint24_t __attribute__((packed)) uint24_t
//

typedef struct {
	uint32_t page_no ; 
	uint32_t offset : 18; 
} arena_t ;

int main(void)
{
	arena_t love;
	printf("%d \n", sizeof(love));
	return 0;
}
