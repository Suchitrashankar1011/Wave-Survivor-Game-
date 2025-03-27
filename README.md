# Wave Survivor Game (Raylib + C++)

## Overview
Wave Survivor is a 2D action game built using the Raylib library in C++. The game features a player character who must survive waves of attacking enemies by dodging, jumping, and attacking them while navigating through different platforms.

## Features
- **Player Controls:** Move left/right, jump, and attack enemies.
- **Enemy Waves:** Enemies appear in waves and attack the player.
- **Health System:** The player starts with 10 hearts; health decreases upon enemy contact.
- **Platforms:** The game includes various platforms for the player and enemies to move on.
- **Attack Mechanic:** A melee attack with a cooldown timer.
- **Animations:** Character and enemy animations, including attack and damage effects.
- **Hit Effect:** A red flash when the player is hit.
- **Background Music:** Different music tracks for various game states.

## Controls
- `LEFT ARROW` - Move left
- `RIGHT ARROW` - Move right
- `SPACE` - Jump
- `X` - Attack

## Dependencies
To build and run the game, you need:
- Raylib (latest version)
- C++ compiler (G++/MinGW or MSVC)
- Visual Studio Code (or any other C++ IDE)

## Installation
1. Install Raylib by following the official [Raylib installation guide](https://www.raylib.com/).
2. Clone this repository:
   ```sh
   git clone https://github.com/your-repo/wave-survivor
   cd wave-survivor
   ```
3. Compile the game using g++:
   ```sh
   g++ -o game main.cpp -Iinclude -Llib -lraylib -lopengl32 -lgdi32 -lwinmm
   ```
   *(Modify paths if necessary depending on your setup.)*
4. Run the executable:
   ```sh
   ./game
   ```

## Assets
- Player sprites
- Enemy sprites
- Background images
- Music files (menu, level 1, level 2, game over)

## Future Improvements
- Adding more enemy types with unique attack patterns.
- Implementing power-ups for the player.
- Enhancing AI behavior for more challenging waves.
- Introducing different levels and progression.

## License
This game is open-source and released under the MIT License.

## Author
[Suchitra Shankar Srivastava] - Created for a Raylib-based C++ project.


