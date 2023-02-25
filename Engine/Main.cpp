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
        &app.fences_in_flight[app.current_frame],
        VK_TRUE,
        NumProps<u64>::max);

    vkResetFences(app.device, 1, &app.fences_in_flight[app.current_frame]);

    u32 image_index;
    vkAcquireNextImageKHR(
        app.device,
        app.swap_chain,
        NumProps<u64>::max,
        app.sems_image_available[app.current_frame],
        VK_NULL_HANDLE,
        &image_index);

    vkResetCommandBuffer(app.cmd_buffers[app.current_frame], 0);

    app.record_cmd_buffer(app.cmd_buffers[app.current_frame], image_index);

    VkSemaphore wait_semaphores[] = {
        app.sems_image_available[app.current_frame]};

    VkPipelineStageFlags wait_stages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

    VkSemaphore signal_semaphores[] = {
        app.sems_render_finished[app.current_frame]};

    VkSubmitInfo submit_info = {
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount   = 1,
        .pWaitSemaphores      = wait_semaphores,
        .pWaitDstStageMask    = wait_stages,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &app.cmd_buffers[app.current_frame],
        .signalSemaphoreCount = ARRAY_COUNT(signal_semaphores),
        .pSignalSemaphores    = signal_semaphores,
    };

    VK_CHECK(vkQueueSubmit(
        app.graphics_queue,
        1,
        &submit_info,
        app.fences_in_flight[app.current_frame]));

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

    app.current_frame = (app.current_frame + 1) % app.max_frames_in_flight;
}

int main(int argc, char const *argv[])
{
    AppConfig app_config;
    load_config(app_config);

    print(LIT("Validation Layers: {}\n"), app_config.validation_layers);

    Window window;
    window.init(640, 480).unwrap();

    App app = {
        .window               = window,
        .config               = app_config,
        .max_frames_in_flight = 2,
        .allocator            = System_Allocator,
    };
    app.init().unwrap();
    app.init_pipeline();
    DEFER(app.deinit());

    // app.record_cmd_buffer(app.cmd_buffer, 0);

    while (window.is_open) {
        window.poll();
        draw(app);
    }

    print(LIT("Closing..."));
    return 0;
}
