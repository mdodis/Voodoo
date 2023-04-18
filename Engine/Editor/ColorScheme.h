#pragma once
#include <glm/glm.hpp>

#include "Reflection.h"

struct EditorColorScheme {
    using Vec4 = glm::vec4;

    Vec4 window_bg;
    Vec4 border;
    Vec4 border_shadow;
    Vec4 menubar_bg;
    Vec4 scrollbar_bg;
    Vec4 scrollbar_grab;
    Vec4 scrollbar_grab_hovered;
    Vec4 scrollbar_grab_active;
    Vec4 separator;
    Vec4 separator_hovered;
    Vec4 separator_active;
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
    Vec4 tab_unfocused;
    Vec4 tab_unfocused_active;
    Vec4 docking_preview;
    Vec4 plot_lines;
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
    Vec4Descriptor border_desc                 = { OFFSET_OF(EditorColorScheme, border),                 LIT("border"),                 };
    Vec4Descriptor frame_bg_desc               = { OFFSET_OF(EditorColorScheme, frame_bg),               LIT("frame_bg"),               };
    Vec4Descriptor frame_bg_hovered_desc       = { OFFSET_OF(EditorColorScheme, frame_bg_hovered),       LIT("frame_bg_hovered"),       };
    Vec4Descriptor frame_bg_active_desc        = { OFFSET_OF(EditorColorScheme, frame_bg_active),        LIT("frame_bg_active"),        };
    Vec4Descriptor title_bg_desc               = { OFFSET_OF(EditorColorScheme, title_bg),               LIT("title_bg"),               };
    Vec4Descriptor title_bg_active_desc        = { OFFSET_OF(EditorColorScheme, title_bg_active),        LIT("title_bg_active"),        };
    Vec4Descriptor check_mark_desc             = { OFFSET_OF(EditorColorScheme, check_mark),             LIT("check_mark"),             };
    Vec4Descriptor slider_grab_desc            = { OFFSET_OF(EditorColorScheme, slider_grab),            LIT("slider_grab"),            };
    Vec4Descriptor slider_grab_active_desc     = { OFFSET_OF(EditorColorScheme, slider_grab_active),     LIT("slider_grab_active"),     };
    Vec4Descriptor button_desc                 = { OFFSET_OF(EditorColorScheme, button),                 LIT("button"),                 };
    Vec4Descriptor button_hovered_desc         = { OFFSET_OF(EditorColorScheme, button_hovered),         LIT("button_hovered"),         };
    Vec4Descriptor button_active_desc          = { OFFSET_OF(EditorColorScheme, button_active),          LIT("button_active"),          };
    Vec4Descriptor header_desc                 = { OFFSET_OF(EditorColorScheme, header),                 LIT("header"),                 };
    Vec4Descriptor header_hovered_desc         = { OFFSET_OF(EditorColorScheme, header_hovered),         LIT("header_hovered"),         };
    Vec4Descriptor header_active_desc          = { OFFSET_OF(EditorColorScheme, header_active),          LIT("header_active"),          };
    Vec4Descriptor resize_grip_desc            = { OFFSET_OF(EditorColorScheme, resize_grip),            LIT("resize_grip"),            };
    Vec4Descriptor resize_grip_hovered_desc    = { OFFSET_OF(EditorColorScheme, resize_grip_hovered),    LIT("resize_grip_hovered"),    };
    Vec4Descriptor resize_grip_active_desc     = { OFFSET_OF(EditorColorScheme, resize_grip_active),     LIT("resize_grip_active"),     };
    Vec4Descriptor tab_desc                    = { OFFSET_OF(EditorColorScheme, tab),                    LIT("tab"),                    };
    Vec4Descriptor tab_hovered_desc            = { OFFSET_OF(EditorColorScheme, tab_hovered),            LIT("tab_hovered"),            };
    Vec4Descriptor tab_active_desc             = { OFFSET_OF(EditorColorScheme, tab_active),             LIT("tab_active"),             };
    Vec4Descriptor tab_unfocused_active_desc   = { OFFSET_OF(EditorColorScheme, tab_unfocused_active),   LIT("tab_unfocused_active"),             };
    Vec4Descriptor docking_preview_desc        = { OFFSET_OF(EditorColorScheme, docking_preview),        LIT("docking_preview"),        };
    Vec4Descriptor plot_lines_hovered_desc     = { OFFSET_OF(EditorColorScheme, plot_lines_hovered),     LIT("plot_lines_hovered"),     };
    Vec4Descriptor plot_histogram_desc         = { OFFSET_OF(EditorColorScheme, plot_histogram),         LIT("plot_histogram"),         };
    Vec4Descriptor plot_histogram_hovered_desc = { OFFSET_OF(EditorColorScheme, plot_histogram_hovered), LIT("plot_histogram_hovered"), };
    Vec4Descriptor text_selection_bg_desc      = { OFFSET_OF(EditorColorScheme, text_selection_bg),      LIT("text_selection_bg"),      };
    Vec4Descriptor drag_drop_target_desc       = { OFFSET_OF(EditorColorScheme, drag_drop_target),       LIT("drag_drop_target"),       };
    // clang-format on

    IDescriptor* descs[28] = {
        &border_desc,
        &frame_bg_desc,
        &frame_bg_hovered_desc,
        &frame_bg_active_desc,
        &title_bg_desc,
        &title_bg_active_desc,
        &check_mark_desc,
        &slider_grab_desc,
        &slider_grab_active_desc,
        &button_desc,
        &button_hovered_desc,
        &button_active_desc,
        &header_desc,
        &header_hovered_desc,
        &header_active_desc,
        &resize_grip_desc,
        &resize_grip_hovered_desc,
        &resize_grip_active_desc,
        &tab_desc,
        &tab_hovered_desc,
        &tab_active_desc,
        &tab_unfocused_active_desc,
        &docking_preview_desc,
        &plot_lines_hovered_desc,
        &plot_histogram_desc,
        &plot_histogram_hovered_desc,
        &text_selection_bg_desc,
        &drag_drop_target_desc,
    };

    CUSTOM_DESC_OBJECT_DEFAULT(EditorColorScheme, descs);
};

DEFINE_DESCRIPTOR_OF_INL(EditorColorScheme);
