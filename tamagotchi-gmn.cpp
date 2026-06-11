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
#include <locale>

#ifdef _WIN32
    #include <windows.h>
#endif

// FTXUI Components
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>

// ─────────────────────────────────────────────
// 1. STRUCT — main entities
// ─────────────────────────────────────────────
struct StatusLog {
    std::string timestamp;
    std::string action;
    std::string detail;
};

struct Tamagotchi {
    std::string name;
    int         hunger;     // 0=full, 100=starving
    int         happiness;  // 0=sad,  100=very happy
    int         energy;     // 0=tired, 100=full energy
    int         health;     // 0=sick,  100=healthy
    int         age;        // days
    bool        alive;
    std::vector<StatusLog> logs;   // action history
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
// 4. NAMESPACE — Utils (helpers)
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

    void addLog(Tamagotchi& t, const std::string& action, const std::string& detail) {
        StatusLog log;
        log.timestamp = currentTime();
        log.action    = action;
        log.detail    = detail;
        t.logs.push_back(log);
    }
}

// ─────────────────────────────────────────────
// 4. NAMESPACE — TamagotchiEngine (core logic)
// ─────────────────────────────────────────────
namespace TamagotchiEngine {
    using ActionFunc = std::function<void(Tamagotchi&)>;

    Tamagotchi* createTamagotchi(const std::string& name, int hunger = 50, int happiness = 70, int energy = 80, int health = 100) {
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

    void applyAction(Tamagotchi& t, ActionFunc action) {
        if (!Utils::isAlive(t)) return;
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

    const EncounterEvent ENCOUNTER_TABLE[] = {
        { "FLU", "Caught the flu! Runny nose all day...", +10, -15, -10, -20 },
        { "FOOD POISONING", "Ate something weird, stomach ache!", +20, -20, -5, -25 },
        { "CAUGHT IN RAIN", "Got drenched without an umbrella!", +5, -10, -15, -15 },
        { "SCARED", "Encountered something terrifying!", 0, -25, -20, -5 },
        { "MOSQUITO BITE", "Got bitten by mosquitoes while sleeping...", +5, -10, -5, -10 },
        { "INSOMNIA", "Couldn't sleep well last night.", +10, -5, -25, -10 },
        { "HEATSTROKE", "Scorching weather, feeling dehydrated!", +15, -10, -20, -15 },
        { "BEAUTIFUL DAY", "Sunny weather, feeling great!", -5, +20, +10, +5 },
        { "SURPRISE GIFT", "Found an unexpected gift on the way!", 0, +25, +10, +10 },
        { "FOUND FRUIT", "Found some fresh and delicious fruit!", -15, +10, +10, +5 },
        { "MASSAGE", "Got a relaxing massage from a friend.", 0, +15, +20, +10 },
        { "LOVELY MUSIC", "Heard some soul-soothing music.", 0, +20, +5, +5 },
    };

    const int ENCOUNTER_COUNT = sizeof(ENCOUNTER_TABLE) / sizeof(ENCOUNTER_TABLE[0]);
    const int ENCOUNTER_CHANCE = 60;

    std::string latestEncounterMessage = "No special event recently.";

    void ageTick(Tamagotchi& t) {
        t.age++;
        t.hunger    = clamp(t.hunger + 10,    0, 100);
        t.happiness = clamp(t.happiness - 5,  0, 100);
        t.energy    = clamp(t.energy - 8,     0, 100);
        if (t.hunger >= 80) t.health = clamp(t.health - 15, 0, 100);
        if (t.happiness <= 20) t.health = clamp(t.health - 5, 0, 100);

        Utils::addLog(t, "TIME", "Day " + std::to_string(t.age) + " | Hunger +10 | Happiness -5 | Energy -8");
    }

    void triggerRandomEncounter(Tamagotchi& t) {
        if ((std::rand() % 100) >= ENCOUNTER_CHANCE) {
            latestEncounterMessage = "Nothing unusual happened today.";
            return;
        }

        const EncounterEvent& ev = ENCOUNTER_TABLE[std::rand() % ENCOUNTER_COUNT];
        latestEncounterMessage = "⚠️ " + ev.name + ": " + ev.description;

        t.hunger    = clamp(t.hunger    + ev.hungerDelta,    0, 100);
        t.happiness = clamp(t.happiness + ev.happinessDelta, 0, 100);
        t.energy    = clamp(t.energy    + ev.energyDelta,    0, 100);
        t.health    = clamp(t.health    + ev.healthDelta,    0, 100);

        Utils::addLog(t, "EVENT", ev.name + ": " + ev.description);
    }

    inline void writeString(std::ofstream& f, const std::string& s) {
        uint32_t len = static_cast<uint32_t>(s.size());
        f.write(reinterpret_cast<const char*>(&len), sizeof(len));
        f.write(s.data(), len);
    }

    inline std::string readString(std::ifstream& f) {
        uint32_t len = 0;
        f.read(reinterpret_cast<char*>(&len), sizeof(len));
        std::string s(len, '\0');
        f.read(&s[0], len);
        return s;
    }

    bool saveGame(const Tamagotchi& t, const std::string& filename = "tamagotchi.sav") {
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) return false;

        const char magic[4] = {'T','A','M','A'};
        const uint8_t version = 1;
        file.write(magic, 4);
        file.write(reinterpret_cast<const char*>(&version), sizeof(version));

        writeString(file, t.name);
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
            writeString(file, log.timestamp);
            writeString(file, log.action);
            writeString(file, log.detail);
        }
        file.close();
        return true;
    }

    Tamagotchi* loadGame(const std::string& filename = "tamagotchi.sav") {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) return nullptr;

        char magic[4];
        file.read(magic, 4);
        if (magic[0]!='T' || magic[1]!='A' || magic[2]!='M' || magic[3]!='A') return nullptr;

        uint8_t version = 0;
        file.read(reinterpret_cast<char*>(&version), sizeof(version));
        if (version != 1) return nullptr;

        Tamagotchi* t = new Tamagotchi();
        t->name = readString(file);
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
            log.timestamp = readString(file);
            log.action    = readString(file);
            log.detail    = readString(file);
            t->logs.push_back(log);
        }
        file.close();
        return t;
    }

    inline bool saveExists(const std::string& filename = "tamagotchi.sav") {
        std::ifstream f(filename);
        return f.good();
    }
} 

// ─────────────────────────────────────────────
// MAIN WITH FTXUI INTERACTIVE TUI
// ─────────────────────────────────────────────
int main() {
    #ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
    #else
        std::setlocale(LC_ALL, "");
    #endif

    std::srand(static_cast<unsigned>(std::time(nullptr)));
    using namespace ftxui;

    auto screen = ScreenInteractive::Fullscreen();
    Tamagotchi* pet = nullptr;
    std::string system_status = "Welcome to Tamagotchi Frutiger Aero!";

    // --- PALET WARNA FRUTIGER AERO ---
    auto ColorSkyBlue   = Color::RGB(0, 180, 255);
    auto ColorAuroraGreen= Color::RGB(50, 220, 100);
    auto ColorGlossyAqua = Color::RGB(10, 240, 200);
    auto ColorSunYellow  = Color::RGB(255, 210, 40);
    auto ColorEcoGlass   = Color::RGB(220, 245, 235);

    // ── STAGE 1: LOADING / CREATION SCREEN ──
    bool choosing_load = TamagotchiEngine::saveExists();
    bool need_name_input = !choosing_load;
    std::string input_name_str = "Tama";

    Component input_name = Input(&input_name_str, "Type name here...");
    
    int menu_load_selected = 0;
    std::vector<std::string> load_options = {" Yes, Load Saved Data ", " No, Create New Pet "};
    Component load_menu = Menu(&load_options, &menu_load_selected);

    auto startup_renderer = Renderer([&] {
        Elements children;
        children.push_back(text("      ◈  F R U T I G E R   A E R O  ◈      ") | bold | color(ColorGlossyAqua) | center);
        children.push_back(text("─ 🌌 🌱 T A M A G O T C H I 🌱 🌌 ─") | color(ColorSkyBlue) | center);
        children.push_back(separatorLight());

        if (choosing_load) {
            children.push_back(text("Save file detected! Would you like to restore it?") | center);
            children.push_back(load_menu->Render() | center | color(ColorAuroraGreen));
            children.push_back(text("Press [Enter] to select") | dim | center);
        } else if (need_name_input) {
            children.push_back(text("Give a beautiful name to your digital lifeform:") | center);
            children.push_back(vbox({
                input_name->Render() | borderLight | color(ColorSkyBlue) | size(WIDTH, LESS_THAN, 30) | center
            }));
            children.push_back(text("Press [Enter] to launch ecosystem") | dim | center);
        }

        return window(text(" ECO-OS v2.6 Initialization ") | bold, vbox(std::move(children))) 
               | color(ColorEcoGlass) | center;
    });

    // Custom Event Loop untuk Start Screen
    auto startup_loop = CatchEvent(startup_renderer, [&](Event event) {
        if (event == Event::Return) {
            if (choosing_load) {
                if (menu_load_selected == 0) {
                    pet = TamagotchiEngine::loadGame();
                    if (pet) {
                        system_status = "Ecosystem successfully loaded.";
                        choosing_load = false;
                    } else {
                        system_status = "Failed loading. Creating a new profile.";
                        choosing_load = false;
                        need_name_input = true;
                    }
                } else {
                    choosing_load = false;
                    need_name_input = true;
                }
            } else if (need_name_input) {
                if(input_name_str.empty()) input_name_str = "Tama";
                pet = TamagotchiEngine::createTamagotchi(input_name_str);
                need_name_input = false;
                screen.ExitLoopClosure()();
            }
            return true;
        }
        if (choosing_load) return load_menu->OnEvent(event);
        if (need_name_input) return input_name->OnEvent(event);
        return false;
    });

    if (choosing_load || need_name_input) {
        screen.Loop(startup_loop);
    }

    // Backup safety if loops skipped
    if (!pet) pet = TamagotchiEngine::createTamagotchi("Tama");

    // ── STAGE 2: MAIN DASHBOARD TUI ──
    int selected_action = 0;
    std::vector<std::string> actions = {
        " 🍏 Feed Nutrient ",
        " ⚽ Play Stimulation ",
        " 💤 Hydro-Sleep Regimen ",
        " 💊 Apply Medical Serum ",
        " ⏳ Cycle Eco-Time (Pass Day) ",
        " 💾 Sync Matrix (Save) ",
        " 🛑 Deactivate Shell (Quit) "
    };

    auto action_menu = Menu(&actions, &selected_action);

    auto main_renderer = Renderer(action_menu, [&] {
        // Render Progress Bar Cantik Bertema Aero (Inverted hunger ke Fullness)
        int fullness = 100 - pet->hunger;
        
        auto make_bar = [](int val, Color col) {
            return gauge(val / 100.0) | color(col) | bgcolor(Color::RGB(20,40,40));
        };

        // Render Logs
        Elements log_elements;
        int start = std::max(0, (int)pet->logs.size() - 6);
        for (size_t i = start; i < pet->logs.size(); ++i) {
            log_elements.push_back(hbox({
                text("[" + pet->logs[i].timestamp + "] ") | color(ColorSkyBlue),
                text(pet->logs[i].action) | bold | color(ColorGlossyAqua) | size(WIDTH, EQUAL, 8),
                text(": " + pet->logs[i].detail) | color(ColorEcoGlass)
            }));
        }
        if(log_elements.empty()) log_elements.push_back(text("No biosphere logs yet."));

        // Aero Graphics ASCII (Water / Glass Bubble Concept)
        auto aero_art = vbox({
            text("      .---.       ") | color(ColorGlossyAqua),
            text("     /     \\     ") | color(ColorSkyBlue),
            text("    |  O_O  |    ") | color(ColorAuroraGreen) | bold,
            text("     \\  -  /     ") | color(ColorSkyBlue),
            text("      '---'       ") | color(ColorGlossyAqua),
            text("  [ " + pet->name + " ]") | center | bold | color(ColorSunYellow)
        });

        // Dashboard layout
        auto stats_view = window(text(" 📊 Eco-Metrics ") | bold | color(ColorSkyBlue), vbox({
            hbox({ text("Fullness:  ") | size(WIDTH, EQUAL, 12), make_bar(fullness, ColorGlossyAqua), text(" " + std::to_string(fullness) + "/100") }),
            hbox({ text("Happiness: ") | size(WIDTH, EQUAL, 12), make_bar(pet->happiness, ColorSunYellow), text(" " + std::to_string(pet->happiness) + "/100") }),
            hbox({ text("Energy:    ") | size(WIDTH, EQUAL, 12), make_bar(pet->energy, ColorSkyBlue), text(" " + std::to_string(pet->energy) + "/100") }),
            hbox({ text("Health:    ") | size(WIDTH, EQUAL, 12), make_bar(pet->health, ColorAuroraGreen), text(" " + std::to_string(pet->health) + "/100") }),
            separatorLight(),
            hbox({ text("Chronology: ") | bold, text(std::to_string(pet->age) + " Days in Orbit") | color(ColorSunYellow) }),
            hbox({ text("Life-Sign:  ") | bold, text(pet->alive ? "🟢 STABLE ACTIVATION" : "🔴 ECO-SYSTEM COLLAPSED") | color(pet->alive ? ColorAuroraGreen : Color::Red) })
        }));

        auto interaction_zone = hbox({
            window(text(" 🎮 Interface Core ") | bold | color(ColorAuroraGreen), action_menu->Render() | frame),
            window(text(" 🫧 Entity Visual ") | bold | color(ColorGlossyAqua), aero_art | center | flex)
        });

        auto event_notifier = window(text(" 📡 Environmental Scanner ") | bold | color(ColorSunYellow), 
            text(TamagotchiEngine::latestEncounterMessage) | italic | color(ColorEcoGlass)
        );

        auto system_bar = window(text(" 💻 Biosphere OS System Log "), 
            text("Status: " + system_status) | color(ColorGlossyAqua)
        );

        return vbox({
            vbox({
                text(" 🌊 FRUTIGER AERO BIOSPHERE ENGINE 🌊 ") | bold | color(ColorEcoGlass) | center,
                text("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━") | color(ColorSkyBlue) | center,
            }),
            hbox({
                stats_view | flex_grow,
                window(text(" 📜 Matrix History (Logs) ") | bold | color(ColorEcoGlass), vbox(std::move(log_elements))) | size(WIDTH, EQUAL, 50)
            }) | flex_grow,
            event_notifier,
            interaction_zone,
            system_bar
        });
    });

    auto main_loop = CatchEvent(main_renderer, [&](Event event) {
        if (!Utils::isAlive(*pet)) {
            if (event == Event::Character('q') || event == Event::Escape || event == Event::Return) {
                screen.ExitLoopClosure()();
                return true;
            }
            return false;
        }

        if (event == Event::Return) {
            switch (selected_action) {
                case 0: // Feed
                    TamagotchiEngine::applyAction(*pet, TamagotchiEngine::feedAction);
                    system_status = pet->name + " processed nutrients successfully.";
                    break;
                case 1: // Play
                    if (pet->energy < 20) {
                        system_status = "⚠️ Action Aborted: Depleted energy core!";
                    } else {
                        TamagotchiEngine::applyAction(*pet, TamagotchiEngine::playAction);
                        system_status = pet->name + " hyper-activity routine complete.";
                    }
                    break;
                case 2: // Sleep
                    TamagotchiEngine::applyAction(*pet, TamagotchiEngine::sleepAction);
                    system_status = pet->name + " initiated regenerative sleep mode.";
                    break;
                case 3: // Heal
                    TamagotchiEngine::applyAction(*pet, TamagotchiEngine::healAction);
                    system_status = "Medical serum injected into " + pet->name + ".";
                    break;
                case 4: // Pass Time
                    TamagotchiEngine::applyAction(*pet, TamagotchiEngine::ageTick);
                    TamagotchiEngine::triggerRandomEncounter(*pet);
                    system_status = "Macro timeline skipped by 1 day cycle.";
                    break;
                case 5: // Save
                    if (TamagotchiEngine::saveGame(*pet)) {
                        system_status = "✅ Biosphere signature saved to 'tamagotchi.sav'.";
                    } else {
                        system_status = "❌ Fatal error writing backup.";
                    }
                    break;
                case 6: // Quit
                    system_status = "Deactivating shell...";
                    screen.ExitLoopClosure()();
                    break;
            }
            return true;
        }
        return action_menu->OnEvent(event);
    });

    screen.Loop(main_loop);

    // ── STAGE 3: GAME OVER / POST-EXIT SCREEN ──
    std::cout << "\n\033[1;36m║ System shutdown complete.\033[0m\n";
    if (!Utils::isAlive(*pet)) {
        std::cout << "\n\033[1;31m [!] ECO-SYSTEM COLLAPSED: " << pet->name << " has passed away at " << pet->age << " days old.\033[0m\n\n";
    } else {
        std::cout << " [✓] " << pet->name << " safely preserved inside the dynamic core.\n\n";
    }

    delete pet;
    return 0;
}