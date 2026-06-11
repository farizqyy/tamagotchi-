#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

using namespace ftxui;

int main() {
    auto screen = ScreenInteractive::Fullscreen();

    int hunger = 70;
    int happiness = 90;
    int energy = 65;
    int health = 100;

    auto renderer = Renderer([&] {

        auto title =
            text(" TAMAGOTCHI ")
            | bold
            | center
            | color(Color::White);

        auto status_panel =
            vbox({
                text("🌤 Frutiger Aero Dashboard") | bold | center,

                separator(),

                hbox({
                    text("🍔 Fullness "),
                    gauge(hunger / 100.f)
                        | color(Color::BlueLight)
                        | flex
                }),

                hbox({
                    text("😊 Happiness "),
                    gauge(happiness / 100.f)
                        | color(Color::CyanLight)
                        | flex
                }),

                hbox({
                    text("⚡ Energy "),
                    gauge(energy / 100.f)
                        | color(Color::GreenLight)
                        | flex
                }),

                hbox({
                    text("❤️ Health "),
                    gauge(health / 100.f)
                        | color(Color::RedLight)
                        | flex
                }),

            })
            | borderDouble
            | bgcolor(Color::RGB(220,240,255))
            | color(Color::Blue)
            | size(WIDTH, GREATER_THAN, 50);

        auto pet =
            vbox({
                text("      /\\_/\\\\"),
                text("     ( o.o )"),
                text("      > ^ <"),
                text(""),
                text("Name : Tama"),
                text("Age  : 3 Days"),
            })
            | borderRounded
            | bgcolor(Color::RGB(235,250,255))
            | color(Color::Black);

        return vbox({
                   title,
                   separator(),
                   hbox({
                       pet | flex,
                       filler(),
                       status_panel | flex,
                   })
               })
               | bgcolor(Color::RGB(180,220,255));
    });

    screen.Loop(renderer);
}