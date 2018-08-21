#ifndef CPPURSES_PAINTER_DETAIL_SCREEN_STATE_HPP
#define CPPURSES_PAINTER_DETAIL_SCREEN_STATE_HPP
#include <cppurses/painter/detail/screen_descriptor.hpp>
#include <cppurses/painter/detail/screen_mask.hpp>
#include <cppurses/painter/detail/staged_changes.hpp>
#include <cppurses/painter/glyph.hpp>

namespace cppurses {
class Paint_buffer;
class Layout;
class Enable_event;
class Disable_event;
class Child_event;
class Move_event;
class Resize_event;
namespace detail {

/// Holds a Screen_descriptor representing the current screen state of a Widget.
///
/// A Widget is the owner of this object, but only the flush function can modify
/// its state.
class Screen_state {
    // holds global coordinates, set by flush.
    Screen_descriptor tiles;

    struct Optimize {
        bool just_enabled{false};
        bool moved{false};
        bool resized{false};
        bool child_event{false};
        Glyph wallpaper;
        detail::Screen_mask move_mask;
        detail::Screen_mask resize_mask;
    };

    Optimize optimize;

    friend class cppurses::Paint_buffer;
    friend class cppurses::Layout;
    friend class cppurses::Enable_event;
    friend class cppurses::Disable_event;
    friend class cppurses::Child_event;
    friend class cppurses::Move_event;
    friend class cppurses::Resize_event;
};

}  // namespace detail
}  // namespace cppurses
#endif  // CPPURSES_PAINTER_DETAIL_SCREEN_STATE_HPP
