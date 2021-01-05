#ifndef _LOSTISLAND_H
#define _LOSTISLAND_H


#include "defines.h"

#include "types.h"
#include "items.h"
#include "ennemies.h"
#include "sprites.h"
#include "block_defines.h"

uint8_t frameRate;
uint16_t frameCount;
uint8_t eachFrameMillis;
long lastFrameStart;
long nextFrameStart;
bool post_render;
uint8_t lastFrameDurationMs;

extern Adafruit_SPIFlash Arcada_QSPI_Flash;

Adafruit_Arcada arcada;
uint16_t *framebuffer;
GFXcanvas16 *canvas = NULL;
int Swidth, Sheight;

Adafruit_ZeroTimer zerotimer3 = Adafruit_ZeroTimer(3);

static pmf_player s_player;
static unsigned s_effect_channel = 0;

typedef enum TGameStates
{
    STATE_MAIN_MENU,
    STATE_PAUSE_MENU,
    STATE_CREATE_CHARACTER,
    STATE_GAME_MENU,
    STATE_OPTIONS,
    STATE_PLAYING,
    STATE_CRAFTING,
    STATE_GAMEOVER,
} TGameStates;

TGameStates gameState = STATE_MAIN_MENU;

Particles parts;

int briquesFRAMES = 0;

int CURRENT_LEVEL = 1;

int NB_WORLD_ENNEMIES = 0;
int NB_WORLD_ITEMS = 0;
int NB_WORLD_ZOMBIES = 0;
int NB_WORLD_SPIDERS = 0;
int NB_WORLD_SKELS = 0;

int CURRENT_QUEUE_ITEMS = 0;

int worldX = 0;
int worldMIN_X = 0;
int worldMAX_X = 0;
int currentOffset_X = 0;

int worldY = 0;
int worldMIN_Y = 0;
int worldMAX_Y = 0;
int currentOffset_Y = 0;

int worldOffset_pX = 0;
int worldOffset_pY = 0;

int counterActionB = 0;
int coolDownActionB = 0;

uint16_t lastRowErosion = 0;
uint16_t lastColErosion = 0;

int itemShakeAmount = 0;

int cameraShakeAmount = 0;
int cameraShakeX = 0;
int cameraShakeY = 0;

TPlayer Player;

//Masque applique sur les tiles pour les eclairer
#define PLAYER_LIGHT_MASK_HEIGHT 9
#define PLAYER_LIGHT_MASK_WIDTH 9
int16_t playerLightMask[PLAYER_LIGHT_MASK_HEIGHT][PLAYER_LIGHT_MASK_WIDTH] = {0};

TworldTile WORLD[WORLD_HEIGHT + 2][WORLD_WIDTH];
#define HEADER_ROW WORLD_HEIGHT
#define REF_ROW WORLD_HEIGHT + 1

uint8_t pressed_buttons = 0;
uint8_t just_pressed = 0;
int SCORE = 0;
bool bWin = false;

int cameraX = 0;
int cameraY = 0;

int currentX_back = 0;

int jumpPhase = 0;
TworldTile brique_UP = {0};
TworldTile brique_DOWN = {0};
TworldTile brique_DOWN_FRONT = {0};
TworldTile brique_FRONT = {0};
TworldTile brique_PLAYER = {0};

struct {
    TworldTile  *tile;
    int16_t     currentLife;
    uint32_t    lastHit;
    uint16_t    itemColor;
    int         wX, wY;
    int         pX, pY;    
} currentTileTarget = {0};

int count_player_die = 0;
int count_player_win = 0;

int16_t hauteurMaxBackground = 0;

unsigned long prevDisplay = 0; // when the digital clock was displayed

void createDropFrom(int wX, int wY, uint8_t type);
void killItem(Titem *currentItem);
void drawItems();
void drawItem(Titem *currentItem);
void drawHud();

void killEnnemy(Tennemy *currentEnnemy, int px, int py);
void drawEnnemy(Tennemy *currentEnnemy);
void checkEnnemyCollisionsWorld(Tennemy *currentEnnemy);
void updateEnnemyIA(Tennemy *currentEnnemy);

void drawEnnemies();
void drawTiles();
void drawParticles();
void drawWorld();
void pixelToWorld(int *pX, int *pY);
bool checkCollisionTo(int origin_x, int origin_y, int dest_x, int dest_y);
void checkPlayerCollisionsEntities();
void updatePlayer();
void updateEnnemies();
void updateItems();
void updateGame();

void displayGame();
void initWorld();
void postInitWorld();


void anim_player_jump();
void anim_player_dying();
void anim_player_wining();
void anim_player_idle();
void anim_player_walk();
void anim_player_falling();
void anim_player_mining();
void anim_player_digging();

#endif