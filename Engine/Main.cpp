#include "App.h"
#include "Base.h"
#include "FileSystem/Extras.h"
#include "FileSystem/FileSystem.h"
#include "Reflection.h"
#include "Serialization/JSON.h"

bool load_config(AppConfig &config)
{
    CREATE_SCOPED_ARENA(&System_Allocator, init_arena, KILOBYTES(2));

    Str config_path = LIT("./app-config.json");

    FileHandle fh = open_file_read(config_path);
    if (!IS_VALID_FILE(fh)) {
        return false;
    }

    TBufferedFileTape<true> json_file(fh, 1024Ui64, init_arena);

    json_deserialize(
        &json_file,
        init_arena,
        descriptor_of(&config),
        (umm)&config);
    return true;
}

void draw(App &app)
{
    vkWaitForFences(
        app.device,
        1,
        &app.fence_in_flight,
        VK_TRUE,
        NumProps<u64>::max);

    vkResetFences(app.device, 1, &app.fence_in_flight);

    u32 image_index;
    vkAcquireNextImageKHR(
        app.device,
        app.swap_chain,
        NumProps<u64>::max,
        app.sem_image_available,
        VK_NULL_HANDLE,
        &image_index);

    vkResetCommandBuffer(app.cmd_buffer, 0);

    app.record_cmd_buffer(app.cmd_buffer, image_index);

    VkSemaphore wait_semaphores[] = {app.sem_image_available};

    VkPipelineStageFlags wait_stages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSemaphore signal_semaphores[] = {app.sem_render_finished};

    VkSubmitInfo submit_info = {
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount   = 1,
        .pWaitSemaphores      = wait_semaphores,
        .pWaitDstStageMask    = wait_stages,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &app.cmd_buffer,
        .signalSemaphoreCount = ARRAY_COUNT(signal_semaphores),
        .pSignalSemaphores    = signal_semaphores,
    };

    VK_CHECK(vkQueueSubmit(
        app.graphics_queue,
        1,
        &submit_info,
        app.fence_in_flight));

    VkSwapchainKHR swap_chains[] = {app.swap_chain};

    VkPresentInfoKHR present_info = {
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = ARRAY_COUNT(signal_semaphores),
        .pWaitSemaphores    = signal_semaphores,
        .swapchainCount     = 1,
        .pSwapchains        = swap_chains,
        .pImageIndices      = &image_index,
        .pResults           = 0,
    };

    VK_CHECK(vkQueuePresentKHR(app.present_queue, &present_info));

    VK_CHECK(vkDeviceWaitIdle(app.device));
}

int main(int argc, char const *argv[])
{
    AppConfig app_config;
    load_config(app_config);

    print(LIT("Validation Layers: {}\n"), app_config.validation_layers);

    Window window;
    window.init(640, 480).unwrap();

    App app = {
        .window = window,
        .config = app_config,
    };
    DEFER(app.deinit());

    app.init_vulkan().unwrap();
    app.init_pipeline();

    // app.record_cmd_buffer(app.cmd_buffer, 0);

    while (window.is_open) {
        window.poll();
        draw(app);
    }

    print(LIT("Closing..."));
    return 0;
}
