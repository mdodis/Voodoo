#include "App.h"
#include "VulkanCommon.h"

auto Validation_Layers = arr<const char*>(
    "VK_LAYER_KHRONOS_validation"
);

static VK_PROC_DEBUG_CALLBACK(debug_callback) {
    if (severity < VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        print(LIT("Vulkan: {}\n"), Str(callback_data->pMessage));
    }
    return VK_FALSE;
}

Result<void, VkResult> App::init_vulkan() {

    CREATE_SCOPED_ARENA(&System_Allocator, temp_alloc, KILOBYTES(4));

    // Get available extensions
    {
        u32 count = 0;
        VK_RETURN_IF_ERR(vkEnumerateInstanceExtensionProperties(0, &count, 0));

        TArray<VkExtensionProperties> properties(&System_Allocator);
        properties.init_range(count);

        VK_RETURN_IF_ERR(vkEnumerateInstanceExtensionProperties(0, &count, properties.data));

        print(LIT("Extensions:\n"));
        for (auto &property : properties) {
            print(LIT("\t {}: {}\n"), 
                property.specVersion,
                Str(property.extensionName));
        }
    }

    // Create instance
    {
        Slice<const char*> extensions = get_win32_required_extensions();

        if (config.validation_layers) {
            if (!check_validation_layers(temp_alloc)) {
                return Err(VK_ERROR_UNKNOWN);
            }
        }

        VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .messageSeverity =
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType =
                VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = debug_callback,
            .pUserData = 0,
        };

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
            .pNext = config.validation_layers ? &debug_create_info : 0,
            .pApplicationInfo = &app_info,
            .enabledLayerCount = (config.validation_layers) ? Validation_Layers.count() : 0,
            .ppEnabledLayerNames = Validation_Layers.elements,
            .enabledExtensionCount = (u32)extensions.count,
            .ppEnabledExtensionNames = extensions.ptr,
        };

        VK_RETURN_IF_ERR(vkCreateInstance(
            &create_info,
            0,
            &instance));
    }

    // Pick physical device
    {
        u32 device_count;
        VK_RETURN_IF_ERR(vkEnumeratePhysicalDevices(instance, &device_count, 0));

        TArray<VkPhysicalDevice> physical_devices(&temp_alloc);
        physical_devices.init_range(device_count);
        VK_RETURN_IF_ERR(vkEnumeratePhysicalDevices(instance, &device_count, physical_devices.data));

        for (VkPhysicalDevice& device : physical_devices) {
            if (is_physical_device_suitable(device)) {
                physical_device = device;
                break;
            }
        }

        if (physical_device == VK_NULL_HANDLE) {
            return Err(VK_ERROR_UNKNOWN);
        }
    }

    // Create logical device
    {
        auto family_indices_result = find_queue_family_indices(temp_alloc, physical_device);
        if (!family_indices_result.ok()) {
            return Err(family_indices_result.err());
        }

        QueueFamilyIndices family_indices = family_indices_result.value();

        float queue_priority = 1.0f;
        VkDeviceQueueCreateInfo queue_create_info = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = (u32)family_indices.graphics,
            .queueCount = 1,
            .pQueuePriorities = &queue_priority
        };

        VkPhysicalDeviceFeatures features = {0};
        VkDeviceCreateInfo create_info = {
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &queue_create_info,
            .enabledLayerCount = (config.validation_layers) ? Validation_Layers.count() : 0,
            .ppEnabledLayerNames = Validation_Layers.elements,
            .pEnabledFeatures = &features,
        };

        VK_RETURN_IF_ERR(vkCreateDevice(physical_device, &create_info, 0, &device));
    }

    return Ok<void>();
}

void App::deinit() {
    window.destroy();
    vkDestroyDevice(device, 0);
    vkDestroyInstance(instance, 0);
}

bool App::check_validation_layers(IAllocator &allocator) {
    TArray<VkLayerProperties> props(&allocator);
    DEFER(props.release());

    u32 layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, 0);

    props.init_range(layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, props.data);

    for (const char* layer : Validation_Layers) {
        bool found = false;

        for (const auto& prop : props) {
            if (strcmp(prop.layerName, layer) == 0) {
                found = true;
                break;
            }
        }

        if (!found) {
            return false;
        }
    }

    return true;
}

bool App::is_physical_device_suitable(VkPhysicalDevice& device) {
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;

    vkGetPhysicalDeviceProperties(device, &properties);
    vkGetPhysicalDeviceFeatures(device, &features);

    if (config.use_discrete_gpu) {
        return properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
    } else {
        return properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
    }
}

Result<QueueFamilyIndices, VkResult> App::find_queue_family_indices(IAllocator &allocator, VkPhysicalDevice device) {
    QueueFamilyIndices families = {
        .graphics = -1
    };

    u32 family_count;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, 0);

    TArray<VkQueueFamilyProperties> properties(&allocator);
    properties.init_range(family_count);
    DEFER(properties.release());

    vkGetPhysicalDeviceQueueFamilyProperties(device, &family_count, properties.data);

    for (int i = 0; i < properties.size; ++i) {
        VkQueueFamilyProperties& family = properties[i];
        if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            families.graphics = i;
        }
    }

    if (families.graphics == -1) {
        return Err(VK_ERROR_UNKNOWN);
    }

    return Ok(families);
}
