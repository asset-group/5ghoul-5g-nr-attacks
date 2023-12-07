#include "imguial_term.h"
#include <iostream>
#include <stdio.h>
#include <ctype.h>

// extern "C"{
//     #include "libs/profiling.h"
// }

ImGuiAl::Crt::Crt(size_t const size)
    : _foregroundColor(CGA::White),
      _metaData(0),
      _scrollToBottom(false),
      _log_to_file(false),
      _gui_enabled(true),
      _max_lines(size),
      _autoscroll(true),
      _linespacing(false),
      _selectable(false),
      _line_selected(-1),
      _cur_line(0),
      _horizontal_flag(ImGuiWindowFlags_HorizontalScrollbar)
{
    _fifo.resize(sizeof(MSGS) * size);
    _fifo.resize(0);
}

void ImGuiAl::Crt::setForegroundColor(ImU32 const color)
{
    _foregroundColor = color;
}

void ImGuiAl::Crt::setMetaData(unsigned const meta_data)
{
    _metaData = meta_data;
}

void ImGuiAl::Crt::printf(char const *const format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

void ImGuiAl::Crt::vprintf(char const *const format, va_list args)
{
    char temp[1024];
    std::vsnprintf(temp, sizeof(temp), format, args);
    va_end(args);

    if (_fifo.size() >= _max_lines)
    {
        _fifo.pop_front();
    }

    _fifo.push_back({std::string(temp), _foregroundColor, _metaData});
}

void ImGuiAl::Crt::enableLogToFile(bool value, const std::string folder_name, const std::string file_name, int exclude_core)
{
    if (value && !_logger.initialized)
    {
        system(("mkdir -p " + folder_name).c_str());
        _logger.initialize(nanolog::NonGuaranteedLogger(4), folder_name + "/", file_name, 10, false, exclude_core);
    }

    _log_to_file = value;
}

void ImGuiAl::Crt::disableGUI(bool value)
{
    _gui_enabled = !value;
}

void ImGuiAl::Crt::forceHorizontalScrollbar(bool value)
{
    _horizontal_flag = (value ? ImGuiWindowFlags_AlwaysHorizontalScrollbar : ImGuiWindowFlags_HorizontalScrollbar);
}

void ImGuiAl::Crt::scrollToBottom()
{
    _scrollToBottom = true;
}

void ImGuiAl::Crt::clear()
{
    _fifo.resize(0);
}

void ImGuiAl::Crt::iterate(const std::function<bool(MSGS const &header, char const *const line)> &iterator)
{

    ImGuiListClipper clipper;
    clipper.Begin(_fifo.size());
    while (clipper.Step())
    {
        // LOG1(clipper.DisplayStart);
        for (int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++)
        {
            mutex_log.lock();
            MSGS &header = _fifo[line_no];
            if (!iterator(header, header.msg.c_str()))
            {
                mutex_log.unlock();
                break;
            }
            mutex_log.unlock();
        }
    }
    clipper.End();
}

void ImGuiAl::Crt::draw(ImVec2 const &size)
{
    draw(size, [](MSGS const &header, char const *const line) -> bool {
        (void)header;
        (void)line;
        return true;
    });
}

void ImGuiAl::Crt::draw(ImVec2 const &size, const std::function<bool(MSGS const &header, char const *const line)> &filter)
{
    char id[64];
    snprintf(id, sizeof(id), "ImGuiAl::Crt@%p", this);
    static int right_clicked = -1;
    ImGui::BeginChild(id, size, false, _horizontal_flag);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4.0f, 1.0f));
    int cur_line = 0;

    iterate([&filter, &cur_line, this](MSGS const &header, char const *const line) -> bool {
        if (filter(header, line))
        {
            if (this->_linespacing)
            {
                ImGui::PushStyleColor(ImGuiCol_Separator, IM_COL32(0x55, 0x55, 0x55, 100));
                ImGui::Separator();
                ImGui::PopStyleColor();
            }

            if (_selectable)
            {
                ImVec2 cur_pos = ImGui::GetCursorPos();
                ImGui::Dummy(ImVec2(ImGui::GetWindowContentRegionWidth(), 14.0f));
                if (ImGui::IsItemHovered())
                {
                    ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetItemRectMin(),
                                                              ImGui::GetItemRectMax(),
                                                              IM_COL32(29, 151, 236, 190), 2.0f);

                    if (ImGui::IsItemClicked(0))
                    {
                        _autoscroll = false;
                        _line_selected = header.idx;
                    }
                }
                else if (_line_selected == header.idx)
                {
                    ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetItemRectMin(),
                                                              ImGui::GetItemRectMax(),
                                                              IM_COL32(0, 119, 200, 190), 2.0f);
                }

                ImGui::SetCursorPos(cur_pos);
                ImGui::Text("%d -", header.idx + 1);
                ImGui::SameLine();
            }

            ImGui::PushStyleColor(ImGuiCol_Text, header.foregroundColor);
            ImGui::TextUnformatted(line);
            ImGui::PopStyleColor();

            cur_line++;
        }

        return true;
    });

    if (this->_linespacing)
    {
        ImGui::PushStyleColor(ImGuiCol_Separator, IM_COL32(0x55, 0x55, 0x55, 100));
        ImGui::Separator();
        ImGui::PopStyleColor();
    }

    // Uncheck autoscrool if mouse wheel changes
    if (ImGui::IsWindowHovered() && ImGui::GetIO().MouseWheel > 0.0)
    {
        _autoscroll = false;
    }

    if (_scrollToBottom)
    {
        ImGui::SetScrollHereY();
        _scrollToBottom = false;
    }

    ImGui::PopStyleVar();
    ImGui::EndChild();
}

static ImU32 changeValue(ImU32 const color, float const delta_value)
{
    ImVec4 rgba = ImGui::ColorConvertU32ToFloat4(color);

    float h, s, v;
    ImGui::ColorConvertRGBtoHSV(rgba.x, rgba.y, rgba.z, h, s, v);
    v += delta_value;

    if (v < 0.0f)
    {
        v = 0.0f;
    }
    else if (v > 1.0f)
    {
        v = 1.0f;
    }

    ImGui::ColorConvertHSVtoRGB(h, s, v, rgba.x, rgba.y, rgba.z);
    return ImGui::ColorConvertFloat4ToU32(rgba);
}

ImGuiAl::Log::Log(size_t const buffer_size, const char *log_name, bool show_advanced, bool show_lines, bool selectable)
    : Crt(buffer_size),
      _debugLabel("Debug"),
      _infoLabel("Info"),
      _warningLabel("Warning"),
      _errorLabel("Error"),
      _cumulativeLabel("Cumulative"),
      _filterLabel("Filter"),
      _log_title(log_name),
      _filterHeaderLabel(nullptr),
      _showFilters(true),
      _actions(nullptr),
      _level(Level::Debug),
      _cumulative(true),
      _show_advanced(show_advanced)
{
    setColor(Level::Debug, CGA::Cyan);
    setColor(Level::Info, CGA::BrightGreen);
    setColor(Level::Warning, CGA::Yellow);
    setColor(Level::Error, CGA::BrightRed);
    _linespacing = show_lines;
    _selectable = selectable;
}

void ImGuiAl::Log::debug(char const *const format, ...)
{
    va_list args;
    va_start(args, format);
    debug(format, args);
    va_end(args);
}

void ImGuiAl::Log::info(char const *const format, ...)
{
    va_list args;
    va_start(args, format);
    info(format, args);
    va_end(args);
}

void ImGuiAl::Log::warning(char const *const format, ...)
{
    va_list args;
    va_start(args, format);
    warning(format, args);
    va_end(args);
}

void ImGuiAl::Log::error(char const *const format, ...)
{
    va_list args;
    va_start(args, format);
    error(format, args);
    va_end(args);
}

void ImGuiAl::Log::debug(char const *const format, va_list args)
{
    setForegroundColor(_debugTextColor);
    setMetaData(static_cast<unsigned>(Level::Debug));
    vprintf(format, args);
}

void ImGuiAl::Log::info(char const *const format, va_list args)
{
    setForegroundColor(_infoTextColor);
    setMetaData(static_cast<unsigned>(Level::Info));
    vprintf(format, args);
}

void ImGuiAl::Log::warning(char const *const format, va_list args)
{
    setForegroundColor(_warningTextColor);
    setMetaData(static_cast<unsigned>(Level::Warning));
    vprintf(format, args);
}

void ImGuiAl::Log::error(char const *const format, va_list args)
{
    setForegroundColor(_errorTextColor);
    setMetaData(static_cast<unsigned>(Level::Error));
    vprintf(format, args);
}

int ImGuiAl::Log::draw(ImVec2 const &size)
{
    int action = 0;
    // ImGuiWindowFlags io;

    ImGui::Begin(this->_log_title, NULL, ImGuiWindowFlags_NoCollapse);

    for (unsigned i = 0; _actions != nullptr && _actions[i] != nullptr; i++)
    {
        if (i != 0)
        {
            ImGui::SameLine();
        }

        if (ImGui::Button(_actions[i]))
        {
            action = i + 1;
        }
    }

    if ((_filterHeaderLabel != nullptr && ImGui::CollapsingHeader(_filterHeaderLabel)) || _showFilters)
    {
        if (_show_advanced)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, _debugButtonColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, _debugButtonHoveredColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, _debugTextColor);
            bool ok = ImGui::Button(_debugLabel);
            ImGui::PopStyleColor(3);

            if (ok)
            {
                _level = Level::Debug;
                scrollToBottom();
            }

            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Button, _infoButtonColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, _infoButtonHoveredColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, _infoTextColor);
            ok = ImGui::Button(_infoLabel);
            ImGui::PopStyleColor(3);

            if (ok)
            {
                _level = Level::Info;
                scrollToBottom();
            }

            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Button, _warningButtonColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, _warningButtonHoveredColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, _warningTextColor);
            ok = ImGui::Button(_warningLabel);
            ImGui::PopStyleColor(3);

            if (ok)
            {
                _level = Level::Warning;
                scrollToBottom();
            }

            ImGui::SameLine();

            ImGui::PushStyleColor(ImGuiCol_Button, _errorButtonColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, _errorButtonHoveredColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, _errorTextColor);
            ok = ImGui::Button(_errorLabel);
            ImGui::PopStyleColor(3);

            if (ok)
            {
                _level = Level::Error;
                scrollToBottom();
            }

            ImGui::SameLine();
            ImGui::Checkbox(_cumulativeLabel, &_cumulative);
        }

        if (!_hide_options)
        {
            if (_show_advanced)
                ImGui::SameLine();
            ImGui::Checkbox("Autoscroll", &_autoscroll);
            if (_autoscroll)
                _line_selected = -1;
            // ImGui::SameLine();
            // _filter.Draw(_filterLabel, 80.0);
        }
    }

    Crt::draw(size, [this](MSGS const &header, char const *const line) -> bool {
        unsigned const level = static_cast<unsigned>(_level);

        bool show = (_cumulative && header.metaData >= level) || header.metaData == level;
        show = show && _filter.PassFilter(line);

        return show;
    });

    if (_autoscroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
    {
        scrollToBottom();
    }

    ImGui::End();

    return action;
}

void ImGuiAl::Log::setColor(Level const level, ImU32 const color)
{
    ImU32 const button_color = changeValue(color, -0.2f);
    ImU32 const hovered_color = changeValue(color, -0.1f);

    switch (level)
    {
    case Level::Debug:
        _debugTextColor = color;
        _debugButtonColor = button_color;
        _debugButtonHoveredColor = hovered_color;
        break;

    case Level::Info:
        _infoTextColor = color;
        _infoButtonColor = button_color;
        _infoButtonHoveredColor = hovered_color;
        break;

    case Level::Warning:
        _warningTextColor = color;
        _warningButtonColor = button_color;
        _warningButtonHoveredColor = hovered_color;
        break;

    case Level::Error:
        _errorTextColor = color;
        _errorButtonColor = button_color;
        _errorButtonHoveredColor = hovered_color;
        break;
    }
}

void ImGuiAl::Log::setLabel(Level const level, char const *const label)
{
    switch (level)
    {
    case Level::Debug:
        _debugLabel = label;
        break;

    case Level::Info:
        _infoLabel = label;
        break;

    case Level::Warning:
        _warningLabel = label;
        break;

    case Level::Error:
        _errorLabel = label;
        break;
    }
}

void ImGuiAl::Log::setCumulativeLabel(char const *const label)
{
    _cumulativeLabel = label;
}

void ImGuiAl::Log::setFilterLabel(char const *const label)
{
    _filterLabel = label;
}

void ImGuiAl::Log::setFilterHeaderLabel(char const *const label)
{
    _filterHeaderLabel = label;
}

void ImGuiAl::Log::setActions(char const *actions[])
{
    _actions = actions;
}

void ImGuiAl::Log::setHideOptions(bool value)
{
    this->_hide_options = value;
}

ImGuiAl::Terminal::Terminal(void *const buffer,
                            size_t const buffer_size,
                            void *const cmd_buf,
                            size_t const cmd_size,
                            std::function<void(Terminal &self, char *const command)> &&execute)
    : Crt(buffer_size),
      _commandBuffer(static_cast<char *>(cmd_buf)),
      _cmdBufferSize(cmd_size),
      _execute(std::move(execute)) {}

void ImGuiAl::Terminal::printf(char const *const format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

void ImGuiAl::Terminal::draw(ImVec2 const &size)
{
    ImVec2 new_size(size.x, size.y - ImGui::GetTextLineHeight() - ImGui::GetTextLineHeightWithSpacing());
    Crt::draw(new_size);
    ImGui::Separator();

    ImGuiInputTextFlags const flags = ImGuiInputTextFlags_EnterReturnsTrue;

    bool reclaim_focus = false;

    if (ImGui::InputText("Command", _commandBuffer, _cmdBufferSize, flags, nullptr, nullptr))
    {
        char *begin = _commandBuffer;

        while (*begin != 0 && isspace(*begin))
        {
            begin++;
        }

        if (*begin != 0)
        {
            char *end = begin + strlen(begin) - 1;

            while (isspace(*end))
            {
                end--;
            }

            end[1] = 0;
            _execute(*this, begin);
        }

        reclaim_focus = true;
    }

    ImGui::SetItemDefaultFocus();

    if (reclaim_focus)
    {
        ImGui::SetKeyboardFocusHere(-1);
    }
}
