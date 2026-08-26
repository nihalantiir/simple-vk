// Isolated translation unit for the Vulkan Memory Allocator implementation.
// VMA is a single-header library; exactly one .cpp in the project must
// define VMA_IMPLEMENTATION before including it. volk.h is included first
// so VMA's calls to Vulkan functions resolve to volk's loaded pointers
// instead of the (disabled, via VK_NO_PROTOTYPES) static prototypes.
#define VMA_IMPLEMENTATION
#include <volk.h>
#include <vk_mem_alloc.h>
