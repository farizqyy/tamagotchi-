# Tamagotchi AERO - Complete Documentation

## Table of Contents
1. [Project Overview](#project-overview)
2. [Architecture](#architecture)
3. [Core C++ Features Used](#core-c-features-used)
4. [Code Structure](#code-structure)
5. [Key Systems](#key-systems)
6. [Game Mechanics](#game-mechanics)
7. [UI Components](#ui-components)

---

## Project Overview

**Tamagotchi AERO** is a terminal-based virtual pet simulation game built in C++ using the FTXUI library for interactive UI rendering. It combines classic Tamagotchi gameplay with a modern achievement system.

### Key Features:
- Interactive pet care with multiple actions
- Save/Load game state
- Achievement tracking system
- Retro Frutiger Aero color scheme
- Real-time activity logging
- Random encounter events

---

## Architecture

### High-Level Flow

```
┌─────────────────┐
│   Main Game     │
│    (main())     │
└────────┬────────┘
         │
    ┌────▼─────────┐
    │  Startup     │
    │  Menu/Setup  │
    └────┬─────────┘
         │
    ┌────▼──────────────────┐
    │  TamagotchiApp        │
    │  (Game State + UI)    │
    └────┬──────────────────┘
         │
    ┌────▼───────────────────────────┐
    │  Game Loop                      │
    │  - Render                       │
    │  - Handle Input                 │
    │  - Update Pet Stats             │
    │  - Check Achievements           │
    └────┬───────────────────────────┘
         │
    ┌────▼──────┐
    │   Exit     │
    │   Game     │
    └───────────┘
```

### Data Flow

```
User Input
    ↓
Event Handler (handle_input lambda)
    ↓
Pet Action Function (feedPet, playWithPet, etc.)
    ↓
Update Pet Stats + Logs
    ↓
Update Achievements (PetStatistics namespace)
    ↓
Renderer (app.render())
    ↓
FTXUI Display
```

---

## Core C++ Features Used

### 1. **Structs** (Data Organization)
```cpp
struct StatusLog {
    std::string timestamp;
    std::string action;
    std::string detail;
};

struct Tamagotchi {
    std::string name;
    int hunger, happiness, energy, health;
    int age;
    bool alive;
    std::vector<StatusLog> logs;  // Dynamic log history
};
```
**Purpose:** Defines entities/objects for the pet and game state.

---

### 2. **References (&)** (Avoid Copying)
```cpp
void feedPet(Tamagotchi& t, PetStatistics::Statistics* stats = nullptr) {
    if (!t.alive) return;
    t.hunger = clamp(t.hunger - 30, 0, 100);
    // ... modify pet directly without copying
}
```
**Purpose:** Pass objects by reference to modify them directly and avoid expensive data copies. The `&` means "pass the actual object, not a copy."

---

### 3. **Pointers (*)** (Dynamic Memory & Navigation)
```cpp
PetStatistics::Achievement* findAchievementByName(
    Statistics& stats, const std::string& name) {
    auto it = std::find_if(stats.achievements.begin(), ...);
    if (it != stats.achievements.end()) {
        return &(*it);  // Return pointer to achievement
    }
    return nullptr;  // Return null pointer if not found
}
```
**Purpose:** Navigate memory, handle optional values, and dynamic allocation.

---

### 4. **Namespace** (Code Organization)
```cpp
namespace PetStatistics {
    class AchievementException : public std::exception { ... }
    struct Achievement { ... }
    struct Statistics { ... }
    void updateAchievement(Statistics& stats, ...) { ... }
    void initializeAchievements(Statistics& stats) { ... }
    // ... more functions
}
```
**Purpose:** Group related functions and classes to avoid naming conflicts and organize code logically.

---

### 5. **Callback Functions** (Functions as Parameters)
```cpp
// Lambda used as callback in sort
std::sort(stats.achievements.begin(), stats.achievements.end(),
    [](const Achievement& a, const Achievement& b) {
        return (float)a.progress / a.required > 
               (float)b.progress / b.required;
    });
```
**Purpose:** Pass functions (lambdas) as parameters to customize behavior (sorting criteria).

---

### 6. **Default Arguments & Inline Functions**
```cpp
// Default Argument
bool savePet(const Tamagotchi& pet, 
             const std::string& filename = "tama_save.txt") {
    // filename defaults to "tama_save.txt" if not provided
}

// Inline Template Function
template<typename T>
inline T clamp(T value, T minVal, T maxVal) {
    if (value < minVal) return minVal;
    if (value > maxVal) return maxVal;
    return value;
}
```
**Purpose:** 
- Default arguments simplify function calls with common values
- `inline` optimizes compiler by replacing function calls with actual code

---

### 7. **Function Overloading** (Same Name, Different Parameters)
```cpp
// Overload 1: Update by achievement name (string)
void updateAchievement(Statistics& stats, 
                      const std::string& achievement_name, 
                      int increment) {
    // Uses find_if to locate achievement
}

// Overload 2: Update by achievement index (int)
void updateAchievement(Statistics& stats, 
                      int achievement_index, 
                      int increment) {
    // Direct index access
}
```
**Purpose:** Same function name with different parameter types for flexibility.

---

### 8. **Exception Handling** (Error Management)
```cpp
class AchievementException : public std::exception {
    std::string message;
public:
    AchievementException(const std::string& msg) : message(msg) {}
    const char* what() const noexcept override { return message.c_str(); }
};

void updateAchievement(Statistics& stats, 
                      const std::string& achievement_name, 
                      int increment) {
    try {
        auto it = std::find_if(...);
        if (it == stats.achievements.end()) {
            throw AchievementException("Achievement not found!");
        }
        // ... update logic
    } catch (const AchievementException& e) {
        std::cerr << "Achievement Error: " << e.what() << std::endl;
    }
}
```
**Purpose:** Handle errors gracefully with try-catch blocks instead of crashing.

---

### 9. **STL Algorithms** (Standard Template Library)

#### `std::find_if` (Find with Condition)
```cpp
auto it = std::find_if(stats.achievements.begin(), 
                      stats.achievements.end(),
    [&achievement_name](const Achievement& a) { 
        return a.name == achievement_name;  // Custom condition
    });
```
Finds first achievement matching the condition.

#### `std::count_if` (Count with Condition)
```cpp
int unlocked = std::count_if(stats.achievements.begin(), 
                            stats.achievements.end(),
    [](const Achievement& a) { 
        return a.unlocked;  // Count unlocked achievements
    });
```
Counts how many achievements satisfy the condition.

#### `std::sort` (Sorting with Lambda)
```cpp
std::sort(stats.achievements.begin(), stats.achievements.end(),
    [](const Achievement& a, const Achievement& b) {
        return (float)a.progress / a.required > 
               (float)b.progress / b.required;  // Sort by progress %
    });
```
Sorts achievements by completion percentage.

#### `std::find` (Simple Find)
```cpp
auto it = std::find(stats->events_encountered.begin(), 
                   stats->events_encountered.end(), 
                   enc.title);
```
Finds if event was already encountered.

---

### 10. **File Handling** (Save/Load)
```cpp
// Writing to file (ofstream)
bool savePet(const Tamagotchi& pet, const std::string& filename) {
    std::ofstream file(filename);  // Open file for writing
    if (!file.is_open()) return false;
    
    file << pet.name << "\n";
    file << pet.hunger << " " << pet.happiness << "\n";
    file.close();
    return true;
}

// Reading from file (ifstream)
bool loadPet(Tamagotchi& pet, const std::string& filename) {
    std::ifstream file(filename);  // Open file for reading
    if (!file.is_open()) return false;
    
    std::getline(file, pet.name);
    file >> pet.hunger >> pet.happiness;
    file.close();
    return true;
}
```
**Purpose:** Persist game state to disk and restore it.

---

### 11. **Lambda Expressions** (Anonymous Functions)
```cpp
// Lambda as callback in sort
[](const Achievement& a, const Achievement& b) { 
    return a.progress > b.progress; 
}

// Lambda in find_if
[&achievement_name](const Achievement& a) { 
    return a.name == achievement_name; 
}

// Lambda capturing local variables
auto handle_input = [&](Event event) {
    if (event == Event::Character('1')) {
        feedPet(app.pet, &app.stats);  // & means capture by reference
    }
    return false;
};
```
**Purpose:** Create inline functions for callbacks, sorting, and filtering without defining separate functions.

---

## Code Structure

### File Organization

```
tamagotchi-cplt-2.cpp
├── Includes (Standard libraries + FTXUI)
├── Structs (StatusLog, Tamagotchi, Encounter)
├── PetStatistics Namespace
│   ├── AchievementException class
│   ├── Achievement struct
│   ├── Statistics struct
│   └── Functions (updateAchievement, initializeAchievements, etc.)
├── Utility Functions (clamp, currentTime, addLog)
├── Encounter Table (Predefined events)
├── Color Constants (Frutiger Aero palette)
├── File I/O Functions (savePet, loadPet, saveFileExists)
├── UI Components (aeroGradientBox, statusBar, achievementDisplay)
├── Pet Action Functions (feedPet, playWithPet, sleepPet, healPet, passTime)
├── TamagotchiApp Class (Main UI orchestration)
└── main() function (Game entry point)
```

---

## Key Systems

### 1. **Pet State System**

The `Tamagotchi` struct holds all pet stats:
- **hunger** (0-100): Lower = fuller, higher = hungrier
- **happiness** (0-100): Pet's mood
- **energy** (0-100): Fatigue level
- **health** (0-100): Overall well-being
- **age**: Days passed (increases with passTime())
- **alive**: Pet status (true = alive, false = died)
- **logs**: History of all actions

### 2. **Achievement System** (PetStatistics Namespace)

**5 Achievements:**
1. **First Meal** - Feed pet 5 times
2. **Playtime Master** - Play 10 times
3. **Survivor** - Keep alive for 20 days
4. **Health Guardian** - Heal 8 times
5. **Event Collector** - Encounter 15 unique events

**Tracking:**
```cpp
struct Achievement {
    std::string name;       // "First Meal"
    std::string description; // "Feed your pet 5 times"
    int progress;           // Current: 3/5
    int required;           // Target: 5
    bool unlocked;          // Achieved?
};
```

### 3. **Encounter System**

Random events modify pet stats:
```cpp
struct Encounter {
    std::string title;              // "Nap Time"
    std::string description;        // What happened
    int hunger_delta;               // Change to hunger
    int happiness_delta;            // Change to happiness
    int energy_delta;               // Change to energy
    int health_delta;               // Change to health
};
```

**16 predefined encounters** ranging from beneficial (Found Food) to harmful (Got Sick).

---

## Game Mechanics

### Action System

Each action modifies pet stats:

| Action | Effect | Cost |
|--------|--------|------|
| **Feed** | hunger -30, happiness +10, energy +5 | Pet must be alive |
| **Play** | happiness +25, energy -20, hunger +15 | Requires energy ≥ 20 |
| **Sleep** | energy +40, hunger +10, health +5 | Restores energy |
| **Heal** | health +30, energy +10 | Costs slightly on overall state |
| **Pass Time** | age +1, hunger +10, happiness -5, energy -8 | May trigger random event |

### Stat Degradation

Over time (each pass time):
- Hunger increases naturally
- High hunger (≥80) reduces health
- Low happiness (≤20) reduces health
- Pet dies if hunger ≥100 OR health ≤0

### Random Encounters

- **60% chance** when passing time
- Randomly selected from 16 encounters
- Examples:
  - ☀️ Sunny Day: -5 hunger, +20 happiness
  - 🤧 Got Sick: +10 hunger, -20 happiness, -30 health
  - 💰 Found Money: -15 hunger, +35 happiness

---

## UI Components

### Rendering Architecture

```
TamagotchiApp::render()
    ├── renderHeader()     → Game title
    ├── renderStatus()     → Pet stats with bars
    ├── renderActions()    → Available actions (1-9)
    ├── renderLogs()       → Recent activity history
    └── achievementDisplay() → Achievement progress (if toggled)
```

### Color Scheme (Frutiger Aero)

```cpp
primary_dark    = RGB(41, 128, 185)      // Steel blue - main UI
primary_light   = RGB(52, 152, 219)      // Sky blue - highlights
accent_orange   = RGB(230, 126, 34)      // Orange - warnings
accent_green    = RGB(39, 174, 96)       // Green - success
accent_cyan     = RGB(26, 188, 156)      // Teal - info
accent_red      = RGB(231, 76, 60)       // Red - danger
bg_light_blue   = RGB(200, 230, 250)     // Light blue background
text_dark       = RGB(44, 62, 80)        // Dark text
text_light      = RGB(255, 255, 255)     // White text
```

### FTXUI Elements Used

- `text()` - Display text
- `vbox()` - Vertical box (stack elements vertically)
- `hbox()` - Horizontal box (arrange elements horizontally)
- `border()` - Add border around element
- `color()` - Apply text color
- `bgcolor()` - Apply background color
- `bold` - Bold text
- `center` - Center alignment
- `flex` - Flexible width
- `separator()` - Horizontal line
- `window()` - Window with title
- `Renderer()` - Custom UI rendering
- `CatchEvent()` - Handle keyboard input

---

## Game Flow

### 1. Startup
```
Start Game
    ↓
Save file exists?
    ├─ YES → Show menu (New/Load)
    └─ NO → Go to naming
    ↓
Name Pet
    ↓
Initialize achievements
    ↓
Main Loop
```

### 2. Main Loop
```
Render current state
    ↓
Wait for input (1-9)
    ├─ 1: Feed
    ├─ 2: Play
    ├─ 3: Sleep
    ├─ 4: Heal
    ├─ 5: Pass Time
    ├─ 6: Save
    ├─ 7: Load
    ├─ 8/q: Quit
    └─ 9: Show Achievements
    ↓
Update pet stats
    ↓
Check achievements
    ↓
Check death condition
    ↓
Render + Loop
```

### 3. Save/Load
```
SAVE:
Pet State → File (tama_save.txt)
├─ Name, Stats, Age, Alive status
├─ Logs (timestamp|action|detail)
└─ File written

LOAD:
File (tama_save.txt) → Pet State
├─ Read name, stats, age, alive status
├─ Parse logs
└─ Restore state
```

---

## Key Functions Reference

### Pet Actions
- `feedPet(Tamagotchi& t, Statistics* stats)` - Reduce hunger
- `playWithPet(Tamagotchi& t, Statistics* stats)` - Increase happiness
- `sleepPet(Tamagotchi& t)` - Restore energy
- `healPet(Tamagotchi& t, Statistics* stats)` - Increase health
- `passTime(Tamagotchi& t, Statistics* stats)` - Age pet, trigger events

### Achievement System
- `PetStatistics::initializeAchievements(Statistics& stats)` - Setup 5 achievements
- `PetStatistics::updateAchievement(Statistics&, string, int)` - Update by name
- `PetStatistics::updateAchievement(Statistics&, int, int)` - Update by index
- `PetStatistics::countUnlocked(Statistics&)` - How many unlocked?
- `PetStatistics::findAchievementByName(Statistics&, string)` - Find achievement

### Utility
- `clamp<T>(T value, T min, T max)` - Keep value in range
- `currentTime()` - Get current time as string
- `addLog(Tamagotchi&, string, string)` - Add activity log

### File I/O
- `savePet(Tamagotchi&, filename)` - Save to file
- `loadPet(Tamagotchi&, filename)` - Load from file
- `saveFileExists(filename)` - Check if save exists

---

## Example Gameplay Session

```
1. Start game, name pet "Fluffy"
2. Feed Fluffy (1) → "Nom nom! 😋" logged
   - Hunger: 50 → 20
   - Achievement: First Meal (1/5)
   
3. Play with Fluffy (2) → "Wheee! 🎮" logged
   - Happiness: 70 → 95
   - Energy: 80 → 60
   - Achievement: Playtime Master (1/10)
   
4. Pass Time (5) → Age increases
   - Day 1 passed
   - 60% chance: "Made a Friend!" event
   - Happiness +30, Energy -10
   - Check achievements
   
5. After 20 days of care:
   - Survivor achievement unlocked ✓
   - View achievements (9)
   - See progress on other achievements
   
6. Save game (6) → State written to "tama_save.txt"
   - Can load later with (7)
```

---

## Tips for Understanding the Code

1. **Start with `main()`** - Understand program flow
2. **Follow a keystroke** - Trace what happens when you press "1"
3. **Read the structs first** - Understand data organization
4. **Study the namespace** - See how achievements work
5. **Explore lambdas** - Understand callbacks and sorting
6. **Check color constants** - See UI design choices

---

## Troubleshooting

| Issue | Solution |
|-------|----------|
| Pet dies too quickly | Increase `feedPet()` hunger reduction or decrease `passTime()` hunger increase |
| Achievements don't unlock | Check `PetStatistics::updateAchievement()` is called after each action |
| Save file not loading | Verify "tama_save.txt" exists and has correct format |
| Color looks wrong | Check FTXUI terminal compatibility |
| Lambda errors | Ensure `[&]` capture includes needed variables |

---

**Happy Pet Caring! 🐾**

