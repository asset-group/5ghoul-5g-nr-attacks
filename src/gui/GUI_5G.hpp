#pragma once

#ifndef __GUIIMG__
#define __GUIIMG__
// imgui includes
#include "gui/CEFHeadlessRenderer.hpp"
#include "libs/imgui/ImGuiFileDialog/ImGuiFileDialog.h"
#include "libs/imgui/imgui.h"
#include "libs/imgui/imgui_impl_glfw.h"
#include "libs/imgui/imgui_impl_opengl3.h"
#include "libs/imgui/imgui_internal.h"
#include "libs/imgui/imgui_memory_editor.hpp"
#include "libs/imgui/imguial_term.h"
#include "libs/imgui/implot/implot.h"
#include "libs/imgui/misc/fonts/DroidSans.hpp"
#include "libs/imgui/misc/fonts/DroidSansMono.hpp"
#include "libs/imgui/misc/fonts/IconsFontAwesome5.hpp"
#include "libs/imgui/misc/freetype/imgui_freetype.h"
#include "libs/log_misc_utils.hpp"
#include "libs/mv_average.hpp"
#include <fstream>
#include <mutex>
#include <queue>
#include <semaphore.h>
#include <stdio.h>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>
#include <graphviz/gvc.h>

extern "C" {
#include "libs/profiling.h"
}

using namespace std::chrono_literals;

#define constrain(amt, low, high) ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))

#define GL1(...) gui_log1.add_msg_color(ImGuiAl::Crt::CGA::BrightWhite, __VA_ARGS__)
#define GL1C(...) gui_log1.add_msg_color(ImGuiAl::Crt::CGA::BrightCyan, __VA_ARGS__)
#define GL1M(...) gui_log1.add_msg_color(ImGuiAl::Crt::CGA::BrightMagenta, __VA_ARGS__)
#define GL1G(...) gui_log1.add_msg_info(__VA_ARGS__)
#define GL1Y(...) gui_log1.add_msg_warning(__VA_ARGS__)
#define GL1R(...) gui_log1.add_msg_error(__VA_ARGS__)

#define GL2(...) gui_log2.add_msg_color(ImGuiAl::Crt::CGA::BrightWhite, __VA_ARGS__)
#define GL2C(...) gui_log2.add_msg_color(ImGuiAl::Crt::CGA::BrightCyan, __VA_ARGS__)
#define GL2M(...) gui_log2.add_msg_color(ImGuiAl::Crt::CGA::BrightMagenta, __VA_ARGS__)
#define GL2G(...) gui_log2.add_msg_info(__VA_ARGS__)
#define GL2Y(...) gui_log2.add_msg_warning(__VA_ARGS__)
#define GL2R(...) gui_log2.add_msg_error(__VA_ARGS__)

#define GL3(...) gui_log3.add_msg_color(ImGuiAl::Crt::CGA::BrightWhite, __VA_ARGS__)
#define GL3C(...) gui_log3.add_msg_color(ImGuiAl::Crt::CGA::BrightCyan, __VA_ARGS__)
#define GL3M(...) gui_log3.add_msg_color(ImGuiAl::Crt::CGA::BrightMagenta, __VA_ARGS__)
#define GL3G(...) gui_log3.add_msg_info(__VA_ARGS__)
#define GL3Y(...) gui_log3.add_msg_warning(__VA_ARGS__)
#define GL3R(...) gui_log3.add_msg_error(__VA_ARGS__)

#define GL4(...) gui_log4.add_msg_color(ImGuiAl::Crt::CGA::BrightWhite, __VA_ARGS__)
#define GL4C(...) gui_log4.add_msg_color(ImGuiAl::Crt::CGA::BrightCyan, __VA_ARGS__)
#define GL4M(...) gui_log4.add_msg_color(ImGuiAl::Crt::CGA::BrightMagenta, __VA_ARGS__)
#define GL4G(...) gui_log4.add_msg_info(__VA_ARGS__)
#define GL4Y(...) gui_log4.add_msg_warning(__VA_ARGS__)
#define GL4R(...) gui_log4.add_msg_error(__VA_ARGS__)

#define GL5(...) gui_log5.add_msg_color(ImGuiAl::Crt::CGA::BrightWhite, __VA_ARGS__)
#define GL5C(...) gui_log5.add_msg_color(ImGuiAl::Crt::CGA::BrightCyan, __VA_ARGS__)
#define GL5M(...) gui_log5.add_msg_color(ImGuiAl::Crt::CGA::BrightMagenta, __VA_ARGS__)
#define GL5G(...) gui_log5.add_msg_info(__VA_ARGS__)
#define GL5Y(...) gui_log5.add_msg_warning(__VA_ARGS__)
#define GL5R(...) gui_log5.add_msg_error(__VA_ARGS__)

#define GL6(...) gui_log6.add_msg_color(ImGuiAl::Crt::CGA::BrightWhite, __VA_ARGS__)
#define GL6C(...) gui_log6.add_msg_color(ImGuiAl::Crt::CGA::BrightCyan, __VA_ARGS__)
#define GL6M(...) gui_log6.add_msg_color(ImGuiAl::Crt::CGA::BrightMagenta, __VA_ARGS__)
#define GL6G(...) gui_log6.add_msg_info(__VA_ARGS__)
#define GL6Y(...) gui_log6.add_msg_warning(__VA_ARGS__)
#define GL6R(...) gui_log6.add_msg_error(__VA_ARGS__)

#define GL12(...) gui_log12.add_msg_color(ImGuiAl::Crt::CGA::BrightWhite, __VA_ARGS__)
#define GL12C(...) gui_log12.add_msg_color(ImGuiAl::Crt::CGA::BrightCyan, __VA_ARGS__)
#define GL12M(...) gui_log12.add_msg_color(ImGuiAl::Crt::CGA::BrightMagenta, __VA_ARGS__)
#define GL12G(...) gui_log12.add_msg_info(__VA_ARGS__)
#define GL12Y(...) gui_log12.add_msg_warning(__VA_ARGS__)
#define GL12R(...) gui_log12.add_msg_error(__VA_ARGS__)

using namespace std::chrono_literals;
using namespace std;

#define GUI_APP_NAME "LTE Fuzzer (WDissector)"
#define GUI_LOG0_NAME "Latency"
#define GUI_LOG1_NAME "Events"
#define GUI_LOG2_NAME "Downlink / Uplink "
#define GUI_LOG3_NAME "SIB"
#define GUI_LOG4_NAME "MIB"
#define GUI_LOG5_NAME "Summary"
#define GUI_LOG6_NAME "PDCP"
#define GUI_LOG7_NAME ICON_FA_NETWORK_WIRED " Open5GS"
#define GUI_LOG8_NAME ICON_FA_BROADCAST_TOWER " OpenAirInterface"
#define GUI_LOG9_NAME ICON_FA_MOBILE_ALT " UE Simulator"
#define GUI_LOG10_NAME ICON_FA_HEARTBEAT " Monitor"
#define GUI_LOG11_NAME ICON_FA_USB " ModemManager"
#define GUI_LOG12_NAME ICON_FA_SERVER " NAS"

#define GUI_CONFIG_PATH "./bin/.lte_fuzz"
#define GUI_LOG_LIMIT 1000   // Max lines
#define GUI_SUMMARY_LIMIT 11 // Max lines for summary

#define GUI_UPDATE_INTERVAL 0.033s // About 30fps
#define GUI_GLSL_VERSION "#version 130"

static std::thread *GUI_Thread_Handler;
bool gui_enabled = false;
GLFWwindow *window = nullptr;
static bool g_sync_100 = true;
static bool g_sync_1000 = true;
mv_average<30> loop_time_avg;
mv_average<3> graphviz_time_avg;

ImFont *font_default;
ImFont *font_large;

CEF::HeadlessClient WebView;
std::mutex mutex_opengl;
static sem_t sem_webview_done;
atomic<bool> webview_has_drawn = false;

GLuint image_states_texture = 0;
struct ImVec2 image_states_size;

static FILE *pFile;

struct gui_main_window {
    bool valid = false;
    int posX;
    int posY;
    int width;
    int heigth;
} current_main_window;

typedef struct
{
    unsigned char *image_data;
    int image_width;
    int image_height;
} FIG_REQ;

ImGuiAl::Log gui_log0(GUI_LOG_LIMIT, GUI_LOG0_NAME, true);
ImGuiAl::Log gui_log1(GUI_LOG_LIMIT, GUI_LOG1_NAME, true);
ImGuiAl::Log gui_log2(GUI_LOG_LIMIT, GUI_LOG2_NAME, false, true, true);
ImGuiAl::Log gui_log3(GUI_LOG_LIMIT, GUI_LOG3_NAME);
ImGuiAl::Log gui_log4(GUI_LOG_LIMIT, GUI_LOG4_NAME);
ImGuiAl::Log gui_log5(GUI_SUMMARY_LIMIT, GUI_LOG5_NAME, false, true);
ImGuiAl::Log gui_log6(GUI_LOG_LIMIT, GUI_LOG6_NAME, false, true, true);
ImGuiAl::Log gui_log7(GUI_LOG_LIMIT, GUI_LOG7_NAME);
ImGuiAl::Log gui_log8(GUI_LOG_LIMIT, GUI_LOG8_NAME);
ImGuiAl::Log gui_log9(GUI_LOG_LIMIT, GUI_LOG9_NAME);
ImGuiAl::Log gui_log10(GUI_LOG_LIMIT, GUI_LOG10_NAME);
ImGuiAl::Log gui_log11(GUI_LOG_LIMIT, GUI_LOG11_NAME);
ImGuiAl::Log gui_log12(GUI_LOG_LIMIT, GUI_LOG12_NAME, false, true, true);

std::queue<FIG_REQ> figure_requests;

struct FreeType {
    enum FontBuildMode {
        FontBuildMode_FreeType,
        FontBuildMode_Stb
    };

    FontBuildMode BuildMode;
    bool WantRebuild;
    float FontsMultiply;
    int FontsPadding;
    unsigned int FontsFlags;

    FreeType()
    {
        BuildMode = FontBuildMode_FreeType;
        WantRebuild = true;
        FontsMultiply = 1.0f;
        FontsPadding = 1;
        FontsFlags = 0;
    }

    // Call _BEFORE_ NewFrame()
    bool UpdateRebuild()
    {
        if (!WantRebuild)
            return false;
        ImGuiIO &io = ImGui::GetIO();
        io.Fonts->TexGlyphPadding = FontsPadding;
        for (int n = 0; n < io.Fonts->ConfigData.Size; n++) {
            ImFontConfig *font_config = (ImFontConfig *)&io.Fonts->ConfigData[n];
            font_config->RasterizerMultiply = FontsMultiply;
            font_config->FontBuilderFlags = (BuildMode == FontBuildMode_FreeType) ? FontsFlags : 0x00;
        }
        if (BuildMode == FontBuildMode_FreeType)
            ImGuiFreeType::BuildFontAtlas(io.Fonts, FontsFlags);
        else if (BuildMode == FontBuildMode_Stb)
            io.Fonts->Build();
        WantRebuild = false;
        return true;
    }

    // Call to draw interface
    void ShowFreetypeOptionsWindow()
    {
        ImGui::Begin("FreeType Options");
        ImGui::ShowFontSelector("Fonts");
        WantRebuild |= ImGui::RadioButton("FreeType", (int *)&BuildMode, FontBuildMode_FreeType);
        ImGui::SameLine();
        WantRebuild |= ImGui::RadioButton("Stb (Default)", (int *)&BuildMode, FontBuildMode_Stb);
        WantRebuild |= ImGui::DragFloat("Multiply", &FontsMultiply, 0.001f, 0.0f, 2.0f);
        WantRebuild |= ImGui::DragInt("Padding", &FontsPadding, 0.1f, 0, 16);
        if (BuildMode == FontBuildMode_FreeType) {
            WantRebuild |= ImGui::CheckboxFlags("NoHinting", &FontsFlags, ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_NoHinting);
            WantRebuild |= ImGui::CheckboxFlags("NoAutoHint", &FontsFlags, ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_NoAutoHint);
            WantRebuild |= ImGui::CheckboxFlags("ForceAutoHint", &FontsFlags, ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_ForceAutoHint);
            WantRebuild |= ImGui::CheckboxFlags("LightHinting", &FontsFlags, ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_LightHinting);
            WantRebuild |= ImGui::CheckboxFlags("MonoHinting", &FontsFlags, ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_MonoHinting);
            WantRebuild |= ImGui::CheckboxFlags("Bold", &FontsFlags, ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Bold);
            WantRebuild |= ImGui::CheckboxFlags("Oblique", &FontsFlags, ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Oblique);
            WantRebuild |= ImGui::CheckboxFlags("Monochrome", &FontsFlags, ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_Monochrome);
        }
        ImGui::End();
    }
};

inline bool GUI_sync_100()
{
    if (g_sync_100) {
        g_sync_100 = false;
        return true;
    }
    else {
        return false;
    }
}

inline bool GUI_sync_1000()
{
    if (g_sync_1000) {
        g_sync_1000 = false;
        return true;
    }
    else {
        return false;
    }
}

static void HelpMarker(const char *desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}
namespace ImGui {

    static inline void CheckboxInputDouble(const char *label, const char *var_name, bool *enable, double *val, double min = 0.0, double max = 1.0, const char *fmt = "%.02f")
    {
        Checkbox(label, enable);
        SameLine();
        const double d_step = 0.05;
        PushItemWidth(85);
        InputDouble(var_name, val, 0.05, 0.05, fmt);
        // InputScalar(var_name, ImGuiDataType_Double, val, &d_step, &d_step, fmt);
        *val = constrain(*val, min, max);
        PopItemWidth();
    }

    static inline void CheckboxInputInt(const char *label, const char *var_name, bool *enable, int *val, int min = 0, int max = 1, int step = 100)
    {
        Checkbox(label, enable);
        SameLine();
        PushItemWidth(85);
        InputInt(var_name, val, step);
        *val = constrain(*val, min, max);
        PopItemWidth();
    }

    static inline void ComboString(const char *label, std::vector<std::string> *str_list, int *idx_var, std::string *target_str = NULL)
    {
        // ComboBox
        const char **str_chars = new const char *[str_list->size()];

        for (size_t i = 0; i < str_list->size(); i++) {
            str_chars[i] = str_list->at(i).c_str();
        }

        if (Combo(label, idx_var, str_chars, str_list->size())) {
            if (target_str != NULL)
                *target_str = str_list->at(*idx_var);
        };

        delete[] str_chars;
    }

    static inline void FileDialog(const std::string &vKey, const char *vName, const char *vFilters,
                                  const std::string &vPath, const std::string &vDefaultFileName,
                                  const int &vCountSelectionMax = 1)
    {
        igfd::ImGuiFileDialog::Instance()->OpenDialog(vKey, vName, vFilters, vPath, vDefaultFileName, vCountSelectionMax);
    }

    // Returns: 0 - No action, 1 - OK, 2 - Cancel
    static inline int CheckFileDialog(const char *label, ImGuiWindowFlags vFlags = 0)
    {
        int ret = 0;
        if (igfd::ImGuiFileDialog::Instance()->FileDialog(label, ImGuiWindowFlags_NoCollapse | vFlags)) {
            // action if OK
            if (igfd::ImGuiFileDialog::Instance()->IsOk == true) {
                ret = 1;
            }
            else {
                ret = 2;
            }
            // close
            igfd::ImGuiFileDialog::Instance()->CloseDialog(label);
        }
        return ret;
    }

    // return selected file
    static inline std::string CheckFileDialogPath()
    {
        return igfd::ImGuiFileDialog::Instance()->GetFilePathName();
    }

    // return file selection list for multiple file selection dialog
    static inline std::vector<std::string> CheckFileDialogPaths()
    {
        std::vector<std::string> paths;
        for (auto &path : igfd::ImGuiFileDialog::Instance()->GetSelection()) {
            paths.push_back(path.second);
        }

        return paths;
    }

} // namespace ImGui

std::vector<void (*)()> gui_user_fcn_list;

void gui_add_user_fcn(void (*function)())
{
    gui_user_fcn_list.push_back(function);
}

void gui_style_vscode()
{
    constexpr auto ColorFromBytes = [](uint8_t r, uint8_t g, uint8_t b) {
        return ImVec4((float)r / 255.0f, (float)g / 255.0f, (float)b / 255.0f, 1.0f);
    };

    auto &style = ImGui::GetStyle();
    ImVec4 *colors = style.Colors;

    const ImVec4 bgColor = ColorFromBytes(37, 37, 38);
    const ImVec4 lightBgColor = ColorFromBytes(82, 82, 85);
    const ImVec4 veryLightBgColor = ColorFromBytes(90, 90, 95);

    const ImVec4 panelColor = ColorFromBytes(51, 51, 55);
    const ImVec4 panelHoverColor = ColorFromBytes(29, 151, 236);
    const ImVec4 panelActiveColor = ColorFromBytes(0, 119, 200);

    const ImVec4 textColor = ColorFromBytes(255, 255, 255);
    const ImVec4 textDisabledColor = ColorFromBytes(151, 151, 151);
    const ImVec4 borderColor = ColorFromBytes(78, 78, 78);

    colors[ImGuiCol_Text] = textColor;
    colors[ImGuiCol_TextDisabled] = textDisabledColor;
    colors[ImGuiCol_TextSelectedBg] = panelActiveColor;
    colors[ImGuiCol_WindowBg] = bgColor;
    colors[ImGuiCol_ChildBg] = bgColor;
    colors[ImGuiCol_PopupBg] = bgColor;
    colors[ImGuiCol_Border] = borderColor;
    colors[ImGuiCol_BorderShadow] = borderColor;
    colors[ImGuiCol_FrameBg] = panelColor;
    colors[ImGuiCol_FrameBgHovered] = panelHoverColor;
    colors[ImGuiCol_FrameBgActive] = panelActiveColor;
    colors[ImGuiCol_TitleBg] = bgColor;
    colors[ImGuiCol_TitleBgActive] = bgColor;
    colors[ImGuiCol_TitleBgCollapsed] = bgColor;
    colors[ImGuiCol_MenuBarBg] = panelColor;
    colors[ImGuiCol_ScrollbarBg] = panelColor;
    colors[ImGuiCol_ScrollbarGrab] = lightBgColor;
    colors[ImGuiCol_ScrollbarGrabHovered] = veryLightBgColor;
    colors[ImGuiCol_ScrollbarGrabActive] = veryLightBgColor;
    colors[ImGuiCol_CheckMark] = panelActiveColor;
    colors[ImGuiCol_SliderGrab] = panelHoverColor;
    colors[ImGuiCol_SliderGrabActive] = panelActiveColor;
    colors[ImGuiCol_Button] = panelColor;
    colors[ImGuiCol_ButtonHovered] = panelHoverColor;
    colors[ImGuiCol_ButtonActive] = panelHoverColor;
    colors[ImGuiCol_Header] = panelColor;
    colors[ImGuiCol_HeaderHovered] = panelHoverColor;
    colors[ImGuiCol_HeaderActive] = panelActiveColor;
    colors[ImGuiCol_Separator] = borderColor;
    colors[ImGuiCol_SeparatorHovered] = borderColor;
    colors[ImGuiCol_SeparatorActive] = borderColor;
    colors[ImGuiCol_ResizeGrip] = bgColor;
    colors[ImGuiCol_ResizeGripHovered] = panelColor;
    colors[ImGuiCol_ResizeGripActive] = lightBgColor;
    colors[ImGuiCol_PlotLines] = panelActiveColor;
    colors[ImGuiCol_PlotLinesHovered] = panelHoverColor;
    colors[ImGuiCol_PlotHistogram] = panelActiveColor;
    colors[ImGuiCol_PlotHistogramHovered] = panelHoverColor;
    colors[ImGuiCol_ModalWindowDimBg] = bgColor;
    colors[ImGuiCol_DragDropTarget] = bgColor;
    colors[ImGuiCol_NavHighlight] = bgColor;
    colors[ImGuiCol_DockingPreview] = panelActiveColor;
    colors[ImGuiCol_Tab] = bgColor;
    colors[ImGuiCol_TabActive] = panelActiveColor;
    colors[ImGuiCol_TabUnfocused] = bgColor;
    colors[ImGuiCol_TabUnfocusedActive] = panelActiveColor;
    colors[ImGuiCol_TabHovered] = panelHoverColor;

    style.WindowRounding = 0.0f;
    style.ChildRounding = 0.0f;
    style.FrameRounding = 0.0f;
    style.GrabRounding = 0.0f;
    style.PopupRounding = 0.0f;
    style.ScrollbarRounding = 0.0f;
    style.TabRounding = 0.0f;

    style.FrameRounding = 2.0f;
}

inline void MainView()
{
    // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
    // because it would be confusing to have two docking targets within each others.
    static ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_NoCloseButton | ImGuiDockNodeFlags_NoWindowMenuButton;

    ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    // window_flags |= ImGuiWindowFlags_NoBackground;

    // Keep Dockspace() active even if minimized
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockView", NULL, window_flags);
    ImGui::PopStyleVar();
    ImGui::PopStyleVar(2);

    // DockSpace
    ImGuiID dockspace_id = ImGui::GetID("MainView");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Close LTE Fuzzer", NULL, false, true)) {
                // TODO: Proper handling
                glfwSetWindowShouldClose(window, 1);
            }
            ImGui::EndMenu();
        }
        ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x - 180, 0));
        ImGui::PushStyleColor(ImGuiCol_Text, ImGuiAl::Crt::CGA::White);
        ImGui::Text("GUI loop time (us): %ld", loop_time_avg.mean());
        ImGui::PopStyleColor();

        ImGui::EndMenuBar();
    }

    ImGui::End();
}

static inline bool LoadTextureFromFile(const char *filename, GLuint *out_texture, struct ImVec2 *image_size)
{
    int image_width = 0;
    int image_height = 0;
    // Load from file
    unsigned char *image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);

    if (image_data == NULL)
        return false;

    if (*out_texture == 0) {
        // Gen texture
        GLuint image_texture = 0;
        glGenTextures(1, &image_texture);
        glBindTexture(GL_TEXTURE_2D, image_texture);
        *out_texture = image_texture;
    }
    else {
        // Update texture
        glBindTexture(GL_TEXTURE_2D, *out_texture);
        // update = 1;
    }

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload pixels into texture
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);

    image_size->x = (float)image_width;
    image_size->y = (float)image_height;
    return true;
}

static inline bool LoadTextureFromMemory(unsigned char const *buffer, int len, GLuint *out_texture, struct ImVec2 *image_size)
{
    int image_width = 0;
    int image_height = 0;
    // Load from memory
    unsigned char *image_data = stbi_load_from_memory(buffer, len, &image_width, &image_height, NULL, 4);
    if (image_data == NULL)
        return false;

    if (*out_texture == 0) {
        // Gen texture
        GLuint image_texture = 0;
        glGenTextures(1, &image_texture);
        glBindTexture(GL_TEXTURE_2D, image_texture);
        *out_texture = image_texture;
    }
    else {
        // Update texture
        glBindTexture(GL_TEXTURE_2D, *out_texture);
    }

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload pixels into texture
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);

    stbi_image_free(image_data);

    image_size->x = (float)image_width;
    image_size->y = (float)image_height;
    // glFinish();
    return true;
}

static inline bool UpdateTexture(FIG_REQ &freq, GLuint *out_texture, struct ImVec2 *image_size)
{

    if (*out_texture == 0) {
        // Gen texture
        GLuint image_texture = 0;
        glGenTextures(1, &image_texture);
        glBindTexture(GL_TEXTURE_2D, image_texture);
        *out_texture = image_texture;
    }
    else {
        // Update texture
        glBindTexture(GL_TEXTURE_2D, *out_texture);
    }

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload pixels into texture
    // glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    // glReadPixels(0, 0, freq.image_width, freq.image_height, GL_RGBA, GL_UNSIGNED_BYTE, freq.image_data);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, freq.image_width, freq.image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, freq.image_data);

    stbi_image_free(freq.image_data);

    image_size->x = (float)freq.image_width;
    image_size->y = (float)freq.image_height;

    return true;
}

static inline bool UpdateTexture(GLuint *out_texture, int width, int height, const void *image_data)
{

    if (*out_texture == 0) {
        // Gen texture
        GLuint image_texture = 0;
        glGenTextures(1, &image_texture);
        *out_texture = image_texture;
    }

    // Select texture
    glBindTexture(GL_TEXTURE_2D, *out_texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Upload pixels into texture
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    // glReadPixels(0, 0, freq.image_width, freq.image_height, GL_RGBA, GL_UNSIGNED_BYTE, freq.image_data);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, image_data);
    glBindTexture(GL_TEXTURE_2D, 0);

    return true;
}

uint8_t gui_set_graph_dot(const string dot_graph)
{
    static GVC_t *gvc = gvContext();
    static std::mutex dot_mutex;

    if (!gui_enabled)
        return 0;

    lock_guard<mutex> m(dot_mutex);
    Agraph_t *g;
    char *buf;
    unsigned int buf_len;

    profiling_timer_start(11);
    g = agmemread(dot_graph.c_str());

    if (g == NULL) {
        return 0;
    }

    gvLayout(gvc, g, "dot");
    gvRenderData(gvc, g, "svg", &buf, &buf_len);
    gvFreeLayout(gvc, g);
    agclose(g);

    if (buf) {
        const string s = buf;
        WebView.ExecuteFunction("SVGUpdate", s);
    }

    gvFreeRenderData(buf);
    uint32_t graphviz_time = profiling_timer_end(11) / 1000000;
    graphviz_time_avg.add_sample(graphviz_time);
    return 1;
}

inline void StateView()
{
    ImGui::Begin(ICON_FA_SITEMAP " States", NULL, ImGuiWindowFlags_NoCollapse);
    if (image_states_texture) {
        static ImVec2 img_size = ImVec2(ImGui::GetWindowWidth(), image_states_size.y);
        static uint8_t op = 0;
        static uint8_t mouse_down_persist = 0;
        float ratio, x_area;

        ImGui::BeginChild("States View", ImVec2(0, 0), false, ImGuiWindowFlags_NoScrollbar);

        if (ImGui::IsWindowHovered() || mouse_down_persist) {
            // Handle pan and zoom
            ImGuiIO &io = ImGui::GetIO();
            ImVec2 window_offset = ImGui::GetCursorScreenPos();
            ImVec2 mouse_pos = io.MousePos;
            mouse_pos.x = mouse_pos.x - window_offset.x;
            mouse_pos.y = mouse_pos.y - window_offset.y;

            WebView.SetMouseMove((int)mouse_pos.x, (int)mouse_pos.y, false);

            if (io.KeyShift) {
                // Horizontal Scrool
                WebView.SetScroll(io.MouseWheel, 0);
            }
            else {
                // Vertical Scrool
                WebView.SetScroll(0, io.MouseWheel);
            }

            if (io.MouseDown[0]) {
                // Pan Image (Not used anymore as webview handles it)
                // ImGui::SetScrollX(ImGui::GetScrollX() - io.MouseDelta.x);
                // ImGui::SetScrollY(ImGui::GetScrollY() - io.MouseDelta.y);

                if (!mouse_down_persist)
                    WebView.SetMouseClick(0, 1, (int)mouse_pos.x, (int)mouse_pos.y);

                mouse_down_persist = 1; // Allow mouse to go outside window
            }
            else {
                WebView.SetMouseClick(0, 0, (int)mouse_pos.x, (int)mouse_pos.y);
                mouse_down_persist = 0;
            }
        }

        switch (op) {
        case 0:
            // Original
            img_size = image_states_size;
            break;
        case 1: // Stretch to fit
            img_size = ImGui::GetWindowSize();
            break;
        case 2: // Fit W
            ratio = (ImGui::GetWindowWidth()) / image_states_size.x;
            img_size = ImVec2((ImGui::GetWindowWidth()), image_states_size.y * ratio);
            break;
        case 3: // Fit H
            ratio = ImGui::GetWindowHeight() / image_states_size.y;
            img_size = ImVec2(image_states_size.x * ratio, ImGui::GetWindowHeight());
            break;
        }

        // Center image to window by creating dummy item
        x_area = ImGui::GetWindowWidth() - (img_size.x);
        if (x_area > 0) {
            ImGui::ItemSize(ImVec2((x_area / 2) - 8.0, 0));
            ImGui::SameLine();
        }

        // Get window space and rectangles
        ImVec2 window_size = ImGui::GetWindowSize();
        ImVec2 min_s = ImGui::GetWindowPos();                           // Absolute coordinate
        ImVec2 max_s(min_s.x + window_size.x, min_s.y + window_size.y); // Absolute coordinate
        // White fill
        ImGui::GetWindowDrawList()->AddRectFilled(min_s, max_s, ImGuiAl::Crt::CGA::BrightWhite);
        // Image
        ImGui::Image((void *)(intptr_t)image_states_texture, ImVec2(img_size.x, img_size.y));
        ImGui::EndChild();

        // Resize WebView
        WebView.SetViewSize((int)window_size.x, (int)window_size.y);
    }
    else {
        // Resize WebView
        ImVec2 window_size = ImGui::GetWindowSize();
        WebView.SetViewSize((int)window_size.x, (int)window_size.y);
        ImGui::PushStyleColor(ImGuiCol_Text, ImGuiAl::Crt::CGA::BrightRed);
        ImGui::TextUnformatted("No preview available...");
        ImGui::PopStyleColor();
    }

    ImGui::End();
}

int GUI_Init(int argc, char **argv, bool enable = true)
{
    if (getuid()) {
        LOGR("Not running as root.");
        exit(1);
    }

    bool test_webview_performance = false;

    // Check if gui is enabled via command line
    for (size_t i = 0; i < argc; i++) {
            // puts(argv[i]);
        if (!strcmp(argv[i], "--gui"))
            enable = true;
        else if (!strcmp(argv[i], "--no-gui"))
            enable = false;
        else if (!strcmp(argv[i], "--test-webview"))
            test_webview_performance = true;
        else if (!strcmp(argv[i], "--type=zygote"))
            enable = true;
        else if (!strcmp(argv[i], "--type=utility"))
            enable = true;
        else if (!strcmp(argv[i], "--type=gpu-process"))
            enable = true;
    }

    if (!enable)
        return 0;

    // Initialize WebView
    if (!test_webview_performance)
        WebView.init(argc, argv, WebView.RelativeFileURL("modules/webview/svg_viewer.html"),
                     30, true, true, "/usr/local/share/wdissector/bin", true);
    else
        WebView.init(argc, argv, "https://alteredqualia.com/xg/examples/twister.html",
                     60, false, true, "/usr/local/share/wdissector/bin");

    gui_log3.setHideOptions(true);
    gui_log4.setHideOptions(true);
    gui_log5.setHideOptions(true);
    gui_log1.forceHorizontalScrollbar(true);
    gui_log2.forceHorizontalScrollbar(true);
    gui_log3.forceHorizontalScrollbar(true);
    gui_log4.forceHorizontalScrollbar(true);
    gui_log6.forceHorizontalScrollbar(true);
    gui_log7.forceHorizontalScrollbar(true);
    gui_log8.forceHorizontalScrollbar(true);
    gui_log9.forceHorizontalScrollbar(true);
    gui_log10.forceHorizontalScrollbar(true);
    gui_log11.forceHorizontalScrollbar(true);
    gui_log12.forceHorizontalScrollbar(true);

    // Register WebView ModuleLoaded callback
    sem_init(&sem_webview_done, 0, 0);
    WebView.OnModuleLoaded([&] {
        sem_post(&sem_webview_done);
    });

    // Register callback to update WebView texture
    WebView.OnTextureUpdate([&](int width, int height, const void *image_data) {
        if (!webview_has_drawn && gui_enabled) {
            mutex_opengl.lock();
            glfwMakeContextCurrent(window);
            glFinish();
            glActiveTexture(GL_TEXTURE0);
            UpdateTexture(&image_states_texture, width, height, image_data);
            image_states_size.x = width;
            image_states_size.y = height;
            glFlush();
            glFinish();
            glfwMakeContextCurrent(NULL);
            webview_has_drawn = true;
            mutex_opengl.unlock();
        }
    });

    // Start a new GUI rendering thread
    GUI_Thread_Handler = new std::thread([&]() {
        // Set schedule priority
        struct sched_param sp;
        sp.sched_priority = sched_get_priority_min(SCHED_IDLE);
        pthread_t this_thread = pthread_self();
        sched_setscheduler(0, SCHED_IDLE, &sp);

        // ----- Initialize glfw and ImGui ------
        // Setup glfw error callback
        glfwSetErrorCallback([](int error, const char *description) {
            LOGR(description);
        });

        if (!glfwInit())
            return 1;

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

        // Restore window config if existent
        pFile = fopen(GUI_CONFIG_PATH, "rb");
        if (pFile) {
            fread((void *)&current_main_window, sizeof(gui_main_window), 1, pFile);
            fclose(pFile);
            window = glfwCreateWindow(current_main_window.width, current_main_window.heigth, GUI_APP_NAME, NULL, NULL);
            glfwSetWindowPos(window, current_main_window.posX, current_main_window.posY);
        }
        else {
            // Create glfw window
            window = glfwCreateWindow(1280, 720, GUI_APP_NAME, NULL, NULL);
        }

        if (window == NULL)
            return 1;

        glfwMakeContextCurrent(window);
        // Set app icon
        std::ifstream iconfile("icon.png");
        if (iconfile.good()) {
            int width, height, nrChannels;
            unsigned char *img = stbi_load("icon.png", &width, &height, &nrChannels, 4);
            GLFWimage icon = {width, height, img};
            glfwSetWindowIcon(window, 1, &icon);
            stbi_image_free(img);
        }
        // Disable vsync (use timer delay instead)
        glfwSwapInterval(0);
        if (gl3wInit() != 0) {
            LOGR("GL3W window closed");
            return 1;
        }
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImPlot::CreateContext();

        ImGuiIO &io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;     // Enable Docking

        // ----- Change Styles or Fonts here ------
        // Setup Dear ImGui style
        gui_style_vscode();
        // Setup Font
        FreeType gui_font;
        AddFont_DroidSansMono(14.0f);
        AddFont_IconsFontAwesome5_Solid(16.0f);
        font_default = AddFont_IconsFontAwesome5_Brands(16.0f);
        // Extra font
        AddFont_DroidSansMono(16.0f);
        AddFont_IconsFontAwesome5_Solid(14.0f);
        font_large = AddFont_IconsFontAwesome5_Brands(14.0f);
        // Configure file explorer options
        igfd::ImGuiFileDialog::Instance()->SetExtentionInfos(".json", ImGui::GetStyle().Colors[ImGuiCol_Text], ICON_FA_SITEMAP);
        igfd::ImGuiFileDialog::Instance()->SetExtentionInfos(".pcap", ImGui::GetStyle().Colors[ImGuiCol_Text], ICON_FA_NETWORK_WIRED);
        igfd::ImGuiFileDialog::Instance()->SetExtentionInfos(".pcapng", ImGui::GetStyle().Colors[ImGuiCol_Text], ICON_FA_NETWORK_WIRED);

        // ----- Setup Platform/Renderer bindings ------
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init(GUI_GLSL_VERSION);

        ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
        uint32_t loop_counter = 0;
        uint32_t loop_time = 0;
        uint8_t sem_done = 0;

        while (!glfwWindowShouldClose(window)) {

            // UI Loop
            mutex_opengl.lock();
            profiling_timer_start(10);
            loop_counter += 1;
            // Take glfw context back
            glfwMakeContextCurrent(window);
            // Poll glfw events
            glfwPollEvents();
            // Create new glfw ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            // Reupload font texture to GPU
            if (gui_font.UpdateRebuild()) {
                ImGui_ImplOpenGL3_DestroyDeviceObjects();
                ImGui_ImplOpenGL3_CreateDeviceObjects();
            }
            // Create new Dear ImGui frame
            ImGui::NewFrame();
            // Render Main layout view
            MainView();
            // Render Logs
            // TODO: Add this as callback
            gui_log1.draw();
            gui_log3.draw();
            gui_log4.draw();
            gui_log2.draw();
            ImGui::PushFont(font_large);
            gui_log5.draw();
            ImGui::PopFont();
            gui_log10.draw();
            gui_log11.draw();
            gui_log12.draw();
            gui_log6.draw();
            gui_log9.draw();
            gui_log8.draw();
            gui_log7.draw();

            // Iterate over custom user functions
            for (size_t i = 0; i < gui_user_fcn_list.size(); i++) {
                gui_user_fcn_list[i]();
            }

            StateView();

            // Render ImGUI
            ImGui::Render();
            int display_w, display_h;
            glfwGetFramebufferSize(window, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapBuffers(window);

            if (!(loop_counter % 3)) // About 100ms
                g_sync_100 = true;

            if (!(loop_counter % 30)) // About 1s
                g_sync_1000 = true;

            // Measure loop time
            loop_time = profiling_timer_end(10) / 1000;
            loop_time_avg.add_sample(loop_time);
            glFinish();
            // Detach glfw context
            glfwMakeContextCurrent(NULL);
            webview_has_drawn = false;
            mutex_opengl.unlock();

            if (!gui_enabled)
                gui_enabled = true;
            // Wait for next update
            std::this_thread::sleep_for(GUI_UPDATE_INTERVAL);
        }

        // Save last window size and pos
        pFile = fopen(GUI_CONFIG_PATH, "wb");
        if (pFile) {

            struct gui_main_window gm;
            glfwGetWindowPos(window, &gm.posX, &gm.posY);
            glfwGetWindowSize(window, &gm.width, &gm.heigth);
            fwrite((void *)&gm, sizeof(gui_main_window), 1, pFile);
            fclose(pFile);
            LOGG("GUI Options Saved");
        }

        // Cleanup
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImPlot::DestroyContext();
        ImGui::DestroyContext();
        if (window) {
            glfwDestroyWindow(window);
        }

        gui_enabled = false;
        kill(getpid(), SIGUSR1); // Notify main thread of closed window
        pthread_exit(NULL);
    });
    pthread_setname_np(GUI_Thread_Handler->native_handle(), "GUI");
    
    LOGG("Waiting GUI Thread Startup...");
    uint8_t wait_gui_count = 0;
    while (!gui_enabled) {
        usleep(10000UL); // Sleep 10ms
    }
    LOGG("GUI Thread Running");

    return 0;
}

#endif