/*
The MIT License (MIT)

Copyright (c) 2019 Andre Leiradella

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once
#include <mutex>
#include <imgui.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include <functional>
#include <sstream>
#include "NanoLog.hpp"
#include <iostream>
#include <libs/log_misc_utils.hpp>
#include <libs/termcolor.hpp>
#include <libs/veque.hpp>

namespace ImGuiAl
{

    class Crt
    {
    public:
        struct CGA
        {
            enum : ImU32
            {
                ANSI = 0x33,
                Black = IM_COL32(0x00, 0x00, 0x00, 0xff),
                Blue = IM_COL32(0x00, 0x00, 0xaa, 0xff),
                Green = IM_COL32(0x00, 0xaa, 0x00, 0xff),
                Cyan = IM_COL32(0x00, 0xaa, 0xaa, 0xff),
                Red = IM_COL32(0xaa, 0x00, 0x00, 0xff),
                Magenta = IM_COL32(0xaa, 0x00, 0xaa, 0xff),
                Brown = IM_COL32(0xaa, 0x55, 0x00, 0xff),
                White = IM_COL32(0xaa, 0xaa, 0xaa, 0xff),
                Gray = IM_COL32(0x55, 0x55, 0x55, 0xff),
                BrightBlue = IM_COL32(0x55, 0x55, 0xff, 0xff),
                BrightGreen = IM_COL32(0x55, 0xff, 0x55, 0xff),
                BrightCyan = IM_COL32(0x55, 0xff, 0xff, 0xff),
                BrightRed = IM_COL32(0xff, 0x55, 0x55, 0xff),
                BrightMagenta = IM_COL32(0xff, 0x55, 0xff, 0xff),
                Yellow = IM_COL32(0xff, 0xff, 0x55, 0xff),
                BrightWhite = IM_COL32(0xff, 0xff, 0xff, 0xff)
            };
        };

        struct Info
        {
            ImU32 foregroundColor;
            uint32_t length;
            unsigned metaData;
        };

        struct MSGS
        {
            std::string msg;
            ImU32 foregroundColor;
            unsigned metaData;
            int idx;
        };

        bool _autoscroll;
        bool _linespacing;
        bool _selectable;
        int _line_selected;
        int _cur_line;
        int _horizontal_flag;

        Crt(size_t const size);

        void enableLogToFile(bool value, const std::string folder_name, const std::string file_name, int exclude_core = -1);
        void disableGUI(bool value);
        void forceHorizontalScrollbar(bool value);

        void setForegroundColor(ImU32 const color);
        void setMetaData(unsigned const meta_data);

        void printf(char const *const format, ...);
        void vprintf(char const *const format, va_list args);

        void scrollToBottom();
        void clear();

        void iterate(const std::function<bool(MSGS const &header, char const *const line)> &iterator);
        void draw(ImVec2 const &size = ImVec2(0.0f, 0.0f));

        template <class... T>
        void add_msg(T &&...args)
        {

            string_converter.clear();
            string_converter.str(std::string()); // Reset string stream
            string_converter.seekp(std::ios::beg);

            (string_converter << ... << args); // Convert n arguments to string

            if (_log_to_file)
            {
                NANO_LOG_INFO(_logger) << string_converter.str();
            }

            if (_gui_enabled)
            {

                mutex_log.lock();
                if (_fifo.size() >= _max_lines)
                {
                    _fifo.pop_front();
                }

                _fifo.push_back({string_converter.str(), _foregroundColor, _metaData, _cur_line++});
                mutex_log.unlock();
            }
            else
            {
                // Print on terminal if GUI is disabled
                switch (_foregroundColor)
                {
                case CGA::Cyan:
                case CGA::BrightCyan:
                    std::cout << termcolor::cyan << string_converter.str() << termcolor::reset << "\n";
                    break;

                case CGA::Magenta:
                case CGA::BrightMagenta:
                    std::cout << termcolor::magenta << string_converter.str() << termcolor::reset << "\n";
                    break;

                case CGA::Green:
                case CGA::BrightGreen:
                    std::cout << termcolor::green << string_converter.str() << termcolor::reset << "\n";
                    break;

                case CGA::Red:
                case CGA::BrightRed:
                    std::cout << termcolor::red << string_converter.str() << termcolor::reset << "\n";
                    break;

                case CGA::Yellow:
                    std::cout << "\u001b[33;1m" << string_converter.str() << termcolor::reset << "\n";
                    break;

                case CGA::White:
                case CGA::BrightWhite:
                    std::cout << string_converter.str() << "\n";
                    break;

                default:
                    std::cout << string_converter.str() << "\n";
                }
            }
        }

        template <class... T>
        void add_msg_color(ImU32 color, T &&...args)
        {
            _foregroundColor = color;
            _metaData = 1; // Info
            add_msg(args...);
        }

    protected:
        void draw(ImVec2 const &size, const std::function<bool(MSGS const &header, char const *const line)> &filter);

        veque::veque<MSGS> _fifo;
        ImU32 _foregroundColor;
        unsigned _metaData;
        bool _scrollToBottom;
        bool _log_to_file;
        bool _gui_enabled;
        uint16_t _max_lines;
        std::ostringstream string_converter;
        std::mutex mutex_log;
        nanolog::NanoLog _logger;
    };

    class Log : public Crt
    {
    public:
        typedef Crt::MSGS MSGS;

        enum class Level
        {
            Debug,
            Info,
            Warning,
            Error
        };

        Log(size_t const buffer_size, const char *log_name, bool show_advanced = false, bool show_lines = false, bool selectable = false);

        void debug(char const *const format, ...);
        void info(char const *const format, ...);
        void warning(char const *const format, ...);
        void error(char const *const format, ...);

        void debug(char const *const format, va_list args);
        void info(char const *const format, va_list args);
        void warning(char const *const format, va_list args);
        void error(char const *const format, va_list args);

        void clear() { Crt::clear(); }
        void iterate(const std::function<bool(MSGS const &header, char const *const line)> &iterator) { Crt::iterate(iterator); }
        inline void scrollToBottom() { Crt::scrollToBottom(); }

        int draw(ImVec2 const &size = ImVec2(0.0f, 0.0f));

        void setColor(Level const level, ImU32 const color);
        void setLabel(Level const level, char const *const label);
        void setCumulativeLabel(char const *const label);
        void setFilterLabel(char const *const label);
        void setFilterHeaderLabel(char const *const label);
        void setActions(char const *actions[]);
        void setHideOptions(bool value);

        template <class... T>
        void add_msg_debug(T &&...args)
        {
            _foregroundColor = _debugTextColor;
            _metaData = static_cast<unsigned>(Level::Debug);
            add_msg(args...);
        }

        template <class... T>
        void add_msg_info(T &&...args)
        {
            _foregroundColor = _infoTextColor;
            _metaData = static_cast<unsigned>(Level::Info);
            add_msg(args...);
        }

        template <class... T>
        void add_msg_warning(T &&...args)
        {
            _foregroundColor = _warningTextColor;
            _metaData = static_cast<unsigned>(Level::Warning);
            add_msg(args...);
        }

        template <class... T>
        void add_msg_error(T &&...args)
        {
            _foregroundColor = _errorTextColor;
            _metaData = static_cast<unsigned>(Level::Error);
            add_msg(args...);
        }

    protected:
        ImU32 _debugTextColor;
        ImU32 _debugButtonColor;
        ImU32 _debugButtonHoveredColor;

        ImU32 _infoTextColor;
        ImU32 _infoButtonColor;
        ImU32 _infoButtonHoveredColor;

        ImU32 _warningTextColor;
        ImU32 _warningButtonColor;
        ImU32 _warningButtonHoveredColor;

        ImU32 _errorTextColor;
        ImU32 _errorButtonColor;
        ImU32 _errorButtonHoveredColor;

        char const *_debugLabel;
        char const *_infoLabel;
        char const *_warningLabel;
        char const *_errorLabel;
        const char *_log_title;

        char const *_cumulativeLabel;
        char const *_filterLabel;
        char const *_filterHeaderLabel;

        bool _showFilters;
        char const *const *_actions;

        Level _level;
        bool _cumulative;
        bool _show_advanced;
        bool _hide_options;
        ImGuiTextFilter _filter;
    };

    class Terminal : protected Crt
    {
    public:
        typedef Crt::MSGS MSGS;

        Terminal(void *const buffer,
                 size_t const buffer_size,
                 void *const cmd_buf,
                 size_t const cmd_size,
                 std::function<void(Terminal &self, char *const command)> &&execute);

        void setForegroundColor(ImU32 const color) { Crt::setForegroundColor(color); }

        void printf(char const *const format, ...);
        void vprintf(char const *const format, va_list args) { Crt::vprintf(format, args); }

        void clear() { Crt::clear(); }
        void iterate(const std::function<bool(MSGS const &header, char const *const line)> &iterator) { Crt::iterate(iterator); }
        void scrollToBottom() { Crt::scrollToBottom(); }

        void draw(ImVec2 const &size = ImVec2(0.0f, 0.0f));

    protected:
        char *_commandBuffer;
        size_t _cmdBufferSize;
        std::function<void(Terminal &self, char *const command)> _execute;
    };

    template <size_t S, size_t R>
    class BufferedTerminal : public Terminal
    {
    public:
        BufferedTerminal(std::function<void(Terminal &self, char *const command)> &&execute)
            : Terminal(S, _commandBuffer, R, std::move(execute)) {}

    protected:
        uint8_t _commandBuffer[R];
    };
} // namespace ImGuiAl
