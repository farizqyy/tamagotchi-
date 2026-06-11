#include <iostream>
#include <string>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cmath>

#include "ftxui/component/component.hpp"
#include "ftxui/component/component_base.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/dom/node.hpp"
#include "ftxui/screen/color.hpp"

using namespace ftxui;

// ═══════════════════════════════════════════════════════════════
// STRUCTS
// ═══════════════════════════════════════════════════════════════

struct StatusLog {
    std::string timestamp;
    std::string action;
    std::string detail;
};

struct Tamagotchi {
    std::string name;
    int hunger     = 50;
    int happiness  = 70;
    int energy     = 80;
    int health     = 100;
    int age        = 0;
    bool alive     = true;
    std::vector<StatusLog> logs;
};

// ═══════════════════════════════════════════════════════════════
// RANDOM ENCOUNTER DEFINITION
// ═══════════════════════════════════════════════════════════════

struct Encounter {
    std::string title;          // Short name
    std::string description;    // What happened to tamagotchi
    int hunger_delta;
    int happiness_delta;
    int energy_delta;
    int health_delta;
};

// ═══════════════════════════════════════════════════════════════
// UTILITIES
// ═══════════════════════════════════════════════════════════════

template<typename T>
inline T clamp(T value, T minVal, T maxVal) {
    if (value < minVal) return minVal;
    if (value > maxVal) return maxVal;
    return value;
}

std::string currentTime() {
    std::time_t t = std::time(nullptr);
    char buf[20];
    std::strftime(buf, sizeof(buf), "%H:%M:%S", std::localtime(&t));
    return std::string(buf);
}

void addLog(Tamagotchi& t, const std::string& action, const std::string& detail) {
    StatusLog log;
    log.timestamp = currentTime();
    log.action = action;
    log.detail = detail;
    t.logs.push_back(log);
    if (t.logs.size() > 50) {
        t.logs.erase(t.logs.begin());
    }
}

// ═══════════════════════════════════════════════════════════════
// ENCOUNTER TABLE
// ═══════════════════════════════════════════════════════════════

const Encounter ENCOUNTER_TABLE[] = {
    // Neutral/Good events
    { "Nap Time", "Tamagotchi fell asleep and took a nap 😴", +5, 0, +20, +5 },
    { "Found Food", "Tamagotchi found some tasty leftovers 🍖", -25, +10, 0, 0 },
    { "Sunny Day", "Beautiful weather made Tamagotchi feel great ☀️", -5, +20, +5, +10 },
    { "Found Fruit", "Tamagotchi discovered a fresh juicy fruit 🍎", -20, +15, 0, +5 },
    
    // Mixed/Bad events
    { "Got Rained On", "Tamagotchi got caught in the rain and got wet 🌧️", +5, -10, -15, -10 },
    { "Got Sick", "Tamagotchi caught a cold and feels awful 🤧", +10, -20, -15, -30 },
    { "Made a Friend", "Tamagotchi made a new friend and played together 👫", +5, +30, -10, +5 },
    { "Bad Dream", "Tamagotchi had a terrible nightmare 😰", 0, -25, 0, -10 },
    
    // More varied impacts
    { "Lost in Thought", "Tamagotchi got distracted and spaced out 💭", +15, -15, 0, -5 },
    { "Ate Too Much", "Tamagotchi ate too much and felt bloated 🤢", +30, -5, -10, -15 },
    { "Danced Around", "Tamagotchi danced around with joy 💃", 0, +25, -20, 0 },
    { "Found a Toy", "Tamagotchi found an old toy to play with 🧸", -10, +20, -5, 0 },
    
    // Extreme encounters
    { "Earthquake!", "The ground shook! Tamagotchi got startled 😱", 0, -30, -20, -20 },
    { "Found Money", "Lucky! Tamagotchi found some coins 💰", -15, +35, +10, +10 },
    { "Fought with Bug", "Tamagotchi fought a big scary bug 🐛", +20, -20, -25, -15 },
    { "Meteor Shower", "Tamagotchi watched beautiful meteor shower ⭐", 0, +30, +5, 0 },
};

const int ENCOUNTER_COUNT = sizeof(ENCOUNTER_TABLE) / sizeof(ENCOUNTER_TABLE[0]);
const int ENCOUNTER_CHANCE = 60;  // 60% chance each day

// ═══════════════════════════════════════════════════════════════
// IMPROVED FRUTIGER AERO COLORS (Better Contrast)
// ═══════════════════════════════════════════════════════════════

// Background: Light gradient
Color bg_light_blue = Color::RGB(200, 230, 250);      // Very light blue
Color bg_gradient = Color::RGB(230, 240, 250);         // Almost white-blue

// Primary UI: Strong blues
Color primary_dark = Color::RGB(41, 128, 185);         // Strong steel blue
Color primary_light = Color::RGB(52, 152, 219);        // Bright sky blue

// Accent colors: High contrast against light background
Color accent_orange = Color::RGB(230, 126, 34);        // Darker orange
Color accent_green = Color::RGB(39, 174, 96);          // Darker green
Color accent_cyan = Color::RGB(26, 188, 156);          // Darker teal
Color accent_red = Color::RGB(231, 76, 60);            // Darker red

// Text colors
Color text_dark = Color::RGB(44, 62, 80);              // Very dark gray
Color text_light = Color::RGB(255, 255, 255);          // White

// ═══════════════════════════════════════════════════════════════
// SAVE/LOAD SUPPORT
// ═══════════════════════════════════════════════════════════════

bool savePet(const Tamagotchi& pet, const std::string& filename = "tama_save.txt") {
    std::ofstream file(filename);
    if (!file.is_open()) return false;
    
    file << pet.name << "\n";
    file << pet.hunger << " " << pet.happiness << " " << pet.energy << " " << pet.health << "\n";
    file << pet.age << " " << (pet.alive ? 1 : 0) << "\n";
    file << pet.logs.size() << "\n";
    
    for (const auto& log : pet.logs) {
        file << log.timestamp << "|" << log.action << "|" << log.detail << "\n";
    }
    
    file.close();
    return true;
}

bool loadPet(Tamagotchi& pet, const std::string& filename = "tama_save.txt") {
    std::ifstream file(filename);
    if (!file.is_open()) return false;
    
    std::getline(file, pet.name);
    file >> pet.hunger >> pet.happiness >> pet.energy >> pet.health;
    file >> pet.age;
    
    int alive_int;
    file >> alive_int;
    pet.alive = (alive_int == 1);
    
    size_t log_count;
    file >> log_count;
    file.ignore();  // Ignore newline after log_count
    
    for (size_t i = 0; i < log_count; i++) {
        std::string line;
        std::getline(file, line);
        
        size_t pos1 = line.find('|');
        size_t pos2 = line.find('|', pos1 + 1);
        
        if (pos1 != std::string::npos && pos2 != std::string::npos) {
            StatusLog log;
            log.timestamp = line.substr(0, pos1);
            log.action = line.substr(pos1 + 1, pos2 - pos1 - 1);
            log.detail = line.substr(pos2 + 1);
            pet.logs.push_back(log);
        }
    }
    
    file.close();
    return true;
}

bool saveFileExists(const std::string& filename = "tama_save.txt") {
    std::ifstream file(filename);
    return file.good();
}

// ═══════════════════════════════════════════════════════════════
// UI COMPONENTS
// ═══════════════════════════════════════════════════════════════

Element aeroGradientBox(const std::string& title, Element content) {
    return window(
        text(title) | bold | color(text_light) | bgcolor(primary_dark),
        content | color(text_dark)
    ) | border | color(primary_dark) | bgcolor(bg_light_blue);
}

Element statusBar(const std::string& label, int value, Color barColor) {
    int width = 20;
    int filled = (value * width) / 100;
    
    std::string bar = "[";
    for (int i = 0; i < width; i++) {
        bar += (i < filled) ? "█" : "░";
    }
    bar += "]";
    
    std::stringstream ss;
    ss << std::setw(3) << value << "%";
    
    return hbox({
        text(label) | size(WIDTH, EQUAL, 12) | color(text_dark),
        text(bar) | color(barColor),
        text(ss.str()) | color(text_dark)
    });
}

Element tamagotchiDisplay(const Tamagotchi& pet) {
    std::vector<Element> stats;
    
    // Title with age
    stats.push_back(
        hbox({
            text("🐾 " + pet.name) | bold | color(primary_dark),
            text(" | Day " + std::to_string(pet.age)) | color(primary_light)
        }) | center
    );
    
    stats.push_back(separator());
    
    // Status indicator
    std::string status = pet.alive ? "✓ ALIVE" : "✗ DECEASED";
    Color statusColor = pet.alive ? accent_green : accent_red;
    stats.push_back(
        text(status) | color(statusColor) | bold | center
    );
    
    stats.push_back(text(""));  // Spacing
    
    // Stats bars
    stats.push_back(text("═══════════════════════") | center | color(primary_light));
    stats.push_back(statusBar("Fullness ", 100 - pet.hunger, accent_orange));
    stats.push_back(statusBar("Happiness", pet.happiness, accent_cyan));
    stats.push_back(statusBar("Energy   ", pet.energy, accent_green));
    stats.push_back(statusBar("Health   ", pet.health, primary_light));
    stats.push_back(text("═══════════════════════") | center | color(primary_light));
    
    return vbox(stats);
}

Element actionButton(const std::string& label, const std::string& key) {
    return hbox({
        text(" " + label + " ") | border | center | bold | color(text_light) | bgcolor(primary_dark),
        text(" [" + key + "]") | color(primary_light)
    });
}

// ═══════════════════════════════════════════════════════════════
// TAMAGOTCHI ENGINE
// ═══════════════════════════════════════════════════════════════

void feedPet(Tamagotchi& t) {
    if (!t.alive) return;
    t.hunger = clamp(t.hunger - 30, 0, 100);
    t.happiness = clamp(t.happiness + 10, 0, 100);
    t.energy = clamp(t.energy + 5, 0, 100);
    addLog(t, "FEED", "Nom nom! 😋");
}

void playWithPet(Tamagotchi& t) {
    if (!t.alive) return;
    if (t.energy < 20) {
        addLog(t, "PLAY", "Too tired... 😴");
        return;
    }
    t.happiness = clamp(t.happiness + 25, 0, 100);
    t.energy = clamp(t.energy - 20, 0, 100);
    t.hunger = clamp(t.hunger + 15, 0, 100);
    addLog(t, "PLAY", "Wheee! 🎮");
}

void sleepPet(Tamagotchi& t) {
    if (!t.alive) return;
    t.energy = clamp(t.energy + 40, 0, 100);
    t.hunger = clamp(t.hunger + 10, 0, 100);
    t.health = clamp(t.health + 5, 0, 100);
    addLog(t, "SLEEP", "Zzzzz... 😴");
}

void healPet(Tamagotchi& t) {
    if (!t.alive) return;
    t.health = clamp(t.health + 30, 0, 100);
    t.energy = clamp(t.energy + 10, 0, 100);
    addLog(t, "HEAL", "Feeling better! 💊");
}

void passTime(Tamagotchi& t) {
    if (!t.alive) return;
    
    // Age the pet
    t.age++;
    t.hunger = clamp(t.hunger + 10, 0, 100);
    t.happiness = clamp(t.happiness - 5, 0, 100);
    t.energy = clamp(t.energy - 8, 0, 100);
    
    // Apply stat penalties
    if (t.hunger >= 80) t.health = clamp(t.health - 15, 0, 100);
    if (t.happiness <= 20) t.health = clamp(t.health - 5, 0, 100);
    
    // Check death condition
    if (t.hunger >= 100 || t.health <= 0) {
        t.alive = false;
        addLog(t, "DIED", "Rest in peace... 💀");
        return;
    }
    
    // Random encounter (60% chance)
    if ((std::rand() % 100) < ENCOUNTER_CHANCE) {
        const Encounter& enc = ENCOUNTER_TABLE[std::rand() % ENCOUNTER_COUNT];
        
        t.hunger = clamp(t.hunger + enc.hunger_delta, 0, 100);
        t.happiness = clamp(t.happiness + enc.happiness_delta, 0, 100);
        t.energy = clamp(t.energy + enc.energy_delta, 0, 100);
        t.health = clamp(t.health + enc.health_delta, 0, 100);
        
        // Log with detailed encounter description
        std::string detail = "Day " + std::to_string(t.age) + " - " + enc.description;
        addLog(t, "PASS TIME", detail);
    } else {
        addLog(t, "PASS TIME", "Day " + std::to_string(t.age) + " passed without incident");
    }
}

// ═══════════════════════════════════════════════════════════════
// MAIN UI COMPONENT
// ═══════════════════════════════════════════════════════════════

class TamagotchiApp {
public:
    Tamagotchi pet;
    std::string status_message;
    int message_timeout = 0;
    
    TamagotchiApp(const std::string& name) : pet() {
        pet.name = name;
        pet.hunger = 50;
        pet.happiness = 70;
        pet.energy = 80;
        pet.health = 100;
        pet.age = 0;
        pet.alive = true;
        status_message = "";
        addLog(pet, "BORN", "Welcome to the world!");
    }
    
    Element renderHeader() {
        return vbox({
            text(" ╔═══════════════════════════════════════╗ ") | color(primary_dark),
            text(" ║     🐾 TAMAGOTCHI AERO 🐾           ║ ") | color(text_light) | bgcolor(primary_dark),
            text(" ╚═══════════════════════════════════════╝ ") | color(primary_dark),
        });
    }
    
    Element renderStatus() {
        return aeroGradientBox("PET STATUS", tamagotchiDisplay(pet));
    }
    
    Element renderActions() {
        return vbox({
            text(" ACTIONS (Press 1-8) ") | bold | color(text_light) | bgcolor(primary_dark),
            text(""),
            hbox({
                actionButton("🍽️  FEED", "1") | flex,
                text(" "),
                actionButton("🎮 PLAY", "2") | flex,
            }),
            text(""),
            hbox({
                actionButton("😴 SLEEP", "3") | flex,
                text(" "),
                actionButton("💊 HEAL", "4") | flex,
            }),
            text(""),
            hbox({
                actionButton("⏳ PASS TIME", "5") | flex,
                text(" "),
                actionButton("💾 SAVE", "6") | flex,
            }),
            text(""),
            hbox({
                actionButton("📂 LOAD", "7") | flex,
                text(" "),
                actionButton("❌ QUIT", "8") | flex,
            }),
        }) | border | color(primary_dark) | bgcolor(bg_light_blue);
    }
    
    Element renderLogs() {
        std::vector<Element> log_elements;
        
        log_elements.push_back(
            text(" ACTIVITY LOG ") | bold | color(text_light) | bgcolor(primary_dark)
        );
        
        int start = std::max(0, (int)pet.logs.size() - 10);
        
        for (int i = start; i < (int)pet.logs.size(); i++) {
            const auto& log = pet.logs[i];
            std::string log_line = "[" + log.timestamp + "] " + 
                                  log.action + ": " + log.detail;
            log_elements.push_back(
                text(log_line) | color(text_dark)
            );
        }
        
        if (!status_message.empty() && message_timeout > 0) {
            log_elements.push_back(text(""));
            log_elements.push_back(
                text(">>> " + status_message) | bold | color(accent_green)
            );
        }
        
        return vbox(log_elements) | border | color(primary_dark) | bgcolor(bg_light_blue) | flex;
    }
    
    void setStatusMessage(const std::string& msg) {
        status_message = msg;
        message_timeout = 3;  // Show for 3 renders
    }
    
    Element render() {
        if (message_timeout > 0) message_timeout--;
        
        return vbox({
            renderHeader(),
            text(""),
            hbox({
                renderStatus() | flex,
                text(" "),
                renderActions() | flex,
            }) | flex,
            text(""),
            renderLogs() | flex,
        }) | bgcolor(bg_gradient);
    }
};

// ═══════════════════════════════════════════════════════════════
// MAIN
// ═══════════════════════════════════════════════════════════════

int main() {
    std::srand(static_cast<unsigned>(std::time(nullptr)));
    
    auto screen = ScreenInteractive::TerminalOutput();
    
    // Check if save file exists
    bool has_save = saveFileExists();
    int startup_choice = 0;  // 0 = new game, 1 = load game
    std::string pet_name = "Tama";
    
    // STARTUP MENU
    if (has_save) {
        // Show choice menu
        std::vector<std::string> options = {"NEW GAME", "LOAD GAME"};
        int selected = 0;
        
        auto menu_renderer = Renderer([&] {
            std::vector<Element> elements;
            elements.push_back(
                text(" ╔═══════════════════════════════════════╗ ") | color(primary_dark)
            );
            elements.push_back(
                text(" ║     🐾 TAMAGOTCHI AERO 🐾           ║ ") | color(text_light) | bgcolor(primary_dark)
            );
            elements.push_back(
                text(" ╚═══════════════════════════════════════╝ ") | color(primary_dark)
            );
            elements.push_back(text(""));
            elements.push_back(
                text(" Save file found! ") | bold | color(primary_dark) | center
            );
            elements.push_back(text(""));
            elements.push_back(
                text(" Select an option (use arrow keys or 1/2): ") | color(text_dark)
            );
            elements.push_back(text(""));
            
            for (int i = 0; i < (int)options.size(); i++) {
                bool is_selected = (i == selected);
                Color bg = is_selected ? primary_dark : bg_light_blue;
                Color fg = is_selected ? text_light : text_dark;
                elements.push_back(
                    text(" " + std::to_string(i + 1) + ". " + options[i] + " ") | bold | bgcolor(bg) | color(fg) | center
                );
            }
            
            elements.push_back(text(""));
            elements.push_back(
                text(" Press ENTER or Space to confirm ") | color(accent_green) | center
            );
            
            return vbox(elements) | border | color(primary_dark) | bgcolor(bg_gradient) | center;
        });
        
        auto menu_component = CatchEvent(menu_renderer, [&](Event event) {
            if (event == Event::Character('1')) {
                selected = 0;
                startup_choice = 0;
                screen.ExitLoopClosure()();
                return true;
            } else if (event == Event::Character('2')) {
                selected = 1;
                startup_choice = 1;
                screen.ExitLoopClosure()();
                return true;
            } else if (event == Event::ArrowUp) {
                selected = (selected - 1 + (int)options.size()) % (int)options.size();
                return true;
            } else if (event == Event::ArrowDown) {
                selected = (selected + 1) % (int)options.size();
                return true;
            } else if (event == Event::Return || event == Event::Character(' ')) {
                startup_choice = selected;
                screen.ExitLoopClosure()();
                return true;
            }
            return false;
        });
        
        screen.Loop(menu_component);
    }
    
    TamagotchiApp app(pet_name);
    
    // Handle startup choice
    if (startup_choice == 1 && has_save) {
        // Load game
        if (loadPet(app.pet)) {
            app.setStatusMessage("Game loaded from tama_save.txt!");
        }
    } else {
        // New game - show naming screen
        std::string input_name = "Tama";
        bool name_confirmed = false;
        
        auto naming_renderer = Renderer([&] {
            std::vector<Element> elements;
            elements.push_back(
                text(" ╔═══════════════════════════════════════╗ ") | color(primary_dark)
            );
            elements.push_back(
                text(" ║     🐾 TAMAGOTCHI AERO 🐾           ║ ") | color(text_light) | bgcolor(primary_dark)
            );
            elements.push_back(
                text(" ╚═══════════════════════════════════════╝ ") | color(primary_dark)
            );
            elements.push_back(text(""));
            elements.push_back(
                text(" What would you like to name your Tamagotchi? ") | bold | color(primary_dark) | center
            );
            elements.push_back(text(""));
            elements.push_back(
                text(" > " + input_name + " ") | border | color(text_light) | bgcolor(primary_dark)
            );
            elements.push_back(text(""));
            elements.push_back(
                text(" (Max 15 characters, press ENTER to continue) ") | color(accent_green) | center
            );
            
            return vbox(elements) | border | color(primary_dark) | bgcolor(bg_gradient) | center;
        });
        
        auto naming_component = CatchEvent(naming_renderer, [&](Event event) {
            if (event.is_character()) {
                char c = event.character()[0];
                if (c >= 32 && c <= 126 && input_name.length() < 15) {
                    input_name += c;
                    return true;
                }
            } else if (event == Event::Backspace && !input_name.empty()) {
                input_name.pop_back();
                return true;
            } else if (event == Event::Return) {
                name_confirmed = true;
                screen.ExitLoopClosure()();
                return true;
            }
            return false;
        });
        
        screen.Loop(naming_component);
        
        app.pet.name = input_name.empty() ? "Tama" : input_name;
        app.setStatusMessage("Welcome " + app.pet.name + "! Let's begin!");
    }
    
    // MAIN GAME LOOP
    std::function<void()> quit = screen.ExitLoopClosure();
    
    auto renderer = Renderer([&] {
        return app.render();
    });
    
    auto handle_input = [&](Event event) {
        if (event == Event::Character('1')) {
            feedPet(app.pet);
        } else if (event == Event::Character('2')) {
            playWithPet(app.pet);
        } else if (event == Event::Character('3')) {
            sleepPet(app.pet);
        } else if (event == Event::Character('4')) {
            healPet(app.pet);
        } else if (event == Event::Character('5')) {
            passTime(app.pet);
        } else if (event == Event::Character('6')) {
            if (savePet(app.pet)) {
                app.setStatusMessage("Game saved to tama_save.txt");
            } else {
                app.setStatusMessage("Failed to save game!");
            }
        } else if (event == Event::Character('7')) {
            Tamagotchi loaded_pet;
            if (loadPet(loaded_pet)) {
                app.pet = loaded_pet;
                app.setStatusMessage("Game loaded from tama_save.txt!");
            } else {
                app.setStatusMessage("Failed to load game!");
            }
        } else if (event == Event::Character('8')) {
            quit();
        } else if (event == Event::Character('q')) {
            quit();
        }
        return false;
    };
    
    auto component_with_input = CatchEvent(renderer, handle_input);
    
    screen.Loop(component_with_input);
    
    return 0;
}
