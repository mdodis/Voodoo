#pragma once
#include <glm/glm.hpp>

#include "Reflection.h"

struct EditorColorScheme {
    using Vec4 = glm::vec4;

    Vec4 border;
    Vec4 frame_bg;
    Vec4 frame_bg_hovered;
    Vec4 frame_bg_active;
    Vec4 title_bg;
    Vec4 title_bg_active;
    Vec4 check_mark;
    Vec4 slider_grab;
    Vec4 slider_grab_active;
    Vec4 button;
    Vec4 button_hovered;
    Vec4 button_active;
    Vec4 header;
    Vec4 header_hovered;
    Vec4 header_active;
    Vec4 resize_grip;
    Vec4 resize_grip_hovered;
    Vec4 resize_grip_active;
    Vec4 tab;
    Vec4 tab_hovered;
    Vec4 tab_active;
    Vec4 docking_preview;
    Vec4 plot_lines_hovered;
    Vec4 plot_histogram;
    Vec4 plot_histogram_hovered;
    Vec4 text_selection_bg;
    Vec4 drag_drop_target;
};

struct EditorColorSchemeDescriptor : IDescriptor {
    using Vec4Descriptor =
        FixedArrayDescriptor<EditorColorScheme::Vec4, float, 4>;
    // clang-format off
    Vec4Descriptor border_desc                 = { LIT("border"),                 OFFSET_OF(EditorColorScheme, border) };
    Vec4Descriptor frame_bg_desc               = { LIT("frame_bg"),               OFFSET_OF(EditorColorScheme, frame_bg) };
    Vec4Descriptor frame_bg_hovered_desc       = { LIT("frame_bg_hovered"),       OFFSET_OF(EditorColorScheme, frame_bg_hovered) };
    Vec4Descriptor frame_bg_active_desc        = { LIT("frame_bg_active"),        OFFSET_OF(EditorColorScheme, frame_bg_active) };
    Vec4Descriptor title_bg_desc               = { LIT("title_bg"),               OFFSET_OF(EditorColorScheme, title_bg) };
    Vec4Descriptor title_bg_active_desc        = { LIT("title_bg_active"),        OFFSET_OF(EditorColorScheme, title_bg_active) };
    Vec4Descriptor check_mark_desc             = { LIT("check_mark"),             OFFSET_OF(EditorColorScheme, check_mark) };
    Vec4Descriptor slider_grab_desc            = { LIT("slider_grab"),            OFFSET_OF(EditorColorScheme, slider_grab) };
    Vec4Descriptor slider_grab_active_desc     = { LIT("slider_grab_active"),     OFFSET_OF(EditorColorScheme, slider_grab_active) };
    Vec4Descriptor button_desc                 = { LIT("button"),                 OFFSET_OF(EditorColorScheme, button) };
    Vec4Descriptor button_hovered_desc         = { LIT("button_hovered"),         OFFSET_OF(EditorColorScheme, button_hovered) };
    Vec4Descriptor button_active_desc          = { LIT("button_active"),          OFFSET_OF(EditorColorScheme, button_active) };
    Vec4Descriptor header_desc                 = { LIT("header"),                 OFFSET_OF(EditorColorScheme, header) };
    Vec4Descriptor header_hovered_desc         = { LIT("header_hovered"),         OFFSET_OF(EditorColorScheme, header_hovered) };
    Vec4Descriptor header_active_desc          = { LIT("header_active"),          OFFSET_OF(EditorColorScheme, header_active) };
    Vec4Descriptor resize_grip_desc            = { LIT("resize_grip"),            OFFSET_OF(EditorColorScheme, resize_grip) };
    Vec4Descriptor resize_grip_hovered_desc    = { LIT("resize_grip_hovered"),    OFFSET_OF(EditorColorScheme, resize_grip_hovered) };
    Vec4Descriptor resize_grip_active_desc     = { LIT("resize_grip_active"),     OFFSET_OF(EditorColorScheme, resize_grip_active) };
    Vec4Descriptor tab_desc                    = { LIT("tab"),                    OFFSET_OF(EditorColorScheme, tab) };
    Vec4Descriptor tab_hovered_desc            = { LIT("tab_hovered"),            OFFSET_OF(EditorColorScheme, tab_hovered) };
    Vec4Descriptor tab_active_desc             = { LIT("tab_active"),             OFFSET_OF(EditorColorScheme, tab_active) };
    Vec4Descriptor docking_preview_desc        = { LIT("docking_preview"),        OFFSET_OF(EditorColorScheme, docking_preview) };
    Vec4Descriptor plot_lines_hovered_desc     = { LIT("plot_lines_hovered"),     OFFSET_OF(EditorColorScheme, plot_lines_hovered) };
    Vec4Descriptor plot_histogram_desc         = { LIT("plot_histogram"),         OFFSET_OF(EditorColorScheme, plot_histogram) };
    Vec4Descriptor plot_histogram_hovered_desc = { LIT("plot_histogram_hovered"), OFFSET_OF(EditorColorScheme, plot_histogram_hovered) };
    Vec4Descriptor text_selection_bg_desc      = { LIT("text_selection_bg"),      OFFSET_OF(EditorColorScheme, text_selection_bg) };
    Vec4Descriptor drag_drop_target_desc       = { LIT("drag_drop_target"),       OFFSET_OF(EditorColorScheme, drag_drop_target) };
    // clang-format on

    IDescriptor* descs[27] = {
        border_desc,
        frame_bg_desc,
        frame_bg_hovered_desc,
        frame_bg_active_desc,
        title_bg_desc,
        title_bg_active_desc,
        check_mark_desc,
        slider_grab_desc,
        slider_grab_active_desc,
        button_desc,
        button_hovered_desc,
        button_active_desc,
        header_desc,
        header_hovered_desc,
        header_active_desc,
        resize_grip_desc,
        resize_grip_hovered_desc,
        resize_grip_active_desc,
        tab_desc,
        tab_hovered_desc,
        tab_active_desc,
        docking_preview_desc,
        plot_lines_hovered_desc,
        plot_histogram_desc,
        plot_histogram_hovered_desc,
        text_selection_bg_desc,
        drag_drop_target_desc,
    };

    CUSTOM_DESC_OBJECT_DEFAULT(EditorColorScheme, descs);
};

DEFINE_DESCRIPTOR_OF_INL(EditorColorScheme);