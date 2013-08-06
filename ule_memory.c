#include "ule_memory_management.h"

#include "string.h"
#include "math.h"
#include "stdio.h"

/*uint32_t max(uint32_t x, uint32_t y) {
	return x ^ ((x ^ y) & -(x < y));
}*/
typedef struct  {
	uint16_t offset;
	uint16_t size;
} memory_object_entry;

static inline uint16_t ceil_division(uint16_t a, uint16_t b) {
	uint16_t tmp = a % b;
	return (a / b) + (tmp > 0);
}

uint8_t read_bitmap(uint32_t bitmap, uint8_t index) {
	return (bitmap >> index) & 0x1;
}

void set_bitmap(uint32_t *bitmap, uint8_t index) {
	(*bitmap) |= (0x1 << index);
}

void unset_bitmap(uint32_t *bitmap, uint8_t index) {
	(*bitmap) &= ~(0x1 << index);
}

uint32_t *memory_metadata_get_bitmap(memory_metadata *metadata) {
	void *metadata_addr = metadata;
	return (uint32_t*) (metadata_addr + sizeof(memory_metadata));
}

memory_object_entry *memory_metadata_get_chunk_list(
	memory_metadata *metadata) 
{
	void *metadata_addr = metadata;
	return (memory_object_entry*) 
		(metadata_addr + sizeof(memory_metadata) + 
			metadata->size * sizeof(uint32_t));
}

void *memory_metadata_get_data(memory_metadata *metadata) {
	void *metadata_addr = metadata;
	return (uint16_t*) 
		(metadata_addr + sizeof(memory_metadata) + 
		 metadata->size * sizeof(uint32_t) +
		 metadata->maximum_objects * sizeof(memory_object_entry));
}

memory_metadata *memory_metadata_create(void *addr, uint16_t size,
	uint16_t maximum_objects)
{
	memory_metadata *metadata = (memory_metadata*) addr;
	metadata->size = size / (chunk_size * 32);
	metadata->maximum_objects = maximum_objects;
	metadata->number_objects = 0; // no chunks just yet!
	
	uint32_t *bitmap = memory_metadata_get_bitmap(metadata);
	uint16_t i;
	for(i=0; i < metadata->size; i++) {
		bitmap[i] = 0;
	}
	
	
	return metadata;
}

#ifdef ULE_DEBUG
void debug_print_bitmap(memory_metadata *meta) {
	uint32_t *bitmap = memory_metadata_get_bitmap(meta);
	printf("Bitmap: ");
	uint16_t z;
	for(z=0; z<meta->size*32; z++) {
		if(z % 32 == 0) {
			printf(" ");
		}
		int bitmap_index  = z / 32;
		int bitmap_offset = z % 32;
		printf("%d",  read_bitmap(bitmap[bitmap_index], bitmap_offset));
	}
	printf("\n");
	
	printf("Objects: ({offset, size}) ");
	memory_object_entry *chunk_list = 
		memory_metadata_get_chunk_list(meta);
	int i;
	for(i = 0; i < meta->number_objects; i++) {
		printf("{%d,%d},", chunk_list[i].offset, chunk_list[i].size);
	}
	printf("\n");
}
#endif

void *ule_malloc(memory_metadata *meta, size_t size_) {
	uint16_t size = ceil_division((uint16_t)size_, chunk_size);
	//uint16_t bitmap_size = size_to_bitmap_size(size);
	
	uint32_t *bitmap = memory_metadata_get_bitmap(meta);
	
	// Scan for a chunk of memory of sufficient size.
	uint8_t success = 0;
	uint16_t i, j;
	for(i=0; i < meta->size * 32 - size + 1; i++) {
		int bitmap_index;
		int bitmap_offset;
		for(j=0; j < size; j++) {
			bitmap_index  = (i+j) / 32;
			bitmap_offset = (i+j) % 32;
			uint8_t value = read_bitmap(
				bitmap[bitmap_index], bitmap_offset);
			if(value) {
				break;
			}
		}
		
		if(j == size) {
			// We managed to fit everything in!
			uint16_t h;
			for(h=i; h < i+j; h++) {
				bitmap_index  = h / 32;
				bitmap_offset = h % 32;
				set_bitmap(&(bitmap[bitmap_index]), bitmap_offset);
			}
			
			// Add to chunk list.
			memory_object_entry *chunk_list = 
				memory_metadata_get_chunk_list(meta);
			chunk_list[meta->number_objects].offset = i;
			chunk_list[meta->number_objects].size = size_;
			meta->number_objects++;
			
			void *data = memory_metadata_get_data(meta);
			#ifdef ULE_DEBUG
				debug_print_bitmap(meta);
			#endif
			return data + i * chunk_size;
		}
	}
	
	// If we get here, that means we found nothing!
	return NULL;
}

void ule_free(memory_metadata* meta, void *addr) {
	void *data = memory_metadata_get_data(meta);
	uint16_t offset = (addr - data) / chunk_size;
	
	// First find the entry, delete it, and adjust the size.
	memory_object_entry *chunk_list = 
		memory_metadata_get_chunk_list(meta);
	uint16_t size_ = 0;
	uint16_t i;
	
	// The index where we deleted, we need to replace this with the
	// last chunk.
	int16_t plug_hole = -1; 
	for(i = 0; i < meta->number_objects; i++) {
		if(chunk_list[i].offset == offset) {
			plug_hole = i;
			size_ = chunk_list[i].size;
		}
	}
	
	//assert(plug_hole != -1);
	// Plug hole by moving the last entry to the place
	// where we just removed an entry.
	chunk_list[plug_hole] = chunk_list[i-1];
	
	// Update size of chunk list.
	meta->number_objects--;
	
	// Now remove from bitmap.
	uint32_t *bitmap = memory_metadata_get_bitmap(meta);
	uint16_t size = ceil_division((uint16_t)size_, chunk_size);
	for(i = offset; i < offset + size; i++) {
		uint16_t bitmap_index = i / 32;
		uint16_t bitmap_offset = i % 32;
		unset_bitmap(&(bitmap[bitmap_index]), bitmap_offset);
	}
	#ifdef ULE_DEBUG
		debug_print_bitmap(meta);
	#endif
	// We're done!
}

uint16_t ule_get_size(memory_metadata* meta, void *addr) {
	void *data = memory_metadata_get_data(meta);
	uint16_t offset = (addr - data) / chunk_size;
	
	memory_object_entry *chunk_list = 
		memory_metadata_get_chunk_list(meta);
		
	int i;
	for(i = 0; i < meta->number_objects; i++) {
		if(chunk_list[i].offset == offset) {
			return chunk_list[i].size;
		}
	}
}

void* ule_realloc(memory_metadata* meta, void* addr, size_t size) {
	uint16_t old_size = ule_get_size(meta, addr);
	ule_free(meta, addr);
	
	char *rval = ule_malloc(meta, size);
	char *addr_as_char = addr;
	
	// Copy over the data
	int i;
	for(i = 0; i < old_size; i++) {
		rval[i] =  addr_as_char[i];
	}
	
	// Done!
	return rval;
}
