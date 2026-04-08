#if USE_LVGL

#include "crosspad/app/AppRegistrar.hpp"
#include "crosspad-gui/platform/IGuiPlatform.h"
#include "crosspad-gui/components/markdown_view.h"
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
    (void)a;

    lv_obj_t* container = lv_obj_create(parent);
    lv_obj_set_size(container, lv_obj_get_content_width(parent),
                    lv_obj_get_content_height(parent));
    lv_obj_set_style_bg_color(container, lv_color_black(), 0);
    lv_obj_set_style_border_width(container, 0, 0);
    lv_obj_set_style_pad_all(container, 0, 0);
    lv_obj_set_style_radius(container, 0, 0);
    lv_obj_center(container);

    lv_obj_t* content = lv_obj_create(container);
    lv_obj_set_size(content, LV_PCT(100), LV_PCT(100));
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

    return container;
}

static void lv_DestroyInstructions(lv_obj_t* obj)
{
    (void)obj;
}

REGISTER_APP(Instructions, nullptr, "info.png",
             lv_CreateInstructions, lv_DestroyInstructions,
             nullptr, nullptr, nullptr, 0)

#endif // USE_LVGL