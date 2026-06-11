#include <iostream>
#include <string>
#include <functional>
#include <vector>
#include <ctime>
#include <iomanip>
#include <algorithm>
#include <fstream>
#include <cstdint>
#include <cstdlib>

// Components
#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/component/component_options.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/loop.hpp>
#include <ftxui/component/mouse.hpp>
#include <ftxui/component/receiver.hpp>
#include <ftxui/component/screen_interactive.hpp>

// DOM
#include <ftxui/dom/canvas.hpp>
#include <ftxui/dom/direction.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/flexbox_config.hpp>
#include <ftxui/dom/node.hpp>
#include <ftxui/dom/table.hpp>

// Screen
#include <ftxui/screen/box.hpp>
#include <ftxui/screen/color.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/screen/string.hpp>
#include <ftxui/screen/terminal.hpp>

#ifdef _WIN32
    #include <windows.h>
#endif
#include <locale>

// ─────────────────────────────────────────────
// STRUCT — main entities
// ─────────────────────────────────────────────
struct StatusLog {
    std::string timestamp;
    std::string action;
    std::string detail;
};

struct Tamagotchi {
    std::string name;
    int         hunger;
    int         happiness;
    int         energy;
    int         health;
    int         age;
    bool        alive;
    std::vector<StatusLog> logs;
};

struct EncounterEvent {
    std::string name;
    std::string description;
    int hungerDelta;
    int happinessDelta;
    int energyDelta;
    int healthDelta;
};

template<typename T>
inline T clamp(T value, T minVal, T maxVal) {
    if (value < minVal) return minVal;
    if (value > maxVal) return maxVal;
    return value;
}

// ─────────────────────────────────────────────
// NAMESPACE — Utils
// ─────────────────────────────────────────────
namespace Utils {
    std::string currentTime() {
        std::time_t t = std::time(nullptr);
        char buf[20];
        std::strftime(buf, sizeof(buf), "%H:%M:%S", std::localtime(&t));
        return std::string(buf);
    }

    inline bool isAlive(const Tamagotchi& t) {
        return t.alive && t.health > 0;
    }

    inline std::string getStatusLabel(int value) {
        if (value >= 75) return "Excellent";
        if (value >= 50) return "Good";
        if (value >= 25) return "Low";
        return "Critical";
    }

    inline std::string bar(int value, int width = 20) {
        int filled = (value * width) / 100;
        std::string b = "[";
        for (int i = 0; i < width; i++)
            b += (i < filled) ? "#" : "-";
        b += "]";
        return b;
    }

    void addLog(Tamagotchi& t, const std::string& action, const std::string& detail) {
        StatusLog log;
        log.timestamp = currentTime();
        log.action    = action;
        log.detail    = detail;
        t.logs.push_back(log);
    }

    void writeString(std::ofstream& f, const std::string& s) {
        uint32_t len = static_cast<uint32_t>(s.size());
        f.write(reinterpret_cast<const char*>(&len), sizeof(len));
        f.write(s.data(), len);
    }

    std::string readString(std::ifstream& f) {
        uint32_t len = 0;
        f.read(reinterpret_cast<char*>(&len), sizeof(len));
        std::string s(len, '\0');
        f.read(&s[0], len);
        return s;
    }
}

// ─────────────────────────────────────────────
// NAMESPACE — TamagotchiEngine
// ─────────────────────────────────────────────
namespace TamagotchiEngine {
    using ActionFunc = std::function<void(Tamagotchi&)>;

    Tamagotchi* createTamagotchi(
        const std::string& name,
        int hunger    = 50,
        int happiness = 70,
        int energy    = 80,
        int health    = 100
    ) {
        Tamagotchi* t = new Tamagotchi();
        t->name      = name;
        t->hunger    = clamp(hunger,    0, 100);
        t->happiness = clamp(happiness, 0, 100);
        t->energy    = clamp(energy,    0, 100);
        t->health    = clamp(health,    0, 100);
        t->age       = 0;
        t->alive     = true;

        Utils::addLog(*t, "BORN", "Tamagotchi " + name + " has been born!");
        return t;
    }

    void applyAction(Tamagotchi& t, ActionFunc action, const std::string& actionName) {
        if (!Utils::isAlive(t)) {
            return;
        }
        action(t);
        if (t.hunger >= 100 || t.health <= 0) {
            t.alive = false;
            Utils::addLog(t, "DIED", "Critical condition — " + t.name + " has passed away.");
        }
    }

    void feedAction(Tamagotchi& t) {
        t.hunger    = clamp(t.hunger - 30,    0, 100);
        t.happiness = clamp(t.happiness + 10, 0, 100);
        t.energy    = clamp(t.energy + 5,     0, 100);
        Utils::addLog(t, "FEED", "Hunger -30 | Happiness +10 | Energy +5");
    }

    void playAction(Tamagotchi& t) {
        if (t.energy < 20) {
            Utils::addLog(t, "PLAY", "Failed — energy too low");
            return;
        }
        t.happiness = clamp(t.happiness + 25, 0, 100);
        t.energy    = clamp(t.energy - 20,    0, 100);
        t.hunger    = clamp(t.hunger + 15,    0, 100);
        Utils::addLog(t, "PLAY", "Happiness +25 | Energy -20 | Hunger +15");
    }

    void sleepAction(Tamagotchi& t) {
        t.energy    = clamp(t.energy + 40,    0, 100);
        t.hunger    = clamp(t.hunger + 10,    0, 100);
        t.health    = clamp(t.health + 5,     0, 100);
        Utils::addLog(t, "SLEEP", "Energy +40 | Hunger +10 | Health +5");
    }

    void healAction(Tamagotchi& t) {
        t.health    = clamp(t.health + 30,    0, 100);
        t.energy    = clamp(t.energy + 10,    0, 100);
        Utils::addLog(t, "HEAL", "Health +30 | Energy +10");
    }

    using EncounterModifier = std::function<void(Tamagotchi&, const EncounterEvent&)>;

    const EncounterEvent ENCOUNTER_TABLE[] = {
        { "FLU",           "Caught the flu! Runny nose all day...",
          +10,  -15,  -10,  -20 },
        { "FOOD POISONING","Ate something weird, stomach ache!",
          +20,  -20,   -5,  -25 },
        { "CAUGHT IN RAIN","Got drenched without an umbrella!",
          +5,   -10,  -15,  -15 },
        { "SCARED",        "Encountered something terrifying!",
           0,   -25,  -20,   -5 },
        { "MOSQUITO BITE", "Got bitten by mosquitoes while sleeping...",
          +5,   -10,   -5,  -10 },
        { "INSOMNIA",      "Couldn't sleep well last night.",
          +10,   -5,  -25,  -10 },
        { "HEATSTROKE",    "Scorching weather, feeling dehydrated!",
          +15,  -10,  -20,  -15 },
        { "BEAUTIFUL DAY", "Sunny weather, feeling great!",
          -5,   +20,  +10,   +5 },
        { "SURPRISE GIFT", "Found an unexpected gift on the way!",
           0,   +25,  +10,  +10 },
        { "FOUND FRUIT",   "Found some fresh and delicious fruit!",
          -15,  +10,  +10,   +5 },
        { "MASSAGE",       "Got a relaxing massage from a friend.",
           0,   +15,  +20,  +10 },
        { "LOVELY MUSIC",  "Heard some soul-soothing music.",
           0,   +20,   +5,   +5 },
    };

    const int ENCOUNTER_COUNT = sizeof(ENCOUNTER_TABLE) / sizeof(ENCOUNTER_TABLE[0]);
    const int ENCOUNTER_CHANCE = 60;

    void applyEncounter(
        Tamagotchi& t,
        const EncounterEvent& ev,
        EncounterModifier modifier
    ) {
        modifier(t, ev);
        Utils::addLog(t, "EVENT", ev.name + ": " + ev.description);
    }

    bool triggerRandomEncounter(Tamagotchi& t) {
        if ((std::rand() % 100) >= ENCOUNTER_CHANCE) return false;

        const EncounterEvent& ev =
            ENCOUNTER_TABLE[std::rand() % ENCOUNTER_COUNT];

        applyEncounter(t, ev, [](Tamagotchi& pet, const EncounterEvent& e) {
            pet.hunger    = clamp(pet.hunger    + e.hungerDelta,    0, 100);
            pet.happiness = clamp(pet.happiness + e.happinessDelta, 0, 100);
            pet.energy    = clamp(pet.energy    + e.energyDelta,    0, 100);
            pet.health    = clamp(pet.health    + e.healthDelta,    0, 100);
        });

        return true;
    }

    void ageTick(Tamagotchi& t) {
        t.age++;
        t.hunger    = clamp(t.hunger + 10,    0, 100);
        t.happiness = clamp(t.happiness - 5,  0, 100);
        t.energy    = clamp(t.energy - 8,     0, 100);
        if (t.hunger >= 80) t.health = clamp(t.health - 15, 0, 100);
        if (t.happiness <= 20) t.health = clamp(t.health - 5, 0, 100);

        Utils::addLog(t, "TIME",
            "Day " + std::to_string(t.age) +
            " | Hunger +10 | Happiness -5 | Energy -8");
    }

    bool saveGame(const Tamagotchi& t, const std::string& filename = "tamagotchi.sav") {
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }

        const char magic[4] = {'T','A','M','A'};
        const uint8_t version = 1;
        file.write(magic, 4);
        file.write(reinterpret_cast<const char*>(&version), sizeof(version));

        Utils::writeString(file, t.name);
        file.write(reinterpret_cast<const char*>(&t.hunger),    sizeof(t.hunger));
        file.write(reinterpret_cast<const char*>(&t.happiness), sizeof(t.happiness));
        file.write(reinterpret_cast<const char*>(&t.energy),    sizeof(t.energy));
        file.write(reinterpret_cast<const char*>(&t.health),    sizeof(t.health));
        file.write(reinterpret_cast<const char*>(&t.age),       sizeof(t.age));
        uint8_t alive = t.alive ? 1 : 0;
        file.write(reinterpret_cast<const char*>(&alive),       sizeof(alive));

        uint32_t logCount = static_cast<uint32_t>(t.logs.size());
        file.write(reinterpret_cast<const char*>(&logCount), sizeof(logCount));
        for (const StatusLog& log : t.logs) {
            Utils::writeString(file, log.timestamp);
            Utils::writeString(file, log.action);
            Utils::writeString(file, log.detail);
        }

        file.close();
        return true;
    }

    Tamagotchi* loadGame(const std::string& filename = "tamagotchi.sav") {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            return nullptr;
        }

        char magic[4];
        file.read(magic, 4);
        if (magic[0]!='T' || magic[1]!='A' || magic[2]!='M' || magic[3]!='A') {
            return nullptr;
        }

        uint8_t version = 0;
        file.read(reinterpret_cast<char*>(&version), sizeof(version));
        if (version != 1) {
            return nullptr;
        }

        Tamagotchi* t = new Tamagotchi();

        t->name = Utils::readString(file);
        file.read(reinterpret_cast<char*>(&t->hunger),    sizeof(t->hunger));
        file.read(reinterpret_cast<char*>(&t->happiness), sizeof(t->happiness));
        file.read(reinterpret_cast<char*>(&t->energy),    sizeof(t->energy));
        file.read(reinterpret_cast<char*>(&t->health),    sizeof(t->health));
        file.read(reinterpret_cast<char*>(&t->age),       sizeof(t->age));
        uint8_t alive = 0;
        file.read(reinterpret_cast<char*>(&alive), sizeof(alive));
        t->alive = (alive == 1);

        uint32_t logCount = 0;
        file.read(reinterpret_cast<char*>(&logCount), sizeof(logCount));
        for (uint32_t i = 0; i < logCount; i++) {
            StatusLog log;
            log.timestamp = Utils::readString(file);
            log.action    = Utils::readString(file);
            log.detail    = Utils::readString(file);
            t->logs.push_back(log);
        }

        file.close();
        Utils::addLog(*t, "LOAD", "Data loaded — day " + std::to_string(t->age));
        return t;
    }

    inline bool saveExists(const std::string& filename = "tamagotchi.sav") {
        std::ifstream f(filename);
        return f.good();
    }
}

// ─────────────────────────────────────────────
// FTXUI TUI APPLICATION — Frutiger Aero Theme
// ─────────────────────────────────────────────
using namespace ftxui;

class TamagotchiApp {
public:
    Tamagotchi* pet;
    int selected_menu = 0;
    std::string encounter_message;
    std::string action_message;
    std::string status_message;
    bool show_encounter = false;
    bool game_over = false;
    bool is_loading = false;

    TamagotchiApp() : pet(nullptr), selected_menu(0) {}

    ~TamagotchiApp() {
        if (pet) delete pet;
    }

    Element renderHeader() {
        return vbox({
            hbox({
                text("✦ ") | color(Color::Cyan),
                text("TAMAGOTCHI") | bold | color(Color::RGB(0, 200, 255)),
                text(" ✦") | color(Color::Magenta)
            }) | center,
            text("~ Virtual Pet Simulator ~") | center | color(Color::RGB(255, 150, 100)),
        }) | bgcolor(Color::RGB(200, 230, 255)) | border;
    }

    Element renderPetStatus() {
        if (!pet || !Utils::isAlive(*pet)) {
            return text("Pet has passed away...") | center | color(Color::Red);
        }

        auto stat_bar = [](const std::string& label, int value, Color color) {
            int filled = (value * 20) / 100;
            std::string bar_content = "";
            for (int i = 0; i < 20; i++) {
                bar_content += (i < filled) ? "█" : "░";
            }

            return hbox({
                text(label) | size(WIDTH, EQUAL, 12) | color(Color::White),
                text(bar_content) | color(color),
                text(" " + std::to_string(value) + "%") | size(WIDTH, EQUAL, 5) | color(Color::White),
            });
        };

        return vbox({
            hbox({
                text("Name: ") | color(Color::Yellow),
                text(pet->name) | bold | color(Color::RGB(255, 200, 100)),
                text(" | Age: ") | color(Color::Yellow),
                text(std::to_string(pet->age) + " days") | bold | color(Color::RGB(100, 200, 255)),
            }),
            separator(),
            stat_bar("Fullness ", 100 - pet->hunger, Color::RGB(255, 100, 150)),
            stat_bar("Happiness", pet->happiness, Color::RGB(255, 200, 100)),
            stat_bar("Energy   ", pet->energy, Color::RGB(150, 255, 100)),
            stat_bar("Health   ", pet->health, Color::RGB(100, 255, 150)),
        }) | bgcolor(Color::RGB(240, 250, 255)) | border;
    }

    Element renderMenu() {
        std::vector<std::string> menu_items = {
            "🍖 Feed",
            "🎮 Play",
            "😴 Sleep",
            "💊 Medicine",
            "⏱ Pass Time",
            "📋 View Log",
            "💾 Save Game",
            "📂 Load Game",
            "❌ Quit"
        };

        std::vector<Element> menu_elements;
        for (size_t i = 0; i < menu_items.size(); i++) {
            auto element = text(menu_items[i]);
            if ((int)i == selected_menu) {
                element = element | bgcolor(Color::RGB(100, 200, 255)) | color(Color::Black) | bold;
            } else {
                element = element | color(Color::White);
            }
            menu_elements.push_back(element);
        }

        return vbox(menu_elements) | bgcolor(Color::RGB(200, 150, 255)) | border;
    }

    Element renderLog() {
        if (!pet) return text("No logs");

        std::vector<Element> log_elements;
        int start = std::max(0, (int)pet->logs.size() - 6);

        for (int i = start; i < (int)pet->logs.size(); i++) {
            const auto& log = pet->logs[i];
            log_elements.push_back(
                hbox({
                    text("[" + log.timestamp + "]") | color(Color::Cyan),
                    text(" "),
                    text(log.action) | color(Color::Yellow) | bold,
                    text(": ") | color(Color::White),
                    text(log.detail) | color(Color::RGB(150, 200, 255)),
                }) | size(HEIGHT, EQUAL, 1)
            );
        }

        return vbox(log_elements) | bgcolor(Color::RGB(220, 240, 255)) | border;
    }

    Element renderAction() {
        if (action_message.empty()) {
            return text("Ready for commands...") | center | color(Color::RGB(100, 150, 255));
        }
        return text(action_message) | center | color(Color::RGB(255, 150, 100)) | bold;
    }

    Element renderEncounter() {
        if (!show_encounter) {
            return vbox({
                text(" ") | center,
                text(" ") | center,
            });
        }

        return vbox({
            text("✨ RANDOM ENCOUNTER ✨") | bold | center | color(Color::RGB(255, 200, 100)),
            separator(),
            text(encounter_message) | center | color(Color::White),
        }) | bgcolor(Color::RGB(255, 150, 100)) | border;
    }

    void handleMenuInput(int key) {
        std::vector<std::string> menu_items = {"feed", "play", "sleep", "heal", "time", "log", "save", "load", "quit"};
        int menu_size = 9;

        if (key == ARROW_DOWN) {
            selected_menu = (selected_menu + 1) % menu_size;
        } else if (key == ARROW_UP) {
            selected_menu = (selected_menu - 1 + menu_size) % menu_size;
        } else if (key == '\n') {
            executeMenuAction(selected_menu);
        }
    }

    void executeMenuAction(int choice) {
        if (!pet) return;

        action_message = "";
        show_encounter = false;

        switch (choice) {
            case 0: // Feed
                if (Utils::isAlive(*pet)) {
                    TamagotchiEngine::applyAction(*pet, TamagotchiEngine::feedAction, "FEED");
                    action_message = pet->name + " eats happily! 😋";
                }
                break;
            case 1: // Play
                if (Utils::isAlive(*pet)) {
                    if (pet->energy < 20) {
                        action_message = pet->name + " is too tired... 😴";
                    } else {
                        TamagotchiEngine::applyAction(*pet, TamagotchiEngine::playAction, "PLAY");
                        action_message = pet->name + " plays joyfully! 🎉";
                    }
                }
                break;
            case 2: // Sleep
                if (Utils::isAlive(*pet)) {
                    TamagotchiEngine::applyAction(*pet, TamagotchiEngine::sleepAction, "SLEEP");
                    action_message = pet->name + " sleeps soundly... Zzz";
                }
                break;
            case 3: // Medicine
                if (Utils::isAlive(*pet)) {
                    TamagotchiEngine::applyAction(*pet, TamagotchiEngine::healAction, "HEAL");
                    action_message = pet->name + " feels better! 💊";
                }
                break;
            case 4: // Pass Time
                if (Utils::isAlive(*pet)) {
                    TamagotchiEngine::applyAction(*pet, TamagotchiEngine::ageTick, "TIME");
                    action_message = "Time passes... Day " + std::to_string(pet->age);
                    if (TamagotchiEngine::triggerRandomEncounter(*pet)) {
                        show_encounter = true;
                        encounter_message = "An event occurred!";
                    }
                }
                break;
            case 5: // View Log
                action_message = "Showing action log...";
                break;
            case 6: // Save
                if (TamagotchiEngine::saveGame(*pet)) {
                    action_message = "Game saved successfully! 💾";
                } else {
                    action_message = "Save failed!";
                }
                break;
            case 7: // Load
                is_loading = true;
                break;
            case 8: // Quit
                std::exit(0);
                break;
        }

        if (!Utils::isAlive(*pet)) {
            action_message = "Oh no! " + pet->name + " has passed away... RIP";
            game_over = true;
        }
    }

    Element render() {
        return vbox({
            renderHeader(),
            text(" ") | size(HEIGHT, EQUAL, 1),
            renderPetStatus(),
            text(" ") | size(HEIGHT, EQUAL, 1),
            hbox({
                renderMenu() | flex,
                text(" ") | size(WIDTH, EQUAL, 2),
                renderLog() | flex,
            }),
            text(" ") | size(HEIGHT, EQUAL, 1),
            renderEncounter(),
            renderAction(),
            text(" ") | size(HEIGHT, EQUAL, 1),
        }) | bgcolor(Color::RGB(230, 240, 255));
    }
};

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#else
    setlocale(LC_ALL, "");
#endif

    std::srand(static_cast<unsigned>(std::time(nullptr)));

    auto screen = ScreenInteractive::TerminalOutput();
    auto app = std::make_shared<TamagotchiApp>();

    // Load or create pet
    if (TamagotchiEngine::saveExists()) {
        auto pet = TamagotchiEngine::loadGame();
        if (pet) {
            app->pet = pet;
        } else {
            app->pet = TamagotchiEngine::createTamagotchi("Tama");
        }
    } else {
        app->pet = TamagotchiEngine::createTamagotchi("Tama");
    }

    std::function<Element()> renderer = [app]() { return app->render(); };

    std::function<void()> input_handler = [app, &screen]() {
        screen.HandleInput([app, &screen](Event event) {
            if (event.is_mouse()) return false;
            if (event.is_character()) {
                if (event.character() == "q") {
                    screen.Exit();
                }
                return false;
            }
            if (event == Event::ArrowUp) {
                app->handleMenuInput(ARROW_UP);
                return true;
            }
            if (event == Event::ArrowDown) {
                app->handleMenuInput(ARROW_DOWN);
                return true;
            }
            if (event == Event::Return) {
                app->handleMenuInput('\n');
                return true;
            }
            return false;
        });
    };

    screen.Loop(renderer);

    if (app->pet) {
        TamagotchiEngine::saveGame(*app->pet);
    }

    return 0;
}
