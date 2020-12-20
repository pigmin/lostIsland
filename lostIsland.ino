#include <Arduino.h>
#include <Adafruit_Arcada.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SPIFlash.h>
#include <AsyncDelay.h>

#undef PIXEL_LIGHT
#undef DEBUG
//#include <FastLED.h>

//#define NO_SOUND
#define FPS (1000 / 25)
#define WORLD_WIDTH 604
#define WORLD_HEIGHT 128

#define AMPLITUDE_HAUTEUR 16
#define MEDIUM_HAUTEUR (2 * WORLD_HEIGHT / 3)

#define MAX_DENSITE 8
#define AMPLITUDE_DENSITE 16

#define MAX_JUMP_PHASE 11

#include "types.h"
#include "items.h"
#include "ennemies.h"
#include "sprites.h"
#include "block_defines.h"

#include "gfx_utils.h"

#include "Particle.h"

#include "MySoundManager.h"

#include "SimplexNoise.h"

#include "musics/bach_tocatta_fugue_d_minor.c"

#if !defined(USE_TINYUSB)
#error("Please select TinyUSB for the USB stack!")
#endif

#include "res/player_idle1.c"
#include "res/player_idle2.c"
#include "res/player_idle3.c"

#include "res/player_walk1.c"
#include "res/player_walk2.c"
#include "res/player_walk3.c"
#include "res/player_walk4.c"
#include "res/player_walk5.c"
#include "res/player_walk6.c"

#include "res/player_action1.c"
#include "res/player_action2.c"
#include "res/player_action3.c"
#include "res/player_action4.c"
#include "res/player_action5.c"
#include "res/player_action6.c"


#include "res/player_falling1.c"

#include "res/player_jump1.c"
#include "res/player_jump2.c"
#include "res/player_jump3.c"

#include "res/back_ground.c"

#include "res/ground_top.c"
#include "res/ground.c"
#include "res/rock_argent.c"
#include "res/rock_charbon.c"
#include "res/rock_cuivre.c"
#include "res/rock_empty.c"
#include "res/rock_fer.c"
#include "res/rock_jade.c"
#include "res/rock_or.c"
#include "res/rock_redstone.c"

#include "res/grass_left_8x8.c"
#include "res/grass_middle_8x8.c"
#include "res/grass_right_8x8.c"

#include "res/tree_bottom_8x8.c"
#include "res/tree_top_8x8.c"

#include "res/montagne_left_8x8.c"
#include "res/montagne_middle_8x8.c"
#include "res/montagne_right_8x8.c"
#include "res/montagne_top_32x8.c"

#include "res/cloud1_32x24.c"
#include "res/cloud2_32x24.c"

#include "res/pioche.c"

#include "sounds/Jump.h"

extern Adafruit_SPIFlash Arcada_QSPI_Flash;

Adafruit_Arcada arcada;
uint16_t *framebuffer;
GFXcanvas16 *canvas = NULL;
int Swidth, Sheight;

typedef enum TGameStates
{
    STATE_MENU,
    STATE_PLAYING,
    STATE_CRAFTING,
    STATE_GAMEOVER,
} TGameStates;

TGameStates gameState = STATE_MENU;

#define BASE_X_PLAYER 7
#define BASE_Y_PLAYER 1

#define FALLING_SPEED 1
#define WALKING_SPEED 2
#define RUNNING_SPEED 2

#define MAX_SPEED_X 4
#define MAX_SPEED_Y 15

#define JUMP_SPEED 11

Particles parts;

int briquesFRAMES = 0;
int RUN_DIR = 1;

int CURRENT_LEVEL = 1;

int CURRENT_LEVEL_ENNEMIES = 0;
int CURRENT_LEVEL_ITEMS = 0;

int player_pos_x = (BASE_X_PLAYER * 16);
int player_pos_y = (BASE_Y_PLAYER * 16);

int player_speed_x = 0;
int player_speed_y = 0;

int new_player_pos_x = 0;
int new_player_pos_y = 0;

int player_anim = 0;
int player_current_framerate = 0;
int playerStateAnim = PLAYER_STATE_IDLE;

int cibleX, cibleY = 0;

int worldX = 0;
int worldMIN_X = 0;
int worldMAX_X = 0;
int currentOffset_X = 0;

int worldY = 0;
int worldMIN_Y = 0;
int worldMAX_Y = 0;
int currentOffset_Y = 0;

int counterActionB = 0;

unsigned char WORLD[WORLD_HEIGHT + 2][WORLD_WIDTH];
#define HEADER_ROW WORLD_HEIGHT
#define REF_ROW WORLD_HEIGHT + 1

#define BACKWORLD_WIDTH (WORLD_WIDTH / 4) + (ARCADA_TFT_WIDTH / 8)
#define BACKWORLD_HEIGHT 16

unsigned char WORLD_BACK[BACKWORLD_HEIGHT][BACKWORLD_WIDTH];

AsyncDelay tick;

void createItem(int wX, int wY, int type);
void killItem(Titem *currentItem);
void drawItems();
void drawItems(Titem *currentItem);
void drawHud();

void killEnnemy(Tennemy *currentEnnemy, int px, int py);
void drawEnnemy(Tennemy *currentEnnemy);
void drawEnnemies();
void drawTiles();
void drawParticles();
void drawWorld();
void pixelToWorld(int *pX, int *pY);
int checkCollisionAt(int newX, int newY);
int checkCollisionTo(int x, int y, int newX, int newY);
void checkPlayerState();
void updatePlayer();
void updateEnnemies();
void updateItems();
void updateGame();

void displayGame();
void initWorld();

void anim_player_jump();
void anim_player_dying();
void anim_player_wining();
void anim_player_idle();
void anim_player_walk();
void anim_player_falling();
void anim_player_mining();

uint8_t pressed_buttons = 0;
uint8_t just_pressed = 0;
int SCORE = 0;
bool bWin = false;
bool bDying = false;
bool bJumping = false;
bool bFalling = false;
bool bWalking = false;
bool bMoving = false;
bool bTouched = false;

int iTouchCountDown = 0;

bool bOnGround = true;

int cameraX = 0;
int cameraY = 0;

int posXPlayerInWorld = 0;
int posYPlayerInWorld = 0;

int posXPlayerInWorld_TL = 0;
int posYPlayerInWorld_TL = 0;
int posXPlayerInWorld_BR = 0;
int posYPlayerInWorld_BR = 0;
int currentX_back = 0;

int posXFront = 0;
int posYDown = 0;
int posYUp = 0;

int jumpPhase = 0;
unsigned char brique_UP = 0;
unsigned char brique_DOWN = 0;
unsigned char brique_DOWN_FRONT = 0;
unsigned char brique_FRONT = 0;
unsigned char brique_PLAYER = 0;

int count_player_die = 0;
int count_player_touched = 0;
int count_player_win = 0;

bool bDoJump = false;
bool bDoWalk = false;
bool bWatch = true;

unsigned long prevDisplay = 0; // when the digital clock was displayed

unsigned char getWorldAtPix(int px, int py)
{
    unsigned char res = 0;

    int x = px / 16;
    int y = py / 16;

    if ((x >= 0 && x < WORLD_WIDTH) && (y >= 0 && y < WORLD_HEIGHT))
        res = WORLD[y][x];

    //Serial.printf("pix:%d,%d  val:%d\n", x, y, res);

    return res;
}

void updateHauteurColonne(int x, int y)
{
    //on  cherche la nouvelle hauteur
    int newH = 0;
    for (int wY = 0; wY < WORLD_HEIGHT; wY++)
    {
        if (WORLD[wY][x] != 0)
        {
            newH = wY;
            break;
        }
    }
    WORLD[HEADER_ROW][x] = newH;
}

unsigned char getWorldAt(int x, int y)
{
    unsigned char res = 0;

    if ((x >= 0 && x < WORLD_WIDTH) && (y >= 0 && y < WORLD_HEIGHT))
        res = WORLD[y][x];

    return res;
}

void setWorldAt(int x, int y, unsigned char val)
{

    if ((x > 0 && x < WORLD_WIDTH) && (y > 0 && y < WORLD_HEIGHT))
        WORLD[y][x] = val;
}

void spawnMushroom(int wX, int wY)
{
    int16_t sX = wX * 16;
    int16_t sY = wY * 16;
    // sndPlayerCanal2.play(AudioSampleSmb_powerup_appears);
    createItem(wX, wY, ITEM_MUSHROOM);
}

static void handleChangeState()
{
    //Serial.println("handleChangeState");
    if (bWatch)
    {

        bWatch = false;
    }
    else
    {
        bWatch = true;
    }
}

void displayText(char *text, int16_t x, int16_t y, uint16_t color = ARCADA_WHITE, bool bCenter = false)
{
    canvas->setTextWrap(false);
    canvas->setTextColor(color);

    if (bCenter)
    {
        int16_t px, py;
        uint16_t w, h;
        canvas->getTextBounds((const char *)text, 0, y, &px, &py, &w, &h);
        int newX = (ARCADA_TFT_WIDTH - w) / 2;
        canvas->setCursor(newX, y);
    }
    else
        canvas->setCursor(x, y);

    canvas->print(text);
}

void setup()
{
    //while (!Serial);

    Serial.begin(115200);

    if (!arcada.arcadaBegin())
    {
        Serial.print("Failed to begin");
        while (1)
            ;
    }

    delay(100);

    arcada.displayBegin();
    Serial.println("Arcada display begin");

    arcada.display->fillScreen(ARCADA_BLUE);

    // Turn on backlight
    arcada.setBacklight(255);

    Swidth = arcada.display->width();
    Sheight = arcada.display->height();
    if (!arcada.createFrameBuffer(Swidth, Sheight))
    {
        arcada.haltBox("Could not allocate framebuffer");
    }

    framebuffer = arcada.getFrameBuffer();
    canvas = arcada.getCanvas();
    /*
  if (!arcada.filesysBeginMSD(ARCADA_FILESYS_QSPI))
  {
    arcada.haltBox("Failed to begin fileSys");
  }*/
    arcada.infoBox("PRESS START", ARCADA_BUTTONMASK_START);

    setupSoundManager();
    /********** Start speaker */
#ifndef NO_SOUND
    arcada.enableSpeaker(true);
#endif

    arcada.display->fillScreen(ARCADA_BLUE);

    arcada.display->setCursor(0, 0);
    arcada.display->setTextWrap(true);
    arcada.display->setTextSize(1);

    arcada.display->setTextColor(ARCADA_GREEN);

    initGame();

    tick.start(FPS, AsyncDelay::MILLIS);

    Serial.println("Starting");
}

void clearRect(int16_t xMove, int16_t yMove, int16_t width, int16_t height)
{
    canvas->fillRect(xMove, yMove, width, height, 0x867D);
}

void initGame()
{
    //arcada.display->setFont(ArialMT_Plain_10);
    //  arcada.display->setTextAlignment(TEXT_ALIGN_LEFT);
    briquesFRAMES = 0;

    RUN_DIR = 1;
    SCORE = 0;
    pressed_buttons = 0;
    bWin = false;
    bDying = false;
    bJumping = false;
    bFalling = false;
    bWalking = false;
    bMoving = false;
    bTouched = false;
    iTouchCountDown = 0;

    bOnGround = true;

    cameraX = BASE_X_PLAYER * 16;
    cameraY = BASE_Y_PLAYER * 16;
    posXPlayerInWorld_TL = posXPlayerInWorld_BR = posXPlayerInWorld = 0;
    posXFront = 0;
    posYDown = 0;
    posYUp = 0;
    posYPlayerInWorld_TL = posYPlayerInWorld_BR = posYPlayerInWorld = 0;
    currentX_back = 0;
    worldX = 0;
    worldMIN_X = 0;
    worldMAX_X = 0;
    currentOffset_X = 0;

    worldY = 0;
    worldMIN_Y = 0;
    worldMAX_Y = 0;
    currentOffset_Y = 0;

    jumpPhase = 0;
    brique_UP = 0;
    brique_DOWN = 0;
    brique_DOWN_FRONT = 0;
    brique_FRONT = 0;
    brique_PLAYER = 0;

    bDoJump = false;
    bDoWalk = false;

    player_pos_x = (BASE_X_PLAYER * 16);
    player_pos_y = (BASE_Y_PLAYER * 16);
    player_speed_x = 0;
    player_speed_y = 0;
    new_player_pos_x = 0;
    new_player_pos_y = 0;
    cibleX, cibleY = 0;

    player_anim = 0;
    player_current_framerate = 0;
    playerStateAnim = PLAYER_STATE_IDLE;

    count_player_die = 0;
    count_player_win = 0;
    count_player_touched = 0;

    initWorld();
}

void set_falling()
{
    if (playerStateAnim != PLAYER_STATE_FALL)
    {
        playerStateAnim = PLAYER_STATE_FALL;
        player_anim = PLAYER_FRAME_FALLING_1;
        player_current_framerate = 0;
    }
}

void set_idle()
{
    if (playerStateAnim != PLAYER_STATE_IDLE)
    {
        playerStateAnim = PLAYER_STATE_IDLE;
        player_anim = PLAYER_FRAME_IDLE_1;
        player_current_framerate = 0;
    }
}

void set_jumping()
{
    if (playerStateAnim != PLAYER_STATE_JUMP)
    {
        playerStateAnim = PLAYER_STATE_JUMP;

        player_anim = PLAYER_FRAME_JUMP_1;
        player_current_framerate = 0;
    }
}

void set_walking()
{
    if (playerStateAnim != PLAYER_STATE_WALK)
    {
        playerStateAnim = PLAYER_STATE_WALK;

        player_anim = PLAYER_FRAME_WALK_1;
        player_current_framerate = 0;
    }
}


void set_digging()
{
    if (playerStateAnim != PLAYER_STATE_DIG)
    {
        playerStateAnim = PLAYER_STATE_DIG;

        player_anim = PLAYER_FRAME_ACTION_1;
        player_current_framerate = 0;
    }
}


void set_dying()
{
    if (playerStateAnim != PLAYER_STATE_DIE)
    {
        playerStateAnim = PLAYER_STATE_DIE;

        stopMusic();
        //    sndPlayerCanal1.play(AudioSamplePlayerdeath);
        bDying = true;
        iTouchCountDown = 0;
        bTouched = false;
        bWalking = true;
        jumpPhase = 0;
        count_player_die = 0;
        player_anim = PLAYER_FRAME_DIE_1;
        player_current_framerate = 0;
    }
}

void set_touched()
{
    /*    if (bPlayerGrand)
    {
        //    sndPlayerCanal3.play(AudioSampleSmb_pipe);
        bTouched = true;
        iTouchCountDown = 2 * FPS;
        count_player_touched = 0;
        //player_anim = PLAYER_FRAME_TOUCH_1;
        //player_current_framerate = 0;
        bPlayerGrand = false;
    }
    else if (iTouchCountDown == 0)*/
    {
        set_dying();
    }
}

void set_wining()
{
    Serial.println("Winning");
    if (playerStateAnim != PLAYER_STATE_WIN)
    {
        playerStateAnim = PLAYER_STATE_WIN;

        //playMusic(FlagpoleFanfare);
        iTouchCountDown = 0;
        bTouched = false;
        bWin = true;
        jumpPhase = 0;
        count_player_win = 0;
        player_anim = PLAYER_FRAME_WIN_1;
        player_current_framerate = 0;
    }
}

void initWorld()
{
    stopMusic();
    //On remplace les espaces par 0
    CURRENT_LEVEL_ENNEMIES = 0;

    //uint8_t zeNoise[WORLD_WIDTH];
    SimplexNoise noiseGen;

    int maxProfondeur = 0;
    for (int wX = 0; wX < WORLD_WIDTH; wX++)
    {
        //        float noise = SimplexNoise::noise(((float)16 * wX) / (float)WORLD_WIDTH);
        float noise = noiseGen.fractal(4, ((float)16 * wX) / (float)WORLD_WIDTH);

        uint8_t rowGround = (WORLD_HEIGHT - 1) - (int(noise * AMPLITUDE_HAUTEUR) + MEDIUM_HAUTEUR);

        maxProfondeur = max(maxProfondeur, rowGround);

        //On sauvegarde la hauteur sol dans la derniere ligne du world (invisible)
        WORLD[HEADER_ROW][wX] = rowGround;
        //On sauve la hauteur originelle, en dessous on est dans le sol (meme si creusé)
        WORLD[REF_ROW][wX] = rowGround;

        for (int wY = 0; wY < WORLD_HEIGHT; wY++)
        {
            if (wY > (rowGround + 7))
                WORLD[wY][wX] = BLOCK_ROCK;
            else if (wY > (rowGround + 3))
                WORLD[wY][wX] = BLOCK_GROUND_ROCK;
            else if (wY > rowGround)
                WORLD[wY][wX] = BLOCK_GROUND;
            else if (wY == rowGround)
                WORLD[wY][wX] = BLOCK_GROUND_TOP;
            else
                WORLD[wY][wX] = 0;
        }
    }
    float minN = 999999;
    float maxN = -99999;
    //Update bricks to cool rendering
    for (int wX = 0; wX < WORLD_WIDTH; wX++)
    {
        int curProdondeur = WORLD[HEADER_ROW][wX];
        curProdondeur = max(4, curProdondeur);

        for (int wY = curProdondeur - 4; wY < WORLD_HEIGHT; wY++)
        {
            //            float noise = noiseGen.fractal(8, (float)16*wX / (float)WORLD_WIDTH, (float)16*wY / (float)WORLD_HEIGHT);
            float noise = SimplexNoise::noise((float)16 * wX / (float)WORLD_WIDTH, (float)16 * wY / (float)WORLD_HEIGHT);
            int16_t densite = int(noise * AMPLITUDE_DENSITE);
            if (WORLD[wY][wX] <= BLOCK_GROUND_ROCK)
            {
                if (densite > MAX_DENSITE)
                    WORLD[wY][wX] = BLOCK_AIR;
            }
            else 
            {
                if (abs(densite) > MAX_DENSITE || densite == 0)
                    WORLD[wY][wX] = BLOCK_UNDERGROUND_AIR;
                else
                    WORLD[wY][wX] = abs(densite) + BLOCK_ROCK;
            }

            if (noise < minN)
                minN = noise;
            if (noise > maxN)
                maxN = noise;
        }
    }
    for (int wX = 0; wX < WORLD_WIDTH; wX++)
    {
        for (int wY = 0; wY < WORLD_HEIGHT; wY++)
        {
            if ((WORLD[wY][wX] != 0) && (WORLD[wY][wX] != BLOCK_UNDERGROUND_AIR))
            {
                WORLD[HEADER_ROW][wX] = wY;
                WORLD[REF_ROW][wX] = wY;

                WORLD[wY][wX] = BLOCK_GROUND_TOP;
                if (wY+1 < (WORLD_HEIGHT-1))
                    WORLD[wY+1][wX] = BLOCK_GROUND;
                
                break;
            }
        }
    }
 
    Serial.printf("Min:%f / Max:%f\n", minN, maxN);

    CURRENT_LEVEL_ITEMS = 0;

    memset(&WORLD_BACK, 0, BACKWORLD_HEIGHT * BACKWORLD_WIDTH);

    //Montagnes grandes
    int montI = random(10);
    while (montI < (BACKWORLD_WIDTH - 10))
    {
        int pX = montI;
        if (random(10) < 5)
        {
            int idx = 0;

            for (idx = pX + 1; idx <= pX + 8; idx++)
                WORLD_BACK[BACKWORLD_HEIGHT - 1][idx] = 152;
            WORLD_BACK[BACKWORLD_HEIGHT - 1][pX] = 150;
            WORLD_BACK[BACKWORLD_HEIGHT - 1][pX + 9] = 151;

            for (idx = pX + 2; idx <= pX + 7; idx++)
                WORLD_BACK[BACKWORLD_HEIGHT - 2][idx] = 152;
            WORLD_BACK[BACKWORLD_HEIGHT - 2][pX + 1] = 150;
            WORLD_BACK[BACKWORLD_HEIGHT - 2][pX + 8] = 151;

            for (idx = pX + 3; idx <= pX + 6; idx++)
                WORLD_BACK[BACKWORLD_HEIGHT - 3][idx] = 152;
            WORLD_BACK[BACKWORLD_HEIGHT - 3][pX + 2] = 150;
            WORLD_BACK[BACKWORLD_HEIGHT - 3][pX + 7] = 151;

            WORLD_BACK[BACKWORLD_HEIGHT - 4][pX + 3] = 153;
        }
        montI += (10 + random(10));
    }

    //Montagnes petites
    montI = random(8);
    while (montI < (BACKWORLD_WIDTH - 7))
    {
        int pX = montI;
        if (random(10) < 6)
        {
            int idx = 0;

            for (idx = pX + 1; idx <= pX + 6; idx++)
                WORLD_BACK[BACKWORLD_HEIGHT - 1][idx] = 152;
            WORLD_BACK[BACKWORLD_HEIGHT - 1][pX] = 150;
            WORLD_BACK[BACKWORLD_HEIGHT - 1][pX + 7] = 151;

            for (idx = pX + 2; idx <= pX + 5; idx++)
                WORLD_BACK[BACKWORLD_HEIGHT - 2][idx] = 152;
            WORLD_BACK[BACKWORLD_HEIGHT - 2][pX + 1] = 150;
            WORLD_BACK[BACKWORLD_HEIGHT - 2][pX + 6] = 151;

            WORLD_BACK[BACKWORLD_HEIGHT - 3][pX + 2] = 153;
        }
        montI += (7 + random(10));
    }
    //nuages
    montI = random(10);
    while (montI < (BACKWORLD_WIDTH - 10))
    {
        int pX = montI;
        int vam = random(10);
        if (vam < 2)
        {
            WORLD_BACK[1 + random(3)][pX] = 140;
        }
        else if (vam < 5)
        {
            WORLD_BACK[1 + random(3)][pX + 4] = 144;
        }
        montI += (4 + random(6));
    }

 //   playMusic(bach_tocatta_fugue_d_minor);
}

void drawPlayer()
{
    if (iTouchCountDown > 0)
    {
        iTouchCountDown--;

        if ((iTouchCountDown % 2) == 0)
            return;
    }

    switch (playerStateAnim)
    {
    case PLAYER_STATE_WALK:
        anim_player_walk();
        break;
    case PLAYER_STATE_JUMP:
        anim_player_jump();
        break;
    case PLAYER_STATE_FALL:
        anim_player_falling();
        break;
    case PLAYER_STATE_DIG:
        anim_player_diging();
        break;
    case PLAYER_STATE_WIN:
    {
        canvas->fillRoundRect(20, 10, ARCADA_TFT_WIDTH - 40, 64, 8, ARCADA_BLUE);
        canvas->drawRoundRect(20, 10, ARCADA_TFT_WIDTH - 40, 64, 8, ARCADA_WHITE);
        canvas->drawRoundRect(24, 14, ARCADA_TFT_WIDTH - 48, 56, 8, ARCADA_WHITE);

        displayText("STAGE CLEAR", 0, 34, ARCADA_WHITE, true);

        char sScore[10];
        sprintf(sScore, "SCORE: %d", SCORE);
        displayText(sScore, 0, 50, ARCADA_WHITE, true);

        anim_player_wining();
        break;
    }
    case PLAYER_STATE_DIE:
    {
        canvas->fillRoundRect(20, 10, ARCADA_TFT_WIDTH - 40, 64, 8, ARCADA_BLUE);
        canvas->drawRoundRect(20, 10, ARCADA_TFT_WIDTH - 40, 64, 8, ARCADA_RED);
        canvas->drawRoundRect(24, 14, ARCADA_TFT_WIDTH - 48, 56, 8, ARCADA_RED);

        displayText("GAME OVER", 0, 34, ARCADA_RED, true);

        char sScore[10];
        sprintf(sScore, "SCORE: %d", SCORE);
        displayText(sScore, 0, 50, ARCADA_WHITE, true);

        anim_player_dying();

        break;
    }
    case PLAYER_STATE_IDLE:
    default:
        anim_player_idle();
        break;
    }
}

void createItem(int wX, int wY, int type)
{

    if (CURRENT_LEVEL_ITEMS < MAX_ITEMS)
    {
        ITEMS[CURRENT_LEVEL_ITEMS].bIsAlive = 255;
        ITEMS[CURRENT_LEVEL_ITEMS].bIsActive = false;
        ITEMS[CURRENT_LEVEL_ITEMS].iSpawning = 1 * FPS;

        ITEMS[CURRENT_LEVEL_ITEMS].worldX = wX;
        ITEMS[CURRENT_LEVEL_ITEMS].worldY = wY;
        ITEMS[CURRENT_LEVEL_ITEMS].x = wX * 16;
        ITEMS[CURRENT_LEVEL_ITEMS].y = wY * 16;
        ITEMS[CURRENT_LEVEL_ITEMS].type = type;
        ITEMS[CURRENT_LEVEL_ITEMS].direction = 1;
        ITEMS[CURRENT_LEVEL_ITEMS].max_frames = 2;
        ITEMS[CURRENT_LEVEL_ITEMS].max_anim = 2;

        CURRENT_LEVEL_ITEMS++;
    }
    else
    {
        Serial.println("Too many items.");
    }
}

void killItem(Titem *currentItem, int px, int py)
{
    currentItem->bIsAlive = 64;
    px = (px - (worldMIN_X * 16)) - currentOffset_X;

    //parts.createExplosion(px, py, 16);
}

void drawItem(Titem *currentItem)
{
    if (currentItem->current_framerate == currentItem->max_frames)
    {
        currentItem->anim++;
        currentItem->current_framerate = 0;
    }
    currentItem->current_framerate++;

    if (currentItem->anim > currentItem->max_anim)
    {
        currentItem->anim = 1;
    }
    if (currentItem->bIsAlive < 127)
    {
        currentItem->anim = 3;
        currentItem->bIsAlive -= 4;
    }

    int px = (currentItem->x - (worldMIN_X * 16)) - currentOffset_X;
    //Recentrage
    //px -= 4;
    if ((px < (ARCADA_TFT_WIDTH - 1)) && (px > -16))
    {
        /*    if (currentItem->iSpawning > 0)
    {
      int tmpH = 16 - (currentItem->iSpawning / 2);
      drawSprite(px, (currentItem->y - 16) + (16 - tmpH), mushroom_16x16.width, tmpH, mushroom_16x16.pixel_data, RUN_DIR);
    }
    else
      drawSprite(px, currentItem->y - 16, mushroom_16x16.width, mushroom_16x16.height, mushroom_16x16.pixel_data, RUN_DIR);*/
    }
}

void drawItems()
{
    int currentOffset_X = cameraX % 16;

    for (int enC = 0; enC < CURRENT_LEVEL_ITEMS; enC++)
    {
        Titem *currentItem = &ITEMS[enC];
        if (currentItem->bIsAlive > 0)
        {
            //Serial.printf("World:%d,%d\n", worldMIN_X, worldMAX_X);
            if (((currentItem->worldX >= (worldMIN_X - 1)) && (currentItem->worldX <= (worldMAX_X + 1))) &&
                ((currentItem->worldY >= (worldMIN_Y - 1)) && (currentItem->worldY <= (worldMAX_Y + 1))))
            {
                drawItem(currentItem);
            }
        }
    }
}

void killEnnemy(Tennemy *currentEnnemy, int px, int py)
{
    currentEnnemy->bIsAlive = 64;
    px = (px - (worldMIN_X * 16)) - currentOffset_X;

    parts.createExplosion(px, py, 16);
}

void drawEnnemy(Tennemy *currentEnnemy)
{

    if (currentEnnemy->current_framerate == currentEnnemy->max_frames)
    {
        currentEnnemy->anim++;
        currentEnnemy->current_framerate = 0;
    }
    currentEnnemy->current_framerate++;

    if (currentEnnemy->anim > currentEnnemy->max_anim)
    {
        currentEnnemy->anim = 1;
    }
    if (currentEnnemy->bIsAlive < 127)
    {
        currentEnnemy->anim = 3;
        currentEnnemy->bIsAlive -= 4;
    }

    int px = (currentEnnemy->x - (worldMIN_X * 16)) - currentOffset_X;
    //Recentrage
    //px -= 4;

    if ((px < (ARCADA_TFT_WIDTH - 1)) && (px > -16))
    { /*
    switch (currentEnnemy->anim)
    {
    case 1:
      drawSprite(px, currentEnnemy->y - 16, koopa1_16x16.width, koopa1_16x16.height, koopa1_16x16.pixel_data, RUN_DIR);
      //      drawSprite(px, currentEnnemy->y, 16, 16, koopa1_bits, 1);
      break;
    case 2:
      drawSprite(px, currentEnnemy->y - 16, koopa2_16x16.width, koopa2_16x16.height, koopa2_16x16.pixel_data, RUN_DIR);
      //drawSprite(px, currentEnnemy->y, 16, 16, koopa1_bits, 1);
      break;
    default:
      drawSprite(px, currentEnnemy->y - 16, koopa_die_16x16.width, koopa_die_16x16.height, koopa_die_16x16.pixel_data, RUN_DIR);
      break;
    }*/
    }
}

void drawEnnemies()
{
    int currentOffset_X = cameraX % 16;

    for (int enC = 0; enC < CURRENT_LEVEL_ENNEMIES; enC++)
    {
        Tennemy *currentEnnemy = &ENNEMIES[enC];
        if (currentEnnemy->bIsAlive > 0)
        {
            //Serial.printf("World:%d,%d\n", worldMIN_X, worldMAX_X);
            if (((currentEnnemy->worldX >= (worldMIN_X - 1)) && (currentEnnemy->worldX <= (worldMAX_X + 1))) &&
                ((currentEnnemy->worldY >= (worldMIN_Y - 1)) && (currentEnnemy->worldY <= (worldMAX_Y + 1))))
            {
                drawEnnemy(currentEnnemy);
            }
        }
    }
}

void drawTiles()
{
    int currentOffset_X_BACK = (cameraX / 4) % 8;
    int worldX_BACK = (cameraX / 4) / 8;

    int worldMIN_BACK = min(worldX_BACK, (BACKWORLD_WIDTH - 1) - (ARCADA_TFT_WIDTH / 8));
    int worldMAX_BACK = worldMIN_BACK + 1 + (ARCADA_TFT_WIDTH / 8);
#ifdef PIXEL_LIGHT
    int playerLightX = player_pos_x + 8;
    int playerLightY = player_pos_y + 8;
#else
//    int playerLightX = posXPlayerInWorld;
//    int playerLightY = posYPlayerInWorld;
    int playerLightX = player_pos_x + 8;
    int playerLightY = player_pos_y + 8;
#endif
    //Serial.printf("worldMIN_X:%d\n", worldMIN_X);
    //Serial.printf("worldMIN_BACK:%d\n", worldMIN_BACK);
    for (int wY = 0; wY < BACKWORLD_HEIGHT; wY++)
    {
        int py = wY * 8;
        if (py < -8)
            continue;
        else if (py > SCREEN_HEIGHT)
            break;

        for (int wX = worldMIN_BACK; wX < worldMAX_BACK; wX++)
        {
            int px = ((wX - worldMIN_BACK) * 8) - currentOffset_X_BACK;
            if ((px < (ARCADA_TFT_WIDTH - 1)) && (px > -8))
            {

                if (WORLD_BACK[wY][wX] == 140) //
                {
                    drawSprite(px, wY * 8, cloud1_32x24.width, cloud1_32x24.height, cloud1_32x24.pixel_data, 1);
                }
                else if (WORLD_BACK[wY][wX] == 144) //
                {
                    drawSprite(px, wY * 8, cloud2_32x24.width, cloud2_32x24.height, cloud2_32x24.pixel_data, 1);
                }
                else if (WORLD_BACK[wY][wX] == 150) //
                {
                    drawSprite(px, wY * 8, montagne_left_8x8.width, montagne_left_8x8.height, montagne_left_8x8.pixel_data, 1);
                }
                else if (WORLD_BACK[wY][wX] == 151) //
                {
                    drawSprite(px, wY * 8, montagne_right_8x8.width, montagne_right_8x8.height, montagne_right_8x8.pixel_data, 1);
                }
                else if (WORLD_BACK[wY][wX] == 152) //
                {
                    drawSprite(px, wY * 8, montagne_middle_8x8.width, montagne_middle_8x8.height, montagne_middle_8x8.pixel_data, 1);
                }
                else if (WORLD_BACK[wY][wX] == 153) //
                {
                    drawSprite(px, wY * 8, montagne_top_32x8.width, montagne_top_32x8.height, montagne_top_32x8.pixel_data, 1);
                }
            }
        }
    }

    for (int wX = worldMIN_X; wX < worldMAX_X; wX++)
    {
#ifdef PIXEL_LIGHT
        int curLight_TL = MAX_LIGHT_INTENSITY;
        int curLight_TR = MAX_LIGHT_INTENSITY;
        int curLight_BL = MAX_LIGHT_INTENSITY;
        int curLight_BR = MAX_LIGHT_INTENSITY;
#endif
        int profondeurColonne = WORLD[HEADER_ROW][wX];

        int px = ((wX - worldMIN_X) * 16) - currentOffset_X;
        if ((px < ARCADA_TFT_WIDTH) && (px > -16))
        {
            //GERER LA LUM DANS LE CREATEWORLD ET LORS DES UPDATES (ajout ou suppr de briques) + la torche du player en realtime1
            //On pourra donc remettre worldMIN_Y ici plutot que 0

            for (int wY = worldMIN_Y; wY < worldMAX_Y; wY++)
            {
                int curLight = MAX_LIGHT_INTENSITY;
                uint8_t value = WORLD[wY][wX];

                int py = ((wY - worldMIN_Y) * 16) - currentOffset_Y;
                if ((py < ARCADA_TFT_HEIGHT) && (py > -16))
                {                    
#ifdef PIXEL_LIGHT
                    float distPlayer = sqrt( ((playerLightX - (px+8)) * (playerLightX - (px+8))) + ((playerLightY - (py+8)) * (playerLightY - (py+8)) ));
                    float distPlayer_TL = sqrt( ((playerLightX - (px)) * (playerLightX - (px))) + ((playerLightY - (py)) * (playerLightY - (py)) ));
                    float distPlayer_TR = sqrt( ((playerLightX - (px+16)) * (playerLightX - (px+16))) + ((playerLightY - (py)) * (playerLightY - (py)) ));
                    float distPlayer_BL = sqrt( ((playerLightX - (px)) * (playerLightX - (px))) + ((playerLightY - (py+16)) * (playerLightY - (py+16)) ));
                    float distPlayer_BR = sqrt( ((playerLightX - (px+16)) * (playerLightX - (px+16))) + ((playerLightY - (py+16)) * (playerLightY - (py+16)) ));
                    if (profondeurColonne != 0)
                    {
                        int delta = (wY - profondeurColonne)+1;
                        if (delta > 0)
                        {
                            curLight = curLight / delta;
                            curLight_TL = curLight_TL / delta;
                            curLight_TR = curLight_TR / delta;
                        }

                        int delta2 = ((wY+1) - profondeurColonne) +1;
                        if (delta2 > 0)
                        {
                            curLight_BL = curLight_BL / delta2;
                            curLight_BR = curLight_BL / delta2;
                        }
                    }
                    if (distPlayer < 25*16)
                    {
                        curLight = curLight + (PLAYER_LIGHT_INTENSITY / sqrt(distPlayer));
                        curLight_TL = curLight_TL + (PLAYER_LIGHT_INTENSITY / sqrt(distPlayer_TL));
                        curLight_TR = curLight_TR + (PLAYER_LIGHT_INTENSITY / sqrt(distPlayer_TR));
                        curLight_BL = curLight_BL + (PLAYER_LIGHT_INTENSITY / sqrt(distPlayer_BL));
                        curLight_BR = curLight_BR + (PLAYER_LIGHT_INTENSITY / sqrt(distPlayer_BR));
                    }
                    curLight = curLight + AMBIENT_LIGHT_INTENSITY;
                    curLight_TL = curLight_TL + AMBIENT_LIGHT_INTENSITY;
                    curLight_TR = curLight_TR + AMBIENT_LIGHT_INTENSITY;
                    curLight_BL = curLight_BL + AMBIENT_LIGHT_INTENSITY;
                    curLight_BR = curLight_BR + AMBIENT_LIGHT_INTENSITY;
                    curLight = min(curLight, MAX_LIGHT_INTENSITY);
#else
                    if (profondeurColonne != 0)
                    {
                        int delta = (wY - profondeurColonne)+1;
                        if (delta > 0)
                            curLight = curLight / delta;
                    }
                    if (wY <= (playerLightY+1) || value == 0 || value == 0x7F) 
                    {
                        float distPlayer = sqrt((playerLightX - px) * (playerLightX - px) + (playerLightY - py) * (playerLightY - py));
                        // @todo gerer la non propagassion de la lumiere dans les murs...
                        if (distPlayer < 64)
                        {
                            curLight = curLight + (PLAYER_LIGHT_INTENSITY / distPlayer);
                        }
                    }
                    curLight = curLight + AMBIENT_LIGHT_INTENSITY;
                    curLight = min(curLight, MAX_LIGHT_INTENSITY);
#endif
#ifdef DEBUG
    curLight = MAX_LIGHT_INTENSITY;
#endif

#ifdef PIXEL_LIGHT
                    if (value == 0)
                    {
                        //rien le fond
                    }
                    else if (value == BLOCK_UNDERGROUND_AIR)
                    {
                        //background de terrassement
                        drawTile2(px, py, back_ground.pixel_data, curLight, curLight_TL, curLight_TR, curLight_BL, curLight_BR);
                        // fond du sous terrain : drawSprite(px, py, ground_top.width, ground_top.height, ground_top.pixel_data, 1, curLight);
                    }
                    else if (value == BLOCK_GROUND_TOP) //
                    {
                        drawTile2(px, py, ground_top.pixel_data, curLight, curLight_TL, curLight_TR, curLight_BL, curLight_BR);
                    }
                    else if (value == BLOCK_GROUND) //
                    {
                        drawTile2(px, py, ground.pixel_data, curLight, curLight_TL, curLight_TR, curLight_BL, curLight_BR);
                    }
                    else if (value == BLOCK_GROUND_ROCK) //
                    {
                        drawTile2(px, py, rock_empty.pixel_data, curLight, curLight_TL, curLight_TR, curLight_BL, curLight_BR);
                    }                    
                    else if (value == BLOCK_ROCK) //
                    {
                        drawTile2(px, py, rock_empty.pixel_data, curLight, curLight_TL, curLight_TR, curLight_BL, curLight_BR);
                    }
                    else if (value == BLOCK_CHARBON) //
                    {
                        drawTile2(px, py, rock_charbon.pixel_data, curLight, curLight_TL, curLight_TR, curLight_BL, curLight_BR);
                    }
                    else if (value == BLOCK_CUIVRE) //
                    {
                        drawTile2(px, py, rock_cuivre.pixel_data, curLight, curLight_TL, curLight_TR, curLight_BL, curLight_BR);
                    }
                    else if (value == BLOCK_FER) //
                    {
                        drawTile2(px, py, rock_fer.pixel_data, curLight, curLight_TL, curLight_TR, curLight_BL, curLight_BR);
                    }
                    else if (value == BLOCK_ARGENT) //
                    {
                        drawTile2(px, py, rock_argent.pixel_data, curLight, curLight_TL, curLight_TR, curLight_BL, curLight_BR);
                    }
                    else if (value == BLOCK_JADE) //
                    {
                        drawTile2(px, py, rock_jade.pixel_data, curLight, curLight_TL, curLight_TR, curLight_BL, curLight_BR);
                    }
                    else if (value == BLOCK_OR) //
                    {
                        drawTile2(px, py, rock_or.pixel_data, curLight, curLight_TL, curLight_TR, curLight_BL, curLight_BR);
                    }
                    else if (value == BLOCK_REDSTONE) //
                    {
                        drawTile2(px, py, rock_redstone.pixel_data, curLight, curLight_TL, curLight_TR, curLight_BL, curLight_BR);
                    }
                   
                    else
                    {
                        drawTile2(px, py, rock_empty.pixel_data, curLight, curLight_TL, curLight_TR, curLight_BL, curLight_BR);
                    }                    
#else
                    if (value == 0)
                    {
                        //rien le fond
                    }
                    else if (value == BLOCK_UNDERGROUND_AIR)
                    {
                        //background de terrassement
                        drawTile(px, py, back_ground.pixel_data, curLight);
                        // fond du sous terrain : drawSprite(px, py, ground_top.width, ground_top.height, ground_top.pixel_data, 1, curLight);
                    }
                    else if (value == BLOCK_GROUND_TOP) //
                    {
                        drawTile(px, py, ground_top.pixel_data, curLight);
                    }
                    else if (value == BLOCK_GROUND) //
                    {
                        drawTile(px, py, ground.pixel_data, curLight);
                    }
                    else if (value == BLOCK_GROUND_ROCK) //
                    {
                        drawTile(px, py, rock_empty.pixel_data, curLight);
                    }                    
                    else if (value == BLOCK_ROCK) //
                    {
                        drawTile(px, py, rock_empty.pixel_data, curLight);
                    }
                    else if (value == BLOCK_CHARBON) //
                    {
                        drawTile(px, py, rock_charbon.pixel_data, curLight);
                    }
                    else if (value == BLOCK_CUIVRE) //
                    {
                        drawTile(px, py, rock_cuivre.pixel_data, curLight);
                    }
                    else if (value == BLOCK_FER) //
                    {
                        drawTile(px, py, rock_fer.pixel_data, curLight);
                    }
                    else if (value == BLOCK_ARGENT) //
                    {
                        drawTile(px, py, rock_argent.pixel_data, curLight);
                    }
                    else if (value == BLOCK_JADE) //
                    {
                        drawTile(px, py, rock_jade.pixel_data, curLight);
                    }
                    else if (value == BLOCK_OR) //
                    {
                        drawTile(px, py, rock_or.pixel_data, curLight);
                    }
                    else if (value == BLOCK_REDSTONE) //
                    {
                        drawTile(px, py, rock_redstone.pixel_data, curLight);
                    }
                    else if (value == 0x60) //
                    {
                        drawSprite(px, py, grass_left_8x8.width, grass_left_8x8.height, grass_left_8x8.pixel_data, 1, curLight);
                    }
                    else if (value == 0x61) //
                    {
                        drawSprite(px, py, grass_middle_8x8.width, grass_middle_8x8.height, grass_middle_8x8.pixel_data, 1, curLight);
                    }
                    else if (value == 0x62) //
                    {
                        drawSprite(px, py, grass_right_8x8.width, grass_right_8x8.height, grass_right_8x8.pixel_data, 1, curLight);
                    }
                    else if (value == 0x63)
                    {
                        //          drawXbm(px, py, tree_bl_width, tree_bl_height, tree_bl_bits, ARCADA_GREEN);
                        drawSprite(px, py, tree_bottom_8x8.width, tree_bottom_8x8.height, tree_bottom_8x8.pixel_data, 1, curLight);
                    }
                    else if (value == 0x64)
                    {
                        //          drawXbm(px, py, tree_tl_width, tree_tl_height, tree_tl_bits, ARCADA_GREEN);
                        drawSprite(px, py, tree_top_8x8.width, tree_top_8x8.height, tree_top_8x8.pixel_data, 1, curLight);
                    }
                    else
                    {
                        drawTile(px, py, rock_empty.pixel_data, curLight);
/*                        int R = (75 + value) * (curLight / MAX_LIGHT_INTENSITY);
                        int G = (72 + value) * (curLight / MAX_LIGHT_INTENSITY);
                        int B = (59 + value) * (curLight / MAX_LIGHT_INTENSITY);
                        canvas->fillRect(px, py, 16, 16, rgbTo565(R, G, B));*/
                    }
                    /*
                    else if (WORLD[wY][wX] == 'o') //16 coin
                    {
                        int currentCoinFrame = (briquesFRAMES / 5) % 4;
                        unsigned char *bitmap = (unsigned char *)coin_8x8.pixel_data;
                        switch (currentCoinFrame)
                        {
                            case 0:
                                bitmap = (unsigned char *)coin_8x8.pixel_data;
                                break;
                            case 1:
                                bitmap = (unsigned char *)coin2_8x8.pixel_data;
                                break;
                            case 2:
                                bitmap = (unsigned char *)coin3_8x8.pixel_data;
                                break;
                            case 3:
                                bitmap = (unsigned char *)coin4_8x8.pixel_data;
                                break;
                        }
                        drawSprite(px, py, coin4_8x8.width, coin4_8x8.height, (const unsigned char *)bitmap, 1);
                    }*/
#endif
                }
            }
        }
    }

    briquesFRAMES++;
}

void drawParticles()
{
    int i;
    for (i = 0; i < parts.getActiveParticles(); i++)
    {
        int x, y, velX, velY;
        x = parts.particles[i].x;
        y = parts.particles[i].y;
        /*  velX = parts.particles[i].velX;
    velY = parts.particles[i].velY;
    
    if (velX > 0) {
      velX++;
    } else {
      velX--;
    }
    if (velY > 0) {
      velY++;
    } else {
      velY--;
    }
*/

        if ((x < 0) || (x >= SCREEN_WIDTH))
            continue;
        if ((x < 0) || (x >= SCREEN_WIDTH))
            continue;

        canvas->drawPixel(x, y, parts.particles[i].color);
        //arcada.display->setPixel(x - (velX/10), y - (velY/10));
        //arcada.display->setPixel(x - (velX/20), y - (velY/20));
        //glcd.setpixel(x - (velX/5), y - (velY/5), ARCADA_WHITE); // uncomment for trail-deleting action!
        // NOTE: this doesn't remove pixels of deleted particles
    }
}

void debugInfos()
{
    static int lastX = 0;
    static int lastY = 0;

    if ((lastX != posXPlayerInWorld) || (lastY != posYPlayerInWorld))
    {
        Serial.printf("WPos1:%d,%d\n", posXPlayerInWorld, posYPlayerInWorld);
        Serial.printf("Pix:%d,%d\n", player_pos_x, player_pos_y);

        lastX = posXPlayerInWorld;
        lastY = posYPlayerInWorld;
    }

    int debugX = (((posXPlayerInWorld)-worldMIN_X) * 16) - currentOffset_X;
    int debugY = (((posYPlayerInWorld)-worldMIN_Y) * 16) - currentOffset_Y;

    int debugXF = (((posXFront)-worldMIN_X) * 16) - currentOffset_X;
    int debugYF = debugY;

    int debugXD = debugX;
    int debugYD = (((posYDown)-worldMIN_Y) * 16) - currentOffset_Y;

    int debugXU = debugX;
    int debugYU = (((posYUp)-worldMIN_Y) * 16) - currentOffset_Y;

    canvas->drawRect(debugX, debugY, 16, 16, ARCADA_WHITE);
    canvas->drawRect(debugXF, debugYF, 16, 16, ARCADA_RED);
    canvas->drawRect(debugXD, debugYD, 16, 16, ARCADA_GREENYELLOW);
    canvas->drawRect(debugXU, debugYU, 16, 16, ARCADA_CYAN);

    //   canvas->drawRect((player_pos_x - (worldMIN_X * 16)) - currentOffset_X, (player_pos_y - (worldMIN_Y * 16)) - currentOffset_Y, 16, 16, ARCADA_BLUE);

    for (int enC = 0; enC < CURRENT_LEVEL_ENNEMIES; enC++)
    {
        Tennemy *currentEnnemy = &ENNEMIES[enC];
        if (currentEnnemy->bIsAlive > 0)
        {
            if ((currentEnnemy->worldX >= worldMIN_X) && (currentEnnemy->worldX <= worldMAX_X))
            {

                int px = currentEnnemy->x;
                int py = currentEnnemy->y;

                canvas->drawRect((px - (worldMIN_X * 16)) - currentOffset_X, py - 16, 16, 16, ARCADA_RED);
            }
        }
    }
}

void drawHud()
{
    if (counterActionB > 0)
    {
        int pX = (((cibleX)-worldMIN_X) * 16) - currentOffset_X;
        int pY = (((cibleY)-worldMIN_Y) * 16) - currentOffset_Y;

        if (pX > -16 && pX < ARCADA_TFT_WIDTH && pY > -16 && pY < ARCADA_TFT_HEIGHT)
        {
            //Test action pour le sprite, pour l'instant la pioche
            drawSprite(pX, pY, pioche.width, pioche.height, pioche.pixel_data, RUN_DIR);
        }
    }
}

void drawWorld()
{
    drawTiles();

    drawEnnemies();
    drawItems();

    drawPlayer();

    drawParticles();

    drawHud();

    // debugInfos();
}

void pixelToWorld(int *pX, int *pY)
{
    int wX = *pX / 16;
    int wY = *pY / 16;
}

int checkCollisionTo(int x, int y, int newX, int newY)
{
    return 0;
}

int checkCollisionAt(int newX, int newY)
{
    int newposXPlayerInWorld;
    int newposYPlayerInWorld;

    //On ajoute 4 a la pos car le sprite est decallé
    newposXPlayerInWorld = newX / 16;
    newposYPlayerInWorld = newY / 16;

    return getWorldAt(newposXPlayerInWorld, newposYPlayerInWorld);
}

void calculatePlayerCoords()
{
    posXPlayerInWorld_TL = player_pos_x / 16;
    posXPlayerInWorld_BR = (player_pos_x + 15) / 16;
    posYPlayerInWorld_TL = player_pos_y / 16;
    posYPlayerInWorld_BR = (player_pos_y + 15) / 16;

    posXPlayerInWorld = (player_pos_x + 8) / 16;
    posYPlayerInWorld = (player_pos_y + 8) / 16;

    if (RUN_DIR <= 0)
    {
        posXFront = (player_pos_x - 1) / 16;
        posXFront = max(posXFront, 0);
    }
    else
    {
        posXFront = (player_pos_x + 16) / 16;
        posXFront = min(posXFront, WORLD_WIDTH - 1);
    }
    posYDown = (player_pos_y + 16) / 16;
    posYDown = min(posYDown, WORLD_HEIGHT - 1);

    posYUp = roundf((player_pos_y - 1) / 16);
    posYDown = max(posYDown, 0);
}

void calculateWorldCoordinates()
{
    cameraX = player_pos_x - (ARCADA_TFT_WIDTH / 2);
    cameraX = max(cameraX, 0);
    cameraX = min(cameraX, (WORLD_WIDTH * 16) - ARCADA_TFT_WIDTH);

    cameraY = player_pos_y - (ARCADA_TFT_HEIGHT / 2);
    cameraY = max(cameraY, 0);
    cameraY = min(cameraY, (WORLD_HEIGHT * 16) - ARCADA_TFT_HEIGHT);

    worldX = cameraX / 16;
    currentOffset_X = cameraX % 16;
    worldMIN_X = min(worldX, (WORLD_WIDTH - 1) - (ARCADA_TFT_WIDTH / 16));
    worldMIN_X = max(worldMIN_X, 0);
    worldMAX_X = worldMIN_X + (ARCADA_TFT_WIDTH / 16);
    worldMAX_X = min(worldMAX_X + 1, WORLD_WIDTH);

    worldY = cameraY / 16;
    currentOffset_Y = cameraY % 16;
    worldMIN_Y = min(worldY, (WORLD_HEIGHT - 1) - (ARCADA_TFT_HEIGHT / 16));
    worldMIN_Y = max(worldMIN_Y, 0);
    worldMAX_Y = worldMIN_Y + (ARCADA_TFT_HEIGHT / 16);
    worldMAX_Y = min(worldMAX_Y + 1, WORLD_HEIGHT);
}

void checkPlayerState()
{
    int posYUp = 0;

    posYUp = posYPlayerInWorld_TL - 1;
    posYUp = max(0, posYUp);

    //Deux briques SUR player
    brique_PLAYER = getWorldAt(posXPlayerInWorld, posYPlayerInWorld);

    //Au dessus
    brique_UP = getWorldAt(posXPlayerInWorld, posYUp);

    //En dessous
    brique_DOWN = getWorldAt(posXPlayerInWorld, posYDown);
    //  brique_DOWN_FRONT = getWorldAt(posXPlayerInWorld, posYPlayerInWorld + 1);

    //Devant
    brique_FRONT = getWorldAt(posXFront, posYPlayerInWorld);

    switch (brique_PLAYER)
    {
    case 'o':
        setWorldAt(posXPlayerInWorld, posYPlayerInWorld, 0);
        SCORE++;
        //    sndPlayerCanal1.play(AudioSampleSmb_coin);
        break;
    case 'w':
    case 'd':
        SCORE += 20;
        set_wining();
        return;
    }

    //Collisions mushrooms
    for (int itC = 0; itC < CURRENT_LEVEL_ITEMS; itC++)
    {
        Titem *currentItem = &ITEMS[itC];
        if ((currentItem->bIsAlive > 127) && currentItem->bIsActive)
        {
            if (((currentItem->worldX >= (worldMIN_X - 1)) && (currentItem->worldX <= (worldMAX_X + 1))) &&
                ((currentItem->worldY >= (worldMIN_Y - 1)) && (currentItem->worldY <= (worldMAX_Y + 1))))
            {

                int px = currentItem->x;
                int py = currentItem->y;

                //BBox
                if (px < player_pos_x + 16 &&
                    px + 16 > player_pos_x &&
                    py < player_pos_y + 16 &&
                    py + 16 > player_pos_y)
                {
                    //  Serial.println("Collision.");
                    //          sndPlayerCanal1.play(AudioSamplePlayershell);
                    SCORE += 50;
                    killItem(currentItem, px, py);
                    //EFFECT
                    //...
                }
            }
        }
    }

    bool bRebond = false;
    //Collisions ennemis
    for (int enC = 0; enC < CURRENT_LEVEL_ENNEMIES; enC++)
    {
        Tennemy *currentEnnemy = &ENNEMIES[enC];
        if (currentEnnemy->bIsAlive > 127)
        {
            if (((currentEnnemy->worldX >= (worldMIN_X - 1)) && (currentEnnemy->worldX <= (worldMAX_X + 1))) &&
                ((currentEnnemy->worldY >= (worldMIN_Y - 1)) && (currentEnnemy->worldY <= (worldMAX_Y + 1))))
            {

                int px = currentEnnemy->x;
                int py = currentEnnemy->y;

                //BBox
                if (px < player_pos_x + 14 &&
                    px + 14 > player_pos_x &&
                    py < player_pos_y + 14 &&
                    py + 14 > player_pos_y)
                {
                    //  Serial.println("Collision.");
                    if (bFalling || bRebond)
                    {
                        //            sndPlayerCanal1.play(AudioSamplePlayershell);
                        SCORE += 5;
                        killEnnemy(currentEnnemy, px, py);
                        //rebond
                        set_jumping();
                        jumpPhase = 6;
                        bRebond = true;
                    }
                    else
                    {
                        set_touched();
                        break;
                    }
                }
            }
        }
    }
}

void updatePlayer()
{
    bool bWasOnGround = bOnGround;

    if (bDying)
    {
        jumpPhase += 2;

        if (jumpPhase < 14)
        {
            player_pos_y = player_pos_y - (14 - jumpPhase);
        }
        else
        {
            player_pos_y = player_pos_y + (FALLING_SPEED / 2);
        }
    }
    else if (bWin)
    {
        calculatePlayerCoords();

        if ((brique_FRONT == 0 || brique_FRONT == BLOCK_UNDERGROUND_AIR) &&
            ((RUN_DIR > 0 && (player_pos_x < ((WORLD_WIDTH - 1) * 16) - ARCADA_TFT_WIDTH)) ||
             (RUN_DIR < 0 && (player_pos_x >= 2))))
        {
            player_pos_x += 1;
            bMoving = true;
        }

        //Particules feu artifice ?
        player_pos_y = player_pos_y + FALLING_SPEED;

        calculatePlayerCoords();
    }
    else
    {
        #ifdef DEBUG
        if (pressed_buttons & ARCADA_BUTTONMASK_LEFT)
        {
            player_pos_x -= 4;
            player_pos_x = max(0, player_pos_x);
        }
        if (pressed_buttons & ARCADA_BUTTONMASK_RIGHT)
        {
            player_pos_x += 4;
            player_pos_x = min(((WORLD_WIDTH - 1) * 16), player_pos_x);
        }
        if (pressed_buttons & ARCADA_BUTTONMASK_UP)
        {
            player_pos_y -= 4;
            player_pos_y = max(0, player_pos_y);
        }
        if (pressed_buttons & ARCADA_BUTTONMASK_DOWN)
        {
            player_pos_y += 4;
            player_pos_y = min(((WORLD_HEIGHT - 1) * 16), player_pos_y);
        }
        #else
            checkPlayerInputs();
            updatePlayerVelocities();
            updatePlayerPosition();

            checkPlayerCollisionsWorld();
        #endif


        //Position player
        calculateWorldCoordinates();
        calculatePlayerCoords();

        computePlayerAnimations();
    }
}

void computePlayerAnimations()
{
    //Serial.printf("counterActionB:%d bJumping:%d bFalling:%d bOnGround:%d bDoWalk:%d bMoving:%d\n", counterActionB, bJumping, bFalling, bOnGround, bDoWalk, bMoving);
    if (counterActionB > 0)
    {
        //@todo : Test type action, 
        //pour le moment dig
        set_digging();
    }
    else
    {
        if (bJumping)
        {
            set_jumping();
        }
        else if (bFalling)
        {
            set_falling();
        }
        else
        {
            if (bOnGround)
            {
                if (bDoWalk) // demannde de deplacement)
                {
                    if (bMoving) //On a bougé
                    {
                        set_walking();
                    }
                    else // @todo push
                        set_idle();
                }
                else
                {
                    set_idle();
                }
            }
        }
    }
}

void updateEnnemies()
{
    for (int enC = 0; enC < CURRENT_LEVEL_ENNEMIES; enC++)
    {
        Tennemy *currentEnnemy = &ENNEMIES[enC];
        if (currentEnnemy->bIsAlive > 127)
        {
            if ((currentEnnemy->worldX >= 1) && (currentEnnemy->worldX <= worldMAX_X))
            {
                currentEnnemy->bIsActive = true;
                //Deplacement
                int newX = currentEnnemy->x + currentEnnemy->direction;
                int posXEnnemyInWorld = newX / 16;
                int posYEnnemyInWorld = currentEnnemy->y / 16;

                //Deux briques SUR player
                int brique_ENNEMY = getWorldAt(currentEnnemy->x / 16, posYEnnemyInWorld);
                int brique_ENNEMY_DOWN = getWorldAt(posXEnnemyInWorld, posYEnnemyInWorld + 1);
                int brique_ENNEMY_FRONT = getWorldAt(posXEnnemyInWorld, posYEnnemyInWorld);
                if ((brique_ENNEMY_FRONT <= 'Z') || (brique_ENNEMY_DOWN == 0))
                {
                    currentEnnemy->direction = -currentEnnemy->direction;
                }
                else
                {
                    currentEnnemy->x = newX;
                    currentEnnemy->worldX = posXEnnemyInWorld;
                    currentEnnemy->worldY = posYEnnemyInWorld;
                }
            }
            else
                currentEnnemy->bIsActive = false;
        }
    }
}

void updateItems()
{
    for (int enC = 0; enC < CURRENT_LEVEL_ITEMS; enC++)
    {
        Titem *currentItem = &ITEMS[enC];
        if (currentItem->bIsAlive > 127)
        {
            if (currentItem->iSpawning > 0)
            {
                currentItem->iSpawning--;
            }
            else
                currentItem->bIsActive = true;

            //if ((currentItem->worldX >= worldMIN_X) && (currentItem->worldX <= worldMAX_X))
            if (currentItem->bIsActive)
            {

                //Deplacement
                int newX = currentItem->x + currentItem->direction;
                int posXItemInWorld = newX / 16;
                int posYItemInWorld = currentItem->y / 16;

                //Deux briques SUR player
                int brique_ITEM = getWorldAt(currentItem->x / 16, posYItemInWorld);
                int brique_ITEM_DOWN = getWorldAt(posXItemInWorld, posYItemInWorld + 1);
                int brique_ITEM_FRONT = getWorldAt(posXItemInWorld, posYItemInWorld);

                if (brique_ITEM_DOWN <= 'Z')
                {
                    currentItem->y = posYItemInWorld * 16;
                }
                else
                {
                    currentItem->y = currentItem->y + FALLING_SPEED;
                }

                if ((brique_ITEM_FRONT <= 'Z'))
                {
                    currentItem->direction = -currentItem->direction;
                }
                else
                {
                    currentItem->x = newX;
                    currentItem->worldX = posXItemInWorld;
                    currentItem->worldY = posYItemInWorld;
                }
            }
            // else
            // currentItem->bIsActive = false;
        }
    }
}

void updatePlayerVelocities()
{
    //Gravity
    player_speed_y = player_speed_y + FALLING_SPEED;

    //Frotements
    if (bOnGround)
    {
        if (player_speed_x > 0)
            player_speed_x--;
        else if (player_speed_x < 0)
            player_speed_x++;
    }

    if (player_speed_x > MAX_SPEED_X)
        player_speed_x = MAX_SPEED_X;
    else if (player_speed_x < -MAX_SPEED_X)
        player_speed_x = -MAX_SPEED_X;

    if (player_speed_y > MAX_SPEED_Y)
        player_speed_y = MAX_SPEED_Y;
    else if (player_speed_y < -MAX_SPEED_Y)
        player_speed_y = -MAX_SPEED_Y;
}

void updatePlayerPosition()
{
    //if (bDoWalk)
    {
        //CHECK COLLISION a refaire propre
        if (player_speed_x > 0)
        {
            new_player_pos_x = player_pos_x + player_speed_x;
            bMoving = true;

            new_player_pos_x = min(((WORLD_WIDTH - 1) * 16), new_player_pos_x);
        }
        else if (player_speed_x < 0)
        {
            new_player_pos_x = player_pos_x + player_speed_x;
            bMoving = true;

            new_player_pos_x = max(0, new_player_pos_x);
        }
    }

    new_player_pos_y = player_pos_y + player_speed_y;
}

void checkPlayerCollisionsWorld()
{
    //X
    if (player_speed_x <= 0)
    {
        uint8_t tileTL = getWorldAtPix(new_player_pos_x, player_pos_y);
        uint8_t tileBL = getWorldAtPix(new_player_pos_x, player_pos_y + 15);
        if ((tileTL != BLOCK_AIR && tileTL != BLOCK_UNDERGROUND_AIR) || (tileBL != BLOCK_AIR && tileBL != BLOCK_UNDERGROUND_AIR))
        {
            new_player_pos_x = (new_player_pos_x - (new_player_pos_x % 16)) + 16;
            player_speed_x = 0;
        }
    }
    else
    {
        uint8_t tileTR = getWorldAtPix(new_player_pos_x + 16, player_pos_y);
        uint8_t tileBR = getWorldAtPix(new_player_pos_x + 16, player_pos_y + 15);
        if ((tileTR != BLOCK_AIR && tileTR != BLOCK_UNDERGROUND_AIR) || (tileBR != BLOCK_AIR && tileBR != BLOCK_UNDERGROUND_AIR))
        {
            new_player_pos_x = (new_player_pos_x - (new_player_pos_x % 16));
            player_speed_x = 0;
        }
    }
    //Y
    bOnGround = false;
    bJumping = false;
    if (player_speed_y <= 0)
    {
        //+5 et +12 au lieu de +0 et +15 pour compenser la boundingbox du sprite
        uint8_t tileTL = getWorldAtPix(new_player_pos_x + 5, new_player_pos_y);
        uint8_t tileTR = getWorldAtPix(new_player_pos_x + 12, new_player_pos_y);
        if ((tileTL != BLOCK_AIR && tileTL != BLOCK_UNDERGROUND_AIR) || (tileTR != BLOCK_AIR && tileTR != BLOCK_UNDERGROUND_AIR))
        {
            new_player_pos_y = (new_player_pos_y - (new_player_pos_y % 16)) + 16;
            player_speed_y = 0;
        }
        else
        {
            bJumping = true;
        }
    }
    else
    {
        bFalling = true;
        //+5 et +12 au lieu de +0 et +15 pour compenser la boundingbox du sprite
        uint8_t tileBL = getWorldAtPix(new_player_pos_x + 5, new_player_pos_y + 16);
        uint8_t tileBR = getWorldAtPix(new_player_pos_x + 12, new_player_pos_y + 16);
        if ((tileBL != BLOCK_AIR && tileBL != BLOCK_UNDERGROUND_AIR) || (tileBR != BLOCK_AIR && tileBR != BLOCK_UNDERGROUND_AIR))
        {
            new_player_pos_y = (new_player_pos_y - (new_player_pos_y % 16));
            bOnGround = true;
            bFalling = false;
            player_speed_y = 0;
        }
    }

    player_pos_x = new_player_pos_x;
    player_pos_y = new_player_pos_y;
}

void checkPlayerInputs()
{
    bDoWalk = false;

    if ((pressed_buttons & ARCADA_BUTTONMASK_B) && bOnGround)
    {
        counterActionB++;
        int lastCibleX = cibleX;
        int lastCibleY = cibleY;
        //Ciblage
        if ((pressed_buttons & ARCADA_BUTTONMASK_LEFT) || (pressed_buttons & ARCADA_BUTTONMASK_RIGHT))
        {
            if (pressed_buttons & ARCADA_BUTTONMASK_DOWN)
            {
                cibleX = posXFront;
                cibleY = posYDown;
            }
            else if (pressed_buttons & ARCADA_BUTTONMASK_UP)
            {
                cibleX = posXFront;
                cibleY = posYUp;
            }
            else
            {
                cibleX = posXFront;
                cibleY = posYPlayerInWorld;
            }
        }
        else
        {
            if (pressed_buttons & ARCADA_BUTTONMASK_UP)
            {
                cibleX = posXPlayerInWorld;
                cibleY = posYUp;
            }
            else
            {
                cibleX = posXPlayerInWorld;
                cibleY = posYDown;
            }
        }

        if ((cibleX != lastCibleX) || (cibleY != lastCibleY))
            counterActionB = 0;

        if (counterActionB > 15 * 3) //Action (pour le moment minage seulement)
        {
            uint8_t value = 0xFF;
            //DIG bDoJump = true;
            //Thrust
            counterActionB = 0;

            value = getWorldAt(cibleX, cibleY);

            //Serial.printf("Dig:%d,%d v:%d\n", cibleX, cibleY, value);
            if (value != 0 && value != BLOCK_UNDERGROUND_AIR && posYDown < (WORLD_HEIGHT - 1))
            {
                if (value < BLOCK_ROCK)
                    value = BLOCK_AIR;
                else
                    value = BLOCK_UNDERGROUND_AIR;

                setWorldAt(cibleX, cibleY, value);
                updateHauteurColonne(cibleX, cibleY);
                //update hauteur colonne
            }
        }
        else
        {
            //Animation action
        }
    }
    else
    {
        counterActionB = 0;
        //cibleX = cibleY = 0;

        if (just_pressed & ARCADA_BUTTONMASK_A)
        {
            if (player_speed_y == 0 && bOnGround)
            {
                bDoJump = true;
                sndPlayerCanal1.play(AudioSample__Jump);
                //Thrust
                player_speed_y = -JUMP_SPEED;
            }
        }

        if (pressed_buttons & ARCADA_BUTTONMASK_LEFT)
        {
            RUN_DIR = -1;
            bDoWalk = true;
            if (bOnGround)
                player_speed_x += -RUNNING_SPEED;
            else
            {
                if (player_speed_y < 0)
                    player_speed_x += -WALKING_SPEED;
                else if (player_speed_x == 0)
                    player_speed_x = -WALKING_SPEED;
            }
        }
        else if (pressed_buttons & ARCADA_BUTTONMASK_RIGHT)
        {
            RUN_DIR = 1;
            bDoWalk = true;
            if (bOnGround)
                player_speed_x += RUNNING_SPEED;
            else
            {
                if (player_speed_y < 0)
                    player_speed_x += WALKING_SPEED;
                else if (player_speed_x == 0)
                    player_speed_x = WALKING_SPEED;
            }
        }
    }
}

void updateGame()
{
    int lastX = 0;
    bMoving = false;

    if (bDying)
    {
        count_player_die++;
        if (count_player_die >= PLAYER_DIE_FRAMES)
        {
            //On affiche gameover et on quitte
            initGame();
        }
        updatePlayer();
    }
    else if (bWin)
    {
        //anim winning puis initGame
        count_player_win++;
        if (count_player_win >= PLAYER_WIN_FRAMES)
        {
            parts.clearParticles();
            //On affiche BRAVO et on quitte

            initGame();
        }
        updatePlayer();
    }
    else
    {

        updatePlayer();

        /*
        updateEnnemies();
        updateItems();*/

        lastX = cameraX;
    }

    //Ps
    parts.moveParticles();
}

void updateMenu()
{
    if (just_pressed & ARCADA_BUTTONMASK_A)
        gameState = STATE_PLAYING;
}

void displayMenu()
{
    // digital clock display of the time
    arcada.display->fillScreen(ARCADA_GREEN);

    arcada.infoBox("Press A", 0);
}

void readInputs()
{
    pressed_buttons = arcada.readButtons();
    just_pressed = arcada.justPressedButtons();
}

void loop()
{
    uint32_t startTime, elapsedTime;

    if (tick.isExpired())
    {
        startTime = millis();

        readInputs();
        if (gameState == STATE_MENU)
        {
            updateMenu();
            displayMenu();
        }
        else if (gameState == STATE_PLAYING)
        {
            updateGame();
            displayGame();
        }

        elapsedTime = millis() - startTime;

      //  Serial.printf("E:%d\n", elapsedTime);
        tick.repeat();
    }
    updateSoundManager();
}

void anim_player_idle()
{
    if (player_current_framerate == PLAYER_FRAMERATE_IDLE)
    {
        player_anim++;
        player_current_framerate = 0;
    }
    player_current_framerate++;

    if (player_anim > PLAYER_FRAME_IDLE_3)
    {
        player_anim = PLAYER_FRAME_IDLE_1;
    }

    switch (player_anim)
    {
    case PLAYER_FRAME_IDLE_1:
        drawSprite(player_pos_x - cameraX, player_pos_y - cameraY, player_idle1.width, player_idle1.height, player_idle1.pixel_data, RUN_DIR);
        break;
    case PLAYER_FRAME_IDLE_2:
        drawSprite(player_pos_x - cameraX, player_pos_y - cameraY, player_idle2.width, player_idle2.height, player_idle2.pixel_data, RUN_DIR);
        break;
    case PLAYER_FRAME_IDLE_3:
        drawSprite(player_pos_x - cameraX, player_pos_y - cameraY, player_idle3.width, player_idle3.height, player_idle3.pixel_data, RUN_DIR);
        break;
    }
}

void anim_player_diging() {
    if (player_current_framerate == PLAYER_FRAMERATE_ACTION)
    {
        player_anim++;
        player_current_framerate = 0;
    }
    player_current_framerate++;

    if (player_anim > PLAYER_FRAME_ACTION_6)
    {
        player_anim = PLAYER_FRAME_ACTION_1;
    }

    switch (player_anim)
    {
    case PLAYER_FRAME_ACTION_1:
        drawSprite(player_pos_x - cameraX, player_pos_y - cameraY, player_action1.width, player_action1.height, player_action1.pixel_data, RUN_DIR);
        break;
    case PLAYER_FRAME_ACTION_2:
        drawSprite(player_pos_x - cameraX, player_pos_y - cameraY, player_action2.width, player_action2.height, player_action2.pixel_data, RUN_DIR);

        break;
    case PLAYER_FRAME_ACTION_3:
        drawSprite(player_pos_x - cameraX, player_pos_y - cameraY, player_action3.width, player_action3.height, player_action3.pixel_data, RUN_DIR);
        break;
    case PLAYER_FRAME_ACTION_4:
        drawSprite(player_pos_x - cameraX, player_pos_y - cameraY, player_action4.width, player_action4.height, player_action4.pixel_data, RUN_DIR);
        break;
    case PLAYER_FRAME_ACTION_5:
        drawSprite(player_pos_x - cameraX, player_pos_y - cameraY, player_action5.width, player_action5.height, player_action5.pixel_data, RUN_DIR);
        break;
    case PLAYER_FRAME_ACTION_6:
        drawSprite(player_pos_x - cameraX, player_pos_y - cameraY, player_action6.width, player_action6.height, player_action6.pixel_data, RUN_DIR);
        break;
    }
}

void anim_player_walk()
{
    if (player_current_framerate == PLAYER_FRAMERATE_WALK)
    {
        player_anim++;
        player_current_framerate = 0;
    }
    player_current_framerate++;

    if (player_anim > PLAYER_FRAME_WALK_6)
    {
        player_anim = PLAYER_FRAME_WALK_1;
    }

    switch (player_anim)
    {
    case PLAYER_FRAME_WALK_1:
        drawSprite(player_pos_x - cameraX, player_pos_y - cameraY, player_walk1.width, player_walk1.height, player_walk1.pixel_data, RUN_DIR);
        break;
    case PLAYER_FRAME_WALK_2:
        drawSprite(player_pos_x - cameraX, player_pos_y - cameraY, player_walk2.width, player_walk2.height, player_walk2.pixel_data, RUN_DIR);

        break;
    case PLAYER_FRAME_WALK_3:
        drawSprite(player_pos_x - cameraX, player_pos_y - cameraY, player_walk3.width, player_walk3.height, player_walk3.pixel_data, RUN_DIR);
        break;
    case PLAYER_FRAME_WALK_4:
        drawSprite(player_pos_x - cameraX, player_pos_y - cameraY, player_walk4.width, player_walk4.height, player_walk4.pixel_data, RUN_DIR);
        break;
    case PLAYER_FRAME_WALK_5:
        drawSprite(player_pos_x - cameraX, player_pos_y - cameraY, player_walk5.width, player_walk5.height, player_walk5.pixel_data, RUN_DIR);
        break;
    case PLAYER_FRAME_WALK_6:
        drawSprite(player_pos_x - cameraX, player_pos_y - cameraY, player_walk6.width, player_walk6.height, player_walk6.pixel_data, RUN_DIR);
        break;
    }
}

void anim_player_jump()
{

    if (player_current_framerate == PLAYER_FRAMERATE_JUMP)
    {
        player_anim++;
        player_current_framerate = 0;
    }
    player_current_framerate++;

    if (player_anim > PLAYER_FRAME_JUMP_3)
    {
        player_anim = PLAYER_FRAME_JUMP_3; //On reste sur 3
    }

    switch (player_anim)
    {
    case PLAYER_FRAME_JUMP_1:
        drawSprite(player_pos_x - cameraX, player_pos_y - cameraY, player_jump1.width, player_jump1.height, player_jump1.pixel_data, RUN_DIR);
        break;
    case PLAYER_FRAME_JUMP_2:
        drawSprite(player_pos_x - cameraX, player_pos_y - cameraY, player_jump2.width, player_jump2.height, player_jump2.pixel_data, RUN_DIR);
        break;
    case PLAYER_FRAME_JUMP_3:
        drawSprite(player_pos_x - cameraX, player_pos_y - cameraY, player_jump3.width, player_jump3.height, player_jump3.pixel_data, RUN_DIR);
        break;
    }
}

void anim_player_falling()
{

    if (player_current_framerate == PLAYER_FRAMERATE_FALLING)
    {
        player_anim++;
        player_current_framerate = 0;
    }
    player_current_framerate++;

    if (player_anim > PLAYER_FRAME_FALLING_1)
    {
        player_anim = PLAYER_FRAME_FALLING_1; //On reste sur 1
    }

    switch (player_anim)
    {
    case PLAYER_FRAME_FALLING_1:
        drawSprite(player_pos_x - cameraX, player_pos_y - cameraY, player_falling1.width, player_falling1.height, player_falling1.pixel_data, RUN_DIR);
        break;
    }
}

void anim_player_mining()
{
}

void anim_player_dying()
{

    if (player_current_framerate == PLAYER_FRAMERATE_DIE)
    {
        player_anim++;
        player_current_framerate = 0;
    }
    player_current_framerate++;

    if (player_anim > PLAYER_FRAME_DIE_1)
    {
        player_anim = PLAYER_FRAME_DIE_1;
    }
    /*
  switch (player_anim)
  {
  case PLAYER_FRAME_DIE_1:
    //    drawSprite(player_pos_x - cameraX, player_pos_y - cameraY, player_width, player_height, player_die_bits, RUN_DIR);
    drawSprite(player_pos_x - cameraX, player_pos_y - cameraY, player_die.width, player_die.height, player_die.pixel_data, RUN_DIR);
    break;
  }*/
}

void anim_player_wining()
{

    if (player_current_framerate == PLAYER_FRAMERATE_WIN)
    {
        player_anim++;
        player_current_framerate = 0;
        if (random(5) == 0)
            parts.createExplosion(random(10, SCREEN_WIDTH - 10), 10 + (rand() % 2 * SCREEN_HEIGHT / 3), 100 + rand() % 50, random(65535));
    }
    player_current_framerate++;

    if (player_anim > PLAYER_FRAME_WIN_3)
    {
        player_anim = PLAYER_FRAME_WIN_1;
    }
    /*
  switch (player_anim)
  {
  case PLAYER_FRAME_WIN_1:
    //    drawSprite(player_pos_x - cameraX, player_pos_y - cameraY, player_width, player_height, player4, RUN_DIR);
      drawSprite(player_pos_x - cameraX, player_pos_y - cameraY, player_idle1.width, player_idle1.height, player_idle1.pixel_data, RUN_DIR);
    break;
  case PLAYER_FRAME_WIN_2:
    //drawSprite(player_pos_x - cameraX, player_pos_y - cameraY, player_width, player_height, player2, RUN_DIR);
      drawSprite(player_pos_x - cameraX, player_pos_y - cameraY, player_run1.width, player_run1.height, player_run1.pixel_data, RUN_DIR);

    break;
  case PLAYER_FRAME_WIN_3:
    //    drawSprite(player_pos_x - cameraX, player_pos_y - cameraY, player_width, player_height, player3, RUN_DIR);
      drawSprite(player_pos_x - cameraX, player_pos_y - cameraY, player_run2.width, player_run2.height, player_run2.pixel_data, RUN_DIR);
    break;
  }*/
}

void displayGame()
{
    // digital clock display of the time
    canvas->fillScreen(0x867D); //84ceef

    drawWorld();

    //arcada.display->display();
    arcada.blitFrameBuffer(0, 0, false, false);
}
