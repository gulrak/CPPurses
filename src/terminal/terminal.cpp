#include <cppurses/terminal/terminal.hpp>

#include <clocale>
#include <csignal>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <chrono>
#include <stdexcept>

#include <ncurses.h>
#include <signal.h>
#include <string.h>

#include <cppurses/painter/color.hpp>
#include <cppurses/painter/color_definition.hpp>
#include <cppurses/painter/palette.hpp>
#include <cppurses/painter/rgb.hpp>
#include <cppurses/system/system.hpp>
#include <cppurses/terminal/input.hpp>

extern "C" void handle_sigint(int /* sig*/)
{
    cppurses::System::terminal.uninitialize();
#if !defined __APPLE__
    std::quick_exit(0);
#else
    std::exit(0);
#endif
}

namespace {
using namespace cppurses;

Underlying_color_t scale(Underlying_color_t value)
{
    const auto value_max   = 255;
    const auto ncurses_max = 1000;
    return static_cast<Underlying_color_t>(
        (static_cast<double>(value) / value_max) * ncurses_max);
}
}  // namespace

namespace cppurses {

void Terminal::initialize()
{
    if (is_initialized_)
        return;
    std::setlocale(LC_ALL, "en_US.UTF-8");

    if (::newterm(std::getenv("TERM"), stdout, stdin) == nullptr &&
        ::newterm("xterm-256color", stdout, stdin) == nullptr) {
        throw std::runtime_error{"Unable to initialize screen."};
    }
    std::signal(SIGINT, &handle_sigint);

    is_initialized_ = true;
    ::noecho();
    ::keypad(::stdscr, true);
    ::ESCDELAY = 1;
    ::mousemask(ALL_MOUSE_EVENTS, nullptr);
    ::mouseinterval(0);
    this->set_refresh_rate(refresh_rate_);
    if (this->has_color()) {
        ::start_color();
        this->initialize_color_pairs();
        this->ncurses_set_palette(palette_);
    }
    this->ncurses_set_raw_mode();
    this->ncurses_set_cursor();
}

void Terminal::uninitialize()
{
    if (!is_initialized_)
        return;
    ::wrefresh(::stdscr);
    is_initialized_ = false;
    ::endwin();
}

// getmaxx/getmaxy are non-standard.
std::size_t Terminal::width() const
{
    int y{0};
    int x{0};
    if (is_initialized_)
        getmaxyx(::stdscr, y, x);
    return x;
}

std::size_t Terminal::height() const
{
    int y{0};
    int x{0};
    if (is_initialized_)
        getmaxyx(::stdscr, y, x);
    return y;
}

auto Terminal::set_refresh_rate(std::chrono::milliseconds duration) -> void
{
    refresh_rate_ = duration;
    if (is_initialized_)
        ::timeout(refresh_rate_.count());
}

void Terminal::set_background(const Glyph& tile)
{
    background_ = tile;
    if (is_initialized_) {
        // TODO Notify Screen::flush() to repaint everything.
    }
}

void Terminal::set_color_palette(const Palette& colors)
{
    palette_ = colors;
    if (is_initialized_ && this->has_color())
        this->ncurses_set_palette(palette_);
}

void Terminal::show_cursor(bool show)
{
    show_cursor_ = show;
    if (is_initialized_)
        this->ncurses_set_cursor();
}

void Terminal::raw_mode(bool enable)
{
    raw_mode_ = enable;
    if (is_initialized_)
        this->ncurses_set_raw_mode();
}

bool Terminal::has_color() const
{
    if (is_initialized_)
        return ::has_colors() == TRUE;
    return false;
}

bool Terminal::has_extended_colors() const
{
    if (is_initialized_)
        return COLORS >= 16;
    return false;
}

short Terminal::color_count() const
{
    if (is_initialized_)
        return COLORS;
    return 0;
}

bool Terminal::can_change_colors() const
{
    if (is_initialized_)
        return ::can_change_color() == TRUE;
    return false;
}

short Terminal::color_pair_count() const
{
    if (is_initialized_)
        return COLOR_PAIRS;
    return 0;
}

short Terminal::color_index(short fg, short bg) const
{
    if (fg == 7 && bg == 0)
        return 0;
    if (fg == 15 && bg == 0)
        return 128;
    const short max_color = this->has_extended_colors() ? 16 : 8;
    return ((max_color - 1) - fg) * max_color + bg;
}

void Terminal::use_default_colors(bool use)
{
    if (!is_initialized_)
        return;
    if (use) {
        ::assume_default_colors(-1, -1);
        init_default_pairs();
    }
    else {
        ::assume_default_colors(7, 0);
        uninit_default_pairs();
    }
}

void Terminal::ncurses_set_palette(const Palette& colors)
{
    if (this->can_change_colors()) {
        for (const Color_definition& def : colors) {
            const auto max_color = this->has_extended_colors() ? 16 : 8;
            const auto ncurses_color_number =
                static_cast<Underlying_color_t>(def.color);
            if (ncurses_color_number < max_color) {
                ::init_color(ncurses_color_number, scale(def.values.red),
                             scale(def.values.green), scale(def.values.blue));
            }
        }
    }
}

void Terminal::ncurses_set_raw_mode() const
{
    if (raw_mode_) {
        ::nocbreak();
        ::raw();
    }
    else {
        ::noraw();
        ::cbreak();
    }
}

void Terminal::ncurses_set_cursor() const
{
    show_cursor_ ? ::curs_set(1) : ::curs_set(0);
}

void Terminal::initialize_color_pairs() const
{
    Underlying_color_t max_color = this->has_extended_colors() ? 16 : 8;
    for (short fg = 0; fg < max_color; ++fg) {
        for (short bg = 0; bg < max_color; ++bg) {
            const auto index = color_index(fg, bg);
            if (index == 0)
                continue;
            ::init_pair(index, fg, bg);
        }
    }
}

void Terminal::init_default_pairs() const
{
    Underlying_color_t max_color = this->has_extended_colors() ? 16 : 8;
    for (Underlying_color_t bg = 1; bg < max_color; ++bg) {
        ::init_pair(color_index(7, bg), -1, bg);
    }
    for (Underlying_color_t fg = 0; fg < max_color; ++fg) {
        if (fg != 7)
            ::init_pair(color_index(fg, 0), fg, -1);
    }
}

void Terminal::uninit_default_pairs() const
{
    Underlying_color_t max_color = this->has_extended_colors() ? 16 : 8;
    for (Underlying_color_t bg = 0; bg < max_color; ++bg) {
        ::init_pair(color_index(7, bg), 7, bg);
    }
    for (Underlying_color_t fg = 0; fg < max_color; ++fg) {
        if (fg != 7)
            ::init_pair(color_index(fg, 0), fg, 0);
    }
}
}  // namespace cppurses
