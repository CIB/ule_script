#include "ule_memory_management.h"

#include "string.h"
#include "math.h"

uint16_t ceil_division(uint16_t a, uint16_t b) {
	return ceil((float) a / (float) b);
}

uint8_t read_bitmap(uint16_t bitmap, uint8_t index) {
	return bitmap & (0x1 << index);
}

void set_bitmap(uint16_t bitmap, uint8_t index) {
	bitmap |= (0x1 << index);
}

void unset_bitmap(uint16_t bitmap, uint8_t index) {
	bitmap &= ~(0x1 << index);
}

uint32_t *memory_metadata_get_bitmap(memory_metadata *metadata) {
	void *metadata_addr = metadata;
	return (uint32_t*) (metadata_addr + sizeof(memory_metadata));
}

uint16_t *memory_metadata_get_chunk_list(memory_metadata *metadata) {
	void *metadata_addr = metadata;
	return (uint16_t*) 
		(metadata_addr + sizeof(memory_metadata) + metadata->size);
}

void *memory_metadata_get_data(memory_metadata *metadata) {
	void *metadata_addr = metadata;
	return (uint16_t*) 
		(metadata_addr + sizeof(memory_metadata) + metadata->size +
		 metadata->maximum_objects * sizeof(uint16_t));
}

memory_metadata *memory_metadata_create(void *addr, uint16_t size,
	uint16_t maximum_objects)
{
	memory_metadata *metadata = (memory_metadata*) addr;
	metadata->size = size / (chunk_size * 32);
	metadata->maximum_objects = maximum_objects;
	metadata->number_objects = 0; // no chunks just yet!
	
	uint32_t *bitmap = memory_metadata_get_bitmap(metadata);
	memset(bitmap, '\0', metadata->size);
	
	
	return metadata;
}

void *ule_malloc(memory_metadata *meta, size_t size_) {
	uint16_t size = ceil_division((uint16_t)size_, chunk_size);
	//uint16_t bitmap_size = size_to_bitmap_size(size);
	
	uint32_t *bitmap = memory_metadata_get_bitmap(meta);
	
	// Scan for a chunk of memory of sufficient size.
	uint8_t success = 0;
	uint16_t i, j;
	for(i=0; i < meta->size * 32; i++) {
		int bitmap_index;
		int bitmap_offset;
		for(j=0; j < size; j++) {
			bitmap_index = (i+j) / 32;
			bitmap_offset = (i+j) % 32;
			uint8_t value = read_bitmap(
				bitmap[bitmap_index], bitmap_offset);
			if(value) {
				break;
			}
		}
		
		if(j+1 == size) {
			// We managed to fit everything in!
			set_bitmap(bitmap[bitmap_index], bitmap_offset);
			
			// Add to chunk list.
			uint16_t *chunk_list = 
				memory_metadata_get_chunk_list(meta);
			chunk_list[++(meta->number_objects)] = size_;
			
			void *data = memory_metadata_get_data(meta);
			return data + i * chunk_size;
		}
	}
	
	// If we get here, that means we found nothing!
	return NULL;
}