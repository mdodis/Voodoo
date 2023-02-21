#include "App.h"
#include "VulkanCommon.h"

Result<void, VkResult> App::init_vulkan() {
    // Get available extensions
    {
        u32 count = 0;
        vkEnumerateInstanceExtensionProperties(0, &count, 0);

        TArray<VkExtensionProperties> properties(&System_Allocator);
        properties.init_range(count);

        vkEnumerateInstanceExtensionProperties(0, &count, properties.data);

        print(LIT("Extensions:\n"));
        for (auto &property : properties) {
            print(LIT("\t {}: {}\n"), 
                property.specVersion,
                Str(property.extensionName));
        }
    }

    // Create instance
    {
        VkApplicationInfo app_info = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "VKX",
            .applicationVersion = VK_MAKE_VERSION(0, 1, 0),
            .pEngineName = "VKX",
            .engineVersion = VK_MAKE_VERSION(0, 1, 0),
            .apiVersion = VK_API_VERSION_1_0,
        };

        VkInstanceCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &app_info,
        };

        VK_RETURN_IF_ERR(vkCreateInstance(
            &create_info,
            0,
            &instance));
    }
    return Ok<void>();
}