#include "cppurses.hpp"

using namespace cppurses;

int main() {
    System sys;
    Horizontal_layout hl;
    // hl.make_child<Textbox>(
    //     Glyph_string{"Edit me", foreground(Color::Light_gray)});
    auto& menu = hl.make_child<Menu>("~ Set Background Color ~");
    menu.set_foreground(Color::Red);
    menu.enable_border();

    menu.make_item("Violet", [&menu] { menu.set_background(Color::Violet); });
    menu.make_item(Glyph_string{"Light Blue", foreground(Color::Yellow)},
                   [&menu] { menu.set_background(Color::Light_blue); });
    menu.make_item("White", [&menu] { menu.set_background(Color::White); });
    menu.make_item("Add Item", [&menu] {
        menu.make_item("N e w  I t e m",
                       [&menu] { menu.set_background(Color::Orange); });
    });

    sys.set_head(&hl);

    return sys.run();
}