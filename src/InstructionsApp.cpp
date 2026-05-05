#if USE_LVGL

#include "crosspad/app/AppRegistrar.hpp"
#include "crosspad-gui/platform/IGuiPlatform.h"
#include "crosspad-gui/components/markdown_view.h"
#include "crosspad-gui/styles/styles.h"
#include "crosspad-gui/app/App.hpp"
#include "lvgl.h"

#include <cstdio>
#include <cstring>
#include <string>

#if !defined(ESP_PLATFORM) && !defined(ARDUINO)
#include <filesystem>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#undef ERROR
#elif defined(__linux__)
#include <unistd.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#endif
#endif

/* ── Markdown file locator ───────────────────────────────────────────── */

static bool fileExists(const char* path)
{
    FILE* f = fopen(path, "r");
    if (f) { fclose(f); return true; }
    return false;
}

static std::string findInstructionsFile()
{
    // First try the platform's asset prefix (works on any platform that
    // mounts assets at a known path, e.g. "/assets/" on ESP-IDF).
    const char* prefix = crosspad_gui::getGuiPlatform().assetPathPrefix();
    if (prefix && prefix[0]) {
        std::string p = std::string(prefix) + "docs/instructions.md";
        if (fileExists(p.c_str())) return p;
    }

    const char* candidates[] = {
#if defined(ESP_PLATFORM)
        // ESP-IDF: LittleFS or SD
        "A:docs/instructions.md",
        "/sdcard/docs/instructions.md",
#elif defined(ARDUINO)
        // Arduino: LittleFS
        "I:/docs/instructions.md",
        "/littlefs/docs/instructions.md",
#else
        // PC / Desktop
        "docs/instructions.md",
        "../docs/instructions.md",
#endif
    };

    for (auto& c : candidates) {
        if (fileExists(c)) return c;
    }

#if !defined(ESP_PLATFORM) && !defined(ARDUINO)
    // Try exe-relative path (distribution bundle)
    namespace fs = std::filesystem;
    fs::path exeDir;
#ifdef _WIN32
    char buf[MAX_PATH] = {};
    if (GetModuleFileNameA(NULL, buf, MAX_PATH) > 0)
        exeDir = fs::path(buf).parent_path();
#elif defined(__linux__)
    char buf[4096];
    ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    if (len > 0) { buf[len] = '\0'; exeDir = fs::path(buf).parent_path(); }
#elif defined(__APPLE__)
    char buf[4096];
    uint32_t size = sizeof(buf);
    if (_NSGetExecutablePath(buf, &size) == 0)
        exeDir = fs::path(buf).parent_path();
#endif
    if (!exeDir.empty()) {
        auto p = exeDir / ".." / "docs" / "instructions.md";
        std::error_code ec;
        if (fs::exists(p, ec)) return fs::canonical(p, ec).string();
    }
#endif

    return {};
}

/* ── App create / destroy ────────────────────────────────────────────── */

static lv_obj_t* lv_CreateInstructions(lv_obj_t* parent, App* a)
{
#if defined(ESP_PLATFORM) || defined(ARDUINO)
    constexpr int BACK_BAR_H = 28;
#else
    (void)a;
    constexpr int BACK_BAR_H = 0;
#endif

    lv_obj_t* container = lv_obj_create(parent);
    lv_obj_set_size(container, lv_obj_get_content_width(parent),
                    lv_obj_get_content_height(parent));
    lv_obj_set_style_bg_color(container, lv_color_black(), 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_all(container, 0, 0);
    lv_obj_set_style_radius(container, 0, 0);
    lv_obj_center(container);

    lv_obj_t* content = lv_obj_create(container);
    lv_obj_set_size(content, LV_PCT(100),
                    lv_obj_get_content_height(parent) - BACK_BAR_H);
    lv_obj_align(content, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(content, lv_color_black(), 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_style_pad_all(content, 8, 0);
    lv_obj_set_style_pad_row(content, 2, 0);
    lv_obj_set_style_radius(content, 0, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_add_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_AUTO);

    std::string path = findInstructionsFile();
    if (path.empty()) {
        lv_obj_t* lbl = lv_label_create(content);
        lv_label_set_text(lbl, "instructions.md not found");
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xFF4444), 0);
    } else {
        crosspad_gui::markdown_render_file(content, path);
        printf("[Instructions] Loaded from %s\n", path.c_str());
    }

#if defined(ESP_PLATFORM) || defined(ARDUINO)
    // Back button + encoder routing for embedded launcher exit
    lv_obj_t* backBtn = lv_button_create(container);
    lv_obj_set_size(backBtn, 70, BACK_BAR_H - 4);
    lv_obj_align(backBtn, LV_ALIGN_BOTTOM_LEFT, 6, -2);
    lv_obj_add_style(backBtn, &styleButton, 0);
    lv_obj_add_style(backBtn, &styleButtonPressed, LV_STATE_PRESSED);
    lv_obj_t* backLabel = lv_label_create(backBtn);
    lv_label_set_text(backLabel, "Back");
    lv_obj_center(backLabel);
    lv_obj_add_event_cb(backBtn, [](lv_event_t* e) {
        auto* app = static_cast<App*>(lv_event_get_user_data(e));
        if (app) app->destroyAppAsync();
    }, LV_EVENT_CLICKED, a);

    lv_indev_t* nav = crosspad_gui::getGuiPlatform().getNavigationEncoder();
    if (nav) {
        static lv_group_t* s_grp = nullptr;
        if (s_grp) lv_group_delete(s_grp);
        s_grp = lv_group_create();
        lv_group_add_obj(s_grp, backBtn);
        lv_group_focus_obj(backBtn);
        lv_indev_set_group(nav, s_grp);
    }
#endif

    return container;
}

static void lv_DestroyInstructions(lv_obj_t* obj)
{
    if (obj) lv_obj_delete_async(obj);
}

REGISTER_APP(Instructions, nullptr, "info.png",
             lv_CreateInstructions, lv_DestroyInstructions,
             nullptr, nullptr, nullptr, 0)

#endif // USE_LVGL