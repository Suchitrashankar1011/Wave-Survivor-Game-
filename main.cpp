#include "raylib.h"
#include <vector>
#include "raymath.h"
#include <algorithm>
#include <stack>

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
    bool isHit;
    float hitTimer;
    float attackRadius;
};

struct Platform {
    Rectangle rect;
};

struct Enemy {
    Vector2 position;
    bool alive;
    Texture2D texture;
    float speed;
    float direction;
    bool facingRight;
    Platform* platform;
};

// Binary search demonstration
bool EnemyExistsAtPosition(const vector<Enemy>& enemies, float xPos) {
    vector<float> enemyPositions;
    for (const auto& enemy : enemies) {
        if (enemy.alive) {
            enemyPositions.push_back(enemy.position.x);
        }
    }
    sort(enemyPositions.begin(), enemyPositions.end());
    return binary_search(enemyPositions.begin(), enemyPositions.end(), xPos);
}

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

    float groundLevel = platforms[0].rect.y;
    float playerBottom = player.position.y + (player.frameRec.height * PLAYER_SCALE);

    if (playerBottom > groundLevel) {
        player.position.y = groundLevel - (player.frameRec.height * PLAYER_SCALE);
        player.velocityY = 0;
        player.canJump = true;
    }

    for (const auto &platform : platforms) {
        if (player.position.y + (player.frameRec.height * PLAYER_SCALE) >= platform.rect.y &&
            player.position.y + (player.frameRec.height * PLAYER_SCALE) <= platform.rect.y + 5 &&
            player.position.x + (player.frameRec.width * PLAYER_SCALE) > platform.rect.x &&
            player.position.x < platform.rect.x + platform.rect.width) {
            
            player.position.y = platform.rect.y - (player.frameRec.height * PLAYER_SCALE);
            player.velocityY = 0;
            player.canJump = true;
        }
    }

    if (player.position.x < 0) player.position.x = 0;
    if (player.position.x > GetScreenWidth()) player.position.x = GetScreenWidth();

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

        if (enemy.platform) {
            enemy.position.x += enemy.speed * enemy.direction * delta;

            if (enemy.position.x < enemy.platform->rect.x) {
                enemy.position.x = enemy.platform->rect.x;
                enemy.direction = 1.0f;
                enemy.facingRight = true;
            } else if (enemy.position.x + (enemy.texture.width * 0.5f) > enemy.platform->rect.x + enemy.platform->rect.width) {
                enemy.position.x = enemy.platform->rect.x + enemy.platform->rect.width - (enemy.texture.width * 0.5f);
                enemy.direction = -1.0f;
                enemy.facingRight = false;
            }
        } else {
            enemy.position.x += enemy.speed * enemy.direction * delta;

            if (enemy.position.x + (enemy.texture.width * 0.5f) > screenWidth) {
                enemy.position.x = screenWidth - (enemy.texture.width * 0.5f);
                enemy.direction = -1.0f;
                enemy.facingRight = false;
            }

            if (enemy.position.x < 0) {
                enemy.position.x = 0;
                enemy.direction = 1.0f;
                enemy.facingRight = true;
            }
        }

        if (player.isAttacking) {
            Vector2 playerCenter = {player.position.x + (player.frameRec.width * PLAYER_SCALE / 2),
                                   player.position.y + (player.frameRec.height * PLAYER_SCALE / 2)};
            Vector2 enemyCenter = {enemy.position.x + (enemy.texture.width * 0.25f),
                                   enemy.position.y + (enemy.texture.height * 0.25f)};
            float distance = Vector2Distance(playerCenter, enemyCenter);

            if (distance <= player.attackRadius) {
                enemy.alive = false;
            }
        }

        Rectangle playerRect = {player.position.x, player.position.y, player.frameRec.width * PLAYER_SCALE * 0.5f , player.frameRec.height * PLAYER_SCALE * 0.5f };
        Rectangle enemyRect = {
            enemy.position.x, 
            enemy.position.y, 
            static_cast<float>(enemy.texture.width) / 3, 
            static_cast<float>(enemy.texture.height) / 3
        };

        if (CheckCollisionRecs(playerRect, enemyRect)) {
            if (!player.isAttacking && !player.isHit) {
                player.health -= 1;
                isHitEffectActive = true;
                hitEffectTimer = 0.2f;
                player.isHit = true;
                player.hitTimer = 0.5f;

                if (player.position.x < enemy.position.x) {
                    player.position.x -= 50;
                } else {
                    player.position.x += 50;
                }
            }
        }
    }
}

void DrawAttackAnimation(Player &player) {
    if (player.isAttacking) {
        DrawCircleLines(player.position.x + (player.frameRec.width * PLAYER_SCALE / 2),
                        player.position.y + (player.frameRec.height * PLAYER_SCALE / 2),
                        player.attackRadius, RED);
    }
}

void DrawHitEffect() {
    if (isHitEffectActive) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(RED, 0.5f));
        hitEffectTimer -= GetFrameTime();
        if (hitEffectTimer <= 0) {
            isHitEffectActive = false;
        }
    }
}

int main() {
    SetConfigFlags(FLAG_FULLSCREEN_MODE);
    InitWindow(0, 0, "Platformer Game");
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    InitAudioDevice();

    Music startMusic = LoadMusicStream("menu.mp3");
    Music oddLevelMusic = LoadMusicStream("level1.mp3");
    Music evenLevelMusic = LoadMusicStream("level2.mp3");
    Music gameOverMusic = LoadMusicStream("menu.mp3");

    startMusic.looping = true;
    oddLevelMusic.looping = true;
    evenLevelMusic.looping = true;
    gameOverMusic.looping = true;

    Texture2D background = LoadTexture("2dgame.jpg");
    if (background.id == 0) { 
        CloseWindow();
        return -1;
    }
    
    Texture2D startBackground = LoadTexture("background2.png");
    if (startBackground.id == 0) {
        CloseWindow();
        return -1;
    }

    Texture2D platformTexture = LoadTexture("1pl2.png");

    Player player = {{screenWidth / 2.0f, screenHeight - 100}, 0, false};
    player.texture = LoadTexture("scarfy.png");
    player.frameRec = {0.0f, 0.0f, (float)player.texture.width / 6, (float)player.texture.height};
    player.health = 10;
    player.attackRadius = 90.0f;

    Texture2D heartTexture = LoadTexture("heart.png");  
    Texture2D enemyTexture = LoadTexture("enemy.png");  

    vector<Platform> platforms = {
        {{0, screenHeight - 50, screenWidth, 50}},
        {{0, screenHeight - 200, 150, 20}},
        {{200, screenHeight - 350, 190, 20}},
        {{400, screenHeight - 500, 200, 20}},
        {{200, screenHeight - 600, 200, 20}},
        {{600, screenHeight - 700, 200, 20}},
        {{0, screenHeight - 800, 400, 20}},
        {{500, screenHeight - 900, 250, 20}},
        {{900, screenHeight - 900, 500, 20}},
        {{1100, screenHeight - 750, 190, 20}},
        {{950, screenHeight - 600, 200, 20}},
        {{1650, screenHeight - 700, 200, 20}},
        {{1400, screenHeight - 500, 200, 20}},
        {{1500, screenHeight - 350, 400, 20}},
        {{1650, screenHeight - 200, 250, 20}},
    };

    vector<Enemy> enemies;

    int currentLevel = 1;
    bool levelTransition = false;
    float levelTransitionTimer = 0.0f;

    auto SpawnEnemies = [&]() {
        enemies.clear();
        int numEnemies = currentLevel * platforms.size();

        for (int i = 0; i < numEnemies; i++) {
            Enemy enemy;
            int platformIndex = i % platforms.size();
            enemy.position = {static_cast<float>(platforms[platformIndex].rect.x + 50), static_cast<float>(platforms[platformIndex].rect.y - 100)};
            enemy.alive = true;
            enemy.texture = enemyTexture;
            enemy.speed = 50.0f + (currentLevel * 10);
            enemy.direction = (i % 2 == 0) ? 1.0f : -1.0f;
            enemy.facingRight = (enemy.direction == 1);
            enemy.platform = &platforms[platformIndex];
            enemies.push_back(enemy);
        }
    };

    SetTargetFPS(60);

    enum GameState { START, PLAYING, LEVEL_TRANSITION, GAME_OVER };
    stack<GameState> gameStateStack;
    gameStateStack.push(START);

    Font pixelFont = LoadFont("font.ttf");

    PlayMusicStream(startMusic);

    while (!WindowShouldClose() && !gameStateStack.empty()) {
        float deltaTime = GetFrameTime();
        GameState currentState = gameStateStack.top();

        UpdateMusicStream(startMusic);
        UpdateMusicStream(oddLevelMusic);
        UpdateMusicStream(evenLevelMusic);
        UpdateMusicStream(gameOverMusic);

        if (currentState == START) {
            BeginDrawing();
            ClearBackground(RAYWHITE);

            DrawTexturePro(
                startBackground,
                {0, 0, (float)startBackground.width, (float)startBackground.height},
                {0, 0, (float)screenWidth, (float)screenHeight},
                {0, 0},
                0.0f,
                WHITE
            );

            Rectangle playButton = {screenWidth / 2 - 100, screenHeight / 2 - 50, 200, 100};
            DrawRectangleRec(playButton, BLUE);
            DrawTextEx(pixelFont, "Play", {screenWidth / 2 - 40, screenHeight / 2 - 20}, 40, 2, WHITE);

            Rectangle quitButton = {screenWidth / 2 - 100, screenHeight / 2 + 70, 200, 100};
            DrawRectangleRec(quitButton, RED);
            DrawTextEx(pixelFont, "Quit", {screenWidth / 2 - 40, screenHeight / 2 + 90}, 40, 2, WHITE);

            if (CheckCollisionPointRec(GetMousePosition(), playButton)) {
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    gameStateStack.push(LEVEL_TRANSITION);
                    currentLevel = 1;
                    player.health = 10;
                    player.position = {screenWidth / 2.0f, screenHeight - 100};
                    SpawnEnemies();
                    levelTransitionTimer = 3.0f;

                    StopMusicStream(startMusic);
                    PlayMusicStream(oddLevelMusic);
                }
            }

            if (CheckCollisionPointRec(GetMousePosition(), quitButton)) {
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    break;
                }
            }

            EndDrawing();
        }
        else if (currentState == LEVEL_TRANSITION) {
            BeginDrawing();
            ClearBackground(BLACK);

            DrawTextEx(pixelFont, TextFormat("Level %d", currentLevel), {screenWidth / 2 - 50, screenHeight / 2 - 20}, 40, 2, WHITE);

            levelTransitionTimer -= deltaTime;
            if (levelTransitionTimer <= 0) {
                gameStateStack.pop();
                gameStateStack.push(PLAYING);
                levelTransition = false;
            }

            EndDrawing();
        }
        else if (currentState == PLAYING) {
            UpdatePlayer(player, platforms, deltaTime);
            UpdateEnemies(enemies, player, deltaTime, screenWidth, screenHeight);

            if (player.health <= 0) {
                gameStateStack.pop();
                gameStateStack.push(GAME_OVER);
                if (currentLevel % 2 == 1) {
                    StopMusicStream(oddLevelMusic);
                } else {
                    StopMusicStream(evenLevelMusic);
                }
                PlayMusicStream(gameOverMusic);
            }

            bool allEnemiesDefeated = true;
            for (const auto &enemy : enemies) {
                if (enemy.alive) {
                    allEnemiesDefeated = false;
                    break;
                }
            }

            if (allEnemiesDefeated && !levelTransition) {
                currentLevel++;
                if (player.health <= 0) {
                    gameStateStack.pop();
                    gameStateStack.push(GAME_OVER);
                } else {
                    levelTransition = true;
                    player.position = {screenWidth / 2.0f, screenHeight - 100};
                    SpawnEnemies();
                    levelTransitionTimer = 2.0f;
                    gameStateStack.pop();
                    gameStateStack.push(LEVEL_TRANSITION);

                    if (currentLevel % 2 == 1) {
                        StopMusicStream(evenLevelMusic);
                        PlayMusicStream(oddLevelMusic);
                    } else {
                        StopMusicStream(oddLevelMusic);
                        PlayMusicStream(evenLevelMusic);
                    }
                }
            }

            BeginDrawing();
            ClearBackground(RAYWHITE);

            DrawTexturePro(background, {0, 0, (float)background.width, (float)background.height}, {0, 0, (float)screenWidth, (float)screenHeight}, {0, 0}, 0, WHITE);

            DrawRectangleRec(platforms[0].rect, GRAY);

            for (size_t i = 1; i < platforms.size(); i++) {
                const auto &platform = platforms[i];
                DrawTexturePro(
                    platformTexture,
                    {0, 0, (float)platformTexture.width, (float)platformTexture.height},
                    {platform.rect.x, platform.rect.y, platform.rect.width, platform.rect.height},
                    {0, 0},
                    0.0f,
                    WHITE
                );
            }

            for (const auto &enemy : enemies) {
                if (enemy.alive) {
                    DrawTextureEx(enemy.texture, enemy.position, 0.0f, 0.5f, 
                                enemy.facingRight ? WHITE : (Color){255, 255, 255, 255});
                }
            }

            DrawTexturePro(player.texture, {player.frameRec.x, player.frameRec.y, player.facingRight ? player.frameRec.width : -player.frameRec.width, player.frameRec.height}, {player.position.x, player.position.y, player.frameRec.width * PLAYER_SCALE, player.frameRec.height * PLAYER_SCALE}, {0, 0}, 0.0f, WHITE);

            DrawAttackAnimation(player);
            DrawHitEffect();

            float heartSpacing = 10.0f;
            float totalHeartsWidth = (10 * heartTexture.width * 0.3f) + (10 * heartSpacing);
            float startX = (screenWidth - totalHeartsWidth) / 2;

            for (int i = 0; i < player.health; i++) {
                DrawTextureEx(heartTexture, {startX + i * (heartTexture.width * 0.3f + heartSpacing), 10}, 0.0f, 0.3f, WHITE);
            }

            DrawTextEx(pixelFont, TextFormat("Level: %d", currentLevel), {10, 10}, 20, 2, WHITE);

            EndDrawing();
        }
        else if (currentState == GAME_OVER) {
            BeginDrawing();
            ClearBackground(BLACK);

            // Game Over Title
            DrawTextEx(pixelFont, "GAME OVER", {screenWidth/2 - MeasureTextEx(pixelFont, "GAME OVER", 50, 2).x/2, 100}, 50, 2, RED);
            
            
            
            // Game Buttons
            Rectangle restartButton = {screenWidth/2 - 100, 450, 200, 50};
            DrawRectangleRec(restartButton, BLUE);
            DrawTextEx(pixelFont, "Restart", {restartButton.x + restartButton.width/2 - MeasureTextEx(pixelFont, "Restart", 30, 2).x/2, 
                                            restartButton.y + restartButton.height/2 - 15}, 30, 2, WHITE);

            Rectangle menuButton = {screenWidth/2 - 100, 520, 200, 50};
            DrawRectangleRec(menuButton, GREEN);
            DrawTextEx(pixelFont, "Main Menu", {menuButton.x + menuButton.width/2 - MeasureTextEx(pixelFont, "Main Menu", 30, 2).x/2, 
                                              menuButton.y + menuButton.height/2 - 15}, 30, 2, WHITE);

            Rectangle quitButton = {screenWidth/2 - 100, 590, 200, 50};
            DrawRectangleRec(quitButton, RED);
            DrawTextEx(pixelFont, "Quit", {quitButton.x + quitButton.width/2 - MeasureTextEx(pixelFont, "Quit", 30, 2).x/2, 
                                         quitButton.y + quitButton.height/2 - 15}, 30, 2, WHITE);

            // Button Interactions
            if (CheckCollisionPointRec(GetMousePosition(), restartButton)) {
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    gameStateStack.pop();
                    gameStateStack.push(LEVEL_TRANSITION);
                    currentLevel = 1;
                    player.health = 10;
                    player.position = {screenWidth / 2.0f, screenHeight - 100};
                    SpawnEnemies();
                    levelTransitionTimer = 3.0f;

                    StopMusicStream(gameOverMusic);
                    PlayMusicStream(oddLevelMusic);
                }
            }

            if (CheckCollisionPointRec(GetMousePosition(), menuButton)) {
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    while (!gameStateStack.empty()) gameStateStack.pop();
                    gameStateStack.push(START);
                    StopMusicStream(gameOverMusic);
                    PlayMusicStream(startMusic);
                }
            }

            if (CheckCollisionPointRec(GetMousePosition(), quitButton)) {
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                    break;
                }
            }

            EndDrawing();
        }
    }

    UnloadMusicStream(startMusic);
    UnloadMusicStream(oddLevelMusic);
    UnloadMusicStream(evenLevelMusic);
    UnloadMusicStream(gameOverMusic);

    CloseAudioDevice();
    CloseWindow();
    return 0;
}
