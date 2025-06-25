# Fruit Ninja Clone

A C++ and OpenGL-based clone of the classic Fruit Ninja game. Slice fruits in real-time with fluid animations and juicy effects.

## Demo and Insights

For an in-depth look at the game's architecture, features, and development insights, visit the full documentation. A short demo video is also available on the site:

[https://dicespinner.github.io/GWebsite/FruitNinja/](https://dicespinner.github.io/GWebsite/FruitNinja/)

## Features

### Gameplay
- Singleplayer classic mode: Fruits will pop up randomly and the player must slice all of them.
- Slicing a fruit gives a point; the goal is to achieve the highest score possible.
- If three fruits are missed or the player slices a bomb, the game ends.

### Technical
- C++20 with CMake-based build system
- Windows-only (uses Winsock) with vcpkg dependency management

## Under Development

The classic singleplayer mode is fully functional and playable. A multiplayer mode is currently under development and planned for future releases.
The documentation for custom network protocol used can be found [here](https://github.com/DiceSpinner/FruitNinja/blob/main/Common/doc/lite_conn_protocol_spec.md).

Additional gameplay features such as combo scoring and critical strikes are also being worked on to enhance scoring depth and player engagement.

## Building the Project

### Prerequisites

- [Visual Studio](https://visualstudio.microsoft.com/) with the C++ development workload installed
- [vcpkg](https://github.com/microsoft/vcpkg)
- [CMake 3.21+](https://cmake.org/) with preset support

### Quick Start

1. **Clone the repository** and open the folder in Visual Studio.
2. **Create a `CMakeUserPresets.json`** file in the project root with the following content, replace the placeholder with the path to `vcpkg`:

```json
{
  "version": 3,
  "configurePresets": [
    {
      "name": "x64-Debug",
      "inherits": "x64-debug",
      "environment": {
        "VCPKG_ROOT": "<path to vcpkg>"
      }
    },
    {
      "name": "x64-Release",
      "inherits": "x64-release",
      "environment": {
        "VCPKG_ROOT": "<path to vcpkg>"
      }
    }
  ]
}
```

3. **Select a target executable** (e.g., Client or Server) in Visual Studio.
4. **Build the project** using the selected configuration.

## Controls

- Hold left click and move mouse to slice fruits
- Press ESC to close the application

### Debug Controls

- R: Unlock/Lock Camera
- F: Pause/Unpause time

## Repository Structure

The repository is organized into several core components:

- **Client/** — Contains the game client logic, rendering, and input handling. The singleplayer mode is complete and fully playable.
- **Server/** — Manages game state and networking for multiplayer sessions (in development).
- **Common/** — Shared utilities, data structures, and logic between client and server.
- **Tests/** — Unit tests written with Catch2.
- **extern/Catch2/** — Included Catch2 framework for testing.
- **images/, models/, shaders/, sounds/, fonts/** — Resource directories used by the game.
