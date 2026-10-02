#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define GAME_TITLE "ECHO GRID"
#define cell_size 30
#define MAZE_SIZE 30
#define SCREEN_OFFSET 110
#define SCREEN_W (MAZE_SIZE * cell_size)
#define SCREEN_H (MAZE_SIZE * cell_size + SCREEN_OFFSET)
#define PX_PIXEL 3

#define DASH_COOLDOWN    3.0f
#define SCAN_COOLDOWN    6.0f
#define SCAN_RADIUS      8
#define SCAN_REVEAL_TIME 2.5f
#define PHASE_COOLDOWN   2.0f
#define ECHO_HISTORY     140
#define ECHO_LAG         8.0f
#define INVULN_TIME      1.5f

typedef enum {
    MENU, MISSION_SELECT, MISSION_BRIEF, PLAYING, MISSION_DEBRIEF,
    GAME_OVER, HOW_TO_PLAY, LEADERBOARD, CREDITS,
} GameState;

GameState gameState = MENU;

int gamelvl = 1;
int score = 0;
int moves = 0;
int player_row = 1, player_col = 1;
int goal_row = 28, goal_col = 28;
const int start_row = 1, start_col = 1;
float timeRemaining = 90.0f;
float levelElapsed = 0;
bool newBestTime = false;
float bestTimes[3] = {0, 0, 0};

int lives = 3;
int maxLives = 3;
float invulnUntil = 0;
float damageFlash = 0;

#define MAX_SCORES 5
int highScores[MAX_SCORES] = {0, 0, 0, 0, 0};
Music backgroundMusic;
bool musicMuted = false;
float musicVolume = 0.35f;
int trail[MAZE_SIZE][MAZE_SIZE];
bool exitGame = false;

Sound moveSound, wallSound, winSound, levelUpSound, gameOverSound;
Sound sillycatSound, introSound, level2Sound, level3Sound;
Sound coinSound, giftSound, collectSound, powerupSound;
Sound chatBlipSound, dashSound, scanSound, paradoxSound, phaseSound;
Sound boulderSound, wormholeSound, sentinelSound, damageSound;

float playerDrawX = 45, playerDrawY = 45;
float lastMoveTime = -10, lastBumpTime = -10;
int lastDirX = 0, lastDirY = 0;
float happyUntil = 0, sadUntil = 0;

float dashCooldown = 0, scanCooldown = 0;
float dashActiveUntil = 0;
int dashDirX = 0, dashDirY = 0;
float scanRevealUntil = 0;
float scanPulseTime = -1;
float scanPulseX = 0, scanPulseY = 0;

int phase = 0;
float phaseCooldown = 0;
float phaseFlipAnim = 0;
int phaseFlipTarget = 0;

typedef struct {
    Vector2 samples[ECHO_HISTORY];
    float sampleTime[ECHO_HISTORY];
    int head, count;
} EchoBuffer;
EchoBuffer echo;
Vector2 echoGhostPos = {0, 0};
bool echoGhostActive = false;
float echoCollideCooldown = 0;
float paradoxFlash = 0;
Color paradoxColor = (Color){255, 60, 80, 255};

#define MAX_PATH 900
int pathR[MAX_PATH], pathC[MAX_PATH];
int pathLen = 0;

#define TRAIL_POINTS 20
Vector2 trailPoints[TRAIL_POINTS];
float litTime[MAZE_SIZE][MAZE_SIZE];

char playerName[16] = "AGENT";
char nameBackup[16] = "AGENT";
bool editingName = false;

float displayScore = 0;
int combo = 0;
float lastCollectTime = -10;
#define COMBO_WINDOW 5.0f

float shakeTime = 0, shakeStrength = 0;
float redFlash = 0, lightFlash = 0;
Color lightFlashColor = WHITE;
float glitchTime = 0, ambientTimer = 0;
Camera2D shakeCam = {0};
float mascotHappy = 0;

float torchLight = 1.0f;
#define TORCH_DECAY 0.055f
typedef struct {
    bool active;
    float row, col;
    int dirRow, dirCol;
    float speed;
    float life;
} Boulder;
Boulder boulders[4];
int pressurePlates[MAZE_SIZE][MAZE_SIZE];
float boulderCooldown = 0;

bool zeroGActive = false;
int zeroGVelX = 0, zeroGVelY = 0;
float zeroGStep = 0;
typedef struct { int r1, c1, r2, c2; } Wormhole;
Wormhole wormholes[3];
int wormholeCount = 0;
float wormholeCooldown = 0;

typedef struct {
    float row, col;
    float timer;
    float speed;
    int routeLen;
    int routeR[8];
    int routeC[8];
    int routeIdx;
    float alertTimer;
    float detectRadius;
} Sentinel;
Sentinel sentinels[3];
int sentinelCount = 0;
int sentinelLockCount = 0;
int sentinelLockR[8];
int sentinelLockC[8];
float sentinelLockTimer = 0;

#define MAX_CHAT_LINES 8
#define MAX_MESSAGE_LEN 96
#define TEXT_SPEED 45.0f

typedef enum { SPEAKER_ORACLE, SPEAKER_PLAYER, SPEAKER_SYSTEM, SPEAKER_ENEMY } SpeakerID;
typedef struct { int speaker; char text[MAX_MESSAGE_LEN]; } ChatLine;
typedef struct {
    ChatLine lines[MAX_CHAT_LINES];
    int lineCount;
    const char* title;
    Color accent;
} MissionChat;

MissionChat briefs[3];
MissionChat debriefs[3];
MissionChat failChat;
int chatIndex = 0;
float chatTyping = 0, chatBlipTimer = 0, chatPanelOpen = 0;

typedef struct {
    Color wallColor, wallHighlight, wallShadow;
    Color pathColor, pathAccent;
    Color wallColorB, wallHighlightB, wallShadowB;
    Color pathColorB, pathAccentB;
    Color startColor, startAccent;
    Color goalColor, goalAccent;
    Color playerColor, playerAccent;
    Color echoColor;
    Color trailColor, trailGlow;
    Color textColor, uiBackground, uiAccent;
    const char* name;
    const char* description;
    Color neonA, neonB, neonC;
    Color bgTop, bgBottom;
    Color fogColor;
} Theme;

Theme themes[3] = {
    {
        .wallColor = {65, 45, 30, 255}, .wallHighlight = {100, 75, 45, 255}, .wallShadow = {35, 20, 12, 255},
        .pathColor = {55, 90, 55, 255}, .pathAccent = {95, 130, 75, 255},
        .wallColorB = {50, 70, 90, 255}, .wallHighlightB = {80, 100, 130, 255}, .wallShadowB = {25, 35, 50, 255},
        .pathColorB = {45, 70, 60, 255}, .pathAccentB = {80, 120, 100, 255},
        .startColor = {30, 60, 35, 255}, .startAccent = {60, 200, 110, 255},
        .goalColor = {255, 200, 60, 255}, .goalAccent = {200, 130, 40, 255},
        .playerColor = {200, 150, 90, 255}, .playerAccent = {120, 80, 40, 255},
        .echoColor = {100, 240, 160, 255},
        .trailColor = {170, 240, 180, 160}, .trailGlow = {90, 200, 130, 120},
        .textColor = {225, 240, 220, 255}, .uiBackground = {25, 40, 25, 255}, .uiAccent = {140, 220, 130, 255},
        .name = "THE OVERGROWN CANOPY", .description = "Torchlight fades. Boulders roll.",
        .neonA = {80, 220, 130, 255}, .neonB = {220, 60, 60, 255}, .neonC = {255, 210, 90, 255},
        .bgTop = {12, 30, 18, 255}, .bgBottom = {35, 60, 40, 255}, .fogColor = {30, 50, 35, 255},
    },
    {
        .wallColor = {45, 45, 75, 255}, .wallHighlight = {80, 80, 120, 255}, .wallShadow = {20, 20, 40, 255},
        .pathColor = {25, 30, 60, 255}, .pathAccent = {90, 110, 180, 255},
        .wallColorB = {75, 40, 90, 255}, .wallHighlightB = {120, 70, 150, 255}, .wallShadowB = {35, 15, 45, 255},
        .pathColorB = {35, 25, 75, 255}, .pathAccentB = {120, 90, 200, 255},
        .startColor = {30, 40, 80, 255}, .startAccent = {80, 220, 255, 255},
        .goalColor = {255, 200, 80, 255}, .goalAccent = {255, 120, 220, 255},
        .playerColor = {235, 240, 250, 255}, .playerAccent = {80, 150, 220, 255},
        .echoColor = {255, 120, 240, 255},
        .trailColor = {150, 220, 255, 170}, .trailGlow = {220, 120, 255, 120},
        .textColor = {220, 230, 255, 255}, .uiBackground = {18, 18, 40, 255}, .uiAccent = {140, 200, 255, 255},
        .name = "UNKNOWN ALIEN WORLD", .description = "Zero-G drift. Wormholes active.",
        .neonA = {100, 220, 255, 255}, .neonB = {255, 100, 220, 255}, .neonC = {255, 255, 255, 255},
        .bgTop = {5, 5, 20, 255}, .bgBottom = {40, 20, 70, 255}, .fogColor = {30, 20, 60, 255},
    },
    {
        .wallColor = {12, 8, 22, 255}, .wallHighlight = {40, 20, 65, 255}, .wallShadow = {5, 2, 12, 255},
        .pathColor = {18, 14, 30, 255}, .pathAccent = {60, 40, 100, 255},
        .wallColorB = {30, 5, 15, 255}, .wallHighlightB = {80, 20, 60, 255}, .wallShadowB = {10, 2, 8, 255},
        .pathColorB = {30, 12, 25, 255}, .pathAccentB = {90, 30, 80, 255},
        .startColor = {8, 30, 24, 255}, .startAccent = {0, 255, 170, 255},
        .goalColor = {0, 240, 255, 255}, .goalAccent = {255, 0, 180, 255},
        .playerColor = {30, 40, 70, 255}, .playerAccent = {0, 220, 255, 255},
        .echoColor = {255, 100, 200, 255},
        .trailColor = {0, 255, 220, 160}, .trailGlow = {255, 0, 180, 120},
        .textColor = {210, 240, 255, 255}, .uiBackground = {8, 2, 18, 255}, .uiAccent = {0, 255, 220, 255},
        .name = "MAINFRAME INFILTRATION", .description = "Sentinels patrol. Phase at will.",
        .neonA = {0, 255, 220, 255}, .neonB = {255, 0, 180, 255}, .neonC = {255, 240, 0, 255},
        .bgTop = {6, 0, 12, 255}, .bgBottom = {20, 4, 40, 255}, .fogColor = {15, 5, 30, 255},
    }
};

int maze_A[MAZE_SIZE][MAZE_SIZE] =
{
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,1,1,1,1,1,1,1,1,1,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1},
    {1,0,0,0,0,0,0,0,0,0,1,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1},
    {1,0,1,1,1,1,1,1,1,0,1,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,1,0,1,1},
    {1,0,1,0,0,0,0,0,1,0,1,0,1,0,1,0,1,1,1,1,1,1,1,1,1,0,1,0,1,1},
    {1,0,1,0,1,1,1,0,1,0,1,0,0,0,1,0,1,0,0,0,0,0,0,0,1,0,1,0,1,1},
    {1,0,1,0,1,0,0,0,1,0,1,0,1,0,1,0,1,0,1,1,1,1,1,0,1,0,1,0,1,1},
    {1,0,1,0,1,0,1,1,1,0,1,0,1,0,1,0,1,0,1,0,0,0,1,0,1,0,1,0,1,1},
    {1,0,1,0,1,0,1,0,0,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1},
    {1,0,1,0,1,0,1,0,1,1,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1},
    {1,0,1,0,1,0,1,0,0,0,0,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1},
    {1,0,1,0,1,0,1,1,1,1,1,1,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1},
    {1,0,1,0,1,0,0,0,0,0,0,0,0,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1},
    {1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1},
    {1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,0,1,0,1,0,1,0,1,0,1,0,1,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,1,0,1,0,1,0,1,1},
    {1,0,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,1,0,1,0,1,0,1,1},
    {1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,1,0,1,0,1,0,1,1},
    {1,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,0,1,0,1,0,1,0,1,1,1,1},
    {1,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1},
    {1,0,1,0,1,0,1,1,1,1,1,1,1,1,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1},
    {1,0,1,0,1,0,1,0,0,0,0,0,0,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1},
    {1,0,1,1,1,0,1,0,1,1,1,1,1,0,1,0,1,0,1,0,1,0,1,0,1,0,0,0,1,1},
    {1,0,1,0,1,0,1,0,1,0,0,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1},
    {1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1},
    {1,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1},
    {1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,0,1,0,1,0,1,0,1,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};
int maze_B[MAZE_SIZE][MAZE_SIZE] =
{
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1},
    {1,0,0,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,1},
    {1,0,1,1,1,1,1,1,1,0,1,0,1,0,1,1,1,1,1,1,1,1,1,1,1,0,1,0,1,1},
    {1,0,1,0,0,0,0,0,1,0,1,0,1,0,1,0,0,0,0,0,0,0,0,0,1,0,1,0,1,1},
    {1,0,1,0,1,1,1,0,1,0,1,0,0,0,1,0,1,1,1,1,1,1,1,0,1,0,1,0,1,1},
    {1,0,1,0,1,0,0,0,1,0,1,0,1,0,1,0,1,0,1,1,1,1,1,0,1,0,1,0,1,1},
    {1,0,1,0,1,0,1,1,1,0,1,0,1,0,1,0,1,0,1,0,0,0,1,0,1,0,1,0,1,1},
    {1,0,1,0,1,0,1,0,0,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1},
    {1,0,1,0,1,0,1,0,1,1,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1},
    {1,0,1,0,1,0,1,0,0,0,0,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1},
    {1,0,1,0,1,0,1,1,1,1,1,1,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1},
    {1,0,1,0,1,0,0,0,0,0,0,0,0,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1},
    {1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1},
    {1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,0,1,0,1,0,1,0,1,0,1,0,1,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,1,0,1,0,1,0,1,1},
    {1,0,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,1,0,1,0,1,0,1,1},
    {1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,1,0,1,0,1,0,1,1},
    {1,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,0,1,0,1,0,1,0,1,1,1,1},
    {1,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1},
    {1,0,1,0,1,0,1,1,1,1,1,1,1,1,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1},
    {1,0,1,0,1,0,1,0,0,0,0,0,0,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1},
    {1,0,1,1,1,0,1,0,1,1,1,1,1,0,1,0,1,0,1,0,1,0,1,0,1,0,0,0,1,1},
    {1,0,1,0,1,0,1,0,1,0,0,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1},
    {1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1},
    {1,0,1,0,0,0,1,0,0,0,1,0,0,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1},
    {1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,0,1,0,1,0,1,0,1,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
};

int readmazeCurrent(int r, int c) { return phase == 0 ? maze_A[r][c] : maze_B[r][c]; }
int IsWallAt(int r, int c) {
    if (r < 0 || r >= MAZE_SIZE || c < 0 || c >= MAZE_SIZE) return 1;
    return readmazeCurrent(r, c) == 1;
}
int Pathwalk(int r, int c) { return !IsWallAt(r, c); }
unsigned int CellHash(int a, int b) {
    unsigned int h = (unsigned int)a * 73856093u ^ (unsigned int)b * 19349663u;
    h ^= h >> 13; h *= 0x5bd1e995u; h ^= h >> 15;
    return h;
}
float RandF(float mn, float mx) { return mn + (mx - mn) * (GetRandomValue(0, 10000) / 10000.0f); }
float Clamp01(float v) { return v < 0 ? 0 : (v > 1 ? 1 : v); }
float WrapHue(float h) { h = fmodf(h, 360.0f); if (h < 0) h += 360.0f; return h; }
Color Rainbow(float h, float s, float a) { return Fade(ColorFromHSV(WrapHue(h), s, 1.0f), a); }
Color MixColor(Color a, Color b, float t) {
    t = Clamp01(t); Color c;
    c.r = (unsigned char)(a.r + (b.r - a.r) * t);
    c.g = (unsigned char)(a.g + (b.g - a.g) * t);
    c.b = (unsigned char)(a.b + (b.b - a.b) * t);
    c.a = (unsigned char)(a.a + (b.a - a.a) * t);
    return c;
}
int SnapX(float x) { return (int)(x / PX_PIXEL) * PX_PIXEL; }
int SnapY(float y) { return (int)(y / PX_PIXEL) * PX_PIXEL; }
void DrawTri(Vector2 a, Vector2 b, Vector2 c, Color col) {
    float cr = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    if (cr > 0) DrawTriangle(a, c, b, col); else DrawTriangle(a, b, c, col);
}
void DrawStarPoly(float cx, float cy, int points, float outerR, float innerR,
                  float rotDeg, float sx, float sy, Color col) {
    Vector2 center = {cx, cy};
    float step = PI / points;
    float rot = rotDeg * DEG2RAD - PI / 2;
    for (int i = 0; i < points * 2; i++) {
        float r1 = (i % 2 == 0) ? outerR : innerR;
        float r2 = (i % 2 == 0) ? innerR : outerR;
        float a1 = rot + i * step, a2 = rot + (i + 1) * step;
        Vector2 p1 = {cx + cosf(a1) * r1 * sx, cy + sinf(a1) * r1 * sy};
        Vector2 p2 = {cx + cosf(a2) * r2 * sx, cy + sinf(a2) * r2 * sy};
        DrawTri(center, p1, p2, col);
    }
}
void DrawSparkle(float x, float y, float sz, float rot, Color col) {
    DrawStarPoly(x, y, 4, sz, sz * 0.28f, rot, 1.0f, 1.0f, col);
}
void DrawCircleGlow(float x, float y, float radius, Color inner, Color outer) {
#if RAYLIB_VERSION_MAJOR >= 6
    DrawCircleGradient((Vector2){x, y}, radius, inner, outer);
#else
    DrawCircleGradient((int)x, (int)y, radius, inner, outer);
#endif
}
void DrawGlow(float x, float y, float r, Color col, float a) {
    DrawCircleGlow(x, y, r, Fade(col, a), Fade(col, 0.0f));
}
void DrawTextGlow(const char* text, int x, int y, int size, Color col, Color glow) {
    Color g = Fade(glow, 0.35f);
    DrawText(text, x - 1, y, size, g);
    DrawText(text, x + 1, y, size, g);
    DrawText(text, x, y - 1, size, g);
    DrawText(text, x, y + 1, size, g);
    DrawText(text, x, y, size, col);
}
void DrawTextCentered(const char* t, int y, int fs, Color c) {
    int w = MeasureText(t, fs);
    DrawText(t, (SCREEN_W - w) / 2, y, fs, c);
}
void DrawTextCenteredGlow(const char* t, int y, int fs, Color c, Color g) {
    int w = MeasureText(t, fs);
    DrawTextGlow(t, (SCREEN_W - w) / 2, y, fs, c, g);
}
void DrawTextInBox(const char* t, int x, int w, int y, int size, Color c) {
    DrawText(t, x + (w - MeasureText(t, size)) / 2, y, size, c);
}
void PixelBlock(int x, int y, int w, int h, Color col) {
    DrawRectangle(SnapX((float)x), SnapY((float)y),
                  (int)(w / PX_PIXEL) * PX_PIXEL,
                  (int)(h / PX_PIXEL) * PX_PIXEL, col);
}
void DrawScanlines(Color col) {
    for (int y = 0; y < SCREEN_H; y += 3) DrawRectangle(0, y, SCREEN_W, 1, col);
}
void DrawPixelFrame(int x, int y, int w, int h, Color outer, Color inner) {
    DrawRectangle(x - 2, y - 2, w + 4, h + 4, outer);
    DrawRectangle(x, y, w, h, inner);
    DrawRectangle(x, y, w, 2, WHITE);
    DrawRectangle(x, y, 2, h, WHITE);
    DrawRectangle(x, y + h - 2, w, 2, BLACK);
    DrawRectangle(x + w - 2, y, 2, h, BLACK);
}

void ComputeGoalPath() {
    pathLen = 0;
    if (player_row == goal_row && player_col == goal_col) {
        pathR[0] = player_row; pathC[0] = player_col; pathLen = 1;
        return;
    }
    static int visited[MAZE_SIZE][MAZE_SIZE];
    static int parentR[MAZE_SIZE][MAZE_SIZE];
    static int parentC[MAZE_SIZE][MAZE_SIZE];
    static int qr[MAZE_SIZE * MAZE_SIZE], qc[MAZE_SIZE * MAZE_SIZE];
    for (int i = 0; i < MAZE_SIZE; i++)
        for (int j = 0; j < MAZE_SIZE; j++) {
            visited[i][j] = 0;
            parentR[i][j] = -1;
            parentC[i][j] = -1;
        }
    int head = 0, tail = 0;
    qr[tail] = player_row; qc[tail] = player_col; tail++;
    visited[player_row][player_col] = 1;
    int dr[4] = {-1, 1, 0, 0};
    int dc[4] = {0, 0, -1, 1};
    int found = 0;
    while (head < tail) {
        int r = qr[head], c = qc[head]; head++;
        if (r == goal_row && c == goal_col) { found = 1; break; }
        for (int d = 0; d < 4; d++) {
            int nr = r + dr[d], nc = c + dc[d];
            if (nr < 0 || nr >= MAZE_SIZE || nc < 0 || nc >= MAZE_SIZE) continue;
            if (visited[nr][nc]) continue;
            if (IsWallAt(nr, nc)) continue;
            visited[nr][nc] = 1;
            parentR[nr][nc] = r;
            parentC[nr][nc] = c;
            qr[tail] = nr; qc[tail] = nc; tail++;
        }
    }
    if (!found) return;
    int tr = goal_row, tc = goal_col;
    int tmpR[MAX_PATH], tmpC[MAX_PATH];
    int len = 0;
    while (tr != -1 && len < MAX_PATH) {
        tmpR[len] = tr; tmpC[len] = tc; len++;
        int pr = parentR[tr][tc], pc = parentC[tr][tc];
        tr = pr; tc = pc;
    }
    if (len > MAX_PATH) len = MAX_PATH;
    pathLen = len;
    for (int i = 0; i < len; i++) {
        pathR[i] = tmpR[len - 1 - i];
        pathC[i] = tmpC[len - 1 - i];
    }
}

void StopAllLevelSounds() {
    StopSound(sillycatSound);
    StopSound(level2Sound);
    StopSound(level3Sound);
}
void StopEverything() {
    StopSound(introSound);
    StopAllLevelSounds();
    StopSound(winSound);
    StopSound(gameOverSound);
    PauseMusicStream(backgroundMusic);
}
void SaveLeaderboard() {
    FILE* f = fopen("leaderboard.txt", "w");
    if (f) { for (int i = 0; i < MAX_SCORES; i++) fprintf(f, "%d\n", highScores[i]); fclose(f); }
}
void LoadLeaderboard() {
    FILE* f = fopen("leaderboard.txt", "r");
    if (f) { for (int i = 0; i < MAX_SCORES; i++) if (fscanf(f, "%d", &highScores[i]) != 1) highScores[i] = 0; fclose(f); }
    else { for (int i = 0; i < MAX_SCORES; i++) highScores[i] = 0; }
}
void UpdateHighScore() {
    int lo = 0;
    for (int i = 1; i < MAX_SCORES; i++) if (highScores[i] < highScores[lo]) lo = i;
    if (score > highScores[lo]) {
        highScores[lo] = score;
        for (int i = 0; i < MAX_SCORES - 1; i++)
            for (int j = 0; j < MAX_SCORES - i - 1; j++)
                if (highScores[j] < highScores[j + 1]) {
                    int t = highScores[j]; highScores[j] = highScores[j + 1]; highScores[j + 1] = t;
                }
        SaveLeaderboard();
    }
}
int GetHighScore() { return highScores[0]; }
void SaveExtras() {
    FILE* f = fopen("profile.txt", "w");
    if (f) { fprintf(f, "%s\n", playerName); for (int i = 0; i < 3; i++) fprintf(f, "%.2f\n", bestTimes[i]); fclose(f); }
}
void LoadExtras() {
    FILE* f = fopen("profile.txt", "r");
    if (f) {
        char line[32];
        if (fgets(line, sizeof(line), f) != NULL) {
            line[strcspn(line, "\r\n")] = '\0';
            if (strlen(line) > 0 && strlen(line) <= 12) strcpy(playerName, line);
        }
        for (int i = 0; i < 3; i++) if (fscanf(f, "%f", &bestTimes[i]) != 1) bestTimes[i] = 0;
        fclose(f);
    }
}
void ClearTrail() {
    for (int i = 0; i < MAZE_SIZE; i++) for (int j = 0; j < MAZE_SIZE; j++) trail[i][j] = 0;
}
void MarkTrail(int r, int c) {
    if (r >= 0 && r < MAZE_SIZE && c >= 0 && c < MAZE_SIZE) trail[r][c] = 1;
}

void LoadSounds() {
    moveSound     = LoadSound("assets/move.wav");
    wallSound     = LoadSound("assets/wall.wav");
    winSound      = LoadSound("assets/win.wav");
    levelUpSound  = LoadSound("assets/levelup.wav");
    gameOverSound = LoadSound("assets/gameover.wav");
    sillycatSound = LoadSound("assets/sillycat.wav");
    introSound    = LoadSound("assets/intro.wav");
    level2Sound   = LoadSound("assets/lev2.wav");
    level3Sound   = LoadSound("assets/lev3.wav");
    coinSound     = LoadSound("assets/coin.wav");
    giftSound     = LoadSound("assets/gift.wav");
    collectSound  = LoadSound("assets/collect.wav");
    powerupSound  = LoadSound("assets/powerup.wav");
    chatBlipSound = LoadSound("assets/move.wav");
    dashSound     = LoadSound("assets/levelup.wav");
    scanSound     = LoadSound("assets/powerup.wav");
    paradoxSound  = LoadSound("assets/gameover.wav");
    phaseSound    = LoadSound("assets/levelup.wav");
    boulderSound  = LoadSound("assets/wall.wav");
    wormholeSound = LoadSound("assets/powerup.wav");
    sentinelSound = LoadSound("assets/gameover.wav");
    damageSound   = LoadSound("assets/wall.wav");
}
void UnloadSounds() {
    UnloadSound(moveSound); UnloadSound(wallSound); UnloadSound(winSound);
    UnloadSound(levelUpSound); UnloadSound(gameOverSound); UnloadSound(sillycatSound);
    UnloadSound(introSound); UnloadSound(level2Sound); UnloadSound(level3Sound);
    UnloadSound(coinSound); UnloadSound(giftSound); UnloadSound(collectSound); UnloadSound(powerupSound);
    UnloadSound(chatBlipSound);
    UnloadSound(dashSound); UnloadSound(scanSound); UnloadSound(paradoxSound); UnloadSound(phaseSound);
    UnloadSound(boulderSound); UnloadSound(wormholeSound); UnloadSound(sentinelSound); UnloadSound(damageSound);
}
void LoadMusic() {
    backgroundMusic = LoadMusicStream("assets/background_music.ogg");
    SetMusicVolume(backgroundMusic, musicVolume);
}
void PlayPitched(Sound s, float p) { SetSoundPitch(s, p); PlaySound(s); }
void ToggleMusic() {
    musicMuted = !musicMuted;
    if (musicMuted) { PauseMusicStream(backgroundMusic); StopSound(introSound); StopAllLevelSounds(); }
}
void UpdateGameMusic() {
    UpdateMusicStream(backgroundMusic);
    bool onMenu = (gameState == MENU || gameState == MISSION_SELECT ||
                   gameState == HOW_TO_PLAY || gameState == LEADERBOARD ||
                   gameState == CREDITS);
    bool onChat = (gameState == MISSION_BRIEF || gameState == MISSION_DEBRIEF);
    if (musicMuted) {
        PauseMusicStream(backgroundMusic);
        StopSound(introSound);
        StopAllLevelSounds();
    }
    else if (onMenu || onChat) {
        StopAllLevelSounds();
        if (IsSoundPlaying(introSound)) PauseMusicStream(backgroundMusic);
        else if (!IsMusicStreamPlaying(backgroundMusic)) ResumeMusicStream(backgroundMusic);
    }
    else if (gameState == PLAYING) {
        PauseMusicStream(backgroundMusic);
        if (gamelvl == 1) {
            if (!IsSoundPlaying(sillycatSound)) PlaySound(sillycatSound);
        } else if (gamelvl == 2) {
            if (!IsSoundPlaying(level2Sound)) PlaySound(level2Sound);
        } else {
            if (!IsSoundPlaying(level3Sound)) PlaySound(level3Sound);
        }
    }
    else {
        PauseMusicStream(backgroundMusic);
        StopAllLevelSounds();
    }
}

#define MAX_PARTICLES 320
typedef enum {
    PT_DOT, PT_SPARKLE, PT_CONFETTI, PT_RAIN, PT_BUBBLE, PT_PUFF, PT_COIN,
    PT_DASH, PT_LEAF, PT_FIREFLY, PT_PLANET, PT_SHOOTING_STAR, PT_RUNE, PT_HOLOGRAM,
    PT_GLITCH_BOX, PT_RAIN_SPLASH, PT_SNOW, PT_ECHO
} ParticleType;

typedef struct {
    bool active; int type;
    Vector2 pos, vel;
    float gravity, life, maxLife, size, rotation, spin;
    Color color;
    float swayPhase, swayAmp;
} Particle;
Particle particles[MAX_PARTICLES];
void ClearParticles() { for (int i = 0; i < MAX_PARTICLES; i++) particles[i].active = false; }
int CountParticles() { int n = 0; for (int i = 0; i < MAX_PARTICLES; i++) if (particles[i].active) n++; return n; }
void SpawnParticle(int t, float x, float y, float vx, float vy, float g, float lf, float sz, Color c) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) {
            particles[i] = (Particle){true, t, {x, y}, {vx, vy}, g, lf, lf, sz,
                                     RandF(0, 360), RandF(-360, 360), c, RandF(0, 6.28f), RandF(10, 40)};
            return;
        }
    }
}
void SpawnBurst(int t, float x, float y, int n, float sp, float g, float lf, float sz, Color c, bool rb) {
    for (int i = 0; i < n; i++) {
        float a = RandF(0, 2 * PI);
        float s = RandF(sp * 0.4f, sp);
        Color cc = rb ? ColorFromHSV(RandF(0, 360), 0.7f, 1.0f) : c;
        SpawnParticle(t, x, y, cosf(a) * s, sinf(a) * s, g, RandF(lf * 0.6f, lf), RandF(sz * 0.6f, sz), cc);
    }
}
void UpdateParticles(float dt) {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) continue;
        particles[i].vel.y += particles[i].gravity * dt;
        if (particles[i].type == PT_CONFETTI) {
            particles[i].vel.x *= 0.98f;
            if (particles[i].vel.y > 120) particles[i].vel.y = 120;
        }
        if (particles[i].type == PT_LEAF) {
            particles[i].swayPhase += dt * 2.0f;
            particles[i].vel.x = sinf(particles[i].swayPhase) * particles[i].swayAmp;
        }
        if (particles[i].type == PT_FIREFLY) {
            particles[i].swayPhase += dt * 3.0f;
            particles[i].pos.x += sinf(particles[i].swayPhase) * 20 * dt;
            particles[i].pos.y += cosf(particles[i].swayPhase * 1.3f) * 15 * dt;
        }
        if (particles[i].type == PT_SHOOTING_STAR) {
            particles[i].vel.x *= 0.998f;
            particles[i].vel.y *= 0.998f;
        }
        particles[i].pos.x += particles[i].vel.x * dt;
        particles[i].pos.y += particles[i].vel.y * dt;
        particles[i].rotation += particles[i].spin * dt;
        particles[i].life -= dt;
        if (particles[i].life <= 0) particles[i].active = false;
    }
}
void DrawParticles() {
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (!particles[i].active) continue;
        Particle p = particles[i];
        float k = p.life / p.maxLife;
        int px = SnapX(p.pos.x), py = SnapY(p.pos.y);
        if (p.type == PT_DOT) DrawRectangle(px, py, PX_PIXEL * 2, PX_PIXEL * 2, Fade(p.color, k));
        else if (p.type == PT_SPARKLE) DrawSparkle(px, py, p.size * (0.5f + 0.5f * k), p.rotation, Fade(p.color, k));
        else if (p.type == PT_CONFETTI) {
            Rectangle r = {(float)px, (float)py, p.size, p.size * 0.5f};
            DrawRectanglePro(r, (Vector2){p.size / 2, p.size / 4}, p.rotation, Fade(p.color, fminf(1.0f, k * 2)));
        } else if (p.type == PT_RAIN) DrawRectangle(px, py, PX_PIXEL, PX_PIXEL * 3, Fade(p.color, 0.55f * fminf(1.0f, k * 3)));
        else if (p.type == PT_BUBBLE) {
            DrawCircleLines(px, py, p.size, Fade(p.color, 0.7f * k));
            DrawRectangle(px - 2, py - 2, PX_PIXEL, PX_PIXEL, Fade(WHITE, 0.6f * k));
        } else if (p.type == PT_PUFF) DrawCircleV(p.pos, p.size * (1.8f - k), Fade(p.color, 0.45f * k));
        else if (p.type == PT_COIN) {
            float w = fabsf(cosf(p.rotation * DEG2RAD)) * p.size + 1;
            DrawEllipse(px, py, w, p.size, Fade((Color){255, 200, 40, 255}, k));
        } else if (p.type == PT_DASH) DrawRectangle(px - PX_PIXEL, py - PX_PIXEL, PX_PIXEL * 3, PX_PIXEL * 3, Fade(p.color, k));
        else if (p.type == PT_LEAF) {
            Color lc = Fade(p.color, k);
            int lw = (int)p.size, lh = (int)(p.size * 0.55f);
            DrawRectangle(px - lw/2, py - lh/2, lw, lh, lc);
        } else if (p.type == PT_FIREFLY) {
            float pulse = 0.5f + 0.5f * sinf(p.rotation * DEG2RAD * 2);
            DrawGlow(p.pos.x, p.pos.y, p.size * 2.5f, p.color, 0.5f * k * pulse);
            DrawCircle(px, py, p.size * 0.6f, Fade(p.color, k * (0.5f + 0.5f * pulse)));
        } else if (p.type == PT_PLANET) {
            float alpha = k * 0.45f;
            DrawCircle(px, py, p.size, Fade(p.color, alpha));
            for (int b = -3; b <= 3; b++)
                DrawRectangle(px - (int)p.size, py + b * (int)(p.size * 0.25f), (int)(p.size * 2), 2,
                              Fade(MixColor(p.color, BLACK, 0.4f), alpha * 0.4f));
        } else if (p.type == PT_SHOOTING_STAR) {
            Vector2 tail = {p.pos.x - p.vel.x * 0.12f, p.pos.y - p.vel.y * 0.12f};
            DrawLineEx(p.pos, tail, 2, Fade(p.color, k));
            DrawCircleV(p.pos, 2, Fade(WHITE, k));
        } else if (p.type == PT_RUNE) {
            Color rc = Fade(p.color, k * 0.9f);
            int s = (int)p.size;
            DrawRectangle(px - s, py - s, s*2, 2, rc);
            DrawRectangle(px - s, py + s, s*2, 2, rc);
            DrawRectangle(px - s, py - s, 2, s*2, rc);
            DrawRectangle(px + s, py - s, 2, s*2, rc);
        } else if (p.type == PT_HOLOGRAM) {
            Color hc = Fade(p.color, k * 0.7f);
            int s = (int)p.size;
            DrawRectangleLines(px - s, py - s*0.4f, s*2, s*0.8f, hc);
        } else if (p.type == PT_GLITCH_BOX) {
            DrawRectangle(px, py, p.size, p.size * 0.4f, Fade(p.color, k * 0.7f));
        } else if (p.type == PT_SNOW) {
            DrawCircle(px, py, p.size, Fade(p.color, k * 0.8f));
        } else if (p.type == PT_ECHO) {
            DrawCircleLines(px, py, p.size * (1 + (1 - k) * 0.5f), Fade(p.color, k * 0.7f));
        }
    }
}

#define MAX_FLOAT_TEXTS 20
typedef struct { bool active; char text[32]; Vector2 pos; float life, maxLife; int size; Color color; } FloatText;
FloatText floatTexts[MAX_FLOAT_TEXTS];
void SpawnFloatText(const char* text, float x, float y, int sz, Color c) {
    for (int i = 0; i < MAX_FLOAT_TEXTS; i++) {
        if (!floatTexts[i].active) {
            floatTexts[i] = (FloatText){true, "", {x, y}, 1.2f, 1.2f, sz, c};
            snprintf(floatTexts[i].text, sizeof(floatTexts[i].text), "%s", text);
            return;
        }
    }
}
void UpdateFloatTexts(float dt) {
    for (int i = 0; i < MAX_FLOAT_TEXTS; i++) {
        if (!floatTexts[i].active) continue;
        floatTexts[i].pos.y -= 45 * dt;
        floatTexts[i].life -= dt;
        if (floatTexts[i].life <= 0) floatTexts[i].active = false;
    }
}
void DrawFloatTexts() {
    for (int i = 0; i < MAX_FLOAT_TEXTS; i++) {
        if (!floatTexts[i].active) continue;
        FloatText f = floatTexts[i];
        float k = f.life / f.maxLife;
        int w = MeasureText(f.text, f.size);
        int x = (int)f.pos.x - w / 2;
        if (x < 4) x = 4;
        if (x > SCREEN_W - w - 4) x = SCREEN_W - w - 4;
        DrawText(f.text, x + 2, (int)f.pos.y + 2, f.size, Fade(BLACK, 0.6f * fminf(1, k * 2)));
        DrawText(f.text, x, (int)f.pos.y, f.size, Fade(f.color, fminf(1.0f, k * 2)));
    }
}

typedef enum { EMOJI_LAUGH, EMOJI_FIRE, EMOJI_SKULL, EMOJI_SPARKLE } EmojiType;
void DrawEmoji(int type, float x, float y, float size, float alpha) {
    float s = size / 14.0f;
    float time = (float)GetTime();
    if (type == EMOJI_LAUGH) {
        Color ink = Fade((Color){70, 40, 20, 255}, alpha);
        DrawCircleV((Vector2){x, y}, 14 * s, Fade((Color){255, 205, 60, 255}, alpha));
        DrawLineEx((Vector2){x - 9 * s, y - 2 * s}, (Vector2){x - 5 * s, y - 6 * s}, 2 * s, ink);
        DrawLineEx((Vector2){x - 5 * s, y - 6 * s}, (Vector2){x - 1 * s, y - 2 * s}, 2 * s, ink);
        DrawLineEx((Vector2){x + 1 * s, y - 2 * s}, (Vector2){x + 5 * s, y - 6 * s}, 2 * s, ink);
        DrawLineEx((Vector2){x + 5 * s, y - 6 * s}, (Vector2){x + 9 * s, y - 2 * s}, 2 * s, ink);
        DrawCircleSector((Vector2){x, y + 2 * s}, 8 * s, 0, 180, 12, ink);
    } else if (type == EMOJI_FIRE) {
        float flick = sinf(time * 18) * 1.5f * s;
        Color red = Fade((Color){255, 70, 30, 255}, alpha);
        DrawCircleV((Vector2){x, y + 4 * s}, 10 * s, red);
        DrawTri((Vector2){x - 10 * s, y + 3 * s}, (Vector2){x + flick, y - 18 * s}, (Vector2){x + 10 * s, y + 3 * s}, red);
        DrawCircleV((Vector2){x, y + 6 * s}, 6 * s, Fade((Color){255, 150, 30, 255}, alpha));
    } else if (type == EMOJI_SKULL) {
        Color bone = Fade((Color){235, 235, 240, 255}, alpha);
        Color dark = Fade((Color){40, 40, 50, 255}, alpha);
        DrawCircleV((Vector2){x, y - 2 * s}, 12 * s, bone);
        DrawRectangle((int)(x - 7 * s), (int)(y + 6 * s), (int)(14 * s), (int)(7 * s), bone);
        DrawCircleV((Vector2){x - 5 * s, y - 2 * s}, 3.5f * s, dark);
        DrawCircleV((Vector2){x + 5 * s, y - 2 * s}, 3.5f * s, dark);
    } else {
        DrawSparkle(x - 2 * s, y, 12 * s, time * 60, Fade((Color){255, 225, 80, 255}, alpha));
    }
}
#define MAX_REACTIONS 12
typedef struct { bool active; int type; Vector2 pos; float life, maxLife, wobble; } Reaction;
Reaction reactions[MAX_REACTIONS];
void SpawnReaction(int t, float x, float y) {
    for (int i = 0; i < MAX_REACTIONS; i++) {
        if (!reactions[i].active) {
            reactions[i] = (Reaction){true, t, {x, y}, 1.6f, 1.6f, RandF(0, 6.28f)};
            return;
        }
    }
}
void UpdateReactions(float dt) {
    for (int i = 0; i < MAX_REACTIONS; i++) {
        if (!reactions[i].active) continue;
        reactions[i].pos.y -= 55 * dt;
        reactions[i].life -= dt;
        if (reactions[i].life <= 0) reactions[i].active = false;
    }
}
void DrawReactions() {
    for (int i = 0; i < MAX_REACTIONS; i++) {
        if (!reactions[i].active) continue;
        Reaction r = reactions[i];
        float k = r.life / r.maxLife;
        float x = r.pos.x + sinf(r.wobble + r.life * 5) * 6;
        float size = (k > 0.8f) ? 13 + (k - 0.8f) * 25 : 13;
        DrawEmoji(r.type, x, r.pos.y, size, fminf(1.0f, k * 2));
    }
}

#define MAX_COLLECTIBLES 10
typedef enum { ITEM_COIN, ITEM_GIFT, ITEM_STAR, ITEM_GEM, ITEM_POWERUP, ITEM_PIZZA, ITEM_BOBA, ITEM_ALIEN } ItemType;
typedef struct { int type, row, col; bool collected; float phase; } Collectible;
Collectible items[MAX_COLLECTIBLES];
int itemCount = 0, itemsCollected = 0;

int ItemPoints(int t) {
    if (t == ITEM_COIN) return 50;
    if (t == ITEM_GIFT) return 100;
    if (t == ITEM_STAR) return 25;
    if (t == ITEM_GEM) return 200;
    if (t == ITEM_PIZZA) return 40;
    if (t == ITEM_BOBA) return 30;
    return 0;
}
Color ItemColor(int t) {
    if (t == ITEM_COIN) return (Color){255, 200, 40, 255};
    if (t == ITEM_GIFT) return (Color){255, 90, 160, 255};
    if (t == ITEM_STAR) return (Color){255, 240, 120, 255};
    if (t == ITEM_GEM) return (Color){90, 230, 255, 255};
    if (t == ITEM_POWERUP) return (Color){140, 200, 255, 255};
    if (t == ITEM_PIZZA) return (Color){255, 170, 70, 255};
    if (t == ITEM_BOBA) return (Color){230, 180, 140, 255};
    return (Color){120, 255, 140, 255};
}
int RandomItemType() {
    int r = GetRandomValue(1, 100);
    if (r <= 28) return ITEM_COIN;
    if (r <= 45) return ITEM_STAR;
    if (r <= 57) return ITEM_BOBA;
    if (r <= 68) return ITEM_PIZZA;
    if (r <= 79) return ITEM_GIFT;
    if (r <= 89) return ITEM_POWERUP;
    if (r <= 95) return ITEM_ALIEN;
    return ITEM_GEM;
}

int reachable[MAZE_SIZE][MAZE_SIZE];
void FindReachableCells() {
    int qr[MAZE_SIZE * MAZE_SIZE], qc[MAZE_SIZE * MAZE_SIZE];
    int head = 0, tail = 0;
    int dr[4] = {-1, 1, 0, 0}, dc[4] = {0, 0, -1, 1};
    for (int i = 0; i < MAZE_SIZE; i++) for (int j = 0; j < MAZE_SIZE; j++) reachable[i][j] = 0;
    reachable[start_row][start_col] = 1;
    qr[tail] = start_row; qc[tail] = start_col; tail++;
    while (head < tail) {
        int r = qr[head], c = qc[head]; head++;
        for (int d = 0; d < 4; d++) {
            int nr = r + dr[d], nc = c + dc[d];
            if (!IsWallAt(nr, nc) && !reachable[nr][nc]) {
                reachable[nr][nc] = 1; qr[tail] = nr; qc[tail] = nc; tail++;
            }
        }
    }
}
void PlaceCollectibles() {
    itemCount = 0;
    int wanted = GetRandomValue(6, MAX_COLLECTIBLES);
    int tries = 0;
    while (itemCount < wanted && tries < 5000) {
        tries++;
        int r = GetRandomValue(1, MAZE_SIZE - 2);
        int c = GetRandomValue(1, MAZE_SIZE - 2);
        if (!reachable[r][c]) continue;
        if (abs(r - start_row) + abs(c - start_col) < 4) continue;
        if (r == goal_row && c == goal_col) continue;
        bool tooClose = false;
        for (int k = 0; k < itemCount; k++)
            if (abs(items[k].row - r) + abs(items[k].col - c) < 4) tooClose = true;
        if (tooClose) continue;
        items[itemCount].type = RandomItemType();
        items[itemCount].row = r;
        items[itemCount].col = c;
        items[itemCount].collected = false;
        items[itemCount].phase = RandF(0, 2 * PI);
        itemCount++;
    }
}
void DrawItemIcon(int type, float x, float y, float s, float time, int theme) {
    Color ink = {60, 30, 30, 255};
    if (type == ITEM_COIN) {
        if (theme == 0) {
            DrawEllipse((int)x, (int)y, 9 * fabsf(s) + 1, 6, (Color){255, 220, 80, 255});
            DrawEllipse((int)x, (int)y, 7 * fabsf(s) + 1, 4, (Color){255, 240, 140, 255});
        } else if (theme == 1) {
            DrawStarPoly(x, y, 4, 11 * fabsf(s), 3, time * 90, 1, 1, (Color){255, 240, 200, 255});
            DrawStarPoly(x, y, 4, 8 * fabsf(s), 2, time * 90, 1, 1, WHITE);
        } else {
            DrawRectangle((int)(x - 9), (int)(y - 9), 18, 18, (Color){40, 20, 60, 255});
            DrawRectangle((int)(x - 7), (int)(y - 7), 14, 14, (Color){0, 220, 200, 255});
            DrawRectangle((int)(x - 4), (int)(y - 4), 8, 8, (Color){10, 40, 60, 255});
        }
    } else if (type == ITEM_GIFT) {
        if (theme == 0) {
            DrawRectangle((int)(x - 8), (int)(y - 4), 16, 12, (Color){150, 90, 50, 255});
            DrawRectangle((int)(x - 10), (int)(y - 7), 20, 4, (Color){110, 60, 30, 255});
        } else if (theme == 1) {
            DrawCircle((int)x, (int)y, 11, Fade((Color){150, 100, 255, 255}, 0.8f));
            DrawCircle((int)x, (int)y, 8, (Color){220, 180, 255, 255});
            DrawCircle((int)x, (int)y, 4, WHITE);
        } else {
            DrawRectangle((int)(x - 9), (int)(y - 6), 18, 12, (Color){30, 20, 40, 255});
            DrawRectangle((int)(x - 7), (int)(y - 4), 14, 8, (Color){255, 30, 200, 255});
        }
    } else if (type == ITEM_STAR) {
        Color c1 = (theme == 0) ? (Color){220, 40, 40, 255} : (theme == 1) ? (Color){80, 220, 255, 255} : (Color){255, 30, 180, 255};
        Color c2 = (theme == 0) ? (Color){255, 120, 120, 255} : (theme == 1) ? (Color){200, 250, 255, 255} : (Color){255, 150, 220, 255};
        DrawTri((Vector2){x - 9, y - 3}, (Vector2){x, y + 10}, (Vector2){x + 9, y - 3}, c1);
        DrawTri((Vector2){x - 9, y - 3}, (Vector2){x, y - 8}, (Vector2){x + 9, y - 3}, c2);
    } else if (type == ITEM_GEM) {
        if (theme == 0) {
            DrawCircle((int)x, (int)y, 12, (Color){255, 200, 90, 255});
            DrawCircle((int)x, (int)y, 9, (Color){180, 130, 50, 255});
        } else if (theme == 1) {
            DrawCircle((int)x, (int)y, 12, Fade((Color){120, 60, 220, 255}, 0.6f));
            DrawCircle((int)x, (int)y, 8, (Color){180, 100, 255, 255});
            DrawCircle((int)x, (int)y, 3, WHITE);
        } else {
            DrawRectangle((int)(x - 9), (int)(y - 9), 18, 18, (Color){255, 200, 40, 255});
            DrawRectangle((int)(x - 7), (int)(y - 7), 14, 14, (Color){255, 240, 100, 255});
            DrawText("B", (int)(x - 4), (int)(y - 6), 12, (Color){120, 70, 0, 255});
        }
    } else if (type == ITEM_POWERUP) {
        Color bg = (theme == 0) ? (Color){80, 220, 130, 255} : (theme == 1) ? (Color){80, 200, 255, 255} : (Color){255, 240, 0, 255};
        DrawEllipse((int)x, (int)y, 10, 12, bg);
        DrawRectangle((int)(x - 4), (int)(y - 14), 8, 3, (Color){180, 180, 200, 255});
        DrawRectangle((int)(x - 2), (int)(y - 4), 4, 8, WHITE);
    } else if (type == ITEM_PIZZA) {
        if (theme == 0) {
            DrawCircle((int)x, (int)y, 10, (Color){255, 180, 60, 255});
            DrawCircle((int)x, (int)y, 8, (Color){255, 220, 100, 255});
            DrawRectangle((int)(x - 2), (int)(y - 8), 4, 3, (Color){100, 60, 20, 255});
        } else {
            DrawTri((Vector2){x - 10, y - 6}, (Vector2){x, y + 11}, (Vector2){x + 10, y - 6}, (Color){255, 210, 90, 255});
            DrawRectangle((int)(x - 11), (int)(y - 8), 22, 4, (Color){200, 120, 50, 255});
            DrawRectangle((int)(x - 4), (int)(y - 3), 5, 5, (Color){220, 50, 50, 255});
        }
    } else if (type == ITEM_BOBA) {
        Color cup = (theme == 2) ? (Color){40, 30, 60, 255} : (Color){225, 185, 145, 255};
        DrawRectangle((int)(x - 7), (int)(y - 5), 14, 16, cup);
        DrawRectangle((int)(x - 8), (int)(y - 7), 16, 3, (Color){250, 250, 255, 255});
        DrawRectangle((int)(x + 2), (int)(y - 14), 3, 8, (Color){255, 110, 180, 255});
        for (int k = 0; k < 4; k++) DrawRectangle((int)(x - 5 + k * 3), (int)(y + 8), 3, 3, (Color){70, 35, 25, 255});
    } else {
        Color ac = (theme == 0) ? (Color){120, 255, 140, 255}
                 : (theme == 1) ? (Color){180, 120, 255, 255}
                                : (Color){255, 100, 220, 255};
        for (int r = 0; r < 5; r++)
            for (int c = 0; c < 5; c++) {
                bool on = false;
                if (r == 0 && (c == 0 || c == 4)) on = true;
                if (r == 1 && c >= 1 && c <= 3) on = true;
                if (r == 2) on = true;
                if (r == 3 && (c == 0 || c == 2 || c == 4)) on = true;
                if (r == 4 && (c == 0 || c == 4)) on = true;
                if (on) DrawRectangle((int)(x - 6 + c * 3), (int)(y - 8 + r * 3), 3, 3, ac);
            }
    }
    (void)ink;
}
void drawItems() {
    float time = (float)GetTime();
    for (int k = 0; k < itemCount; k++) {
        if (items[k].collected) continue;
        float cx = items[k].col * cell_size + cell_size / 2.0f;
        float cy = items[k].row * cell_size + cell_size / 2.0f;
        float bob = sinf(time * 3 + items[k].phase) * 3;
        float spin = cosf(time * 2.2f + items[k].phase);
        if (fabsf(spin) < 0.15f) spin = (spin < 0) ? -0.15f : 0.15f;
        float glow = 0.35f + 0.2f * sinf(time * 5 + items[k].phase);
        Color c = ItemColor(items[k].type);
        if (time < scanRevealUntil) {
            glow = 0.9f;
            float dx = cx - playerDrawX, dy = cy - playerDrawY;
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist > 20) {
                int segs = (int)(dist / 8);
                for (int s = 0; s < segs; s += 2) {
                    float px = playerDrawX + dx * (s / (float)segs);
                    float py = playerDrawY + dy * (s / (float)segs);
                    DrawRectangle((int)px - 1, (int)py - 1, 3, 3, Fade(c, 0.35f));
                }
            }
        }
        DrawEllipse((int)cx, (int)(cy + 11), 7 - bob * 0.5f, 2.5f, Fade(BLACK, 0.3f));
        DrawGlow(cx, cy + bob, 20, c, glow);
        DrawItemIcon(items[k].type, cx, cy + bob, spin, time + items[k].phase, gamelvl - 1);
    }
}

void EchoReset() {
    for (int i = 0; i < ECHO_HISTORY; i++) { echo.samples[i] = (Vector2){-100, -100}; echo.sampleTime[i] = 0; }
    echo.head = 0; echo.count = 0;
    echoGhostPos = (Vector2){-100, -100};
    echoGhostActive = false;
    echoCollideCooldown = 0;
}
void EchoRecord(Vector2 p, float t) {
    echo.samples[echo.head] = p;
    echo.sampleTime[echo.head] = t;
    echo.head = (echo.head + 1) % ECHO_HISTORY;
    if (echo.count < ECHO_HISTORY) echo.count++;
}
Vector2 EchoGetDelayed(float now, float lag, bool* valid) {
    *valid = false;
    float target = now - lag;
    float bestDelta = 1e9f; int bestIdx = -1;
    for (int i = 0; i < echo.count; i++) {
        float dt = fabsf(echo.sampleTime[i] - target);
        if (dt < bestDelta) { bestDelta = dt; bestIdx = i; }
    }
    if (bestIdx >= 0 && bestDelta < 1.0f) { *valid = true; return echo.samples[bestIdx]; }
    return (Vector2){-100, -100};
}

void DamagePlayer(const char* cause, Color col) {
    if (GetTime() < invulnUntil) return;
    lives--;
    invulnUntil = (float)GetTime() + INVULN_TIME;
    damageFlash = 1.0f;
    shakeTime = 0.5f; shakeStrength = 14;
    redFlash = 1.0f;
    paradoxFlash = 0.8f;
    paradoxColor = col;
    SpawnFloatText(cause, playerDrawX, playerDrawY - 30, 22, (Color){255, 100, 100, 255});
    SpawnBurst(PT_DOT, playerDrawX, playerDrawY, 30, 260, 0, 1.0f, 4, col, false);
    PlayPitched(damageSound, 0.6f);
    if (lives <= 0) {
        lives = 0;
        StopAllLevelSounds();
        PlaySound(gameOverSound);
        UpdateHighScore();
        chatIndex = 0; chatTyping = 0; chatPanelOpen = 0;
        gameState = MISSION_DEBRIEF;
    } else {
        player_row = start_row;
        player_col = start_col;
        playerDrawX = player_col * cell_size + cell_size / 2.0f;
        playerDrawY = player_row * cell_size + cell_size / 2.0f;
    }
}

void SetupJungleMechanics() {
    torchLight = 1.0f;
    for (int i = 0; i < 4; i++) boulders[i].active = false;
    boulderCooldown = 0;
    for (int i = 0; i < MAZE_SIZE; i++)
        for (int j = 0; j < MAZE_SIZE; j++) pressurePlates[i][j] = 0;
    int placed = 0, tries = 0;
    while (placed < 6 && tries < 500) {
        tries++;
        int r = GetRandomValue(4, MAZE_SIZE - 5);
        int c = GetRandomValue(4, MAZE_SIZE - 5);
        if (!reachable[r][c]) continue;
        if (abs(r - start_row) + abs(c - start_col) < 6) continue;
        if (r == goal_row && c == goal_col) continue;
        pressurePlates[r][c] = 1;
        placed++;
    }
}
void SetupCosmicMechanics() {
    wormholeCount = 0;
    zeroGActive = false;
    zeroGStep = 0;
    wormholeCooldown = 0;
    int pairs = 2;
    for (int p = 0; p < pairs; p++) {
        int tries = 0, r1 = 0, c1 = 0, r2 = 0, c2 = 0;
        bool ok = false;
        while (!ok && tries < 500) {
            tries++;
            r1 = GetRandomValue(2, MAZE_SIZE / 2 - 1);
            c1 = GetRandomValue(2, MAZE_SIZE - 3);
            r2 = GetRandomValue(MAZE_SIZE / 2 + 1, MAZE_SIZE - 3);
            c2 = GetRandomValue(2, MAZE_SIZE - 3);
            if (!reachable[r1][c1] || !reachable[r2][c2]) continue;
            if (abs(r1 - start_row) + abs(c1 - start_col) < 3) continue;
            if (r2 == goal_row && c2 == goal_col) continue;
            ok = true;
        }
        if (ok) {
            wormholes[wormholeCount].r1 = r1; wormholes[wormholeCount].c1 = c1;
            wormholes[wormholeCount].r2 = r2; wormholes[wormholeCount].c2 = c2;
            wormholeCount++;
        }
    }
}
void SetupCyberMechanics() {
    sentinelCount = 0;
    sentinelLockCount = 0;
    sentinelLockTimer = 0;
    int num = 3;
    for (int s = 0; s < num; s++) {
        int tries = 0, r = 10, c = 10;
        while (tries < 300) {
            tries++;
            r = GetRandomValue(4, MAZE_SIZE - 6);
            c = GetRandomValue(4, MAZE_SIZE - 6);
            if (!reachable[r][c]) continue;
            if (abs(r - start_row) + abs(c - start_col) < 8) continue;
            if (abs(r - goal_row) + abs(c - goal_col) < 4) continue;
            break;
        }
        Sentinel* sn = &sentinels[sentinelCount];
        sn->row = (float)r; sn->col = (float)c;
        sn->timer = 0;
        sn->speed = RandF(1.5f, 2.5f);
        sn->routeLen = 4;
        sn->routeIdx = 0;
        sn->routeR[0] = r; sn->routeC[0] = c;
        sn->routeR[1] = r; sn->routeC[1] = c + 3;
        sn->routeR[2] = r + 3; sn->routeC[2] = c + 3;
        sn->routeR[3] = r + 3; sn->routeC[3] = c;
        for (int k = 0; k < 4; k++) {
            int rr = sn->routeR[k], cc = sn->routeC[k];
            if (rr < 0 || rr >= MAZE_SIZE || cc < 0 || cc >= MAZE_SIZE || IsWallAt(rr, cc)) {
                sn->routeR[k] = r; sn->routeC[k] = c;
            }
        }
        sn->alertTimer = 0;
        sn->detectRadius = 3.5f;
        sentinelCount++;
    }
}
void UpdateJungleMechanics(float dt) {
    if (gameState == PLAYING) {
        torchLight -= TORCH_DECAY * dt;
        if (torchLight < 0) torchLight = 0;
    }
    for (int i = 0; i < 4; i++) {
        Boulder* b = &boulders[i];
        if (!b->active) continue;
        b->life -= dt;
        if (b->life <= 0) { b->active = false; continue; }
        b->row += b->dirRow * b->speed * dt;
        b->col += b->dirCol * b->speed * dt;
        int rr = (int)(b->row + 0.5f);
        int cc = (int)(b->col + 0.5f);
        if (IsWallAt(rr, cc)) { b->active = false; continue; }
        if (rr == player_row && cc == player_col) {
            DamagePlayer("BOULDER!", (Color){200, 120, 60, 255});
            b->active = false;
            continue;
        }
        SpawnParticle(PT_PUFF, b->col * cell_size + cell_size / 2.0f, b->row * cell_size + cell_size / 2.0f,
                      0, 0, 0, 0.4f, 6, (Color){140, 100, 70, 200});
    }
    if (gameState == PLAYING && boulderCooldown <= 0 &&
        pressurePlates[player_row][player_col]) {
        pressurePlates[player_row][player_col] = 0;
        boulderCooldown = 2.0f;
        for (int i = 0; i < 4; i++) {
            if (!boulders[i].active) {
                boulders[i].active = true;
                boulders[i].row = (float)player_row;
                boulders[i].col = (float)player_col;
                int bestDist = 0, bestDX = 0, bestDY = 0;
                int dirs[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
                for (int d = 0; d < 4; d++) {
                    int dist = 0;
                    while (dist < 12 && !IsWallAt(player_row + dirs[d][0] * (dist + 1),
                                                  player_col + dirs[d][1] * (dist + 1))) dist++;
                    if (dist > bestDist) { bestDist = dist; bestDX = dirs[d][0]; bestDY = dirs[d][1]; }
                }
                if (bestDist == 0) { boulders[i].active = false; break; }
                boulders[i].dirRow = bestDX;
                boulders[i].dirCol = bestDY;
                boulders[i].speed = RandF(3.5f, 5.0f);
                boulders[i].life = 6.0f;
                PlayPitched(boulderSound, 0.6f);
                SpawnFloatText("BOULDER!", playerDrawX, playerDrawY - 40, 22, (Color){200, 140, 60, 255});
                break;
            }
        }
    }
    if (boulderCooldown > 0) boulderCooldown -= dt;
}
void UpdateCosmicMechanics(float dt) {
    wormholeCooldown -= dt;
    if (wormholeCooldown <= 0 && gameState == PLAYING) {
        for (int i = 0; i < wormholeCount; i++) {
            Wormhole* w = &wormholes[i];
            if (player_row == w->r1 && player_col == w->c1) {
                player_row = w->r2; player_col = w->c2;
                wormholeCooldown = 1.5f;
                PlayPitched(wormholeSound, 1.6f);
                SpawnFloatText("WARP!", playerDrawX, playerDrawY - 30, 22, themes[1].neonB);
                lightFlash = 0.5f; lightFlashColor = themes[1].neonB;
                SpawnBurst(PT_SPARKLE, playerDrawX, playerDrawY, 20, 220, 0, 0.8f, 7, themes[1].neonA, false);
                return;
            }
            if (player_row == w->r2 && player_col == w->c2) {
                player_row = w->r1; player_col = w->c1;
                wormholeCooldown = 1.5f;
                PlayPitched(wormholeSound, 1.6f);
                SpawnFloatText("WARP!", playerDrawX, playerDrawY - 30, 22, themes[1].neonB);
                lightFlash = 0.5f; lightFlashColor = themes[1].neonB;
                SpawnBurst(PT_SPARKLE, playerDrawX, playerDrawY, 20, 220, 0, 0.8f, 7, themes[1].neonA, false);
                return;
            }
        }
    }
    if (zeroGActive && gameState == PLAYING) {
        zeroGStep -= dt;
        if (zeroGStep <= 0) {
            zeroGStep = 0.16f;
            int nr = player_row + zeroGVelY;
            int nc = player_col + zeroGVelX;
            if (!Pathwalk(nr, nc)) {
                zeroGActive = false;
                shakeTime = 0.3f; shakeStrength = 8;
                PlaySound(wallSound);
            } else {
                player_row = nr; player_col = nc;
                MarkTrail(nr, nc);
                litTime[nr][nc] = (float)GetTime();
                moves++;
                SpawnParticle(PT_DASH, playerDrawX, playerDrawY, 0, 0, 0, 0.4f, 3, themes[1].neonA);
                for (int i = 0; i < itemCount; i++)
                    if (!items[i].collected && items[i].row == player_row && items[i].col == player_col) {
                        items[i].collected = true;
                        itemsCollected++;
                        int pts = ItemPoints(items[i].type);
                        score += pts;
                        if (items[i].type == ITEM_POWERUP) timeRemaining += 10;
                        SpawnFloatText(TextFormat("+%d", pts), playerDrawX, playerDrawY - 20, 20, ItemColor(items[i].type));
                        SpawnBurst(PT_SPARKLE, playerDrawX, playerDrawY, 12, 140, 0, 0.7f, 7, ItemColor(items[i].type), false);
                        PlayPitched(coinSound, 1.2f);
                    }
            }
        }
    }
}
void UpdateCyberMechanics(float dt) {
    for (int s = 0; s < sentinelCount; s++) {
        Sentinel* sn = &sentinels[s];
        sn->timer += dt * sn->speed;
        if (sn->timer >= 1.0f) {
            sn->timer -= 1.0f;
            sn->routeIdx = (sn->routeIdx + 1) % sn->routeLen;
        }
        int idx = sn->routeIdx;
        int next = (sn->routeIdx + 1) % sn->routeLen;
        float nr = (float)sn->routeR[idx] + ((float)sn->routeR[next] - (float)sn->routeR[idx]) * sn->timer;
        float nc = (float)sn->routeC[idx] + ((float)sn->routeC[next] - (float)sn->routeC[idx]) * sn->timer;
        sn->row = nr; sn->col = nc;
        if (sn->alertTimer > 0) sn->alertTimer -= dt;
        if (gameState == PLAYING) {
            float dx = (float)player_row - nr;
            float dy = (float)player_col - nc;
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist < sn->detectRadius) {
                if (sn->alertTimer <= 0) {
                    sn->alertTimer = 1.5f;
                    if (sentinelLockCount < 8) {
                        sentinelLockR[sentinelLockCount] = (int)(nr + 0.5f);
                        sentinelLockC[sentinelLockCount] = (int)(nc + 0.5f);
                        sentinelLockCount++;
                        sentinelLockTimer = 2.5f;
                        PlayPitched(sentinelSound, 0.9f);
                        SpawnFloatText("DETECTED!", playerDrawX, playerDrawY - 30, 22, (Color){255, 40, 80, 255});
                        DamagePlayer("SENTINEL!", (Color){255, 40, 80, 255});
                    }
                }
            }
        }
    }
    if (sentinelLockTimer > 0) {
        sentinelLockTimer -= dt;
        if (sentinelLockTimer <= 0) sentinelLockCount = 0;
    }
}

void ClearEffects() {
    ClearParticles();
    for (int i = 0; i < MAX_FLOAT_TEXTS; i++) floatTexts[i].active = false;
    for (int i = 0; i < MAX_REACTIONS; i++) reactions[i].active = false;
    shakeTime = 0; redFlash = 0; lightFlash = 0; glitchTime = 0; paradoxFlash = 0;
}

void loadLevel(int level) {
    gamelvl = level;
    player_row = start_row; player_col = start_col;
    goal_row = 28; goal_col = 28;
    moves = 0;
    phase = 0;
    phaseCooldown = 0;
    phaseFlipAnim = 0;
    phaseFlipTarget = 0;
    ClearTrail(); MarkTrail(start_row, start_col);

    if (level == 1) timeRemaining = 90.0f;
    else if (level == 2) timeRemaining = 75.0f;
    else timeRemaining = 60.0f;

    ClearEffects();
    levelElapsed = 0; newBestTime = false;
    combo = 0; lastCollectTime = -10;
    itemsCollected = 0;
    displayScore = 0;
    happyUntil = 0; sadUntil = 0;
    lastMoveTime = -10; lastBumpTime = -10;
    dashCooldown = 0; scanCooldown = 0;
    dashActiveUntil = 0; dashDirX = dashDirY = 0;
    scanRevealUntil = 0; scanPulseTime = -1;
    lives = maxLives;
    invulnUntil = 0;
    damageFlash = 0;
    pathLen = 0;

    playerDrawX = start_col * cell_size + cell_size / 2.0f;
    playerDrawY = start_row * cell_size + cell_size / 2.0f;
    for (int k = 0; k < TRAIL_POINTS; k++) trailPoints[k] = (Vector2){playerDrawX, playerDrawY};

    for (int i = 0; i < MAZE_SIZE; i++)
        for (int j = 0; j < MAZE_SIZE; j++) litTime[i][j] = -100;
    litTime[start_row][start_col] = (float)GetTime();

    FindReachableCells();
    if (!reachable[goal_row][goal_col]) {
        int bestDist = 999999, newR = goal_row, newC = goal_col;
        for (int i = 1; i < MAZE_SIZE - 1; i++)
            for (int j = 1; j < MAZE_SIZE - 1; j++) {
                if (!reachable[i][j]) continue;
                int d = abs(i - goal_row) + abs(j - goal_col);
                if (d < bestDist) { bestDist = d; newR = i; newC = j; }
            }
        goal_row = newR; goal_col = newC;
    }

    PlaceCollectibles();

    EchoReset();
    EchoRecord((Vector2){playerDrawX, playerDrawY}, (float)GetTime());

    if (level == 1) SetupJungleMechanics();
    else if (level == 2) SetupCosmicMechanics();
    else SetupCyberMechanics();
}

void PlayItemSound(int type, int comboCount) {
    float pitch = RandF(0.95f, 1.05f) + 0.06f * (comboCount - 1);
    if      (type == ITEM_COIN)    PlayPitched(coinSound,    pitch);
    else if (type == ITEM_STAR)    PlayPitched(coinSound,    pitch * 1.5f);
    else if (type == ITEM_GEM)     PlayPitched(giftSound,    pitch * 1.4f);
    else if (type == ITEM_GIFT)    PlayPitched(giftSound,    pitch);
    else if (type == ITEM_PIZZA)   PlayPitched(collectSound, pitch);
    else if (type == ITEM_BOBA)    PlayPitched(collectSound, pitch);
    else if (type == ITEM_POWERUP) PlayPitched(powerupSound, pitch);
    else                            PlayPitched(powerupSound, pitch * 0.7f);
}

void CollectItem(int i) {
    float now = (float)GetTime();
    int type = items[i].type;
    items[i].collected = true;
    itemsCollected++;

    if (now - lastCollectTime <= COMBO_WINDOW) combo++;
    else combo = 1;
    lastCollectTime = now;
    int mult = combo; if (mult > 5) mult = 5;

    int pts = ItemPoints(type);
    if (type == ITEM_ALIEN) pts = GetRandomValue(5, 25) * 10;
    score += pts * mult;
    if (type == ITEM_POWERUP) timeRemaining += 10;

    float x = items[i].col * cell_size + cell_size / 2.0f;
    float y = items[i].row * cell_size + cell_size / 2.0f;
    Color c = ItemColor(type);

    if (gamelvl == 1) torchLight = fminf(1.0f, torchLight + 0.35f);

    if (type == ITEM_POWERUP) SpawnFloatText("+10 SEC", x, y - 12, 22, (Color){140, 220, 255, 255});
    else if (mult > 1) SpawnFloatText(TextFormat("+%d x%d", pts, mult), x, y - 12, 22, c);
    else SpawnFloatText(TextFormat("+%d", pts), x, y - 12, 22, c);
    if (combo >= 2) SpawnFloatText(TextFormat("COMBO x%d", combo), x, y - 42, 24, Rainbow(now * 300, 0.6f, 1));

    SpawnBurst(PT_SPARKLE, x, y, 14, 140, 0, 0.7f, 7, c, false);
    SpawnBurst(PT_DOT, x, y, 10, 90, 0, 0.5f, 3, WHITE, false);
    if (type == ITEM_GIFT) SpawnBurst(PT_CONFETTI, x, y, 40, 260, 300, 1.4f, 9, WHITE, true);

    lightFlash = 0.35f; lightFlashColor = c;
    happyUntil = now + 1.2f;
    SpawnReaction(GetRandomValue(0, 1) == 0 ? EMOJI_FIRE : EMOJI_SPARKLE, x + 18, y - 18);
    PlayItemSound(type, combo);
}

void movement(int nr, int nc) {
    float now = (float)GetTime();
    int dirX = nc - player_col;
    int dirY = nr - player_row;

    if (Pathwalk(nr, nc)) {
        float ox = player_col * cell_size + cell_size / 2.0f;
        float oy = player_row * cell_size + cell_size / 2.0f;
        for (int k = 0; k < 5; k++)
            SpawnParticle(PT_PUFF, ox + RandF(-6, 6), oy + RandF(0, 8),
                          -dirX * RandF(20, 50) + RandF(-15, 15),
                          -dirY * RandF(20, 50) + RandF(-15, 15), 0, 0.45f, 5, (Color){225, 225, 240, 255});

        player_row = nr; player_col = nc;
        MarkTrail(nr, nc);
        litTime[nr][nc] = now;
        moves++;

        lastMoveTime = now; lastDirX = dirX; lastDirY = dirY;
        if (gamelvl == 3) glitchTime = 0.12f;
        PlayPitched(moveSound, RandF(0.9f, 1.1f));

        for (int i = 0; i < itemCount; i++)
            if (!items[i].collected && items[i].row == player_row && items[i].col == player_col)
                CollectItem(i);
    } else {
        lastBumpTime = now; lastDirX = dirX; lastDirY = dirY;
        sadUntil = now + 1.0f; happyUntil = 0;
        shakeTime = 0.25f; shakeStrength = 7; redFlash = 0.45f;
        float hx = player_col * cell_size + cell_size / 2.0f + dirX * cell_size / 2.0f;
        float hy = player_row * cell_size + cell_size / 2.0f + dirY * cell_size / 2.0f;
        SpawnBurst(PT_SPARKLE, hx, hy, 10, 150, 0, 0.4f, 6, themes[gamelvl - 1].neonA, false);
        PlaySound(wallSound);
    }
}

void TryDash() {
    if (dashCooldown > 0) return;
    int dx = lastDirX, dy = lastDirY;
    if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) { dx = 0; dy = -1; }
    else if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) { dx = 0; dy = 1; }
    else if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) { dx = -1; dy = 0; }
    else if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) { dx = 1; dy = 0; }
    if (dx == 0 && dy == 0) return;

    int steps = 0;
    for (int s = 1; s <= 2; s++) {
        int nr = player_row + dy * s, nc = player_col + dx * s;
        if (!Pathwalk(nr, nc)) break;
        steps = s;
    }
    if (steps == 0) { dashCooldown = 0.5f; PlaySound(wallSound); return; }

    for (int s = 1; s <= steps; s++) {
        int nr = player_row + dy, nc = player_col + dx;
        player_row = nr; player_col = nc;
        MarkTrail(nr, nc);
        litTime[nr][nc] = (float)GetTime();
        moves++;
        for (int i = 0; i < itemCount; i++)
            if (!items[i].collected && items[i].row == nr && items[i].col == nc)
                CollectItem(i);
    }

    float now = (float)GetTime();
    lastMoveTime = now;
    lastDirX = dx; lastDirY = dy;
    dashActiveUntil = now + 0.35f;
    dashDirX = dx; dashDirY = dy;
    dashCooldown = DASH_COOLDOWN;

    for (int k = 0; k < 24; k++) {
        float px = playerDrawX - dx * RandF(20, 120);
        float py = playerDrawY - dy * RandF(20, 120);
        Color c = (RandF(0, 1) < 0.5f) ? themes[gamelvl - 1].neonA : themes[gamelvl - 1].neonC;
        SpawnParticle(PT_DASH, px, py, -dx * 60, -dy * 60, 0, RandF(0.3f, 0.5f), 3, c);
    }
    SpawnBurst(PT_SPARKLE, playerDrawX, playerDrawY, 12, 220, 0, 0.5f, 6, themes[gamelvl - 1].neonA, false);
    lightFlash = 0.3f; lightFlashColor = themes[gamelvl - 1].neonA;
    PlayPitched(dashSound, 1.6f);

    if (gamelvl == 2) {
        zeroGActive = true;
        zeroGVelX = dx; zeroGVelY = dy;
        zeroGStep = 0.16f;
    }
}

void TryScan() {
    if (scanCooldown > 0) return;
    scanCooldown = SCAN_COOLDOWN;
    float now = (float)GetTime();
    scanRevealUntil = now + SCAN_REVEAL_TIME;
    scanPulseTime = 0;
    scanPulseX = playerDrawX; scanPulseY = playerDrawY;
    SpawnBurst(PT_DOT, playerDrawX, playerDrawY, 30, 260, 0, 0.8f, 4, themes[gamelvl - 1].neonB, false);
    lightFlash = 0.25f; lightFlashColor = themes[gamelvl - 1].neonB;
    PlayPitched(scanSound, 1.3f);
    ComputeGoalPath();
}

void TryPhaseFlip() {
    if (phaseCooldown > 0) return;
    phaseCooldown = PHASE_COOLDOWN;
    phaseFlipTarget = 1 - phase;
    phaseFlipAnim = 1.0f;
    PlayPitched(phaseSound, 0.8f);
    SpawnFloatText("PHASE FLIP", playerDrawX, playerDrawY - 30, 20, themes[gamelvl - 1].neonC);
    SpawnBurst(PT_DOT, playerDrawX, playerDrawY, 30, 240, 0, 0.8f, 4, themes[gamelvl - 1].neonC, false);
    lightFlash = 0.4f; lightFlashColor = themes[gamelvl - 1].neonC;
}

void StartLevel(int level) {
    StopEverything();
    gamelvl = level;
    score = 0; moves = 0;
    loadLevel(level);
    chatIndex = 0; chatTyping = 0; chatPanelOpen = 0;
    gameState = MISSION_BRIEF;
}

void TriggerGameOver() {
    timeRemaining = 0;
    StopAllLevelSounds();
    PlaySound(gameOverSound);
    UpdateHighScore();
    shakeTime = 0.5f; shakeStrength = 12; redFlash = 1.0f;
    sadUntil = (float)GetTime() + 1000;
    happyUntil = 0;
    SpawnReaction(EMOJI_SKULL, playerDrawX, playerDrawY - 20);
    SpawnBurst(PT_DOT, playerDrawX, playerDrawY, 30, 200, 300, 1.0f, 4, RED, false);
    chatIndex = 0; chatTyping = 0; chatPanelOpen = 0;
    gameState = MISSION_DEBRIEF;
}

void Goalreached() {
    if (player_row == goal_row && player_col == goal_col) {
        StopAllLevelSounds();
        int bonus = 100 - moves; if (bonus < 0) bonus = 0;
        int timeBonus = (int)timeRemaining * 2;
        score += 100 + bonus + timeBonus + lives * 50;
        if (bestTimes[gamelvl - 1] <= 0 || levelElapsed < bestTimes[gamelvl - 1]) {
            bestTimes[gamelvl - 1] = levelElapsed;
            newBestTime = true;
            SaveExtras();
        }
        float gx = goal_col * cell_size + cell_size / 2.0f;
        float gy = goal_row * cell_size + cell_size / 2.0f;
        SpawnBurst(PT_CONFETTI, gx, gy, 70, 420, 380, 2.2f, 10, WHITE, true);
        SpawnBurst(PT_SPARKLE, gx, gy, 25, 300, 0, 1.2f, 10, GOLD, false);
        lightFlash = 0.6f; lightFlashColor = WHITE;
        happyUntil = (float)GetTime() + 1000;
        PlaySound(winSound);
        UpdateHighScore();
        chatIndex = 0; chatTyping = 0; chatPanelOpen = 0;
        gameState = MISSION_DEBRIEF;
    }
}

void InitMissionChats() {
    briefs[0].lineCount = 5;
    briefs[0].title = "MISSION 01 :: THE OVERGROWN CANOPY";
    briefs[0].accent = themes[0].neonA;
    briefs[0].lines[0] = (ChatLine){SPEAKER_SYSTEM, "ESTABLISHING SECURE LINK..."};
    briefs[0].lines[1] = (ChatLine){SPEAKER_ORACLE, "Agent. Welcome to ECHO GRID."};
    briefs[0].lines[2] = (ChatLine){SPEAKER_ORACLE, "Torchlight fades. Find fruit to see."};
    briefs[0].lines[3] = (ChatLine){SPEAKER_ORACLE, "Press F to SCAN - it reveals the route to the goal."};
    briefs[0].lines[4] = (ChatLine){SPEAKER_SYSTEM, "3 LIVES. TIME LIMIT :: 90 SEC."};

    debriefs[0].lineCount = 4;
    debriefs[0].title = "MISSION 01 :: DEBRIEF";
    debriefs[0].accent = themes[0].neonA;
    debriefs[0].lines[0] = (ChatLine){SPEAKER_SYSTEM, "RELIC ACQUIRED."};
    debriefs[0].lines[1] = (ChatLine){SPEAKER_ORACLE, "Clean jungle run. Nice work."};
    debriefs[0].lines[2] = (ChatLine){SPEAKER_ORACLE, "Next: Alien World. No gravity. Watch it."};
    debriefs[0].lines[3] = (ChatLine){SPEAKER_PLAYER, "Point me at it."};

    briefs[1].lineCount = 5;
    briefs[1].title = "MISSION 02 :: UNKNOWN ALIEN WORLD";
    briefs[1].accent = themes[1].neonA;
    briefs[1].lines[0] = (ChatLine){SPEAKER_SYSTEM, "TELEPORT INBOUND..."};
    briefs[1].lines[1] = (ChatLine){SPEAKER_ORACLE, "DASH causes zero-G sliding. Use it wisely."};
    briefs[1].lines[2] = (ChatLine){SPEAKER_ORACLE, "Wormholes connect quadrants. Free shortcuts."};
    briefs[1].lines[3] = (ChatLine){SPEAKER_PLAYER, "Gravity?"};
    briefs[1].lines[4] = (ChatLine){SPEAKER_ORACLE, "Whatever you bring with you. SCAN for the route."};

    debriefs[1].lineCount = 4;
    debriefs[1].title = "MISSION 02 :: DEBRIEF";
    debriefs[1].accent = themes[1].neonA;
    debriefs[1].lines[0] = (ChatLine){SPEAKER_SYSTEM, "AIRLOCK UNLOCKED."};
    debriefs[1].lines[1] = (ChatLine){SPEAKER_ORACLE, "Great drift. Signal traced to the Core."};
    debriefs[1].lines[2] = (ChatLine){SPEAKER_ENEMY,  "...we see you."};
    debriefs[1].lines[3] = (ChatLine){SPEAKER_PLAYER, "Let's finish this."};

    briefs[2].lineCount = 5;
    briefs[2].title = "MISSION 03 :: MAINFRAME INFILTRATION";
    briefs[2].accent = themes[2].neonA;
    briefs[2].lines[0] = (ChatLine){SPEAKER_SYSTEM, "ALERT :: SENTINELS ACTIVE."};
    briefs[2].lines[1] = (ChatLine){SPEAKER_ORACLE, "Red drones lock down corridors. Stay invisible."};
    briefs[2].lines[2] = (ChatLine){SPEAKER_ORACLE, "Press E to phase-flip. Walls become paths."};
    briefs[2].lines[3] = (ChatLine){SPEAKER_ENEMY,  "You will not leave this city."};
    briefs[2].lines[4] = (ChatLine){SPEAKER_PLAYER, "Watch me."};

    debriefs[2].lineCount = 4;
    debriefs[2].title = "MISSION 03 :: DEBRIEF";
    debriefs[2].accent = themes[2].neonA;
    debriefs[2].lines[0] = (ChatLine){SPEAKER_SYSTEM, "SOURCE EXTRACTED."};
    debriefs[2].lines[1] = (ChatLine){SPEAKER_ORACLE, "You did it."};
    debriefs[2].lines[2] = (ChatLine){SPEAKER_ORACLE, "ECHO GRID complete. Legend status."};
    debriefs[2].lines[3] = (ChatLine){SPEAKER_PLAYER, "GG."};

    failChat.lineCount = 4;
    failChat.title = "CONNECTION LOST";
    failChat.accent = (Color){255, 60, 80, 255};
    failChat.lines[0] = (ChatLine){SPEAKER_SYSTEM, "TRACE COMPLETE."};
    failChat.lines[1] = (ChatLine){SPEAKER_SYSTEM, "AGENT IDENTIFIED."};
    failChat.lines[2] = (ChatLine){SPEAKER_ORACLE, "Agent? Come in... static..."};
    failChat.lines[3] = (ChatLine){SPEAKER_ENEMY,  "One more try?"};
}

typedef enum { PORTRAIT_ORACLE, PORTRAIT_PLAYER, PORTRAIT_SYSTEM, PORTRAIT_ENEMY } PortraitID;
void DrawPortrait(int id, int x, int y) {
    float t = (float)GetTime();
    DrawPixelFrame(x - 4, y - 4, 104, 104, BLACK, (Color){20, 15, 30, 255});
    if (id == PORTRAIT_ORACLE) {
        DrawGlow(x + 48, y + 48, 46, (Color){0, 255, 180, 255}, 0.4f);
        PixelBlock(x + 24, y + 20, 48, 56, (Color){10, 30, 25, 255});
        PixelBlock(x + 28, y + 24, 40, 48, (Color){0, 60, 50, 255});
        for (int sy = y + 26; sy < y + 72; sy += 4) PixelBlock(x + 28, sy, 40, 2, (Color){0, 90, 70, 255});
        PixelBlock(x + 34, y + 40, 28, 16, (Color){0, 20, 15, 255});
        int blink = (int)(t * 1.2f) % 4 == 0 ? 0 : 1;
        if (blink) {
            PixelBlock(x + 38, y + 44, 20, 8, (Color){0, 255, 180, 255});
            PixelBlock(x + 44, y + 42, 8, 12, BLACK);
        }
    } else if (id == PORTRAIT_PLAYER) {
        DrawGlow(x + 48, y + 48, 46, (Color){0, 200, 255, 255}, 0.35f);
        PixelBlock(x + 20, y + 24, 56, 52, (Color){15, 20, 40, 255});
        PixelBlock(x + 24, y + 28, 48, 44, (Color){30, 50, 80, 255});
        PixelBlock(x + 28, y + 36, 40, 18, (Color){0, 15, 25, 255});
        PixelBlock(x + 30, y + 38, 36, 14, (Color){0, 180, 220, 255});
    } else if (id == PORTRAIT_SYSTEM) {
        DrawPixelFrame(x + 8, y + 8, 80, 80, (Color){60, 60, 80, 255}, (Color){5, 10, 20, 255});
        for (int r = 0; r < 6; r++)
            for (int c = 0; c < 8; c++) {
                int b = CellHash(r, c) % 3 == 0 ? 1 : 0;
                unsigned char v = (unsigned char)(40 + b * 120);
                PixelBlock(x + 16 + c * 8, y + 18 + r * 10, 6, 6, (Color){0, v, (unsigned char)(v / 2), 255});
            }
    } else {
        DrawGlow(x + 48, y + 48, 46, (Color){255, 40, 80, 255}, 0.5f);
        for (int k = 0; k < 24; k++) {
            unsigned int h = CellHash(k, (int)(t * 8));
            PixelBlock(x + 16 + (h % 64), y + 16 + ((h >> 8) % 64), 4, 4, (Color){255, 30, 60, 255});
        }
        PixelBlock(x + 28, y + 32, 40, 32, (Color){30, 0, 10, 255});
        PixelBlock(x + 34, y + 40, 8, 8, (Color){255, 60, 60, 255});
        PixelBlock(x + 54, y + 40, 8, 8, (Color){255, 60, 60, 255});
    }
}

void drawBackground() {
    Theme th = themes[gamelvl - 1];
    float time = (float)GetTime();
    DrawRectangleGradientV(0, 0, SCREEN_W, SCREEN_W, th.bgTop, th.bgBottom);
    if (gamelvl == 1) {
        int baseY = SCREEN_W - 20;
        for (int layer = 0; layer < 4; layer++) {
            float h = 180 - layer * 30;
            int shade = 40 - layer * 8;
            Color col = (Color){(unsigned char)(shade / 2), (unsigned char)shade, (unsigned char)(shade / 2 + 5), 255};
            for (int x = 0; x < SCREEN_W; x += 20) {
                float wave = sinf(time * 0.15f + x * 0.01f + layer * 1.7f) * 25;
                int top = (int)(baseY - h + wave + layer * 40);
                DrawRectangle(x, top, 20, baseY - top, col);
            }
        }
    } else if (gamelvl == 2) {
        for (int k = 0; k < 5; k++) {
            float x = 100 + k * 200 + sinf(time * 0.1f + k * 1.2f) * 80;
            float y = 200 + (k % 3) * 250 + cosf(time * 0.08f + k * 2.1f) * 60;
            Color c = (k % 2 == 0) ? (Color){80, 40, 140, 255} : (Color){40, 60, 130, 255};
            DrawGlow(x, y, 280, c, 0.35f);
        }
        for (int k = 0; k < 100; k++) {
            unsigned int h = CellHash(k, 11);
            int x = h % SCREEN_W, y = (h / SCREEN_W) % SCREEN_W;
            float tw = 0.4f + 0.6f * sinf(time * 2 + k * 0.7f);
            DrawRectangle(x, y, 1 + (h % 2), 1 + (h % 2), Fade(WHITE, tw * 0.9f));
        }
    } else {
        for (int b = 0; b < 16; b++) {
            unsigned int h = CellHash(b, 3);
            int w = 35 + h % 30;
            int bh = 80 + h % 220;
            int bx = b * 60 - 10;
            int by = SCREEN_W - bh;
            DrawRectangle(bx, by, w, bh, (Color){12, 2, 26, 255});
            for (int wy = by + 8; wy < SCREEN_W - 8; wy += 14)
                for (int wx = bx + 4; wx < bx + w - 6; wx += 10) {
                    unsigned int wh = CellHash(wx, wy);
                    if (wh % 3 != 0) continue;
                    bool on = ((int)(time * 1.2f) + wh) % 4 != 0;
                    if (on) DrawRectangle(wx, wy, 3, 5, Fade(wh % 2 ? themes[2].neonA : themes[2].neonB, 0.5f));
                }
        }
    }
}

void drawPaths() {
    Theme th = themes[gamelvl - 1];
    float time = (float)GetTime();
    Color pc = (phase == 0) ? th.pathColor : th.pathColorB;
    Color pa = (phase == 0) ? th.pathAccent : th.pathAccentB;

    for (int i = 0; i < MAZE_SIZE; i++) {
        for (int j = 0; j < MAZE_SIZE; j++) {
            if (readmazeCurrent(i, j) == 1) continue;
            int x = j * cell_size, y = i * cell_size;
            if (gamelvl == 1) {
                PixelBlock(x, y, cell_size, cell_size, Fade(pc, 0.92f));
                unsigned int h = CellHash(i, j);
                if (h % 7 == 0) DrawEllipse(x + 15, y + 15, 10, 7, Fade(pa, 0.35f));
                if (h % 13 == 0) {
                    float pulse = 0.3f + 0.3f * sinf(time * 1.5f + h);
                    DrawRectangle(x + 4, y + 4, 4, 4, Fade(th.neonA, pulse * 0.6f));
                }
            } else if (gamelvl == 2) {
                PixelBlock(x, y, cell_size, cell_size, Fade(pc, 0.9f));
                float pulse = 0.4f + 0.4f * sinf(time * 2 + i * 0.3f + j * 0.2f);
                DrawRectangle(x + 2, y + 2, cell_size - 4, 2, Fade(th.neonA, pulse * 0.5f));
                DrawRectangle(x + 2, y + cell_size - 4, cell_size - 4, 2, Fade(th.neonA, pulse * 0.5f));
            } else {
                PixelBlock(x, y, cell_size, cell_size, Fade(pc, 0.95f));
                DrawRectangle(x, y, cell_size, 1, Fade(th.neonA, 0.15f));
                DrawRectangle(x, y, 1, cell_size, Fade(th.neonA, 0.15f));
                if (CellHash(i, j) % 5 == 0) DrawRectangle(x + 6, y + 6, 3, 3, Fade(th.neonB, 0.6f));
            }
            if (IsWallAt(i - 1, j)) PixelBlock(x, y, cell_size, 6, Fade(BLACK, 0.35f));
            if (IsWallAt(i, j - 1)) PixelBlock(x, y, 5, cell_size, Fade(BLACK, 0.25f));
            if (trail[i][j] == 1) {
                float since = time - litTime[i][j];
                float bright = 0.3f + 0.6f * Clamp01(1 - since / 1.2f);
                PixelBlock(x + 3, y + 3, cell_size - 6, cell_size - 6, Fade(th.trailColor, bright * 0.6f));
                DrawRectangleLines(x + 3, y + 3, cell_size - 6, cell_size - 6, Fade(th.trailGlow, bright * 0.8f));
            }
        }
    }
    if (gamelvl == 1) {
        for (int i = 0; i < MAZE_SIZE; i++)
            for (int j = 0; j < MAZE_SIZE; j++) {
                if (!pressurePlates[i][j]) continue;
                int x = j * cell_size, y = i * cell_size;
                float pulse = 0.5f + 0.5f * sinf(time * 3 + i + j);
                DrawRectangle(x + 5, y + 5, cell_size - 10, cell_size - 10,
                              Fade((Color){200, 100, 60, 255}, 0.4f + pulse * 0.4f));
                DrawRectangleLines(x + 5, y + 5, cell_size - 10, cell_size - 10,
                                   Fade((Color){255, 220, 100, 255}, 0.6f + pulse * 0.4f));
            }
    }
    if (gamelvl == 2) {
        for (int i = 0; i < wormholeCount; i++) {
            int r1 = wormholes[i].r1, c1 = wormholes[i].c1;
            int r2 = wormholes[i].r2, c2 = wormholes[i].c2;
            float pulse = 0.5f + 0.5f * sinf(time * 4 + i);
            int x1 = c1 * cell_size + cell_size / 2;
            int y1 = r1 * cell_size + cell_size / 2;
            int x2 = c2 * cell_size + cell_size / 2;
            int y2 = r2 * cell_size + cell_size / 2;
            DrawGlow(x1, y1, 22, (i == 0) ? themes[1].neonA : themes[1].neonB, 0.6f + pulse * 0.3f);
            DrawGlow(x2, y2, 22, (i == 0) ? themes[1].neonA : themes[1].neonB, 0.6f + pulse * 0.3f);
            DrawCircleLines(x1, y1, 12 + pulse * 3, Fade(themes[1].neonC, 0.8f));
            DrawCircleLines(x2, y2, 12 + pulse * 3, Fade(themes[1].neonC, 0.8f));
        }
    }
}
void drawWalls() {
    Theme th = themes[gamelvl - 1];
    float time = (float)GetTime();
    float breathe = 0.6f + 0.4f * sinf(time * 2.2f);
    Color wBase = (phase == 0) ? th.wallColor : th.wallColorB;
    Color wHi = (phase == 0) ? th.wallHighlight : th.wallHighlightB;
    Color wSh = (phase == 0) ? th.wallShadow : th.wallShadowB;

    for (int i = 0; i < MAZE_SIZE; i++) {
        for (int j = 0; j < MAZE_SIZE; j++) {
            if (readmazeCurrent(i, j) != 1) continue;
            int x = j * cell_size, y = i * cell_size;
            unsigned int h = CellHash(i, j);

            PixelBlock(x, y, cell_size, cell_size, wBase);
            PixelBlock(x, y, cell_size, 3, wHi);
            PixelBlock(x, y, 3, cell_size, wHi);
            PixelBlock(x, y + cell_size - 3, cell_size, 3, wSh);
            PixelBlock(x + cell_size - 3, y, 3, cell_size, wSh);

            if (gamelvl == 1 && h % 4 == 0) {
                DrawRectangle(x + 4, y + 4, 6, 3, Fade(th.pathAccent, 0.5f));
                DrawRectangle(x + 18, y + 18, 8, 4, Fade(th.pathAccent, 0.4f));
            }
            if (gamelvl == 2) {
                PixelBlock(x + 4, y + 4, 3, 3, Fade(th.neonA, 0.5f));
                PixelBlock(x + cell_size - 7, y + cell_size - 7, 3, 3, Fade(th.neonB, 0.5f));
            }
            if (gamelvl == 3) {
                PixelBlock(x + 4, y + 4, 3, 3, Fade(th.neonA, 0.6f));
                PixelBlock(x + cell_size - 7, y + cell_size - 7, 3, 3, Fade(th.neonB, 0.6f));
                DrawRectangle(x + 8, y + cell_size / 2, cell_size - 16, 1, Fade(th.neonC, 0.3f));
            }

            Color edge = th.neonA;
            if (gamelvl == 1) {
                int pick = ((i / 4) + (j / 4) + (int)(time * 0.5f)) % 3;
                edge = (pick == 0) ? th.neonA : (pick == 1 ? th.neonB : th.neonC);
            } else if (gamelvl == 2) {
                edge = ((i / 5) % 2 == 0) ? th.neonA : th.neonB;
            } else {
                edge = Rainbow((i + j) * 10 + time * 120, 0.6f, 1.0f);
            }
            float a = breathe;
            if (!IsWallAt(i - 1, j)) PixelBlock(x, y, cell_size, 3, Fade(edge, a));
            if (!IsWallAt(i + 1, j)) PixelBlock(x, y + cell_size - 3, cell_size, 3, Fade(edge, a));
            if (!IsWallAt(i, j - 1)) PixelBlock(x, y, 3, cell_size, Fade(edge, a));
            if (!IsWallAt(i, j + 1)) PixelBlock(x + cell_size - 3, y, 3, cell_size, Fade(edge, a));
        }
    }
    if (gamelvl == 3) {
        for (int s = 0; s < sentinelCount; s++) {
            Sentinel* sn = &sentinels[s];
            float cx = sn->col * cell_size + cell_size / 2;
            float cy = sn->row * cell_size + cell_size / 2;
            Color c = (sn->alertTimer > 0) ? RED : (Color){255, 60, 80, 255};
            DrawGlow(cx, cy, 22, c, 0.5f + (sn->alertTimer > 0 ? 0.3f * sinf(time * 12) : 0));
            DrawCircle((int)cx, (int)cy, 8, (Color){20, 0, 10, 255});
            DrawCircleLines((int)cx, (int)cy, 8, c);
            DrawCircle((int)cx, (int)cy, 4, c);
            DrawCircle((int)cx, (int)cy, 2, WHITE);
        }
    }
}
void startdraw() {
    Theme t = themes[gamelvl - 1];
    int x = start_col * cell_size, y = start_row * cell_size;
    int cx = x + cell_size / 2, cy = y + cell_size / 2;
    DrawGlow(cx, cy, 30, t.startAccent, 0.4f);
    PixelBlock(x + 4, y + 4, 22, 22, (Color){15, 8, 30, 255});
    PixelBlock(x + 8, y + 8, 14, 14, t.startAccent);
    PixelBlock(x + 11, y + 11, 8, 8, WHITE);
}
void goaldraw() {
    Theme t = themes[gamelvl - 1];
    float time = (float)GetTime();
    float cx = goal_col * cell_size + cell_size / 2.0f;
    float cy = goal_row * cell_size + cell_size / 2.0f;
    float pulse = (sinf(time * 5.0f) + 1.0f) * 0.5f;
    DrawGlow(cx, cy, 30 + pulse * 8, t.goalColor, 0.55f);

    if (gamelvl == 1) {
        PixelBlock((int)cx - 8, (int)cy - 8, 16, 16, (Color){255, 200, 60, 255});
        PixelBlock((int)cx - 6, (int)cy - 6, 4, 4, (Color){80, 40, 20, 255});
        PixelBlock((int)cx + 2, (int)cy - 6, 4, 4, (Color){80, 40, 20, 255});
    } else if (gamelvl == 2) {
        DrawStarPoly(cx, cy, 6, 12 + pulse * 3, 5, time * 30, 1, 1, t.goalColor);
        DrawStarPoly(cx, cy, 6, 8 + pulse * 2, 3, time * 30, 1, 1, WHITE);
    } else {
        PixelBlock((int)cx - 8, (int)cy - 8, 16, 16, (Color){15, 30, 40, 255});
        PixelBlock((int)cx - 6, (int)cy - 6, 12, 12, t.goalColor);
        PixelBlock((int)cx - 3, (int)cy - 3, 6, 6, WHITE);
    }
    for (int k = 0; k < 4; k++) {
        float a = time * 3 + k * 1.57f;
        int px = (int)(cx + cosf(a) * 14);
        int py = (int)(cy + sinf(a) * 14);
        PixelBlock(px - 2, py - 2, 4, 4, t.neonC);
    }
    if (time < scanRevealUntil) {
        float markerPulse = 0.5f + 0.5f * sinf(time * 6);
        DrawCircleLines((int)cx, (int)cy, 22 + (int)(markerPulse * 4), Fade(t.goalColor, 0.6f));
    }
}

void drawGoalPath() {
    float time = (float)GetTime();
    if (time >= scanRevealUntil) return;
    if (pathLen < 2) return;
    Theme t = themes[gamelvl - 1];
    for (int i = 1; i < pathLen; i++) {
        float pulse = 0.5f + 0.5f * sinf(time * 6 - i * 0.3f);
        float px = pathC[i] * cell_size + cell_size / 2.0f;
        float py = pathR[i] * cell_size + cell_size / 2.0f;
        if (i % 2 == 0) {
            DrawGlow(px, py, 12 + pulse * 4, t.neonB, 0.55f + 0.25f * pulse);
            DrawCircle((int)px, (int)py, 3 + pulse * 1.5f, Fade(WHITE, 0.9f));
            DrawCircle((int)px, (int)py, 5, Fade(t.neonB, 0.6f * pulse));
        } else {
            DrawCircle((int)px, (int)py, 2, Fade(t.neonC, 0.75f));
        }
        if (i > 0) {
            int pr = pathR[i - 1], pc = pathC[i - 1];
            float qx = pc * cell_size + cell_size / 2.0f;
            float qy = pr * cell_size + cell_size / 2.0f;
            DrawLineEx((Vector2){qx, qy}, (Vector2){px, py}, 2, Fade(t.neonB, 0.25f));
        }
    }
    float gx = goal_col * cell_size + cell_size / 2.0f;
    float gy = goal_row * cell_size + cell_size / 2.0f;
    DrawCircleLines((int)gx, (int)gy, 18 + (int)(sinf(time * 8) * 3), Fade(t.neonC, 0.8f));
    if (pathLen >= 2) {
        float px = playerDrawX, py = playerDrawY;
        float nx = pathC[1] * cell_size + cell_size / 2.0f;
        float ny = pathR[1] * cell_size + cell_size / 2.0f;
        float dx = nx - px, dy = ny - py;
        float len = sqrtf(dx * dx + dy * dy);
        if (len > 0.1f) {
            dx /= len; dy /= len;
            float ax = px + dx * 26;
            float ay = py + dy * 26;
            float perpX = -dy, perpY = dx;
            DrawTriangle(
                (Vector2){ax, ay},
                (Vector2){ax - dx * 10 + perpX * 7, ay - dy * 10 + perpY * 7},
                (Vector2){ax - dx * 10 - perpX * 7, ay - dy * 10 - perpY * 7},
                Fade(t.neonC, 0.9f));
        }
    }
}

void DrawMonkey(float cx, float cy, float sx, float sy, bool happy, bool sad, float time) {
    Theme t = themes[0];
    DrawEllipse((int)cx, (int)(cy + 3), 11 * sx, 13 * sy, t.playerColor);
    DrawCircle((int)cx, (int)(cy - 8), 8, t.playerColor);
    DrawCircle((int)(cx - 8), (int)(cy - 8), 3, t.playerColor);
    DrawCircle((int)(cx + 8), (int)(cy - 8), 3, t.playerColor);
    DrawCircle((int)cx, (int)(cy - 6), 6, (Color){230, 200, 160, 255});
    if (happy) {
        DrawRectangle((int)(cx - 4), (int)(cy - 8), 3, 2, BLACK);
        DrawRectangle((int)(cx + 1), (int)(cy - 8), 3, 2, BLACK);
    } else if (sad) {
        DrawCircle((int)(cx - 3), (int)(cy - 7), 1, BLACK);
        DrawCircle((int)(cx + 3), (int)(cy - 7), 1, BLACK);
        DrawLine((int)(cx - 2), (int)(cy - 4), (int)(cx + 2), (int)(cy - 5), BLACK);
    } else {
        DrawCircle((int)(cx - 3), (int)(cy - 7), 1, BLACK);
        DrawCircle((int)(cx + 3), (int)(cy - 7), 1, BLACK);
    }
    float tail = sinf(time * 6) * 6;
    DrawLineEx((Vector2){cx + 10, cy + 6}, (Vector2){cx + 18 + tail, cy + 2}, 3, t.playerAccent);
    DrawLineEx((Vector2){cx + 18 + tail, cy + 2}, (Vector2){cx + 20 + tail * 1.2f, cy - 4}, 3, t.playerAccent);
}
void DrawAstronaut(float cx, float cy, float sx, float sy, bool happy, bool sad, float time) {
    Theme t = themes[1];
    DrawRectangle((int)(cx - 9 * sx), (int)(cy - 4), (int)(18 * sx), (int)(14 * sy), t.playerColor);
    DrawCircle((int)cx, (int)(cy - 8), 10, t.playerColor);
    DrawCircle((int)cx, (int)(cy - 8), 8, t.playerAccent);
    DrawCircle((int)cx, (int)(cy - 8), 6, (Color){10, 30, 60, 255});
    DrawCircle((int)(cx - 2), (int)(cy - 10), 3, Fade(WHITE, 0.5f));
    DrawRectangle((int)(cx + 8), (int)(cy - 3), 6, 10, (Color){120, 120, 140, 255});
    DrawCircle((int)(cx + 11), (int)(cy + 8), 3, Fade((Color){80, 200, 255, 255}, 0.7f + 0.3f * sinf(time * 20)));
    if (!sad) {
        DrawCircle((int)(cx - 3), (int)(cy - 8), 1, happy ? GREEN : WHITE);
        DrawCircle((int)(cx + 3), (int)(cy - 8), 1, happy ? GREEN : WHITE);
    } else {
        DrawLine((int)(cx - 4), (int)(cy - 8), (int)(cx - 2), (int)(cy - 8), RED);
        DrawLine((int)(cx + 2), (int)(cy - 8), (int)(cx + 4), (int)(cy - 8), RED);
    }
}
void DrawNetrunner(float cx, float cy, float sx, float sy, bool happy, bool sad, float time) {
    Theme t = themes[2];
    DrawRectangle((int)(cx - 10 * sx), (int)(cy - 4), (int)(20 * sx), (int)(16 * sy), t.playerColor);
    DrawRectangle((int)(cx - 12 * sx), (int)(cy + 4), 6, 10, t.playerColor);
    DrawRectangle((int)(cx + 6 * sx), (int)(cy + 4), 6, 10, t.playerColor);
    DrawCircle((int)cx, (int)(cy - 8), 7, t.playerColor);
    Color vc = happy ? GREEN : (sad ? RED : t.playerAccent);
    DrawRectangle((int)(cx - 6), (int)(cy - 10), 12, 4, vc);
    DrawGlow(cx, cy - 8, 12, vc, 0.6f + 0.3f * sinf(time * 5));
    DrawRectangle((int)(cx - 6 + fmodf(time * 20, 12)), (int)(cy - 10), 2, 4, WHITE);
    DrawTriangle((Vector2){cx - 3, cy - 14}, (Vector2){cx + 3, cy - 14}, (Vector2){cx, cy - 18}, t.playerAccent);
}
void playerdraw() {
    Theme t = themes[gamelvl - 1];
    float time = (float)GetTime();
    float cx = playerDrawX, cy = playerDrawY;
    bool invuln = time < invulnUntil;
    float phaseT = fmodf(time, 1.0f);
    float pulse = (phaseT < 0.2f) ? sinf(phaseT / 0.2f * PI) * 0.15f : 0.0f;
    float sx = 1.0f, sy = 1.0f;
    float sinceMove = time - lastMoveTime;
    float sinceBump = time - lastBumpTime;
    if (sinceMove < 0.18f) {
        float k = 1 - sinceMove / 0.18f;
        if (lastDirX != 0) { sx += 0.35f * k; sy -= 0.25f * k; }
        else { sy += 0.35f * k; sx -= 0.25f * k; }
    }
    if (sinceBump < 0.25f) {
        float k = 1 - sinceBump / 0.25f;
        if (lastDirX != 0) { sx -= 0.35f * k; sy += 0.25f * k; }
        else { sy -= 0.35f * k; sx += 0.25f * k; }
        cx += lastDirX * 5 * k;
        cy += lastDirY * 5 * k;
    }
    sx *= 1 + pulse; sy *= 1 + pulse;

    if (invuln && ((int)(time * 20)) % 2 == 0) return;

    if (time < dashActiveUntil) {
        float dk = (dashActiveUntil - time) / 0.35f;
        DrawGlow(cx, cy, 30 + dk * 40, t.neonA, 0.6f * dk);
    }
    DrawGlow(cx, cy, 30 + pulse * 40, t.playerAccent, 0.5f + pulse);

    bool happy = time < happyUntil;
    bool sad = time < sadUntil && !happy;

    if (gamelvl == 1) DrawMonkey(cx, cy, sx, sy, happy, sad, time);
    else if (gamelvl == 2) DrawAstronaut(cx, cy, sx, sy, happy, sad, time);
    else DrawNetrunner(cx, cy, sx, sy, happy, sad, time);
}
void drawEchoGhost() {
    if (!echoGhostActive) return;
    Theme t = themes[gamelvl - 1];
    float time = (float)GetTime();
    float cx = echoGhostPos.x, cy = echoGhostPos.y;
    float pulse = 0.5f + 0.5f * sinf(time * 6);
    DrawGlow(cx, cy, 30 + pulse * 8, t.echoColor, 0.35f + pulse * 0.15f);
    DrawCircleLines((int)cx, (int)cy, 15, Fade(t.echoColor, 0.7f));
    DrawCircleLines((int)cx, (int)cy, 18 + (int)(pulse * 3), Fade(t.echoColor, 0.4f));
    DrawStarPoly(cx, cy, 4, 13, 4, time * 60, 1, 1, Fade(t.echoColor, 0.5f));
    int px = SnapX(cx), py = SnapY(cy);
    DrawRectangle(px - 5, py - 3, 3, 3, Fade(BLACK, 0.7f));
    DrawRectangle(px + 2, py - 3, 3, 3, Fade(BLACK, 0.7f));
    DrawText("ECHO", (int)cx - 14, (int)cy - 30, 10, Fade(t.echoColor, 0.6f));
}
void drawTrail() {
    float time = (float)GetTime();
    for (int k = TRAIL_POINTS - 1; k >= 1; k--) {
        float a = 1.0f - (float)k / TRAIL_POINTS;
        int px = SnapX(trailPoints[k].x);
        int py = SnapY(trailPoints[k].y);
        Color c = Rainbow(time * 300 + k * 15, 0.7f, 0.45f * a);
        DrawRectangle(px - PX_PIXEL, py - PX_PIXEL, PX_PIXEL * 2, PX_PIXEL * 2, c);
    }
}
void drawmaze() {
    drawBackground();
    drawPaths();
    drawWalls();
}

void drawScreenEffects() {
    Theme t = themes[gamelvl - 1];
    float time = (float)GetTime();
    int W = SCREEN_W, H = SCREEN_W;

    DrawScanlines(Fade(BLACK, 0.12f));

    if (phaseFlipAnim > 0) {
        float k = 1 - phaseFlipAnim;
        float radius = k * 900;
        Color pc = t.neonC;
        for (int ring = 0; ring < 3; ring++) {
            float rr = radius - ring * 30;
            if (rr > 0) DrawCircleLines(W/2, H/2, (int)rr, Fade(pc, (0.5f - ring * 0.15f) * phaseFlipAnim));
        }
    }

    if (gamelvl == 1) {
        for (int k = 0; k < 2; k++) {
            unsigned int h = CellHash(k + 7, (int)(time * 0.5f));
            if ((h % 100) < 6) {
                int rx = (h * 13) % W, ry = (h * 7) % H;
                if (IsWallAt(ry / cell_size, rx / cell_size)) {
                    float pulse = 0.5f + 0.5f * sinf(time * 4);
                    DrawRectangleLines(rx - 12, ry - 12, 24, 24, Fade(t.neonC, pulse));
                    DrawRectangle(rx - 3, ry - 3, 6, 6, Fade(t.neonC, pulse));
                }
            }
        }
    } else if (gamelvl == 2) {
        static float lastShoot = 0;
        if (time - lastShoot > RandF(1.5f, 3.5f)) {
            lastShoot = time;
            float sx = RandF(-100, W);
            float sy = RandF(-50, 200);
            float sp = RandF(600, 900);
            SpawnParticle(PT_SHOOTING_STAR, sx, sy, sp, sp * 0.6f, 0, 1.2f, 3, (Color){255, 255, 220, 255});
        }
    } else {
        if (glitchTime > 0 || (RandF(0, 1) < 0.01f)) {
            for (int k = 0; k < 3; k++) {
                int gy = GetRandomValue(0, H);
                int gh = GetRandomValue(2, 10);
                int gox = GetRandomValue(-10, 10);
                DrawRectangle(gox, gy, W, gh, Fade(t.neonB, 0.25f));
                DrawRectangle(gox + 2, gy + 1, W, 1, Fade(WHITE, 0.15f));
            }
        }
        for (int y = 0; y < H; y += 3) DrawRectangle(0, y, W, 1, Fade(BLACK, 0.15f));
    }

    if (lightFlash > 0) DrawRectangle(0, 0, W, SCREEN_H, Fade(lightFlashColor, lightFlash * 0.3f));
    if (redFlash > 0) {
        DrawRectangle(0, 0, W, SCREEN_H, Fade(RED, redFlash * 0.25f));
        DrawRectangleGradientV(0, 0, W, 80, Fade(RED, redFlash * 0.5f), Fade(RED, 0));
    }
    if (paradoxFlash > 0) {
        DrawRectangle(0, 0, W, SCREEN_H, Fade(paradoxColor, paradoxFlash * 0.4f));
    }
    if (damageFlash > 0) {
        DrawRectangle(0, 0, W, SCREEN_H, Fade(RED, damageFlash * 0.35f));
        DrawRectangleGradientV(0, 0, W, 60, Fade(RED, damageFlash * 0.6f), Fade(RED, 0));
        DrawRectangleGradientV(0, H - 60, W, 60, Fade(RED, 0), Fade(RED, damageFlash * 0.6f));
    }

    if (gameState == PLAYING && timeRemaining < 10) {
        float p = 0.5f + 0.5f * sinf(time * 8);
        DrawRectangleGradientV(0, 0, W, 100, Fade(RED, 0.3f * p), Fade(RED, 0));
        DrawRectangleGradientV(0, H - 100, W, 100, Fade(RED, 0), Fade(RED, 0.3f * p));
    }

    if (scanPulseTime >= 0) {
        float dur = 0.9f;
        if (scanPulseTime > dur) scanPulseTime = -1;
        else {
            float k = scanPulseTime / dur;
            float radius = k * (SCAN_RADIUS * cell_size);
            float a = (1 - k) * 0.7f;
            Color sc = themes[gamelvl - 1].neonB;
            DrawCircleLines((int)scanPulseX, (int)scanPulseY, (int)radius, Fade(sc, a));
            DrawCircleLines((int)scanPulseX, (int)scanPulseY, (int)(radius * 0.9f), Fade(sc, a * 0.5f));
        }
    }

    if (gamelvl == 1 && gameState == PLAYING) {
        float vig = 1.0f - torchLight;
        int radius = (int)(SCREEN_W * 0.65f * (1 - vig * 0.7f));
        int cx = (int)playerDrawX;
        int cy = (int)playerDrawY;
        float alphaMax = vig * 0.9f;
        DrawRectangleGradientV(0, 0, W, cy - radius, Fade(BLACK, alphaMax), Fade(BLACK, alphaMax * 0.3f));
        DrawRectangleGradientV(0, cy + radius, W, H - cy - radius, Fade(BLACK, alphaMax * 0.3f), Fade(BLACK, alphaMax));
        DrawRectangleGradientH(0, 0, cx - radius, H, Fade(BLACK, alphaMax), Fade(BLACK, alphaMax * 0.3f));
        DrawRectangleGradientH(cx + radius, 0, W - cx - radius, H, Fade(BLACK, alphaMax * 0.3f), Fade(BLACK, alphaMax));
    }
}

void SpawnAmbientParticles(float dt) {
    Theme th = themes[gamelvl - 1];
    bool inMenu = (gameState == MENU || gameState == MISSION_SELECT ||
                   gameState == HOW_TO_PLAY || gameState == LEADERBOARD || gameState == CREDITS);
    bool inChat = (gameState == MISSION_BRIEF || gameState == MISSION_DEBRIEF);
    float rate = 12;
    if (inMenu || inChat) rate = 8;
    if (gameState == PLAYING && timeRemaining < 20) rate = 45;
    ambientTimer += dt * rate;
    while (ambientTimer >= 1) {
        ambientTimer -= 1;
        if (CountParticles() > 240) continue;
        if (inMenu || inChat) {
            SpawnParticle(PT_DOT, RandF(0, SCREEN_W), RandF(0, SCREEN_H),
                          RandF(-10, 10), RandF(-30, -10), 0, RandF(1.5f, 3),
                          PX_PIXEL, Rainbow(RandF(0, 360), 0.5f, 1));
        } else if (gamelvl == 1) {
            if (GetRandomValue(0, 9) < 6) {
                SpawnParticle(PT_LEAF, RandF(0, SCREEN_W), -20, 0, RandF(30, 60), 0,
                              RandF(4, 7), RandF(6, 12),
                              GetRandomValue(0, 1) ? (Color){80, 130, 60, 255} : (Color){110, 90, 40, 255});
            } else {
                SpawnParticle(PT_FIREFLY, RandF(0, SCREEN_W), RandF(0, SCREEN_W),
                              0, 0, 0, RandF(2, 4), RandF(3, 5), th.neonA);
            }
        } else if (gamelvl == 2) {
            SpawnParticle(PT_DOT, RandF(0, SCREEN_W), RandF(0, SCREEN_W),
                          RandF(-5, 5), RandF(-15, -5), 0, RandF(2, 4),
                          RandF(2, 4), Fade(th.neonA, 0.7f));
        } else {
            SpawnParticle(PT_RAIN, RandF(0, SCREEN_W + 100), -10, -80, RandF(500, 700),
                          0, 2.0f, 1, Fade(th.neonA, 0.6f));
        }
    }
}

void DrawHeart(int x, int y, bool full) {
    Color c = full ? (Color){240, 60, 90, 255} : (Color){70, 30, 40, 255};
    DrawCircle(x - 3, y, 4, c);
    DrawCircle(x + 3, y, 4, c);
    DrawTriangle((Vector2){(float)(x - 7), (float)y + 1},
                 (Vector2){(float)(x + 7), (float)y + 1},
                 (Vector2){(float)x, (float)y + 10}, c);
}
void DrawCooldownBar(const char* label, int x, int y, float cooldown, float maxCooldown,
                     Color readyColor, Color coolingColor, bool usable) {
    int barW = 90, barH = 8;
    DrawText(label, x, y - 2, 12, usable ? readyColor : coolingColor);
    DrawRectangle(x + 42, y, barW, barH, (Color){40, 30, 55, 255});
    DrawRectangleLines(x + 42, y, barW, barH, BLACK);
    float fill = 1.0f - Clamp01(cooldown / maxCooldown);
    if (fill > 0) {
        int fillW = (int)(barW * fill);
        DrawRectangle(x + 42, y, fillW, barH, usable ? readyColor : coolingColor);
    }
    if (usable && ((int)(GetTime() * 6)) % 2 == 0)
        DrawRectangle(x + 42, y, barW, barH, Fade(WHITE, 0.4f));
}
void drawDisplay() {
    int Y = SCREEN_W;
    Theme t = themes[gamelvl - 1];
    float time = (float)GetTime();
    DrawRectangleGradientV(0, Y, SCREEN_W, SCREEN_OFFSET, t.uiBackground, MixColor(t.uiBackground, BLACK, 0.5f));

    if (gamelvl == 1) for (int k = 0; k < 30; k++) PixelBlock(k * 30, Y, 30, 3, Fade(themes[0].neonA, 0.6f));
    else if (gamelvl == 2) for (int k = 0; k < 30; k++) PixelBlock(k * 30, Y, 30, 3, MixColor(t.neonA, t.neonB, k / 30.0f));
    else for (int k = 0; k < 30; k++) {
        int pick = (k + (int)(time * 8)) % 3;
        Color c = (pick == 0) ? t.neonA : (pick == 1 ? t.neonB : t.neonC);
        PixelBlock(k * 30, Y, 30, 3, c);
    }

    Color phaseCol = (phase == 0) ? t.neonA : t.neonB;
    const char* phaseName = (phase == 0) ? "A" : "B";
    DrawTextGlow(TextFormat("M%d [%s]", gamelvl, phaseName), 15, Y + 14, 18, phaseCol, phaseCol);
    DrawText(TextFormat("@%s", playerName), 15, Y + 46, 16, t.textColor);

    DrawText("LIVES", 95, Y + 14, 14, t.textColor);
    for (int k = 0; k < maxLives; k++) DrawHeart(160 + k * 22, Y + 22, k < lives);

    DrawTextGlow(TextFormat("SCORE %d", (int)(displayScore + 0.5f)), 290, Y + 12, 22, GOLD, ORANGE);
    DrawText(TextFormat("BEST %d", GetHighScore()), 290, Y + 46, 16, t.textColor);

    float coinSpin = cosf(time * 3);
    if (fabsf(coinSpin) < 0.15f) coinSpin = 0.15f;
    DrawItemIcon(ITEM_COIN, 480, Y + 24, coinSpin, time, gamelvl - 1);
    DrawText(TextFormat("%d/%d", itemsCollected, itemCount), 498, Y + 14, 18, t.textColor);

    if (combo >= 2) {
        Color rb = Rainbow(time * 300, 0.6f, 1);
        DrawText(TextFormat("x%d", combo), 570, Y + 14, 20, rb);
    }

    Color tc = t.textColor;
    if (timeRemaining < 15) tc = RED;
    else if (timeRemaining < 30) tc = ORANGE;
    int ts = 22;
    if (timeRemaining < 10) ts = 22 + (int)(fabsf(sinf(time * 8)) * 4);
    DrawTextGlow(TextFormat("TIME %.1f", timeRemaining), 640, Y + 12, ts, tc, tc);
    DrawText(musicMuted ? "[M] OFF" : "[M] ON", 640, Y + 46, 16, t.textColor);

    bool dr = dashCooldown <= 0;
    bool sr = scanCooldown <= 0;
    bool pr = phaseCooldown <= 0;
    DrawCooldownBar("SHIFT", 15, Y + 72, dashCooldown, DASH_COOLDOWN, t.neonA, (Color){80, 80, 120, 255}, dr);
    DrawCooldownBar("[F]  ", 175, Y + 72, scanCooldown, SCAN_COOLDOWN, t.neonB, (Color){80, 80, 120, 255}, sr);
    DrawCooldownBar("[E]  ", 335, Y + 72, phaseCooldown, PHASE_COOLDOWN, t.neonC, (Color){80, 80, 120, 255}, pr);

    if (gamelvl == 1) {
        DrawText("TORCH", 500, Y + 72, 12, torchLight > 0.3f ? t.neonA : RED);
        DrawRectangle(545, Y + 74, 100, 8, (Color){40, 30, 55, 255});
        DrawRectangle(545, Y + 74, (int)(100 * torchLight), 8, torchLight > 0.3f ? t.neonA : RED);
    } else if (gamelvl == 2) {
        if (zeroGActive) DrawText("ZERO-G!", 500, Y + 72, 14, t.neonB);
    } else {
        DrawText(TextFormat("SENTINELS %d", sentinelCount), 500, Y + 72, 12, t.neonB);
    }
}

void drawMenuBackground() {
    float time = (float)GetTime();
    DrawRectangleGradientV(0, 0, SCREEN_W, SCREEN_H, (Color){8, 4, 20, 255}, (Color){30, 8, 50, 255});
    for (int y = 0; y < SCREEN_H; y += 30)
        for (int x = 0; x < SCREEN_W; x += 30) {
            unsigned int h = CellHash(x, y);
            if (h % 5 == 0) PixelBlock(x, y, PX_PIXEL, PX_PIXEL, Fade((Color){0, 255, 180, 255}, 0.12f));
        }
    for (int k = 0; k < 5; k++) {
        float x = 120 + k * 180 + sinf(time * 0.4f + k * 1.3f) * 120;
        float y = 250 + cosf(time * 0.35f + k * 1.7f) * 280;
        DrawGlow(x, y, 280, Rainbow(k * 72 + time * 15, 0.7f, 1), 0.22f);
    }
    int horizon = SCREEN_H - 220;
    for (int k = 0; k < 12; k++) {
        float f = fmodf(k / 12.0f + time * 0.1f, 1.0f);
        int y = horizon + (int)(f * f * (SCREEN_H - horizon));
        DrawRectangle(0, y, SCREEN_W, 1, Fade((Color){255, 60, 200, 255}, 0.15f + 0.5f * f));
    }
    for (int k = -12; k <= 12; k++)
        DrawRectangle(SCREEN_W/2 + k * 8, horizon, 1, SCREEN_H - horizon, Fade((Color){255, 60, 200, 255}, 0.2f));
}
void drawMascot(float centerX, float centerY, float hoverState) {
    float time = (float)GetTime();
    float bob = sinf(time * 2.5f) * 4;
    float scale = 1.0f + hoverState * 0.15f;
    centerY += bob - (hoverState > 0.5f ? fabsf(sinf(time * 8)) * 4 : 0);
    DrawGlow(centerX, centerY, 70 * scale, (Color){0, 220, 255, 255}, 0.35f + hoverState * 0.3f);
    int baseSize = (int)(50 * scale);
    PixelBlock((int)centerX - baseSize/2, (int)centerY - baseSize/2, baseSize, baseSize, (Color){20, 35, 60, 255});
    PixelBlock((int)centerX - baseSize/2 + 6, (int)centerY - baseSize/2 + 6, baseSize - 12, baseSize - 12, (Color){40, 70, 110, 255});
    PixelBlock((int)centerX - baseSize/2 + 10, (int)centerY - 6, baseSize - 20, 14, (Color){0, 20, 30, 255});
    float visorPulse = 0.6f + 0.4f * sinf(time * 4);
    PixelBlock((int)centerX - baseSize/2 + 12, (int)centerY - 4, baseSize - 24, 10, Fade((Color){0, 220, 255, 255}, visorPulse));
    PixelBlock((int)centerX - baseSize/2 + 16, (int)centerY - 2, 8, 4, WHITE);
    int antX = (int)centerX, antY = (int)centerY - baseSize/2;
    DrawRectangle(antX - 1, antY - 10, 3, 10, (Color){180, 180, 200, 255});
    DrawCircle(antX, antY - 12, 3, (Color){255, 80, 80, 255});
    DrawGlow(antX, antY - 12, 8, (Color){255, 80, 80, 255}, 0.6f + 0.4f * sinf(time * 6));
    for (int k = 0; k < 3; k++) {
        bool on = ((int)(time * 4) + k) % 3 == 0;
        PixelBlock((int)centerX - baseSize/2 + 10 + k * 8, (int)centerY + baseSize/2 - 8, 4, 4,
                   on ? (Color){0, 255, 180, 255} : (Color){40, 80, 60, 255});
    }
    if (hoverState > 0.3f) {
        for (int k = 0; k < 3; k++) {
            float a = time * 4 + k * 2.09f;
            float sxp = centerX + cosf(a) * (50 + hoverState * 20);
            float syp = centerY + sinf(a) * (50 + hoverState * 20);
            DrawSparkle(sxp, syp, 5 + hoverState * 3, time * 120, (Color){255, 240, 100, 255});
        }
    }
}
bool ArcadeButton(const char* text, int x, int y, int w, int h, Color base, float* outHover) {
    Rectangle r = {x, y, w, h};
    bool hover = CheckCollisionPointRec(GetMousePosition(), r);
    float time = (float)GetTime();
    static float scales[32] = {1};
    static int initFlag = 0;
    if (!initFlag) { for (int i = 0; i < 32; i++) scales[i] = 1.0f; initFlag = 1; }
    int idx = (x + y * 7) % 32;
    float target = hover ? 1.05f : 1.0f;
    scales[idx] += (target - scales[idx]) * fminf(1.0f, GetFrameTime() * 12);
    if (outHover) *outHover = (scales[idx] - 1.0f) / 0.05f;
    int sw = (int)(w * scales[idx]);
    int sh = (int)(h * scales[idx]);
    int sx = x - (sw - w) / 2, sy = y - (sh - h) / 2;
    if (hover) DrawGlow(sx + sw/2, sy + sh/2, sw * 0.6f, base, 0.5f);
    DrawRectangle(sx - 3, sy - 3, sw + 6, sh + 6, BLACK);
    DrawRectangle(sx, sy, sw, sh, hover ? base : MixColor(base, BLACK, 0.5f));
    Color br = hover ? WHITE : Fade(WHITE, 0.75f);
    DrawRectangle(sx, sy, 6, 3, br); DrawRectangle(sx, sy, 3, 6, br);
    DrawRectangle(sx + sw - 6, sy, 6, 3, br); DrawRectangle(sx + sw - 3, sy, 3, 6, br);
    DrawRectangle(sx, sy + sh - 3, 6, 3, br); DrawRectangle(sx, sy + sh - 6, 3, 6, br);
    DrawRectangle(sx + sw - 6, sy + sh - 3, 6, 3, br); DrawRectangle(sx + sw - 3, sy + sh - 6, 3, 6, br);
    int size = 22;
    if (hover) size = 22 + (int)(sinf(time * 8) * 2);
    int tx = sx + (sw - MeasureText(text, size)) / 2;
    int ty = sy + (sh - size) / 2;
    DrawText(text, tx + 2, ty + 2, size, Fade(BLACK, 0.7f));
    DrawText(text, tx, ty, size, WHITE);
    if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        for (int k = 0; k < 12; k++) {
            float a = RandF(0, 2 * PI);
            SpawnParticle(PT_SPARKLE, sx + sw/2 + cosf(a) * 20, sy + sh/2 + sinf(a) * 20,
                          cosf(a) * 200, sinf(a) * 200, 0, 0.5f, 5, base);
        }
        return true;
    }
    return false;
}
void drawNameBox(int y) {
    int w = 340, h = 42;
    int x = (SCREEN_W - w) / 2;
    Rectangle box = {x, y, w, h};
    bool hover = CheckCollisionPointRec(GetMousePosition(), box);
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (hover && !editingName) {
            strcpy(nameBackup, playerName);
            playerName[0] = '\0';
            editingName = true;
        } else if (!hover && editingName) {
            if (strlen(playerName) == 0) strcpy(playerName, nameBackup);
            editingName = false;
            SaveExtras();
        }
    }
    Color border = editingName ? (Color){255, 0, 200, 255} : (hover ? WHITE : (Color){120, 200, 160, 255});
    DrawRectangle(x - 2, y - 2, w + 4, h + 4, border);
    DrawRectangle(x, y, w, h, (Color){15, 10, 30, 255});
    DrawText("AGENT:", x + 14, y + 12, 20, (Color){120, 220, 180, 255});
    DrawText(playerName, x + 105, y + 11, 22, WHITE);
    if (editingName) {
        if (((int)(GetTime() * 2)) % 2 == 0)
            DrawRectangle(x + 107 + MeasureText(playerName, 22), y + 10, 3, 22, WHITE);
        DrawTextCentered("TYPE NAME - ENTER = SAVE", y + h + 6, 14, LIGHTGRAY);
    }
}
void UpdateNameTyping() {
    int ch = GetCharPressed();
    while (ch > 0) {
        int len = (int)strlen(playerName);
        bool allowed = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') ||
                       (ch >= '0' && ch <= '9') || ch == '_' || ch == '-';
        if (allowed && len < 12) {
            if (ch >= 'a' && ch <= 'z') ch = ch - 'a' + 'A';
            playerName[len] = (char)ch;
            playerName[len + 1] = '\0';
        }
        ch = GetCharPressed();
    }
    if (IsKeyPressed(KEY_BACKSPACE) && strlen(playerName) > 0) playerName[strlen(playerName) - 1] = '\0';
    if (IsKeyPressed(KEY_ESCAPE)) strcpy(playerName, nameBackup);
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_ESCAPE)) {
        if (strlen(playerName) == 0) strcpy(playerName, nameBackup);
        editingName = false;
        SaveExtras();
    }
}
void drawMenu() {
    drawMenuBackground();
    DrawParticles();
    float time = (float)GetTime();
    const char* title = "ECHO GRID";
    int titleSize = 90;
    int tw = MeasureText(title, titleSize);
    int tx = (SCREEN_W - tw) / 2;
    int ty = 30 + (int)(sinf(time * 1.8f) * 3);
    for (int d = 10; d >= 1; d--) {
        Color sc = MixColor((Color){40, 0, 70, 255}, (Color){255, 0, 180, 255}, (10 - d) / 10.0f);
        DrawText(title, tx + d, ty + d, titleSize, Fade(sc, 0.85f));
    }
    for (int g = 0; g < 4; g++) {
        DrawText(title, tx - 2 - g, ty, titleSize, Fade((Color){0, 255, 220, 255}, 0.15f));
        DrawText(title, tx + 2 + g, ty, titleSize, Fade((Color){0, 255, 220, 255}, 0.15f));
    }
    Color titlePulse = MixColor(WHITE, (Color){255, 240, 100, 255}, 0.5f + 0.5f * sinf(time * 3));
    DrawText(title, tx, ty, titleSize, titlePulse);
    if (fmodf(time, 0.9f) < 0.55f)
        DrawRectangle(tx + tw + 12, ty + 15, 30, 55, (Color){0, 255, 180, 255});
    bool blinkText = fmodf(time, 1.4f) < 1.0f;
    if (blinkText)
        DrawTextCenteredGlow(">> PARADOX RUNNER <<", 128, 18, (Color){0, 255, 180, 255}, (Color){0, 255, 180, 255});
    else
        DrawTextCentered(">> . . . . . . . <<", 128, 18, Fade((Color){0, 255, 180, 255}, 0.4f));
    DrawTextCentered(TextFormat("HIGH SCORE: %d", GetHighScore()), 155, 18, GOLD);
    drawNameBox(180);
    drawMascot(130, 480, mascotHappy);

    int bx = (SCREEN_W - 260) / 2;
    float hoverTmp = 0;
    Vector2 mouse = GetMousePosition();
    bool anyHover = false;
    for (int k = 0; k < 6; k++) {
        Rectangle r = {bx, 250 + k * 60, 260, 48};
        if (CheckCollisionPointRec(mouse, r)) { anyHover = true; break; }
    }
    mascotHappy += ((anyHover ? 1.0f : 0.0f) - mascotHappy) * fminf(1.0f, GetFrameTime() * 8);

    if (ArcadeButton("START MISSIONS", bx, 250, 260, 48, (Color){40, 200, 120, 255}, &hoverTmp)) {
        StopSound(introSound); PlaySound(moveSound); gameState = MISSION_SELECT;
    }
    if (ArcadeButton("HOW TO PLAY", bx, 310, 260, 48, (Color){60, 140, 255, 255}, &hoverTmp)) {
        StopSound(introSound); PlaySound(moveSound); gameState = HOW_TO_PLAY;
    }
    if (ArcadeButton("LEADERBOARD", bx, 370, 260, 48, (Color){230, 160, 40, 255}, &hoverTmp)) {
        StopSound(introSound); PlaySound(moveSound); gameState = LEADERBOARD;
    }
    Color mc = musicMuted ? (Color){200, 60, 80, 255} : (Color){40, 180, 100, 255};
    if (ArcadeButton(musicMuted ? "MUSIC: OFF" : "MUSIC: ON", bx, 430, 260, 48, mc, &hoverTmp)) {
        StopSound(introSound); PlaySound(moveSound); ToggleMusic();
    }
    if (ArcadeButton("CREDITS", bx, 490, 260, 48, (Color){160, 90, 230, 255}, &hoverTmp)) {
        StopSound(introSound); PlaySound(moveSound); gameState = CREDITS;
    }
    if (ArcadeButton("DISCONNECT", bx, 550, 260, 48, (Color){230, 60, 80, 255}, &hoverTmp)) {
        StopSound(introSound); exitGame = true;
    }

    const char* tips[4] = {
        "> 3 LIVES. ECHO GHOST hunts you 8s behind.",
        "> E flips PHASE. Walls become paths.",
        "> SHIFT DASH. F SCAN reveals the route.",
        "> reach the goal before the timer ends"
    };
    DrawTextCentered(tips[((int)(time / 3.5f)) % 4], 650, 16, Fade(LIGHTGRAY, 0.9f));

    DrawRectangle(0, SCREEN_H - 40, SCREEN_W, 40, (Color){10, 4, 20, 255});
    for (int k = 0; k < 40; k++)
        PixelBlock(k * 24, SCREEN_H - 42, 12, 2, Fade((Color){0, 255, 180, 255}, 0.5f));
    DrawTextCentered("// ECHO GRID ONLINE - AWAITING AGENT", SCREEN_H - 28, 18, (Color){0, 255, 180, 255});
}
void drawMissionSelect() {
    drawMenuBackground();
    DrawParticles();
    float time = (float)GetTime();
    const char* title = "MISSION SELECT";
    int tw = MeasureText(title, 45);
    DrawText(title, (SCREEN_W - tw) / 2, 30, 45, WHITE);
    DrawTextCentered("> choose your target <", 88, 20, (Color){0, 255, 180, 255});
    for (int k = 0; k < 3; k++) {
        Theme th = themes[k];
        int w = 250, h = 400;
        int x = 45 + k * 280;
        int y = 140;
        bool hover = CheckCollisionPointRec(GetMousePosition(), (Rectangle){x, y, w, h});
        int cy = hover ? y - 6 : y;
        if (hover) DrawGlow(x + w / 2.0f, cy + h / 2.0f, 230, th.neonA, 0.4f);
        DrawRectangle(x - 4, cy - 4, w + 8, h + 8, BLACK);
        DrawRectangle(x, cy, w, h, th.uiBackground);
        DrawRectangleLinesEx((Rectangle){x, cy, w, h}, hover ? 4 : 3, hover ? WHITE : th.neonA);
        DrawRectangle(x, cy, 6, 6, th.neonA); DrawRectangle(x + w - 6, cy, 6, 6, th.neonA);
        DrawRectangle(x, cy + h - 6, 6, 6, th.neonA); DrawRectangle(x + w - 6, cy + h - 6, 6, 6, th.neonA);
        int mini = 6;
        int mx = x + (w - MAZE_SIZE * mini) / 2;
        int my = cy + 18;
        DrawRectangleGradientV(mx, my, MAZE_SIZE * mini, MAZE_SIZE * mini, th.bgTop, th.bgBottom);
        for (int i = 0; i < MAZE_SIZE; i++)
            for (int j = 0; j < MAZE_SIZE; j++) {
                Color c;
                if (maze_A[i][j] == 1) c = th.wallColor;
                else c = Fade(th.neonA, 0.35f);
                DrawRectangle(mx + j * mini, my + i * mini, mini, mini, c);
            }
        DrawRectangleLines(mx - 1, my - 1, MAZE_SIZE * mini + 2, MAZE_SIZE * mini + 2, th.neonB);
        DrawTextInBox(TextFormat("MISSION %02d", k + 1), x, w, cy + 220, 22, WHITE);
        DrawTextInBox(th.name, x, w, cy + 250, 14, th.neonA);
        DrawTextInBox(th.description, x, w, cy + 275, 12, LIGHTGRAY);
        DrawTextInBox("3 LIVES / ECHO GHOST", x, w, cy + 300, 12, th.neonC);
        if (bestTimes[k] > 0) DrawTextInBox(TextFormat("BEST: %.1fs", bestTimes[k]), x, w, cy + 322, 14, GOLD);
        else DrawTextInBox("BEST: --", x, w, cy + 322, 14, GOLD);
        if (hover) DrawTextInBox("> BEGIN MISSION <", x, w, cy + 348 + (int)(sinf(time * 8) * 2), 16, th.neonA);
        if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) StartLevel(k + 1);
    }
    DrawTextCentered("Each mission is independent", 580, 18, GOLD);
    DrawTextCentered("Press ESC to return", 608, 18, LIGHTGRAY);
}
void drawHowToPlay() {
    drawMenuBackground();
    DrawParticles();
    const char* title = "OPERATOR MANUAL";
    int tw = MeasureText(title, 45);
    DrawText(title, (SCREEN_W - tw) / 2, 30, 45, WHITE);
    int lx = 60, rx = 480;
    DrawTextGlow("CONTROLS", lx, 105, 22, SKYBLUE, SKYBLUE);
    DrawText("WASD / ARROWS  Move", lx, 138, 16, WHITE);
    DrawText("SHIFT          Dash (2 cells)", lx, 160, 16, (Color){0, 255, 255, 255});
    DrawText("F              Scan radar (reveals route)", lx, 182, 16, (Color){255, 100, 255, 255});
    DrawText("E              Phase Flip", lx, 204, 16, (Color){255, 240, 0, 255});
    DrawText("R              Restart", lx, 226, 16, WHITE);
    DrawText("M              Music on/off", lx, 248, 16, WHITE);
    DrawText("ESC            Menu", lx, 270, 16, WHITE);
    DrawTextGlow("SIGNATURE MECHANICS", rx, 105, 22, GOLD, GOLD);
    DrawText("ECHO GHOST", rx, 138, 16, (Color){100, 240, 160, 255});
    DrawText("  Your last 8s replays behind you.", rx, 158, 14, WHITE);
    DrawText("  Touch it -> PARADOX (-1 LIFE)", rx, 176, 14, (Color){255, 100, 100, 255});
    DrawText("PHASE FLIP", rx, 200, 16, (Color){255, 240, 0, 255});
    DrawText("  E swaps between dual maze layers.", rx, 220, 14, WHITE);
    DrawText("DASH + SCAN", rx, 244, 16, (Color){0, 255, 255, 255});
    DrawText("  Burst 2 cells / reveal goal route.", rx, 264, 14, WHITE);
    DrawTextCenteredGlow("LIVES", 305, 22, (Color){240, 80, 120, 255}, (Color){240, 80, 120, 255});
    DrawTextCentered("3 hearts per mission.", 335, 16, WHITE);
    DrawTextCentered("Damage (paradox, boulder, sentinel) = -1 life.", 358, 14, LIGHTGRAY);
    DrawTextCentered("Invulnerability flash after hit.", 378, 14, LIGHTGRAY);
    DrawTextCenteredGlow("THEME MECHANICS", 410, 22, (Color){255, 200, 100, 255}, GOLD);
    DrawTextCentered("JUNGLE  - Torchlight fades + boulders roll.", 440, 14, themes[0].neonA);
    DrawTextCentered("COSMIC  - Zero-G drift + wormhole warp.", 462, 14, themes[1].neonA);
    DrawTextCentered("CYBER   - Sentinels patrol + phase locks.", 484, 14, themes[2].neonA);
    DrawTextCenteredGlow("Tip: Press F to light up the path to the goal.", 530, 18, GOLD, GOLD);
    DrawTextCentered("Press SPACE or ESC to return", SCREEN_H - 40, 20, GOLD);
}
void drawLeaderboard() {
    drawMenuBackground();
    DrawParticles();
    int centerX = SCREEN_W / 2;
    const char* title = "TOP AGENTS";
    int tw = MeasureText(title, 50);
    DrawText(title, (SCREEN_W - tw) / 2, 40, 50, GOLD);
    DrawTextCentered("> rank by score <", 105, 20, LIGHTGRAY);
    DrawRectangle(centerX - 250, 150, 500, 45, (Color){40, 20, 60, 255});
    DrawRectangleLines(centerX - 250, 150, 500, 45, GOLD);
    DrawText("RANK", centerX - 220, 162, 22, GOLD);
    DrawText("SCORE", centerX + 100, 162, 22, GOLD);
    Color rankColors[5] = {GOLD, LIGHTGRAY, (Color){205, 127, 50, 255}, WHITE, WHITE};
    for (int i = 0; i < MAX_SCORES; i++) {
        int y = 205 + i * 58;
        if (i == 0) DrawGlow(centerX, y + 25, 260, GOLD, 0.3f);
        DrawRectangle(centerX - 250, y, 500, 50, (Color){25, 15, 45, 255});
        DrawRectangleLines(centerX - 250, y, 500, 50, rankColors[i]);
        DrawText(TextFormat("#%d", i + 1), centerX - 220, y + 15, 22, rankColors[i]);
        if (highScores[i] > 0) DrawText(TextFormat("%d", highScores[i]), centerX + 100, y + 15, 22, WHITE);
        else DrawText("---", centerX + 100, y + 15, 22, DARKGRAY);
    }
    DrawTextCentered("Press SPACE or ESC to return", SCREEN_H - 40, 20, GOLD);
}
void DrawLink(const char* url, int y, int fontSize) {
    int width = MeasureText(url, fontSize);
    int x = (SCREEN_W - width) / 2;
    Rectangle linkBox = {x, y, width, fontSize};
    bool hover = CheckCollisionPointRec(GetMousePosition(), linkBox);
    Color linkColor = hover ? YELLOW : SKYBLUE;
    DrawText(url, x, y, fontSize, linkColor);
    DrawRectangle(x, y + fontSize + 2, width, 2, linkColor);
    if (hover && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) OpenURL(url);
}
void drawCredits() {
    drawMenuBackground();
    DrawParticles();
    int centerX = SCREEN_W / 2;
    const char* title = "CREDITS";
    int tw = MeasureText(title, 50);
    DrawText(title, (SCREEN_W - tw) / 2, 40, 50, WHITE);
    DrawTextCentered("> acknowledgements <", 100, 20, LIGHTGRAY);
    DrawRectangle(centerX - 300, 160, 600, 130, (Color){25, 15, 45, 255});
    DrawRectangleLines(centerX - 300, 160, 600, 130, GOLD);
    DrawTextCentered("PIXABAY", 178, 32, WHITE);
    DrawTextCentered("Free sound effects", 215, 18, LIGHTGRAY);
    DrawLink("https://pixabay.com/sound-effects/search/game/", 245, 20);
    DrawRectangle(centerX - 300, 320, 600, 130, (Color){25, 15, 45, 255});
    DrawRectangleLines(centerX - 300, 320, 600, 130, GOLD);
    DrawTextCentered("MIXKIT", 338, 32, WHITE);
    DrawTextCentered("Free sound effects", 375, 18, LIGHTGRAY);
    DrawLink("https://mixkit.co/free-sound-effects/game/", 405, 20);
    DrawTextCentered("All sounds used under free licenses", 480, 16, LIGHTGRAY);
    DrawTextCentered("All graphics drawn with raylib primitives", 520, 16, LIGHTGRAY);
    DrawTextCentered("Press SPACE or ESC to return", SCREEN_H - 40, 20, GOLD);
}
void drawGameOver() {
    Theme t = themes[gamelvl - 1];
    DrawRectangle(0, 0, SCREEN_W, SCREEN_H, Fade(BLACK, 0.75f));
    int boxW = 540, boxH = 420;
    int boxX = (SCREEN_W - boxW) / 2;
    int boxY = (SCREEN_H - boxH) / 2;
    DrawPixelFrame(boxX, boxY, boxW, boxH, RED, (Color){20, 8, 20, 255});
    const char* title = "MISSION FAILED";
    int tx = (SCREEN_W - MeasureText(title, 50)) / 2;
    int jx = GetRandomValue(-2, 2);
    DrawText(title, tx - 3 + jx, boxY + 30, 50, Fade(RED, 0.8f));
    DrawText(title, tx + 3 - jx, boxY + 30, 50, Fade((Color){0, 255, 255, 255}, 0.5f));
    DrawText(title, tx, boxY + 30, 50, WHITE);
    DrawTextCentered(TextFormat("Mission %d - %s", gamelvl, t.name), boxY + 100, 20, t.uiAccent);
    DrawTextCentered(TextFormat("@%s", playerName), boxY + 130, 20, LIGHTGRAY);
    DrawTextCentered(TextFormat("Score: %d", score), boxY + 175, 26, WHITE);
    DrawTextCentered(TextFormat("Loot: %d/%d", itemsCollected, itemCount), boxY + 213, 22, LIGHTGRAY);
    DrawTextCentered(TextFormat("High Score: %d", GetHighScore()), boxY + 247, 24, GOLD);
    DrawTextCentered("[R] RETRY   [ESC] MENU", boxY + 320, 22, WHITE);
}
void drawGameWon() {
    Theme t = themes[gamelvl - 1];
    float time = (float)GetTime();
    int boxW = 500, boxH = 470;
    int boxX = (SCREEN_W - boxW) / 2;
    int boxY = (SCREEN_H - boxH) / 2;
    DrawGlow(SCREEN_W / 2.0f, boxY + boxH / 2.0f, 400, t.neonA, 0.35f);
    DrawPixelFrame(boxX, boxY, boxW, boxH, t.neonA, Fade(t.uiBackground, 0.95f));
    const char* title = "MISSION COMPLETE";
    int tw = MeasureText(title, 42);
    DrawText(title, (SCREEN_W - tw) / 2, boxY + 25, 42, WHITE);
    DrawTextCentered(TextFormat("@%s - M%d", playerName, gamelvl), boxY + 90, 20, LIGHTGRAY);
    DrawTextCentered(t.name, boxY + 118, 20, t.uiAccent);
    DrawTextCentered(TextFormat("Time: %.1fs   Moves: %d", levelElapsed, moves), boxY + 158, 20, t.textColor);
    DrawTextCentered(TextFormat("Loot: %d/%d   Lives: %d", itemsCollected, itemCount, lives), boxY + 186, 20, t.textColor);
    DrawTextCentered(TextFormat("SCORE  %d", (int)(displayScore + 0.5f)), boxY + 220, 36, GOLD);
    if (newBestTime) {
        if (((int)(time * 4)) % 2 == 0)
            DrawTextCenteredGlow("NEW PERSONAL BEST!", boxY + 273, 22, GREEN, GREEN);
    } else {
        DrawTextCentered(TextFormat("Personal best: %.1fs", bestTimes[gamelvl - 1]), boxY + 273, 20, LIGHTGRAY);
    }
    DrawTextCentered(TextFormat("High Score: %d", GetHighScore()), boxY + 308, 22, GOLD);
    DrawTextCentered("[R] PICK MISSION   [ESC] MENU", boxY + 400, 20, WHITE);
}

void UpdateEffects(float dt) {
    float time = (float)GetTime();
    UpdateParticles(dt);
    UpdateFloatTexts(dt);
    UpdateReactions(dt);
    if (shakeTime > 0) shakeTime -= dt;
    redFlash = fmaxf(0, redFlash - dt * 1.5f);
    lightFlash = fmaxf(0, lightFlash - dt * 1.8f);
    damageFlash = fmaxf(0, damageFlash - dt * 1.2f);
    if (glitchTime > 0) glitchTime -= dt;
    if (paradoxFlash > 0) paradoxFlash -= dt * 2.0f;
    if (combo > 0 && time - lastCollectTime > COMBO_WINDOW) combo = 0;
    if (dashCooldown > 0) dashCooldown -= dt;
    if (scanCooldown > 0) scanCooldown -= dt;
    if (phaseCooldown > 0) phaseCooldown -= dt;
    if (echoCollideCooldown > 0) echoCollideCooldown -= dt;

    if (phaseFlipAnim > 0) {
        phaseFlipAnim -= dt * 2.5f;
        if (phaseFlipAnim <= 0) { phaseFlipAnim = 0; phase = phaseFlipTarget; }
    }
    if (scanPulseTime >= 0) scanPulseTime += dt;

    displayScore += (score - displayScore) * fminf(1.0f, dt * 5);
    if (fabsf(score - displayScore) < 0.5f) displayScore = (float)score;

    float targetX = player_col * cell_size + cell_size / 2.0f;
    float targetY = player_row * cell_size + cell_size / 2.0f;
    float ease = (time < dashActiveUntil) ? 40.0f : 18.0f;
    playerDrawX += (targetX - playerDrawX) * fminf(1.0f, dt * ease);
    playerDrawY += (targetY - playerDrawY) * fminf(1.0f, dt * ease);

    for (int k = TRAIL_POINTS - 1; k > 0; k--) trailPoints[k] = trailPoints[k - 1];
    trailPoints[0] = (Vector2){playerDrawX, playerDrawY};

    if (gameState == PLAYING) {
        EchoRecord((Vector2){playerDrawX, playerDrawY}, time);
        bool valid = false;
        Vector2 delayed = EchoGetDelayed(time, ECHO_LAG, &valid);
        if (valid) {
            echoGhostPos = delayed;
            echoGhostActive = true;
            float dx = echoGhostPos.x - playerDrawX;
            float dy = echoGhostPos.y - playerDrawY;
            float distSq = dx * dx + dy * dy;
            if (distSq < 22 * 22) {
                if (echoCollideCooldown <= 0) {
                    echoCollideCooldown = 1.5f;
                    DamagePlayer("PARADOX!", themes[gamelvl - 1].neonB);
                }
            }
        } else echoGhostActive = false;
        if (echoGhostActive && (int)(time * 8) % 2 == 0)
            SpawnParticle(PT_ECHO, echoGhostPos.x, echoGhostPos.y, 0, -8, 0, 1.0f, 2, themes[gamelvl - 1].echoColor);
    }

    if (shakeTime > 0) {
        float power = shakeStrength * fminf(1.0f, shakeTime / 0.25f);
        shakeCam.offset = (Vector2){RandF(-power, power), RandF(-power, power)};
    } else {
        shakeCam.offset = (Vector2){0, 0};
    }
}

void UpdateChat(float dt) {
    MissionChat* chat = NULL;
    if (gameState == MISSION_BRIEF) chat = &briefs[gamelvl - 1];
    else if (gameState == MISSION_DEBRIEF) {
        if (timeRemaining <= 0 || lives <= 0) chat = &failChat;
        else chat = &debriefs[gamelvl - 1];
    }
    if (!chat) return;
    chatPanelOpen += dt * 4;
    if (chatPanelOpen > 1) chatPanelOpen = 1;
    if (chatIndex < chat->lineCount) {
        const char* fullText = chat->lines[chatIndex].text;
        int fullLen = (int)strlen(fullText);
        chatTyping += dt * TEXT_SPEED;
        chatBlipTimer -= dt;
        if (chatTyping < fullLen && chatBlipTimer <= 0) {
            PlayPitched(chatBlipSound, RandF(1.4f, 1.7f));
            chatBlipTimer = 0.05f;
        }
        bool finished = chatTyping >= fullLen;
        bool advance = IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        if (advance) {
            if (finished) { chatIndex++; chatTyping = 0; chatBlipTimer = 0; }
            else chatTyping = (float)fullLen;
        }
    } else {
        bool advance = IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
        if (advance) {
            if (gameState == MISSION_BRIEF) {
                if (!musicMuted) {
                    if (gamelvl == 1) PlaySound(sillycatSound);
                    else if (gamelvl == 2) PlaySound(level2Sound);
                    else if (gamelvl == 3) PlaySound(level3Sound);
                }
                gameState = PLAYING;
            } else {
                if (timeRemaining <= 0 || lives <= 0) gameState = GAME_OVER;
                else gameState = MISSION_SELECT;
            }
        }
    }
}

const char* SpeakerName(int id) {
    if (id == SPEAKER_ORACLE) return "ORACLE";
    if (id == SPEAKER_PLAYER) return "AGENT";
    if (id == SPEAKER_SYSTEM) return "SYSTEM";
    return "???";
}
Color SpeakerColor(int id) {
    if (id == SPEAKER_ORACLE) return (Color){0, 255, 180, 255};
    if (id == SPEAKER_PLAYER) return (Color){0, 200, 255, 255};
    if (id == SPEAKER_SYSTEM) return (Color){180, 180, 200, 255};
    return (Color){255, 60, 80, 255};
}
int SpeakerPortrait(int id) {
    if (id == SPEAKER_ORACLE) return PORTRAIT_ORACLE;
    if (id == SPEAKER_PLAYER) return PORTRAIT_PLAYER;
    if (id == SPEAKER_SYSTEM) return PORTRAIT_SYSTEM;
    return PORTRAIT_ENEMY;
}

void drawMissionChat() {
    MissionChat* chat = NULL;
    if (gameState == MISSION_BRIEF) chat = &briefs[gamelvl - 1];
    else if (gameState == MISSION_DEBRIEF) {
        if (timeRemaining <= 0 || lives <= 0) chat = &failChat;
        else chat = &debriefs[gamelvl - 1];
    }
    if (!chat) return;
    Theme t = themes[gamelvl - 1];
    float time = (float)GetTime();
    DrawRectangleGradientV(0, 0, SCREEN_W, SCREEN_H, Fade(t.bgTop, 0.9f), Fade(t.bgBottom, 0.9f));
    for (int y = 0; y < SCREEN_H; y += 6) DrawRectangle(0, y, SCREEN_W, 3, Fade(BLACK, 0.35f));
    for (int x = 0; x < SCREEN_W; x += 30) DrawRectangle(x, 0, 1, SCREEN_H, Fade(chat->accent, 0.08f));

    int barY = 20;
    DrawPixelFrame(30, barY, SCREEN_W - 60, 50, chat->accent, (Color){15, 10, 30, 255});
    DrawTextGlow(chat->title, 46, barY + 12, 26, chat->accent, chat->accent);
    if (fmodf(time, 1.0f) < 0.5f) DrawRectangle(SCREEN_W - 60, barY + 20, 8, 12, chat->accent);

    float slide = chatPanelOpen * chatPanelOpen;
    int panelH = 300;
    int panelY = (int)(SCREEN_H - panelH * slide - 30);
    int portraitX = 60;
    int portraitY = panelY + 90;

    if (chatIndex < chat->lineCount) {
        int speaker = chat->lines[chatIndex].speaker;
        int portrait = SpeakerPortrait(speaker);
        Color sc = SpeakerColor(speaker);
        DrawGlow(portraitX + 50, portraitY + 50, 90, sc, 0.35f);
        DrawPixelFrame(portraitX - 6, portraitY - 6, 116, 116, sc, (Color){10, 8, 20, 255});
        DrawPortrait(portrait, portraitX, portraitY);
        int nameW = MeasureText(SpeakerName(speaker), 20) + 30;
        DrawPixelFrame(portraitX - 6, portraitY + 120, nameW, 32, sc, (Color){15, 10, 30, 255});
        DrawText(SpeakerName(speaker), portraitX + 10, portraitY + 128, 20, sc);

        int dx = portraitX + 150;
        int dw = SCREEN_W - dx - 60;
        int dh = 200;
        int dy = panelY + 50;
        DrawPixelFrame(dx, dy, dw, dh, sc, (Color){12, 8, 22, 240});

        const char* fullText = chat->lines[chatIndex].text;
        int reveal = (int)chatTyping;
        if (reveal > (int)strlen(fullText)) reveal = (int)strlen(fullText);
        char shown[128];
        snprintf(shown, sizeof(shown), "%.*s", reveal, fullText);
        int len = (int)strlen(shown);
        int startY = dy + 26;
        int lineStart = 0, lineNum = 0;
        for (int i = 0; i <= len; i++) {
            if (i == len || (shown[i] == ' ' && (i - lineStart) > 42)) {
                char chunk[80];
                int clen = i - lineStart;
                if (clen > 79) clen = 79;
                strncpy(chunk, shown + lineStart, clen);
                chunk[clen] = '\0';
                DrawText(chunk, dx + 24, startY + lineNum * 30, 22, WHITE);
                lineStart = i + 1;
                lineNum++;
                if (lineNum > 4) break;
            }
        }
        if (reveal >= (int)strlen(fullText) && fmodf(time, 1.0f) < 0.5f) {
            int cursorX = dx + 24 + MeasureText(shown + lineStart, 22) + 4;
            int cursorY = startY + (lineNum - 1) * 30;
            DrawRectangle(cursorX, cursorY, 10, 20, sc);
        }
    } else {
        int px = SCREEN_W / 2 - 200;
        int py = panelY + 100;
        DrawPixelFrame(px, py, 400, 80, chat->accent, (Color){15, 10, 30, 255});
        if (fmodf(time, 1.0f) < 0.5f) DrawTextCentered("[ SPACE / CLICK TO CONTINUE ]", py + 30, 20, chat->accent);
    }
    DrawTextCentered("SPACE / CLICK = NEXT   ESC = SKIP", SCREEN_H - 30, 16, Fade(WHITE, 0.5f));
}

int main(void) {
    InitWindow(SCREEN_W, SCREEN_H, GAME_TITLE);
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);
    InitAudioDevice();
    LoadSounds();
    LoadMusic();
    LoadLeaderboard();
    LoadExtras();
    InitMissionChats();
    shakeCam.zoom = 1.0f;
    EchoReset();
    loadLevel(1);
    gameState = MENU;

    PlayMusicStream(backgroundMusic);
    PauseMusicStream(backgroundMusic);
    PlaySound(introSound);

    while (!WindowShouldClose() && !exitGame) {
        float dt = GetFrameTime();
        if (IsKeyPressed(KEY_M) && !editingName) ToggleMusic();

        if (gameState == MENU) {
            if (editingName) UpdateNameTyping();
        } else if (gameState == MISSION_SELECT) {
            if (IsKeyPressed(KEY_ESCAPE)) gameState = MENU;
        } else if (gameState == HOW_TO_PLAY) {
            if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ESCAPE)) gameState = MENU;
        } else if (gameState == LEADERBOARD) {
            if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ESCAPE)) gameState = MENU;
        } else if (gameState == CREDITS) {
            if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ESCAPE)) gameState = MENU;
        } else if (gameState == MISSION_BRIEF || gameState == MISSION_DEBRIEF) {
            if (IsKeyPressed(KEY_ESCAPE)) {
                if (gameState == MISSION_BRIEF) {
                    if (!musicMuted) {
                        if (gamelvl == 1) PlaySound(sillycatSound);
                        else if (gamelvl == 2) PlaySound(level2Sound);
                        else if (gamelvl == 3) PlaySound(level3Sound);
                    }
                    gameState = PLAYING;
                } else {
                    if (timeRemaining <= 0 || lives <= 0) gameState = GAME_OVER;
                    else gameState = MISSION_SELECT;
                }
            } else {
                UpdateChat(dt);
            }
        } else if (gameState == PLAYING) {
            if (IsKeyPressed(KEY_ESCAPE)) {
                StopEverything(); gameState = MENU;
            } else if (IsKeyPressed(KEY_R)) {
                StartLevel(gamelvl);
            } else {
                timeRemaining -= dt;
                levelElapsed += dt;
                if (timeRemaining <= 0) {
                    TriggerGameOver();
                } else {
                    if (IsKeyPressed(KEY_LEFT_SHIFT) || IsKeyPressed(KEY_RIGHT_SHIFT)) TryDash();
                    if (IsKeyPressed(KEY_F)) TryScan();
                    if (IsKeyPressed(KEY_E)) TryPhaseFlip();

                    int nr = player_row, nc = player_col;
                    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) nr--;
                    else if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) nr++;
                    else if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) nc--;
                    else if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) nc++;

                    if (nr != player_row || nc != player_col) movement(nr, nc);
                    Goalreached();

                    UpdateJungleMechanics(dt);
                    UpdateCosmicMechanics(dt);
                    UpdateCyberMechanics(dt);
                }
            }
        } else if (gameState == GAME_OVER) {
            if (IsKeyPressed(KEY_R)) StartLevel(gamelvl);
            else if (IsKeyPressed(KEY_ESCAPE)) { StopEverything(); gameState = MENU; }
        }

        UpdateGameMusic();
        UpdateEffects(dt);
        SpawnAmbientParticles(dt);

        BeginDrawing();
        ClearBackground(BLACK);

        switch (gameState) {
            case MENU:              drawMenu(); break;
            case MISSION_SELECT:    drawMissionSelect(); break;
            case HOW_TO_PLAY:       drawHowToPlay(); break;
            case LEADERBOARD:       drawLeaderboard(); break;
            case CREDITS:           drawCredits(); break;
            case MISSION_BRIEF:
            case MISSION_DEBRIEF:   drawMissionChat(); break;
            case PLAYING:
                BeginMode2D(shakeCam);
                drawmaze();
                startdraw();
                goaldraw();
                drawItems();
                drawGoalPath();
                drawEchoGhost();
                drawTrail();
                playerdraw();
                DrawParticles();
                DrawFloatTexts();
                DrawReactions();
                EndMode2D();
                drawScreenEffects();
                drawDisplay();
                break;
            case GAME_OVER:
                BeginMode2D(shakeCam);
                drawmaze();
                startdraw();
                goaldraw();
                drawItems();
                playerdraw();
                DrawParticles();
                DrawReactions();
                EndMode2D();
                drawScreenEffects();
                drawDisplay();
                drawGameOver();
                break;
        }
        EndDrawing();
    }

    UnloadMusicStream(backgroundMusic);
    UnloadSounds();
    CloseAudioDevice();
    CloseWindow();
    return 0;
}