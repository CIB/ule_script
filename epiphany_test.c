#include <malloc.h>
#include <stdio.h>
#include "ule_memory_management.h"

typedef struct {} empty;
//empty heap_begin __attribute__ ((section (".data_bank3")));

int main(int argc, char** argv) {
	uint8_t heap[10000];
	memory_metadata *metadata = memory_metadata_create(&heap[0], 2048,
		128);
	void *a = ule_malloc(metadata, 512);
	void *b = ule_malloc(metadata, 16);
	void *c = ule_malloc(metadata, 128);
	printf("1 B is at %u\n", (int) b);
	
	
	// Now we deallocate b
	ule_free(metadata, b);
	
	// And allocate it again at the same size, it should be in the same spot!
	b = ule_malloc(metadata, 16);
	printf("2 B is at %u\n", (int) b);
	
	// Free once again
	ule_free(metadata, b);
	
	// Now allocate but larger, it should be elsewhere
	b = ule_malloc(metadata, 128);
	printf("3 B is at %u\n", (int) b);
	
	return 0;
}
