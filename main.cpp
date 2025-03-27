#include "raylib.h"
#include <vector>
#include "raymath.h"

using namespace std;

#define G 400
#define PLAYER_JUMP_SPD 350.0f
#define PLAYER_HOR_SPD 200.0f
#define PLAYER_SCALE 1.0f
#define ATTACK_DURATION 0.5f
#define ATTACK_COOLDOWN 0.5f
#define ATTACK_RANGE 60.0f

// Global variables for hit effect
bool isHitEffectActive = false;
float hitEffectTimer = 0.0f;

struct Player {
    Vector2 position;
    float velocityY;
    bool canJump;
    Texture2D texture;
    Rectangle frameRec;
    int currentFrame;
    int framesCounter;
    bool facingRight;
    bool isAttacking;
    float attackTimer;
    float cooldownTimer;
    int health;
    bool isHit; // For hit animation
    float hitTimer; // For hit animation
    float attackRadius; // Radius of the attack area
};

struct Platform {
    Rectangle rect;
};

struct Enemy {
    Vector2 position;
    bool alive;
    Texture2D texture;
    float speed;
    float direction; // 1 for right, -1 for left
    bool facingRight; // Track facing direction for rendering
    Platform* platform; // Pointer to the platform the enemy is on (if any)
};

void UpdatePlayer(Player &player, vector<Platform> &platforms, float delta) {
    bool moving = false;

    if (IsKeyDown(KEY_LEFT)) {
        player.position.x -= PLAYER_HOR_SPD * delta;
        moving = true;
        player.facingRight = false;
    }
    if (IsKeyDown(KEY_RIGHT)) {
        player.position.x += PLAYER_HOR_SPD * delta;
        moving = true;
        player.facingRight = true;
    }

    if (IsKeyPressed(KEY_SPACE) && player.canJump) {
        player.velocityY = -PLAYER_JUMP_SPD;
        player.canJump = false;
    }

    player.velocityY += G * delta;
    player.position.y += player.velocityY * delta;

    // Ground level constraint
    float groundLevel = platforms[0].rect.y; // Ground platform's top surface
    float playerBottom = player.position.y + (player.frameRec.height * PLAYER_SCALE);

    // Ensure player does not go below the ground
    if (playerBottom > groundLevel) {
        player.position.y = groundLevel - (player.frameRec.height * PLAYER_SCALE);
        player.velocityY = 0;
        player.canJump = true;
    }

    // Check collision with all platforms
    for (const auto &platform : platforms) {
        if (player.position.y + (player.frameRec.height * PLAYER_SCALE) >= platform.rect.y &&
            player.position.y + (player.frameRec.height * PLAYER_SCALE) <= platform.rect.y + 5 &&
            player.position.x + (player.frameRec.width * PLAYER_SCALE) > platform.rect.x &&
            player.position.x < platform.rect.x + platform.rect.width) {
            
            // Player lands on the platform
            player.position.y = platform.rect.y - (player.frameRec.height * PLAYER_SCALE);
            player.velocityY = 0;
            player.canJump = true;
        }
    }

    // Prevent player from moving outside the screen horizontally
    if (player.position.x < 0) player.position.x = 0;
    if (player.position.x > GetScreenWidth()) player.position.x = GetScreenWidth();

    // Update player animation
    player.framesCounter++;
    if (player.framesCounter >= 10) {
        player.framesCounter = 0;
        if (moving) {
            player.currentFrame++;
            if (player.currentFrame > 5) player.currentFrame = 0;
        } else {
            player.currentFrame = 0;
        }
        player.frameRec.x = (float)player.currentFrame * (float)player.texture.width / 6;
    }

    // Handle player attack
    if (IsKeyPressed(KEY_X) && player.cooldownTimer <= 0.0f) {
        player.isAttacking = true;
        player.attackTimer = ATTACK_DURATION;
        player.cooldownTimer = ATTACK_COOLDOWN;
    }

    if (player.isAttacking) {
        player.attackTimer -= delta;
        if (player.attackTimer <= 0) {
            player.isAttacking = false;
        }
    }

    if (player.cooldownTimer > 0.0f) {
        player.cooldownTimer -= delta;
    }

    // Handle hit animation
    if (player.isHit) {
        player.hitTimer -= delta;
        if (player.hitTimer <= 0) {
            player.isHit = false;
        }
    }
}

void UpdateEnemies(vector<Enemy> &enemies, Player &player, float delta, int screenWidth, int screenHeight) {
    for (size_t i = 0; i < enemies.size(); i++) {
        Enemy &enemy = enemies[i];
        if (!enemy.alive) continue;

        // Move enemy within platform boundaries (if platform exists)
        if (enemy.platform) {
            // Move enemy within the platform bounds
            enemy.position.x += enemy.speed * enemy.direction * delta;

            // Reverse direction if enemy reaches platform edges
            if (enemy.position.x < enemy.platform->rect.x) {
                enemy.position.x = enemy.platform->rect.x; // Clamp to left edge
                enemy.direction = 1.0f; // Move right
                enemy.facingRight = true;
            } else if (enemy.position.x + (enemy.texture.width * 0.5f) > enemy.platform->rect.x + enemy.platform->rect.width) {
                enemy.position.x = enemy.platform->rect.x + enemy.platform->rect.width - (enemy.texture.width * 0.5f); // Clamp to right edge
                enemy.direction = -1.0f; // Move left
                enemy.facingRight = false;
            }
        } else {
            // Move enemy from left to right (for ground enemies)
            enemy.position.x += enemy.speed * enemy.direction * delta;

            // Prevent enemy from moving off the right side of the screen
            if (enemy.position.x + (enemy.texture.width * 0.5f) > screenWidth) {
                enemy.position.x = screenWidth - (enemy.texture.width * 0.5f); // Clamp to right edge
                enemy.direction = -1.0f; // Move left
                enemy.facingRight = false;
            }

            // Prevent enemy from moving off the left side of the screen
            if (enemy.position.x < 0) {
                enemy.position.x = 0; // Clamp to left edge
                enemy.direction = 1.0f; // Move right
                enemy.facingRight = true;
            }
        }

        // Player attacks enemy
        if (player.isAttacking) {
            Vector2 playerCenter = {player.position.x + (player.frameRec.width * PLAYER_SCALE / 2),
                                   player.position.y + (player.frameRec.height * PLAYER_SCALE / 2)};
            Vector2 enemyCenter = {enemy.position.x + (enemy.texture.width * 0.25f), // Adjusted for scaling
                                   enemy.position.y + (enemy.texture.height * 0.25f)};
            float distance = Vector2Distance(playerCenter, enemyCenter);

            // Check if the enemy is within the attack radius
            if (distance <= player.attackRadius) {
                enemy.alive = false; // Enemy dies
            }
        }

        // Player touches enemy (no attack)
        Rectangle playerRect = {player.position.x, player.position.y, player.frameRec.width * PLAYER_SCALE * 0.5f , player.frameRec.height * PLAYER_SCALE * 0.5f };
        Rectangle enemyRect = {
            enemy.position.x, 
            enemy.position.y, 
            static_cast<float>(enemy.texture.width) / 3, 
            static_cast<float>(enemy.texture.height) / 3
        };

        if (CheckCollisionRecs(playerRect, enemyRect)) {
            if (!player.isAttacking && !player.isHit) {
                player.health -= 1; // Player loses one heart
                isHitEffectActive = true; // Trigger red screen effect
                hitEffectTimer = 0.2f; // Duration of red screen effect
                player.isHit = true; // Trigger hit animation
                player.hitTimer = 0.5f; // Duration of hit animation

                // Push player back
                if (player.position.x < enemy.position.x) {
                    player.position.x -= 50; // Push left
                } else {
                    player.position.x += 50; // Push right
                }
            }
        }
    }
}

void DrawAttackAnimation(Player &player) {
    if (player.isAttacking) {
        // Draw a circle around the player to represent the attack radius
        DrawCircleLines(player.position.x + (player.frameRec.width * PLAYER_SCALE / 2),
                        player.position.y + (player.frameRec.height * PLAYER_SCALE / 2),
                        player.attackRadius, RED);
    }
}

void DrawHitEffect() {
    if (isHitEffectActive) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(RED, 0.5f)); // Semi-transparent red flash
        hitEffectTimer -= GetFrameTime();
        if (hitEffectTimer <= 0) {
            isHitEffectActive = false;
        }
    }
}

int main() {
    // Enable full-screen mode
    SetConfigFlags(FLAG_FULLSCREEN_MODE);
    InitWindow(0, 0, "Platformer Game"); // Use 0, 0 to automatically use the monitor's resolution
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    // Initialize audio device
    InitAudioDevice();

    // Load music files
    Music startMusic = LoadMusicStream("menu.mp3"); // Replace with your start screen music file
    Music oddLevelMusic = LoadMusicStream("level1.mp3"); // Replace with your odd level music file
    Music evenLevelMusic = LoadMusicStream("level2.mp3"); // Replace with your even level music file
    Music gameOverMusic = LoadMusicStream("menu.mp3"); // Replace with your game over music file

    // Set music to loop
    startMusic.looping = true;
    oddLevelMusic.looping = true;
    evenLevelMusic.looping = true;
    gameOverMusic.looping = true;

    Texture2D background = LoadTexture("2dgame.jpg");
    if (background.id == 0) { 
        CloseWindow();
        return -1;
    }
    
    Texture2D startBackground = LoadTexture("background2.png"); // Load start screen background
    if (startBackground.id == 0) {
        CloseWindow();
        return -1;
    }

    // Load the platform texture
    Texture2D platformTexture = LoadTexture("1pl2.png"); // Replace with your platform image

    Player player = {{screenWidth / 2.0f, screenHeight - 100}, 0, false}; // Start at the center of the ground
    player.texture = LoadTexture("scarfy.png");
    player.frameRec = {0.0f, 0.0f, (float)player.texture.width / 6, (float)player.texture.height};
    player.health = 10; // Player starts with 10 hearts
    player.attackRadius = 90.0f; // 6 cm radius (240 pixels)

    Texture2D heartTexture = LoadTexture("heart.png");  
    Texture2D enemyTexture = LoadTexture("enemy.png");  

    vector<Platform> platforms = {
        {{0, screenHeight - 50, screenWidth, 50}}, // Ground platform
        {{0, screenHeight - 200, 150, 20}}, // Platform 1
        {{200, screenHeight - 350, 190, 20}}, // Platform 2
        {{400, screenHeight - 500, 200, 20}}, // Platform 3
        {{200, screenHeight - 600, 200, 20}}, // Platform 4
        {{600, screenHeight - 700, 200, 20}}, // Platform 5
        {{0, screenHeight - 800, 400, 20}}, // Platform 6
        {{500, screenHeight - 900, 250, 20}}, // Platform 7
        {{900, screenHeight - 900, 500, 20}}, // Platform 8
        {{1100, screenHeight - 750, 190, 20}}, // Platform 9
        {{950, screenHeight - 600, 200, 20}}, // Platform 10
        {{1650, screenHeight - 700, 200, 20}}, // Platform 11
        {{1400, screenHeight - 500, 200, 20}}, // Platform 12
        {{1500, screenHeight - 350, 400, 20}}, // Platform 13
        {{1650, screenHeight - 200, 250, 20}}, // Platform 14
    };

    vector<Enemy> enemies;

    // Level system
    int currentLevel = 1;
    bool levelTransition = false;
    float levelTransitionTimer = 0.0f;

    // Function to spawn enemies based on the current level
    auto SpawnEnemies = [&]() {
        enemies.clear();
        int numEnemies = currentLevel * platforms.size(); // Number of enemies = level * platforms

        // Assign enemies to platforms
        for (int i = 0; i < numEnemies; i++) {
            Enemy enemy;
            int platformIndex = i % platforms.size(); // Cycle through platforms
            enemy.position = {static_cast<float>(platforms[platformIndex].rect.x + 50), static_cast<float>(platforms[platformIndex].rect.y - 100)};
            enemy.alive = true;
            enemy.texture = enemyTexture;
            enemy.speed = 50.0f + (currentLevel * 10); // Increase speed with level
            enemy.direction = (i % 2 == 0) ? 1.0f : -1.0f; // Alternate direction
            enemy.facingRight = (enemy.direction == 1);
            enemy.platform = &platforms[platformIndex]; // Assign platform pointer
            enemies.push_back(enemy);
        }
    };

    SetTargetFPS(60);

    enum GameState { START, PLAYING, LEVEL_TRANSITION, GAME_OVER };
    GameState gameState = START;

    // Load pixelated font
    Font pixelFont = LoadFont("font.ttf"); // Replace with your pixelated font file

    // Play start screen music
    PlayMusicStream(startMusic);

    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();

        // Update music streams
        UpdateMusicStream(startMusic);
        UpdateMusicStream(oddLevelMusic);
        UpdateMusicStream(evenLevelMusic);
        UpdateMusicStream(gameOverMusic);

        if (gameState == START) {
            // Draw start screen
            BeginDrawing();
            ClearBackground(RAYWHITE);

            // Draw the start background texture to fit the full screen
            DrawTexturePro(
                startBackground, // Texture to draw
                {0, 0, (float)startBackground.width, (float)startBackground.height}, // Source rectangle (entire texture)
                {0, 0, (float)screenWidth, (float)screenHeight}, // Destination rectangle (full screen)
                {0, 0}, // Origin (top-left corner)
                0.0f, // Rotation
                WHITE // Tint color
            );

            // Draw "Play" button
            Rectangle playButton = {screenWidth / 2 - 100, screenHeight / 2 - 50, 200, 100};
            DrawRectangleRec(playButton, BLUE);
            DrawTextEx(pixelFont, "Play", {screenWidth / 2 - 40, screenHeight / 2 - 20}, 40, 2, WHITE);

            // Draw "Quit" button
            Rectangle quitButton = {screenWidth / 2 - 100, screenHeight / 2 + 70, 200, 100};
            DrawRectangleRec(quitButton, RED);
            DrawTextEx(pixelFont, "Quit", {screenWidth / 2 - 40, screenHeight / 2 + 90}, 40, 2, WHITE);

            // Check for button clicks
            if (CheckCollisionPointRec(GetMousePosition(), playButton)) {
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    gameState = LEVEL_TRANSITION;
                    currentLevel = 1; // Reset to level 1
                    player.health = 10; // Reset player health to 10 hearts
                    player.position = {screenWidth / 2.0f, screenHeight - 100}; // Reset player position to center of ground
                    SpawnEnemies(); // Spawn enemies for level 1
                    levelTransitionTimer = 3.0f; // Show level transition screen for 3 seconds

                    // Stop start music and play odd level music
                    StopMusicStream(startMusic);
                    PlayMusicStream(oddLevelMusic);
                }
            }

            if (CheckCollisionPointRec(GetMousePosition(), quitButton)) {
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    break; // Exit the game loop
                }
            }

            EndDrawing();
        } else if (gameState == LEVEL_TRANSITION) {
            // Draw level transition screen
            BeginDrawing();
            ClearBackground(BLACK); // Set background to black

            DrawTextEx(pixelFont, TextFormat("Level %d", currentLevel), {screenWidth / 2 - 50, screenHeight / 2 - 20}, 40, 2, WHITE);

            levelTransitionTimer -= deltaTime;
            if (levelTransitionTimer <= 0) {
                gameState = PLAYING;
                levelTransition = false; // Reset the transition flag
            }

            EndDrawing();
        } else if (gameState == PLAYING) {
            UpdatePlayer(player, platforms, deltaTime);
            UpdateEnemies(enemies, player, deltaTime, screenWidth, screenHeight);

            // Check if player's health reaches 0
            if (player.health <= 0) {
                gameState = GAME_OVER;
                // Stop current level music and play game over music
                if (currentLevel % 2 == 1) {
                    StopMusicStream(oddLevelMusic);
                } else {
                    StopMusicStream(evenLevelMusic);
                }
                PlayMusicStream(gameOverMusic);
            }

            // Check if all enemies are defeated
            bool allEnemiesDefeated = true;
            for (const auto &enemy : enemies) {
                if (enemy.alive) {
                    allEnemiesDefeated = false;
                    break;
                }
            }

            // Transition to the next level if all enemies are defeated
            if (allEnemiesDefeated && !levelTransition) {
                currentLevel++;
                if (player.health <= 0) {
                    gameState = GAME_OVER; // Game over if player has no health
                } else {
                    levelTransition = true;
                    player.position = {screenWidth / 2.0f, screenHeight - 100}; // Reset player position to center of ground
                    SpawnEnemies(); // Spawn enemies for the next level
                    levelTransitionTimer = 2.0f; // Show level transition screen for 3 seconds
                    gameState = LEVEL_TRANSITION;

                    // Switch music based on level
                    if (currentLevel % 2 == 1) {
                        StopMusicStream(evenLevelMusic);
                        PlayMusicStream(oddLevelMusic);
                    } else {
                        StopMusicStream(oddLevelMusic);
                        PlayMusicStream(evenLevelMusic);
                    }
                }
            }

            // Inside the main game loop where platforms are drawn
BeginDrawing();
ClearBackground(RAYWHITE);

DrawTexturePro(background, {0, 0, (float)background.width, (float)background.height}, {0, 0, (float)screenWidth, (float)screenHeight}, {0, 0}, 0, WHITE);

// Draw the ground platform as a simple rectangle (skip the texture)
DrawRectangleRec(platforms[0].rect, GRAY); // Predefined medium grey color // Use BROWN or any color for the ground

// Draw other platforms with the platform texture
for (size_t i = 1; i < platforms.size(); i++) { // Start from index 1 to skip the ground platform
    const auto &platform = platforms[i];
    DrawTexturePro(
        platformTexture, // Texture to draw
        {0, 0, (float)platformTexture.width, (float)platformTexture.height}, // Source rectangle (entire texture)
        {platform.rect.x, platform.rect.y, platform.rect.width, platform.rect.height}, // Destination rectangle (platform size)
        {0, 0}, // Origin (top-left corner)
        0.0f, // Rotation
        WHITE // Tint color
    );
}

// Draw enemies
for (const auto &enemy : enemies) {
    if (enemy.alive) {
        // Draw enemy with correct facing direction
        DrawTextureEx(enemy.texture, enemy.position, 0.0f, 0.5f, 
                     enemy.facingRight ? WHITE : (Color){255, 255, 255, 255}); // Flip texture if facing left
    }
}

// Draw player
DrawTexturePro(player.texture, {player.frameRec.x, player.frameRec.y, player.facingRight ? player.frameRec.width : -player.frameRec.width, player.frameRec.height}, {player.position.x, player.position.y, player.frameRec.width * PLAYER_SCALE, player.frameRec.height * PLAYER_SCALE}, {0, 0}, 0.0f, WHITE);

DrawAttackAnimation(player); // Draw the attack animation
DrawHitEffect(); // Draw the hit effect

// Draw hearts at the top center of the screen with 1 cm (40 pixels) spacing
float heartSpacing = 10.0f; // 1 cm spacing
float totalHeartsWidth = (10 * heartTexture.width * 0.3f) + (10 * heartSpacing); // Total width of all hearts
float startX = (screenWidth - totalHeartsWidth) / 2; // Starting X position for hearts

for (int i = 0; i < player.health; i++) {
    DrawTextureEx(heartTexture, {startX + i * (heartTexture.width * 0.3f + heartSpacing), 10}, 0.0f, 0.3f, WHITE);
}

// Draw current level text
DrawTextEx(pixelFont, TextFormat("Level: %d", currentLevel), {10, 10}, 20, 2, WHITE);

EndDrawing();
        } else if (gameState == GAME_OVER) {
            // Draw game over screen
            BeginDrawing();
            ClearBackground(BLACK);

            DrawTextEx(pixelFont, "Game Over", {screenWidth / 2 - 100, screenHeight / 2 - 60}, 40, 2, WHITE);

            // Draw "Restart" button
            Rectangle restartButton = {screenWidth / 2 - 100, screenHeight / 2 - 10, 200, 50};
            DrawRectangleRec(restartButton, BLUE);
            DrawTextEx(pixelFont, "Restart", {screenWidth / 2 - 50, screenHeight / 2}, 30, 2, WHITE);

            // Draw "Menu" button
            Rectangle menuButton = {screenWidth / 2 - 100, screenHeight / 2 + 60, 200, 50};
            DrawRectangleRec(menuButton, GREEN);
            DrawTextEx(pixelFont, "Menu", {screenWidth / 2 - 40, screenHeight / 2 + 70}, 30, 2, WHITE);

            // Draw "Quit" button
            Rectangle quitButton = {screenWidth / 2 - 100, screenHeight / 2 + 130, 200, 50};
            DrawRectangleRec(quitButton, RED);
            DrawTextEx(pixelFont, "Quit", {screenWidth / 2 - 40, screenHeight / 2 + 140}, 30, 2, WHITE);

            // Check for button clicks
            if (CheckCollisionPointRec(GetMousePosition(), restartButton)) {
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    gameState = LEVEL_TRANSITION;
                    currentLevel = 1; // Reset to level 1
                    player.health = 10; // Reset player health to 10 hearts
                    player.position = {screenWidth / 2.0f, screenHeight - 100}; // Reset player position to center of ground
                    SpawnEnemies(); // Spawn enemies for level 1
                    levelTransitionTimer = 3.0f; // Show level transition screen for 3 seconds

                    // Stop game over music and play odd level music
                    StopMusicStream(gameOverMusic);
                    PlayMusicStream(oddLevelMusic);
                }
            }

            if (CheckCollisionPointRec(GetMousePosition(), menuButton)) {
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    gameState = START;
                    // Stop game over music and play start music
                    StopMusicStream(gameOverMusic);
                    PlayMusicStream(startMusic);
                }
            }

            if (CheckCollisionPointRec(GetMousePosition(), quitButton)) {
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    break; // Exit the game loop
                }
            }

            EndDrawing();
        }
    }

    // Unload music streams
    UnloadMusicStream(startMusic);
    UnloadMusicStream(oddLevelMusic);
    UnloadMusicStream(evenLevelMusic);
    UnloadMusicStream(gameOverMusic);

    // Close audio device
    CloseAudioDevice();

    CloseWindow();
    return 0;
}