// ============================================================
//  TAMAGOTCHI - FTXUI Edition
//  Tema: Frutiger Aero (soft, organic, nature)
// ============================================================

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/color.hpp>


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

#ifdef _WIN32
    #include <windows.h>
#endif
#include <locale>

// ============================================================
// 1. STRUCT — Main Entities
// ============================================================
struct StatusLog {
    std::string timestamp;
    std::string action;
    std::string detail;
};

struct Tamagotchi {
    std::string name;
    int         hunger;     // 0=full, 100=starving
    int         happiness; // 0=sad,  100=very happy
    int         energy;     // 0=tired, 100=full energy
    int         health;     // 0=sick,  100=healthy
    int         age;       // days
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

// ============================================================
// Template Helpers
// ============================================================
template<typename T>
T clamp(T value, T minVal, T maxVal) {
    if (value < minVal) return minVal;
    if (value > maxVal) return maxVal;
    return value;
}

// ============================================================
// 2. NAMESPACE — Constants
// ============================================================
namespace Constants {
    // Frutiger Aero Color Palette
    // Soft, organic colors reminiscent of nature and water
    constexpr int COLOR_AERO_CYAN     = 0;    // Deep cyan
    constexpr int COLOR_AERO_SKY      = 1;    // Sky blue  
    constexpr int COLOR_AERO_MINT      = 2;    // Mint green
    constexpr int COLOR_AERO_SPRING   = 3;    // Spring green
    constexpr int COLOR_AERO_WATER   = 4;    // Water blue
    constexpr int COLOR_AERO_LIGHT    = 5;    // Light cyan
    constexpr int COLOR_AERO_WHITE    = 6;    // Soft white
    constexpr int COLOR_AERO_PEARL    = 7;    // Pearl
    
    const std::vector<ftxui::Color> FRUTIGER_AERO = {
    ftxui::Color::RGB(0,   188, 212), // #00BCD4 - Deep cyan
    ftxui::Color::RGB(178, 235, 242), // #B2EBF2 - Sky cyan
    ftxui::Color::RGB(76,  175, 80),  // #4CAF50 - Mint
    ftxui::Color::RGB(200, 230, 201), // #C8E6C9 - Spring
    ftxui::Color::RGB(33,  150, 243), // #2196F3 - Water
    ftxui::Color::RGB(128, 222, 234), // #80DEEA - Light cyan
    ftxui::Color::RGB(255, 255, 255), // #FFFFFF - White
    ftxui::Color::RGB(245, 245, 245)  // #F5F5F5 - Gray8 equivalent
    };

    // Encounter table
    const std::vector<EncounterEvent> ENCOUNTER_TABLE = {
        {"FLU", "Caught the flu! Runny nose...", +10, -15, -10, -20},
        {"FOOD POISONING", "Ate something weird!", +20, -20, -5, -25},
        {"CAUGHT IN RAIN", "Got drenched without umbrella!", +5, -10, -15, -15},
        {"SCARED", "Encountered something terrifying!", 0, -25, -20, -5},
        {"MOSQUITO BITE", "Mosquitoes while sleeping...", +5, -10, -5, -10},
        {"INSOMNIA", "Couldn't sleep well last night.", +10, -5, -25, -10},
        {"HEATSTROKE", "Scorching weather!", +15, -10, -20, -15},
        {"BEAUTIFUL DAY", "Sunny weather, feeling great!", -5, +20, +10, +5},
        {"SURPRISE GIFT", "Found an unexpected gift!", 0, +25, +10, +10},
        {"FOUND FRUIT", "Fresh and delicious fruit!", -15, +10, +10, +5},
        {"MASSAGE", "Relaxing massage from friend!", 0, +15, +20, +10},
        {"LOVELY MUSIC", "Soul-soothing music.", 0, +20, +5, +5},
    };

    const int ENCOUNTER_CHANCE = 60;
}

// ============================================================
// 3. NAMESPACE — Engine
// ============================================================
namespace Engine {

    std::string currentTime() {
        std::time_t t = std::time(nullptr);
        char buf[20];
        std::strftime(buf, sizeof(buf), "%H:%M:%S", std::localtime(&t));
        return std::string(buf);
    }

    bool isAlive(const Tamagotchi& t) {
        return t.alive && t.health > 0;
    }

    std::string getStatusLabel(int value) {
        if (value >= 75) return "Excellent";
        if (value >= 50) return "Good";
        if (value >= 25) return "Low";
        return "Critical";
    }

    void addLog(Tamagotchi& t, const std::string& action, const std::string& detail) {
        StatusLog log;
        log.timestamp = currentTime();
        log.action = action;
        log.detail = detail;
        t.logs.push_back(log);
    }

    // Create new Tamagotchi
    Tamagotchi* create(const std::string& name,
        int hunger = 50, int happiness = 70, int energy = 80, int health = 100) {
        
        Tamagotchi* t = new Tamagotchi();
        t->name = name;
        t->hunger = clamp(hunger, 0, 100);
        t->happiness = clamp(happiness, 0, 100);
        t->energy = clamp(energy, 0, 100);
        t->health = clamp(health, 0, 100);
        t->age = 0;
        t->alive = true;
        addLog(*t, "BORN", "Tamagotchi " + name + " has been born!");
        return t;
    }

    // Actions
    void feed(Tamagotchi& t) {
        t.hunger = clamp(t.hunger - 30, 0, 100);
        t.happiness = clamp(t.happiness + 10, 0, 100);
        t.energy = clamp(t.energy + 5, 0, 100);
        addLog(t, "FEED", "Hunger -30 | Happiness +10");
    }

    void play(Tamagotchi& t) {
        if (t.energy < 20) {
            addLog(t, "PLAY", "Failed — too tired!");
            return;
        }
        t.happiness = clamp(t.happiness + 25, 0, 100);
        t.energy = clamp(t.energy - 20, 0, 100);
        t.hunger = clamp(t.hunger + 15, 0, 100);
        addLog(t, "PLAY", "Happiness +25 | Energy -20");
    }

    void sleep(Tamagotchi& t) {
        t.energy = clamp(t.energy + 40, 0, 100);
        t.hunger = clamp(t.hunger + 10, 0, 100);
        t.health = clamp(t.health + 5, 0, 100);
        addLog(t, "SLEEP", "Energy +40 | Health +5");
    }

    void heal(Tamagotchi& t) {
        t.health = clamp(t.health + 30, 0, 100);
        t.energy = clamp(t.energy + 10, 0, 100);
        addLog(t, "HEAL", "Health +30 | Energy +10");
    }

    // Aging + Random encounter
    void ageTick(Tamagotchi& t) {
        t.age++;
        t.hunger = clamp(t.hunger + 10, 0, 100);
        t.happiness = clamp(t.happiness - 5, 0, 100);
        t.energy = clamp(t.energy - 8, 0, 100);
        if (t.hunger >= 80) t.health = clamp(t.health - 15, 0, 100);
        if (t.happiness <= 20) t.health = clamp(t.health - 5, 0, 100);
        addLog(t, "TIME", "Day " + std::to_string(t.age));
    }

    EncounterEvent* triggerRandomEncounter() {
        if ((std::rand() % 100) >= Constants::ENCOUNTER_CHANCE) return nullptr;
        
        const auto& ev = Constants::ENCOUNTER_TABLE[std::rand() % Constants::ENCOUNTER_TABLE.size()];
        return new EncounterEvent(ev);
    }

    // Apply encounter to Tamagotchi
    void applyEncounter(Tamagotchi& t, const EncounterEvent& ev) {
        t.hunger = clamp(t.hunger + ev.hungerDelta, 0, 100);
        t.happiness = clamp(t.happiness + ev.happinessDelta, 0, 100);
        t.energy = clamp(t.energy + ev.energyDelta, 0, 100);
        t.health = clamp(t.health + ev.healthDelta, 0, 100);
        addLog(t, "EVENT", ev.name + ": " + ev.description);
    }

    // Check death
    void checkDeath(Tamagotchi& t) {
        if (t.hunger >= 100 || t.health <= 0) {
            t.alive = false;
            addLog(t, "DIED", t.name + " has passed away.");
        }
    }

    // Save/Load
    bool saveGame(const Tamagotchi& t, const std::string& filename = "tamagotchi.sav") {
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) return false;
        
        const char magic[4] = {'T','A','M','A'};
        const uint8_t version = 1;
        file.write(magic, 4);
        file.write(reinterpret_cast<const char*>(&version), 1);
        
        auto writeString = [&](const std::string& s) {
            uint32_t len = s.size();
            file.write(reinterpret_cast<const char*>(&len), 4);
            file.write(s.data(), len);
        };
        
        writeString(t.name);
        file.write(reinterpret_cast<const char*>(&t.hunger), 4);
        file.write(reinterpret_cast<const char*>(&t.happiness), 4);
        file.write(reinterpret_cast<const char*>(&t.energy), 4);
        file.write(reinterpret_cast<const char*>(&t.health), 4);
        file.write(reinterpret_cast<const char*>(&t.age), 4);
        uint8_t alive = t.alive ? 1 : 0;
        file.write(reinterpret_cast<const char*>(&alive), 1);
        
        uint32_t logCount = t.logs.size();
        file.write(reinterpret_cast<const char*>(&logCount), 4);
        for (const auto& log : t.logs) {
            writeString(log.timestamp);
            writeString(log.action);
            writeString(log.detail);
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
        
        uint8_t version;
        file.read(reinterpret_cast<char*>(&version), 1);
        if (version != 1) return nullptr;
        
        auto readString = [&]() -> std::string {
            uint32_t len;
            file.read(reinterpret_cast<char*>(&len), 4);
            std::string s(len, '\0');
            file.read(&s[0], len);
            return s;
        };
        
        Tamagotchi* t = new Tamagotchi();
        t->name = readString();
        file.read(reinterpret_cast<char*>(&t->hunger), 4);
        file.read(reinterpret_cast<char*>(&t->happiness), 4);
        file.read(reinterpret_cast<char*>(&t->energy), 4);
        file.read(reinterpret_cast<char*>(&t->health), 4);
        file.read(reinterpret_cast<char*>(&t->age), 4);
        uint8_t alive;
        file.read(reinterpret_cast<char*>(&alive), 1);
        t->alive = (alive == 1);
        
        uint32_t logCount;
        file.read(reinterpret_cast<char*>(&logCount), 4);
        for (uint32_t i = 0; i < logCount; i++) {
            StatusLog log;
            log.timestamp = readString();
            log.action = readString();
            log.detail = readString();
            t->logs.push_back(log);
        }
        file.close();
        addLog(*t, "LOAD", "Data loaded — day " + std::to_string(t->age));
        return t;
    }

    bool saveExists(const std::string& filename = "tamagotchi.sav") {
        std::ifstream f(filename);
        return f.good();
    }
}

// ============================================================
// 4. FTXUI COMPONENTS - Frutiger Aero Theme
// ============================================================
using namespace ftxui;

namespace Theme {

    // Frutiger Aero inspired color scheme
    Color AERO_CYAN()   { return Color::RGB(0, 188, 212); }   // #00BCD4
    Color AERO_SKY()    { return Color::RGB(178, 235, 242); } // #B2EBF2
    Color AERO_MINT()   { return Color::RGB(76, 175, 80); }   // #4CAF50
    Color AERO_SPRING() { return Color::RGB(200, 230, 201); } // #C8E6C9
    Color AERO_WATER()  { return Color::RGB(33, 150, 243); }  // #2196F3
    Color AERO_LIGHT()  { return Color::RGB(128, 222, 234); } // #80DEEA
    
    // Gradient effect using border style
    Element gradientBorder(Element inner, bool active = false) {
        if (active) {
            color(Color::RGB(0, 191, 255)) | bold;
        }
        color(Color::RGB(180, 180, 180)); // Soft gray
    }

    // Animated progress bar with gradient colors
    Element aeroGauge(int value, int width = 20) {
        std::string bar;
        int filled = (value * width) / 100;
        for (int i = 0; i < width; i++) {
            if (i < filled) {
                // Gradient effect: different colors along the bar
                if (i < width * 0.3) bar += "█";  // Red zone
                else if (i < width * 0.7) bar += "█"; // Yellow zone
                else bar += "█";  // Green zone
            } else {
                bar += "░";
            }
        }
        
        // Color based on value
        Color barColor =
            value >= 75 ? Color::RGB(0, 255, 127) :   // SpringGreen
            value >= 50 ? Color::RGB(0, 191, 255) :   // Aero Cyan
            value >= 25 ? Color::RGB(255, 215, 0) :   // Gold
                          Color::RGB(255, 99, 71);    // Tomato Red
        return text(bar) | color(barColor);
    }

    // Status indicator with glow effect
    Element statusIndicator(const std::string& label, int value, bool inverse = false) {
        int displayValue = inverse ? 100 - value : value;
        std::string status = Engine::getStatusLabel(displayValue);
        
        Color statusColor = displayValue >= 75 ? Color::green3 :
                           displayValue >= 50 ? Color::cyan3 :
                           displayValue >= 25