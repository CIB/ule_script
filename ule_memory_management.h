#include <stdint.h>
#include "string.h"

// The minimum size of a managed memory chunk.
static const uint16_t chunk_size = 16;

/**
 * This structure contains information about the available memory.
 */
typedef struct {
	// This is actually the amount of the managed memory chunks times 32,
	// (1 uint32 per chunk)
	uint16_t size;
	
	// The maximum number of objects that can be allocated at any time.
	uint16_t maximum_objects;
	
	// How many objects are currently allocated.
	uint16_t number_objects;
} memory_metadata;

/**
 * Create a new memory_metadata structure at the given address.
 * 
 * Make sure you're not using the memory between addr and 
 * addr + sizeof(memory_metadata) + @size + @maximum_objects * 2
 * for anything else!
 * 
 * @param addr The address at which to create the metadata
 * @param size The size of the memory we're going to manage. Must be
 *             a multiple of chunk_size * 32
 * @param maximum_chunks The maximum amount of objects that can be
 *                       allocated at once.
 */
memory_metadata *memory_metadata_create(void *addr, uint16_t size,
	uint16_t maximum_objects);

/**
 * This call is similar to malloc.
 * 
 * The implementation is specialized for much smaller amounts of data,
 * and will reduce memory usage in exchange for CPU cycles.
 * 
 * @return A pointer to the begin of the allocated memory, or NULL
 *         on failure.
 */
void *ule_malloc(memory_metadata* meta, size_t size);


/**
 * Free the memory object starting at the given address.
 */
void ule_free(memory_metadata* meta, void *addr);

/**
 * Resize the given memory object.
 *
 * This may result in moving the entire memory object in memory, so
 * the possibly different address of the memory object is returned.
 * 
 * @todo implement
 * 
 * @return The new address of the memory object.
 */
void* ule_realloc(memory_metadata* meta, void* addr, size_t size);

/**
 * Get the size of the memory object at the given address.
 * 
 * @todo implement
 */
int ule_get_size(void *addr);
