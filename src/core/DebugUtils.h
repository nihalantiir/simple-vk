#pragma once

#include <volk.h>

namespace core {

// Tags a Vulkan object with a human-readable name via VK_EXT_debug_utils, so
// validation messages and tools like RenderDoc/Nsight show real names
// instead of raw handles. Safe to call unconditionally: the function pointer
// is only non-null when the debug utils extension was actually enabled
// (validation layers available in a Debug build), so this is a no-op
// everywhere else.
inline void setDebugObjectName(VkDevice device, VkObjectType type, uint64_t handle, const char* name) {
    if (!vkSetDebugUtilsObjectNameEXT) {
        return;
    }

    VkDebugUtilsObjectNameInfoEXT info{VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
    info.objectType = type;
    info.objectHandle = handle;
    info.pObjectName = name;
    vkSetDebugUtilsObjectNameEXT(device, &info);
}

} // namespace core
