#include <Arduino.h>
#include <Adafruit_Arcada.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SPIFlash.h>
#include <Adafruit_ZeroTimer.h>

#undef DEBUG
//#define NO_SOUND
#define FPS 25

// @todo passer en time based et non frame based
// @todo idem pour les speed et anims frames des sprites...
#define FRAMES_LOCK_ACTION_B int(300 / (1000 / FPS))
#define FRAMES_ANIM_ACTION_B int(550 / (1000 / FPS))
#define FRAMES_ACTION_B int(2000 / (1000 / FPS))
#define FRAMES_COOLDOWN_B int(1000 / (1000 / FPS))

#define WORLD_WIDTH 200
#define WORLD_HEIGHT 96

#define AMPLITUDE_HAUTEUR 16
#define MEDIUM_HAUTEUR int(2 * WORLD_HEIGHT / 3)

#define MAX_DENSITE 8
#define AMPLITUDE_DENSITE 16

#define HUD_ITEMS_X 16
#define HUD_ITEMS_Y ARCADA_TFT_HEIGHT - 18

#define BASE_X_PLAYER 7
#define BASE_Y_PLAYER 1

#define FALLING_SPEED 1
#define WALKING_SPEED 2
#define RUNNING_SPEED 2

#define MAX_SPEED_X 4
#define MAX_SPEED_Y 15

#define JUMP_SPEED 10
#define DOUBLE_JUMP_SPEED 8




#include "types.h"
#include "items.h"
#include "ennemies.h"
#include "sprites.h"
#include "block_defines.h"

#include "gfx_utils.h"

#include "Particle.h"

#include "MySoundManager.h"

#include "SimplexNoise.h"

static const uint32_t PROGMEM pmf_ninja[] =
    {
#include "musics/ninja.h"
};

static const uint32_t PROGMEM pmf_aceman[] =
    {
#include "musics/aceman.h"
};

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

#include "res/player_falling1.c"

#include "res/player_jump1.c"
#include "res/player_jump2.c"
#include "res/player_jump3.c"
#include "res/player_jump4.c"
#include "res/player_jump5.c"

#include "res/fx_jump1.c"
#include "res/fx_jump2.c"
#include "res/fx_jump3.c"
#include "res/fx_jump4.c"
#include "res/fx_jump5.c"

#include "res/spider_walk1.c"
#include "res/spider_walk2.c"

#include "res/zombi_walk1.c"
#include "res/zombi_walk2.c"
#include "res/zombi_walk3.c"

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

#include "res/argent_small.c"
#include "res/charbon_small.c"
#include "res/cuivre_small.c"
#include "res/rock_small.c"
#include "res/fer_small.c"
#include "res/jade_small.c"
#include "res/or_small.c"
#include "res/redstone_small.c"

#include "res/pioche.c"

#include "sounds/Jump.h"
#include "sounds/Pioche.h"
#include "sounds/RockBreaks.h"
#include "sounds/ItemPickup.h"

#include "pmf_player.h"


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

TPlayer Player;

TworldTile WORLD[WORLD_HEIGHT + 2][WORLD_WIDTH];
#define HEADER_ROW WORLD_HEIGHT
#define REF_ROW WORLD_HEIGHT + 1

unsigned char SCREEN_WORLD_OVERLAY[(ARCADA_TFT_WIDTH + 1) / 16][(ARCADA_TFT_HEIGHT + 1) / 16];

void createItem(int wX, int wY, uint8_t type);
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
TworldTile checkCollisionAt(int newX, int newY);
TworldTile checkCollisionTo(int x, int y, int newX, int newY);
void checkPlayerCollisionsEntities();
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
void anim_player_digging();

uint8_t pressed_buttons = 0;
uint8_t just_pressed = 0;
int SCORE = 0;
bool bWin = false;

int cameraX = 0;
int cameraY = 0;

int currentX_back = 0;

int jumpPhase = 0;
TworldTile brique_UP = {0};
TworldTile brique_DOWN =  {0};
TworldTile brique_DOWN_FRONT =  {0};
TworldTile brique_FRONT =  {0};
TworldTile brique_PLAYER =  {0};

int count_player_die = 0;
int count_player_win = 0;

unsigned long prevDisplay = 0; // when the digital clock was displayed

void PlayMOD(const void *pmf_file)
{
    s_player.load(pmf_file);
    s_player.start(uint32_t(AUDIO_SAMPLE_RATE_EXACT + 0.5f));
}

void StopMOD()
{
    if (s_player.is_playing())
        s_player.stop();
    //  zerotimer3.enable(false);
}
void MOD_callback(void)
{
    AudioNoInterrupts();
    if (s_player.is_playing())
        s_player.update(); // keep updating the audio buffer...
    AudioInterrupts();
}

void TC3_Handler()
{
    Adafruit_ZeroTimer::timerHandler(3);
}

void setup_timer3(float freq)
{
    uint8_t divider = 1;
    uint16_t compare = 1;

    tc_clock_prescaler prescaler = TC_CLOCK_PRESCALER_DIV1;

    if ((freq < 24000000) && (freq > 800))
    {
        divider = 1;
        prescaler = TC_CLOCK_PRESCALER_DIV1;
        compare = 48000000 / freq;
    }
    else if (freq > 400)
    {
        divider = 2;
        prescaler = TC_CLOCK_PRESCALER_DIV2;
        compare = (48000000 / 2) / freq;
    }
    else if (freq > 200)
    {
        divider = 4;
        prescaler = TC_CLOCK_PRESCALER_DIV4;
        compare = (48000000 / 4) / freq;
    }
    else if (freq > 100)
    {
        divider = 8;
        prescaler = TC_CLOCK_PRESCALER_DIV8;
        compare = (48000000 / 8) / freq;
    }
    else if (freq > 50)
    {
        divider = 16;
        prescaler = TC_CLOCK_PRESCALER_DIV16;
        compare = (48000000 / 16) / freq;
    }
    else if (freq > 12)
    {
        divider = 64;
        prescaler = TC_CLOCK_PRESCALER_DIV64;
        compare = (48000000 / 64) / freq;
    }
    else if (freq > 3)
    {
        divider = 256;
        prescaler = TC_CLOCK_PRESCALER_DIV256;
        compare = (48000000 / 256) / freq;
    }
    else if (freq >= 0.75)
    {
        divider = 1024;
        prescaler = TC_CLOCK_PRESCALER_DIV1024;
        compare = (48000000 / 1024) / freq;
    }
    else
    {
        Serial.println("Invalid frequency");
        while (1)
            delay(10);
    }

    zerotimer3.enable(false);
    zerotimer3.configure(prescaler,                   // prescaler
                         TC_COUNTER_SIZE_16BIT,       // bit width of timer/counter
                         TC_WAVE_GENERATION_MATCH_PWM // frequency or PWM mode
    );

    zerotimer3.setCompare(0, compare);
    zerotimer3.setCallback(true, TC_CALLBACK_CC_CHANNEL0, MOD_callback);
    zerotimer3.enable(true);
}

void setFrameRate(uint8_t rate)
{
    frameRate = rate;
    eachFrameMillis = 1000 / rate;
}

bool everyXFrames(uint8_t frames)
{
    return frameCount % frames == 0;
}

bool nextFrame()
{
    long now = millis();
    uint8_t remaining;

    // post render
    if (post_render)
    {
        lastFrameDurationMs = now - lastFrameStart;
        frameCount++;
        post_render = false;
    }

    // if it's not time for the next frame yet
    if (now < nextFrameStart)
    {
        remaining = nextFrameStart - now;
        // if we have more than 1ms to spare, lets sleep
        // we should be woken up by timer0 every 1ms, so this should be ok
        //  if (remaining > 1)
        //    nop();
        return false;
    }

    // pre-render

    // technically next frame should be last frame + each frame but if we're
    // running a slow render we would constnatly be behind the clock
    // keep an eye on this and see how it works.  If it works well the
    // lastFrameStart variable could be eliminated completely
    nextFrameStart = now + eachFrameMillis;
    lastFrameStart = now;
    post_render = true;
    return post_render;
}

TworldTile getWorldAtPix(int px, int py)
{
    TworldTile res = {0};

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
        if (WORLD[wY][x].id != 0)
        {
            newH = wY;
            break;
        }
    }
    WORLD[HEADER_ROW][x].id = newH;
}

TworldTile getWorldAt(int x, int y)
{
    TworldTile res = {0};
    
    if ((x >= 0 && x < WORLD_WIDTH) && (y >= 0 && y < WORLD_HEIGHT))
        res = WORLD[y][x];

    return res;
}

void setWorldAt(int x, int y, TworldTile val)
{
    if ((x > 0 && x < WORLD_WIDTH) && (y > 0 && y < WORLD_HEIGHT))
        WORLD[y][x] = val;
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
        Serial.println("Error, failed to initialize filesysBeginMSD!");
        arcada.haltBox("Failed to begin filesysBeginMSD");
    }
*/
    Arcada_FilesystemType foundFS = arcada.filesysBegin();
    if (foundFS == ARCADA_FILESYS_NONE)
    {
        Serial.println("Error, failed to initialize filesys!");
        arcada.haltBox("Failed to begin fileSys");
    }
    arcada.mkdir("/saves");

    setupSoundManager();
    /********** Start speaker */
#ifndef NO_SOUND
    arcada.enableSpeaker(true);
#endif

    arcada.display->fillScreen(ARCADA_BLUE);

    arcada.display->setCursor(0, 0);
    arcada.display->setTextWrap(true);
    arcada.display->setTextSize(1);

    arcada.display->setTextColor(ARCADA_WHITE);

    initMenu();

    setFrameRate(FPS);

    stopMusic();
    //playMusic(Pinball);

    setup_timer3(100);
    //arcada.timerCallback(100, MOD_callback);

    PlayMOD(pmf_aceman);
    Serial.println("Starting");
}

void clearRect(int16_t xMove, int16_t yMove, int16_t width, int16_t height)
{
    canvas->fillRect(xMove, yMove, width, height, 0x867D);
}

void initPlayer()
{
    memset(&Player, 0, sizeof(Player));
    Player.pos.pX = BASE_X_PLAYER * 16;
    Player.pos.pY = BASE_Y_PLAYER * 16;
    Player.pos.direction = 1;

    Player.pos.XFront = 0;
    Player.pos.YDown = 0;
    Player.pos.YUp = 0;
    Player.pos.worldX = 0;
    Player.pos.worldY = 0;

    Player.anim_frame = 0;
    Player.current_framerate = 0;
    Player.stateAnim = PLAYER_STATE_IDLE;

    Player.cible_wX = Player.cible_wY = 0;
    Player.cible_pX, Player.cible_pY = 0;
    /*
  unsigned char state;  
  unsigned char action, action_cooldown, action_perf;
  unsigned char depth;
  unsigned char health;
  unsigned char level;
  unsigned char stars_kills;
  unsigned short armour_weapon;
*/

    Player.bDying = false;
    Player.bJumping = false;
    Player.bFalling = false;
    Player.bWalking = false;
    Player.bMoving = false;
    Player.bTouched = false;
    Player.iTouchCountDown = 0;

    Player.bOnGround = true;

    Player.max_health = 20;
    Player.health = 20;
}

void initGame()
{
    arcada.pixels.fill(0x000000);
    //    arcada.pixels.setPixelColor(4, 0x00FF00);
    arcada.pixels.show();
    //arcada.display->setFont(ArialMT_Plain_10);
    //  arcada.display->setTextAlignment(TEXT_ALIGN_LEFT);
    briquesFRAMES = 0;

    SCORE = 0;
    pressed_buttons = 0;
    bWin = false;

    currentX_back = 0;

    calculatePlayerCoords();

    counterActionB = 0;
    coolDownActionB = 0;

    jumpPhase = 0;

    Player.bWantJump = false;
    Player.bWantDoubleJump = false;
    Player.bWantWalk = false;

    Player.pos.speedX = 0;
    Player.pos.speedY = 0;
    Player.pos.newX = 0;
    Player.pos.newY = 0;
    Player.cible_wX, Player.cible_wY = 0;

    Player.anim_frame = 0;
    Player.current_framerate = 0;
    Player.stateAnim = PLAYER_STATE_IDLE;

    count_player_die = 0;
    count_player_win = 0;

    initWorld();
    initPlayer();
}

void set_falling()
{
    if (Player.stateAnim != PLAYER_STATE_FALL)
    {
        Player.stateAnim = PLAYER_STATE_FALL;
        Player.anim_frame = PLAYER_FRAME_FALLING_1;
        Player.current_framerate = 0;
    }
}

void set_idle()
{
    if (Player.stateAnim != PLAYER_STATE_IDLE)
    {
        Player.stateAnim = PLAYER_STATE_IDLE;
        Player.anim_frame = PLAYER_FRAME_IDLE_1;
        Player.current_framerate = 0;
    }
}

void set_jumping()
{
    if (Player.stateAnim != PLAYER_STATE_JUMP)
    {
        Player.stateAnim = PLAYER_STATE_JUMP;

        Player.anim_frame = PLAYER_FRAME_JUMP_1;
        Player.current_framerate = 0;
    }
}

void set_walking()
{
    if (Player.stateAnim != PLAYER_STATE_WALK)
    {
        Player.stateAnim = PLAYER_STATE_WALK;

        Player.anim_frame = PLAYER_FRAME_WALK_1;
        Player.current_framerate = 0;
    }
}

void set_digging()
{
    if (Player.stateAnim != PLAYER_STATE_DIG)
    {
        Player.stateAnim = PLAYER_STATE_DIG;

        Player.anim_frame = PLAYER_FRAME_ACTION_1;
        Player.current_framerate = 0;
    }
}

void set_dying()
{
    if (Player.stateAnim != PLAYER_STATE_DIE)
    {
        Player.stateAnim = PLAYER_STATE_DIE;

        //stopMusic();
        //    sndPlayerCanal1.play(AudioSamplePlayerdeath);
        Player.bDying = true;
        Player.iTouchCountDown = 0;
        Player.bTouched = false;
        jumpPhase = 0;
        count_player_die = 0;
        Player.anim_frame = PLAYER_FRAME_DIE_1;
        Player.current_framerate = 0;
    }
}

void set_double_jump_fx(void)
{
    if (Player.FX.stateAnim != FX_DOUBLE_JUMP)
    {
        Player.FX.stateAnim = FX_DOUBLE_JUMP;
        Player.FX.anim_frame = 0;
        Player.FX.current_framerate = FX_FRAMERATE_DOUBLE_JUMP;
        Player.FX.direction = Player.pos.direction;
        Player.FX.pX = Player.pos.pX;
        Player.FX.pY = Player.pos.pY;
    }
}

void set_touched()
{
    // @todo : gerer les type de monstres
    if (Player.iTouchCountDown == 0)
    {
        Player.health -= 10;

        if (Player.health > 0)
        {
            //    sndPlayerCanal3.play(AudioSampleSmb_pipe);
            Player.bTouched = true;
            Player.iTouchCountDown = 2 * FPS;
            //Player.anim_frame = PLAYER_FRAME_TOUCH_1;
            //Player.current_framerate = 0;
        }
        else if (Player.iTouchCountDown == 0)
        {
            set_dying();
        }
    }
}

void set_wining()
{
    if (Player.stateAnim != PLAYER_STATE_WIN)
    {
        Player.stateAnim = PLAYER_STATE_WIN;

        //playMusic(FlagpoleFanfare);
        Player.iTouchCountDown = 0;
        Player.bTouched = false;
        bWin = true;
        jumpPhase = 0;
        count_player_win = 0;
        Player.anim_frame = PLAYER_FRAME_WIN_1;
        Player.current_framerate = 0;
    }
}

void initWorld()
{

    //uint8_t zeNoise[WORLD_WIDTH];
    memset(WORLD, 0, sizeof(WORLD));
    SimplexNoise noiseGen;

    int maxProfondeur = 0;
    for (int wX = 0; wX < WORLD_WIDTH; wX++)
    {
        //        float noise = SimplexNoise::noise(((float)16 * wX) / (float)WORLD_WIDTH);
        float noise = noiseGen.fractal(4, ((float)16 * wX) / (float)WORLD_WIDTH);

        uint8_t rowGround = (WORLD_HEIGHT - 1) - (int(noise * AMPLITUDE_HAUTEUR) + MEDIUM_HAUTEUR);

        maxProfondeur = max(maxProfondeur, rowGround);

        //On sauvegarde la hauteur sol dans la derniere ligne du world (invisible)
        WORLD[HEADER_ROW][wX].id = rowGround;
        //On sauve la hauteur originelle, en dessous on est dans le sol (meme si creusé)
        WORLD[REF_ROW][wX].id = rowGround;

        for (int wY = 0; wY < WORLD_HEIGHT; wY++)
        {
            if (wY > (rowGround + 7))
                WORLD[wY][wX].id = BLOCK_ROCK;
            else if (wY > (rowGround + 3))
                WORLD[wY][wX].id = BLOCK_GROUND_ROCK;
            else if (wY > rowGround)
                WORLD[wY][wX].id = BLOCK_GROUND;
            else if (wY == rowGround)
                WORLD[wY][wX].id = BLOCK_GROUND_TOP;
            else
                WORLD[wY][wX].id = 0;
        }
    }
    float minN = 999999;
    float maxN = -99999;
    //Update bricks to cool rendering
    for (int wX = 0; wX < WORLD_WIDTH; wX++)
    {
        int curProdondeur = WORLD[HEADER_ROW][wX].id;
        curProdondeur = max(4, curProdondeur);

        for (int wY = curProdondeur - 4; wY < WORLD_HEIGHT; wY++)
        {
            //            float noise = noiseGen.fractal(8, (float)16*wX / (float)WORLD_WIDTH, (float)16*wY / (float)WORLD_HEIGHT);
            float noise = SimplexNoise::noise((float)16 * wX / (float)WORLD_WIDTH, (float)16 * wY / (float)WORLD_HEIGHT);
            int16_t densite = int(noise * AMPLITUDE_DENSITE);
            if (WORLD[wY][wX].id <= BLOCK_GROUND_ROCK)
            {
                if (densite > MAX_DENSITE)
                    WORLD[wY][wX].id = BLOCK_AIR;
            }
            else
            {
                if (abs(densite) > MAX_DENSITE || densite == 0)
                    WORLD[wY][wX].id = BLOCK_UNDERGROUND_AIR;
                else
                    WORLD[wY][wX].id = abs(densite) + BLOCK_ROCK;
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
            if ((WORLD[wY][wX].id != 0) && (WORLD[wY][wX].id != BLOCK_UNDERGROUND_AIR))
            {
                WORLD[HEADER_ROW][wX].id = wY;
                WORLD[REF_ROW][wX].id = wY;

                WORLD[wY][wX].id = BLOCK_GROUND_TOP;
                if (wY + 1 < (WORLD_HEIGHT - 1))
                    WORLD[wY + 1][wX].id = BLOCK_GROUND;

                break;
            }
        }
    }

    NB_WORLD_ENNEMIES = 0;
    for (int wX = 1; wX < WORLD_WIDTH - 1; wX += 3)
    {
        for (int wY = 1; wY < WORLD_HEIGHT - 1; wY++)
        {
            if (NB_WORLD_ENNEMIES < MAX_ENNEMIES)
            {
                int haut = WORLD[HEADER_ROW][wX].id;
                int seed = random(100);
                if (seed < 10 && NB_WORLD_SKELS < MAX_SKELS)
                {
                    if (WORLD[HEADER_ROW][wX].id == 0)
                    {
                        /*
                        //Zombi
                        ENNEMIES[CURRENT_LEVEL_ENNEMIES].bIsAlive = 255;
                        ENNEMIES[CURRENT_LEVEL_ENNEMIES].worldX = wX;
                        ENNEMIES[CURRENT_LEVEL_ENNEMIES].worldY = haut;
                        ENNEMIES[CURRENT_LEVEL_ENNEMIES].x = ENNEMIES[CURRENT_LEVEL_ENNEMIES].worldX * 16;
                        ENNEMIES[CURRENT_LEVEL_ENNEMIES].y = ENNEMIES[CURRENT_LEVEL_ENNEMIES].worldY * 16;
                        ENNEMIES[CURRENT_LEVEL_ENNEMIES].new_x = 0;
                        ENNEMIES[CURRENT_LEVEL_ENNEMIES].new_y = 0;
                        ENNEMIES[CURRENT_LEVEL_ENNEMIES].type = SKEL_ENNEMY;
                        ENNEMIES[CURRENT_LEVEL_ENNEMIES].speed_x = SKEL_WALKING_SPEED;
                        ENNEMIES[CURRENT_LEVEL_ENNEMIES].speed_y = 0;
                        ENNEMIES[CURRENT_LEVEL_ENNEMIES].max_frames = 4;
                        ENNEMIES[CURRENT_LEVEL_ENNEMIES].nb_anim_frames = 3;

                        ENNEMIES[CURRENT_LEVEL_ENNEMIES].bFalling = false;
                        ENNEMIES[CURRENT_LEVEL_ENNEMIES].bJumping = false;
                        ENNEMIES[CURRENT_LEVEL_ENNEMIES].bOnGround = false;

                        ENNEMIES[NB_WORLD_ENNEMIES].max_health = ZOMBI_HEALTH;
                        ENNEMIES[NB_WORLD_ENNEMIES].health = ZOMBI_HEALTH;
*/
                        NB_WORLD_SKELS++;
                        //CURRENT_LEVEL_ENNEMIES++;
                    }
                }
                else if (seed < 33 && NB_WORLD_ZOMBIES < MAX_ZOMBIES)
                {
                    if (WORLD[wY][wX].id == 0)
                    {

                        //Zombi
                        ENNEMIES[NB_WORLD_ENNEMIES].bIsAlive = 255;
                        ENNEMIES[NB_WORLD_ENNEMIES].worldX = wX;
                        ENNEMIES[NB_WORLD_ENNEMIES].worldY = haut;
                        ENNEMIES[NB_WORLD_ENNEMIES].x = ENNEMIES[NB_WORLD_ENNEMIES].worldX * 16;
                        ENNEMIES[NB_WORLD_ENNEMIES].y = ENNEMIES[NB_WORLD_ENNEMIES].worldY * 16;
                        ENNEMIES[NB_WORLD_ENNEMIES].new_x = 0;
                        ENNEMIES[NB_WORLD_ENNEMIES].new_y = 0;
                        ENNEMIES[NB_WORLD_ENNEMIES].type = ZOMBI_ENNEMY;
                        ENNEMIES[NB_WORLD_ENNEMIES].speed_x = ZOMBI_WALKING_SPEED;
                        ENNEMIES[NB_WORLD_ENNEMIES].speed_y = 0;
                        ENNEMIES[NB_WORLD_ENNEMIES].max_frames = 4;
                        ENNEMIES[NB_WORLD_ENNEMIES].nb_anim_frames = 3;

                        ENNEMIES[NB_WORLD_ENNEMIES].bFalling = false;
                        ENNEMIES[NB_WORLD_ENNEMIES].bJumping = false;
                        ENNEMIES[NB_WORLD_ENNEMIES].bOnGround = false;

                        ENNEMIES[NB_WORLD_ENNEMIES].max_health = ZOMBI_HEALTH;
                        ENNEMIES[NB_WORLD_ENNEMIES].health = ZOMBI_HEALTH;

                        NB_WORLD_ENNEMIES++;
                        NB_WORLD_ZOMBIES++;
                    }
                }
                else if (seed <= 50)
                {
                    if (WORLD[wY][wX].id == 0x7f && WORLD[wY][wX + 1].id == 0x7f && WORLD[wY][wX - 1].id == 0x7f && WORLD[wY - 1][wX].id == 0x7f)
                    {
                        //Spiders
                        ENNEMIES[NB_WORLD_ENNEMIES].bIsAlive = 255;
                        ENNEMIES[NB_WORLD_ENNEMIES].worldX = wX;
                        ENNEMIES[NB_WORLD_ENNEMIES].worldY = wY;
                        ENNEMIES[NB_WORLD_ENNEMIES].x = ENNEMIES[NB_WORLD_ENNEMIES].worldX * 16;
                        ENNEMIES[NB_WORLD_ENNEMIES].y = ENNEMIES[NB_WORLD_ENNEMIES].worldY * 16;
                        ENNEMIES[NB_WORLD_ENNEMIES].new_x = 0;
                        ENNEMIES[NB_WORLD_ENNEMIES].new_y = 0;
                        ENNEMIES[NB_WORLD_ENNEMIES].type = SPIDER_ENNEMY;
                        ENNEMIES[NB_WORLD_ENNEMIES].speed_x = SPIDER_WALKING_SPEED;
                        ENNEMIES[NB_WORLD_ENNEMIES].speed_y = 0;
                        ENNEMIES[NB_WORLD_ENNEMIES].max_frames = 2;
                        ENNEMIES[NB_WORLD_ENNEMIES].nb_anim_frames = 2;

                        ENNEMIES[NB_WORLD_ENNEMIES].bFalling = false;
                        ENNEMIES[NB_WORLD_ENNEMIES].bJumping = false;
                        ENNEMIES[NB_WORLD_ENNEMIES].bOnGround = false;

                        ENNEMIES[NB_WORLD_ENNEMIES].max_health = SPIDER_HEALTH;
                        ENNEMIES[NB_WORLD_ENNEMIES].health = SPIDER_HEALTH;

                        NB_WORLD_ENNEMIES++;
                        NB_WORLD_SPIDERS++;
                    }
                }
            }
            else
            {
                break;
            }
        }
    }

    Serial.printf("CURRENT_LEVEL_ENNEMIES:%d\n", NB_WORLD_ENNEMIES);
    Serial.printf("Min:%f / Max:%f\n", minN, maxN);

    NB_WORLD_ITEMS = 0;
    CURRENT_QUEUE_ITEMS = 0;

    StopMOD();
    PlayMOD(pmf_ninja);
}

void drawPlayer()
{
    if (Player.iTouchCountDown > 0)
    {
        Player.iTouchCountDown--;

        if ((Player.iTouchCountDown % 2) == 0)
            return;
    }

    switch (Player.stateAnim)
    {
    case PLAYER_STATE_WALK:
        anim_player_walk();
        break;
    case PLAYER_STATE_JUMP:
    case PLAYER_STATE_DOUBLE_JUMP:
        anim_player_jump();
        break;
    case PLAYER_STATE_FALL:
        anim_player_falling();
        break;
    case PLAYER_STATE_DIG:
        anim_player_digging();
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
    switch (Player.FX.stateAnim)
    {
    case FX_NONE:

        break;
    case FX_DOUBLE_JUMP:
    {
        anim_player_fx_double_jump();
        break;
    }
    }
}

void createItem(int wX, int wY, uint8_t type)
{
    ITEMS[CURRENT_QUEUE_ITEMS].bIsAlive = 255;
    ITEMS[CURRENT_QUEUE_ITEMS].bIsActive = true;
    ITEMS[CURRENT_QUEUE_ITEMS].iSpawning = 1 * FPS;

    ITEMS[CURRENT_QUEUE_ITEMS].worldX = wX;
    ITEMS[CURRENT_QUEUE_ITEMS].worldY = wY;
    ITEMS[CURRENT_QUEUE_ITEMS].x = wX * 16;
    ITEMS[CURRENT_QUEUE_ITEMS].y = wY * 16;
    ITEMS[CURRENT_QUEUE_ITEMS].type = type;
    ITEMS[CURRENT_QUEUE_ITEMS].speed_x = 0;
    ITEMS[CURRENT_QUEUE_ITEMS].speed_y = 0;
    ITEMS[CURRENT_QUEUE_ITEMS].max_frames = 1;
    ITEMS[CURRENT_QUEUE_ITEMS].nb_anim_frames = 0;

    CURRENT_QUEUE_ITEMS++;
    //On boucle sur la liste des items
    // @todo : faire une liste et trouver une "place dispo"
    if (CURRENT_QUEUE_ITEMS == MAX_ITEMS)
    {
        NB_WORLD_ITEMS = MAX_ITEMS;
        CURRENT_QUEUE_ITEMS = 0;
        ITEMS[CURRENT_QUEUE_ITEMS].bIsAlive = 0;
        ITEMS[CURRENT_QUEUE_ITEMS].bIsActive = false;
        ITEMS[CURRENT_QUEUE_ITEMS].type = ITEM_NONE;
    }
    else
        NB_WORLD_ITEMS++;
}

void killItem(Titem *currentItem, int px, int py)
{
    currentItem->bIsAlive = 4;

    //parts.createExplosion(px - cameraX, py - cameraY, 16);
}

void drawItem(Titem *currentItem)
{
    if (currentItem->current_framerate == currentItem->max_frames)
    {
        currentItem->anim_frame++;
        currentItem->current_framerate = 0;
    }
    currentItem->current_framerate++;

    if (currentItem->anim_frame > currentItem->nb_anim_frames)
    {
        currentItem->anim_frame = 1;
    }
    if (currentItem->bIsAlive < 127)
    {
        //Il va disparaitre apres quelques frames, ca nous laisse le temps de faire un effet de disparition eventuel
        currentItem->bIsAlive -= 4;
    }

    int px = currentItem->x - worldOffset_pX;
    int py = currentItem->y - worldOffset_pY;

    /*
        /*    if (currentItem->iSpawning > 0)
    {
      int tmpH = 16 - (currentItem->iSpawning / 2);
      drawSprite(px, (currentItem->y ) + (16 - tmpH), mushroom_16x16.width, tmpH, mushroom_16x16.pixel_data, Player.pos.direction);
    }
    else

    */
    //Recentrage
    //px -= 4;
    const int8_t lookUpItems[7] = {0, 1, 2, 4, 4, 2, 1};
    py = py - lookUpItems[(briquesFRAMES / 4) % 7];
    //Serial.printf("Item:%d %d,%d\n", currentItem->type, px, py);
    if (px < ARCADA_TFT_WIDTH && px > -16 && py < ARCADA_TFT_HEIGHT && py > -16)
    {
        switch (currentItem->type)
        {
        case ITEM_GROUND_ROCK:
        case ITEM_ROCK:
            drawSprite(px, py, rock_small.width, rock_small.height, rock_small.pixel_data, 1);
            break;
        case ITEM_CHARBON:
            drawSprite(px, py, charbon_small.width, charbon_small.height, charbon_small.pixel_data, 1);
            break;
        case ITEM_CUIVRE:
            drawSprite(px, py, cuivre_small.width, cuivre_small.height, cuivre_small.pixel_data, 1);
            break;
        case ITEM_FER:
            drawSprite(px, py, fer_small.width, fer_small.height, fer_small.pixel_data, 1);
            break;
        case ITEM_ARGENT:
            drawSprite(px, py, argent_small.width, argent_small.height, argent_small.pixel_data, 1);
            break;
        case ITEM_JADE:
            drawSprite(px, py, jade_small.width, jade_small.height, jade_small.pixel_data, 1);
            break;
        case ITEM_OR:
            drawSprite(px, py, or_small.width, or_small.height, or_small.pixel_data, 1);
            break;
        case ITEM_REDSTONE:
            drawSprite(px, py, redstone_small.width, redstone_small.height, redstone_small.pixel_data, 1);
            break;
        }
    }
}

void drawItems()
{
    for (int enC = 0; enC < NB_WORLD_ITEMS; enC++)
    {
        Titem *currentItem = &ITEMS[enC];
        if (currentItem->bIsAlive > 0)
        {
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

    px -= worldOffset_pX;
    py -= worldOffset_pY;

    parts.createExplosion(px, py, 16);
}

void drawEnnemy(Tennemy *currentEnnemy)
{

    if (currentEnnemy->current_framerate == currentEnnemy->max_frames)
    {
        currentEnnemy->anim_frame++;
        currentEnnemy->current_framerate = 0;
    }
    currentEnnemy->current_framerate++;

    if (currentEnnemy->anim_frame > currentEnnemy->nb_anim_frames)
    {
        currentEnnemy->anim_frame = 1;
    }
    if (currentEnnemy->bIsAlive < 127)
    {
        //On fixe a la frames + 1 (dans le case du drraw on dessinera la frame "dying" )
        currentEnnemy->anim_frame = currentEnnemy->nb_anim_frames + 1;
        currentEnnemy->bIsAlive -= 4;
    }

    int px = currentEnnemy->x - worldOffset_pX;
    int py = currentEnnemy->y - worldOffset_pY;
    //Recentrage
    //px -= 4;
    if (px < ARCADA_TFT_WIDTH && px > -16 && py < ARCADA_TFT_HEIGHT && py > -16)
    {
        switch (currentEnnemy->type)
        {
            case SPIDER_ENNEMY:
            {
                switch (currentEnnemy->anim_frame)
                {
                    case 1:
                        drawSprite(px, py, spider_walk1.width, spider_walk1.height, spider_walk1.pixel_data, 1);
                        break;
                    case 2:
                        drawSprite(px, py, spider_walk2.width, spider_walk2.height, spider_walk2.pixel_data, 1);
                        break;
                }
                break;
            }
            case ZOMBI_ENNEMY:
            {
                int DIR = currentEnnemy->speed_x > 0 ? 1 : -1;
                switch (currentEnnemy->anim_frame)
                {
                    case 1:
                        drawSprite(px, py, zombi_walk1.width, zombi_walk1.height, zombi_walk1.pixel_data, DIR);
                        break;
                    case 2:
                        drawSprite(px, py, zombi_walk2.width, zombi_walk2.height, zombi_walk2.pixel_data, DIR);
                        break;
                    case 3:
                        drawSprite(px, py, zombi_walk3.width, zombi_walk3.height, zombi_walk3.pixel_data, DIR);
                        break;
                }
                break;
            }
        }
        if (currentEnnemy->bIsAlive >= 127)
        {
            int lgHealth = int(14 * currentEnnemy->health / currentEnnemy->max_health);
            canvas->drawFastHLine(px+1, py-4, lgHealth, lgHealth < 8 ? 0xC180 : 0xAE6A);
            canvas->drawFastHLine(px+1, py-3, lgHealth, lgHealth < 8 ? 0x8940 : 0x5C64);
            canvas->drawRect(px, py-5, 16, 4, ARCADA_BLACK);
        }
    }
}

void drawEnnemies()
{
    for (int enC = 0; enC < NB_WORLD_ENNEMIES; enC++)
    {
        Tennemy *currentEnnemy = &ENNEMIES[enC];
        if (currentEnnemy->bIsAlive > 0)
        {
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
    int playerLightX = Player.pos.worldX;
    int playerLightY = Player.pos.worldY;

    for (int wX = worldMIN_X; wX < worldMAX_X; wX++)
    {
        int profondeurColonne = WORLD[HEADER_ROW][wX].id;

        int px = ((wX - worldMIN_X) * 16) - currentOffset_X;
        if ((px < ARCADA_TFT_WIDTH) && (px > -16))
        {
            for (int wY = worldMIN_Y; wY < worldMAX_Y; wY++)
            {
                int py = ((wY - worldMIN_Y) * 16) - currentOffset_Y;
                if ((py < ARCADA_TFT_HEIGHT) && (py > -16))
                {
                    uint8_t value = WORLD[wY][wX].id;

                    int curLight = MAX_LIGHT_INTENSITY;

                    if (profondeurColonne != 0)
                    {
                        int delta = (wY - profondeurColonne) + 1;
                        if (delta > 0)
                            curLight = curLight / delta;
                    }
                    // @todo : creer un masque de X*X autour du player pour "ajouter de la light"
                    if (wY <= (playerLightY + 1) || value == 0 || value == 0x7F)
                    {
                        //Ray cast du player vers decor ?
                        float distPlayer = sqrt((playerLightX - wX) * (playerLightX - wX) + (playerLightY - wY) * (playerLightY - wY));
                        //Serial.printf("player:%d,%d tile:%d,%d dist:%f\n", playerLightX, playerLightY, px, py,  distPlayer);
                        // @todo gerer la non propagassion de la lumiere dans les murs...
                        if (distPlayer < 4)
                        {
                            curLight = curLight + (PLAYER_LIGHT_INTENSITY / distPlayer);
                        }
                    }
                    //   curLight = curLight + AMBIENT_LIGHT_INTENSITY;
                    curLight = min(curLight, MAX_LIGHT_INTENSITY);

#ifdef DEBUG
                    curLight = MAX_LIGHT_INTENSITY;
#endif

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
                    /*                    else if (value == 0x60) //
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
                    */
                    else
                    {
                        drawTile(px, py, rock_empty.pixel_data, curLight);
                        /*                        int R = (75 + value) * (curLight / MAX_LIGHT_INTENSITY);
                        int G = (72 + value) * (curLight / MAX_LIGHT_INTENSITY);
                        int B = (59 + value) * (curLight / MAX_LIGHT_INTENSITY);
                        canvas->fillRect(px, py, 16, 16, rgbTo565(R, G, B));*/
                    }
                    /*
                    else if (WORLD[wY][wX].id == 'o') //16 coin
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

    if ((lastX != Player.pos.worldX) || (lastY != Player.pos.worldY))
    {
        Serial.printf("WPos1:%d,%d\n", Player.pos.worldX, Player.pos.worldY);
        Serial.printf("Pix:%d,%d\n", Player.pos.pX, Player.pos.pY);

        lastX = Player.pos.worldX;
        lastY = Player.pos.worldY;
    }

    int debugX = (((Player.pos.worldX) - worldMIN_X) * 16) - currentOffset_X;
    int debugY = (((Player.pos.worldY) - worldMIN_Y) * 16) - currentOffset_Y;

    int debugXF = (((Player.pos.XFront) - worldMIN_X) * 16) - currentOffset_X;
    int debugYF = debugY;

    int debugXD = debugX;
    int debugYD = (((Player.pos.YDown) - worldMIN_Y) * 16) - currentOffset_Y;

    int debugXU = debugX;
    int debugYU = (((Player.pos.YUp) - worldMIN_Y) * 16) - currentOffset_Y;

    canvas->drawRect(debugX, debugY, 16, 16, ARCADA_WHITE);
    canvas->drawRect(debugXF, debugYF, 16, 16, ARCADA_RED);
    canvas->drawRect(debugXD, debugYD, 16, 16, ARCADA_GREENYELLOW);
    canvas->drawRect(debugXU, debugYU, 16, 16, ARCADA_CYAN);

    //   canvas->drawRect((Player.pos.pX - (worldMIN_X * 16)) - currentOffset_X, (Player.pos.pY - (worldMIN_Y * 16)) - currentOffset_Y, 16, 16, ARCADA_BLUE);

    for (int enC = 0; enC < NB_WORLD_ENNEMIES; enC++)
    {
        Tennemy *currentEnnemy = &ENNEMIES[enC];
        if (currentEnnemy->bIsAlive > 0)
        {
            if ((currentEnnemy->worldX >= worldMIN_X - 1) && (currentEnnemy->worldX <= worldMAX_X + 1) && (currentEnnemy->worldY >= worldMIN_Y - 1) && (currentEnnemy->worldY <= worldMAX_Y + 1))
            {

                int px = currentEnnemy->x - worldOffset_pX;
                int py = currentEnnemy->y - worldOffset_pY;

                canvas->drawRect(px, py, 16, 16, ARCADA_RED);
            }
        }
    }
}

void drawHud()
{
    // @todo : pas vraiment ici que ce devrait etre fait...
    if (counterActionB > 0)
    {
        if (Player.cible_pX > -16 && Player.cible_pX < ARCADA_TFT_WIDTH && Player.cible_pY > -16 && Player.cible_pY < ARCADA_TFT_HEIGHT)
        {
            //Test action pour le sprite, pour l'instant la pioche
            // drawSprite(Player.cible_pX, Player.cible_pY, pioche.width, pioche.height, pioche.pixel_data, Player.pos.direction);
            if ((counterActionB % FRAMES_ANIM_ACTION_B) == 0)
            {
                parts.createExplosion(Player.cible_pX + 8, Player.cible_pY + 8, 8); //@todo colorer avec la couleur de la brique en cours de travail
                sndPlayerCanal1.play(AudioSamplePioche);
            }
        }
    }

    //Quick items
    for (uint8_t it = 0; it < MAX_QUICK_ITEMS; it++)
    {
        if (it == Player.currentItemSelected)
            canvas->drawRect(HUD_ITEMS_X + (it * 16), HUD_ITEMS_Y, 16, 16, ARCADA_MAGENTA);
        else
            canvas->drawRect(HUD_ITEMS_X + (it * 16), HUD_ITEMS_Y, 16, 16, ARCADA_WHITE);
        uint8_t item_id = Player.quick_inventory[it];
        if (item_id != 0)
        {
            ///draw Item sprite
        }
    }
    //Health / Magie...
    int lgHealth = 30 * Player.health / Player.max_health;

    canvas->fillRect(2, 2, lgHealth, 3, lgHealth < 8 ? 0xC180 : 0xAE6A);
    canvas->fillRect(2, 5, lgHealth, 3, lgHealth < 8 ? 0x8940 : 0x5C64);
    canvas->drawRect(1, 1, 32, 8, ARCADA_BLACK);
        
    arcada.pixels.setPixelColor(4, 0x00FF00);
    //    arcada.pixels.setPixelColor(4, 0x00FF00);
    arcada.pixels.show();
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

TworldTile checkCollisionTo(int x, int y, int newX, int newY)
{
    TworldTile res = {0};

    for (int wX = x; wX <= newX; wX++)
    {
        for (int wY = y; wY <= newY; wY++)
        {
            TworldTile value = WORLD[wY][wX];
            if (value.id != 0 && value.id != 0x7F)
            {
                return value;
            }
        }
    }
    return res;
}

TworldTile checkCollisionAt(int newX, int newY)
{
    int check_worldX = newX / 16;
    int check_worldY = newY / 16;

    return getWorldAt(check_worldX, check_worldY);
}

void calculatePlayerCoords()
{
    Player.pos.worldX = (Player.pos.pX + 8) / 16;
    Player.pos.worldY = (Player.pos.pY + 8) / 16;

    if (Player.pos.direction <= 0)
    {
        Player.pos.XFront = (Player.pos.pX - 1) / 16;
        Player.pos.XFront = max(Player.pos.XFront, 0);
    }
    else
    {
        Player.pos.XFront = (Player.pos.pX + 16) / 16;
        Player.pos.XFront = min(Player.pos.XFront, WORLD_WIDTH - 1);
    }
    Player.pos.YDown = (Player.pos.pY + 16) / 16;
    Player.pos.YDown = min(Player.pos.YDown, WORLD_HEIGHT - 1);

    Player.pos.YUp = (Player.pos.pY - 1) / 16;
    Player.pos.YDown = max(Player.pos.YDown, 0);
}

void calculateWorldCoordinates()
{
    cameraX = Player.pos.pX - (ARCADA_TFT_WIDTH / 2);
    cameraX = max(cameraX, 0);
    cameraX = min(cameraX, (WORLD_WIDTH * 16) - ARCADA_TFT_WIDTH);

    cameraY = Player.pos.pY - (ARCADA_TFT_HEIGHT / 2);
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

    //Utile pour dessiner les sprites a l'ecran
    worldOffset_pX = (worldMIN_X * 16) + currentOffset_X;
    worldOffset_pY = (worldMIN_Y * 16) + currentOffset_Y;
}

int playerPickupItem(Titem *currentItem){
    int ret = 0;

    //inventaire ?
    ret = 1;

    return ret;
}

void checkPlayerCollisionsEntities()
{
    // Check for pickups!
    /*
		if (GetTile(fNewPlayerPosX + 0.0f, fNewPlayerPosY + 0.0f) == L'o')
			SetTile(fNewPlayerPosX + 0.0f, fNewPlayerPosY + 0.0f, L'.');

		if (GetTile(fNewPlayerPosX + 0.0f, fNewPlayerPosY + 1.0f) == L'o')
			SetTile(fNewPlayerPosX + 0.0f, fNewPlayerPosY + 1.0f, L'.');

		if (GetTile(fNewPlayerPosX + 1.0f, fNewPlayerPosY + 0.0f) == L'o')
			SetTile(fNewPlayerPosX + 1.0f, fNewPlayerPosY + 0.0f, L'.');

		if (GetTile(fNewPlayerPosX + 1.0f, fNewPlayerPosY + 1.0f) == L'o')
			SetTile(fNewPlayerPosX + 1.0f, fNewPlayerPosY + 1.0f, L'.');
            
        uint8_t tileTL = getWorldAtPix(Player.pos.newX, Player.pos.pY);
        uint8_t tileBL = getWorldAtPix(Player.pos.newX, Player.pos.pY + 15);

*/
    //Deux briques SUR player ?
    brique_PLAYER = getWorldAt(Player.pos.worldX, Player.pos.worldY);

    //Au dessus
    brique_UP = getWorldAt(Player.pos.worldX, Player.pos.YUp);

    //En dessous
    brique_DOWN = getWorldAt(Player.pos.worldX, Player.pos.YDown);
    //  brique_DOWN_FRONT = getWorldAt(Player.pos.worldX, Player.pos.worldY + 1);

    //Devant
    brique_FRONT = getWorldAt(Player.pos.XFront, Player.pos.worldY);
    /*
    switch (brique_PLAYER)
    {
    case 'o':
        setWorldAt(Player.pos.worldX, Player.pos.worldY, 0);
        SCORE++;
        //    sndPlayerCanal1.play(AudioSampleSmb_coin);
        break;
    case 'w':
    case 'd':
        SCORE += 20;
        set_wining();
        return;
    }
*/
    //Collisions mushrooms
    for (int itC = 0; itC < NB_WORLD_ITEMS; itC++)
    {
        Titem *currentItem = &ITEMS[itC];
        if ((currentItem->bIsAlive > 127) && currentItem->bIsActive)
        {
            //       if (((currentItem->worldX >= (worldMIN_X - 1)) && (currentItem->worldX <= (worldMAX_X + 1))) &&
            //           ((currentItem->worldY >= (worldMIN_Y - 1)) && (currentItem->worldY <= (worldMAX_Y + 1))))
            {

                int px = currentItem->x;
                int py = currentItem->y;

                //BBox
                if (px < Player.pos.pX + 16 &&
                    px + 16 > Player.pos.pX &&
                    py < Player.pos.pY + 16 &&
                    py + 16 > Player.pos.pY)
                {
                    //  Serial.println("Collision.");
                    //On ajoute dans l'inventaire

                    if (playerPickupItem(currentItem))
                    {
                        sndPlayerCanal2.play(AudioSample__ItemPickup);
                        killItem(currentItem, px, py);
                    }
                    else
                    {
                        //On le fait disparaitre doucement ?
                    }

                    //EFFECT
                    //...
                }
            }
        }
    }

    //Collisions ennemis
    for (int enC = 0; enC < NB_WORLD_ENNEMIES; enC++)
    {
        Tennemy *currentEnnemy = &ENNEMIES[enC];
        if (currentEnnemy->bIsAlive > 127)
        {
            //  if (((currentEnnemy->worldX >= (worldMIN_X - 1)) && (currentEnnemy->worldX <= (worldMAX_X + 1))) &&
            //      ((currentEnnemy->worldY >= (worldMIN_Y - 1)) && (currentEnnemy->worldY <= (worldMAX_Y + 1))))
            {

                int px = currentEnnemy->x;
                int py = currentEnnemy->y;

                //BBox
                if (px < Player.pos.pX + 14 &&
                    px + 14 > Player.pos.pX &&
                    py < Player.pos.pY + 14 &&
                    py + 14 > Player.pos.pY)
                {
                    //  Serial.println("Collision.");
                    if (Player.bFalling)
                    {
                        //            sndPlayerCanal1.play(AudioSamplePlayershell);
                        SCORE += 5;
                        killEnnemy(currentEnnemy, px, py);
                        //rebond
                        Player.pos.speedY = JUMP_SPEED / 3;
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
    bool bWasOnGround = Player.bOnGround;

    if (Player.bDying)
    {
        jumpPhase += 2;

        if (jumpPhase < 14)
        {
            Player.pos.pY = Player.pos.pY - (14 - jumpPhase);
        }
        else
        {
            Player.pos.pY = Player.pos.pY + (FALLING_SPEED / 2);
        }
    }
    else if (bWin)
    {
        calculatePlayerCoords();

        if ((brique_FRONT.id == 0 || brique_FRONT.id == BLOCK_UNDERGROUND_AIR) &&
            ((Player.pos.direction > 0 && (Player.pos.pX < ((WORLD_WIDTH - 1) * 16) - ARCADA_TFT_WIDTH)) ||
             (Player.pos.direction < 0 && (Player.pos.pX >= 2))))
        {
            Player.pos.pX += 1;
            Player.bMoving = true;
        }

        //Particules feu artifice ?
        Player.pos.pY = Player.pos.pY + FALLING_SPEED;

        calculatePlayerCoords();
    }
    else
    {
#ifdef DEBUG
        if (pressed_buttons & ARCADA_BUTTONMASK_LEFT)
        {
            Player.pos.pX -= 4;
            Player.pos.pX = max(0, Player.pos.pX);
        }
        if (pressed_buttons & ARCADA_BUTTONMASK_RIGHT)
        {
            Player.pos.pX += 4;
            Player.pos.pX = min(((WORLD_WIDTH - 1) * 16), Player.pos.pX);
        }
        if (pressed_buttons & ARCADA_BUTTONMASK_UP)
        {
            Player.pos.pY -= 4;
            Player.pos.pY = max(0, Player.pos.pY);
        }
        if (pressed_buttons & ARCADA_BUTTONMASK_DOWN)
        {
            Player.pos.pY += 4;
            Player.pos.pY = min(((WORLD_HEIGHT - 1) * 16), Player.pos.pY);
        }
#else
        checkPlayerInputs();
        updatePlayerVelocities();
        updatePlayerPosition();

        checkPlayerCollisionsWorld();

        checkPlayerCollisionsEntities();
#endif

        //Position player
        calculateWorldCoordinates();
        calculatePlayerCoords();

        computePlayerAnimations();
    }
}

void computePlayerAnimations()
{
    //Serial.printf("counterActionB:%d Player.bJumping:%d Player.bFalling:%d bOnGround:%d Player.bWantWalk:%d Player.bMoving:%d\n", counterActionB, Player.bJumping, Player.bFalling, bOnGround, Player.bWantWalk, Player.bMoving);
    if (counterActionB > 0)
    {
        //@todo : Test type action,
        //pour le moment dig
        set_digging();
    }
    else
    {
        if (Player.bJumping)
        {
            set_jumping();
        }
        else if (Player.bFalling)
        {
            set_falling();
        }
        else
        {
            if (Player.bOnGround)
            {
                if (Player.bWantWalk) // demannde de deplacement)
                {
                    if (Player.bMoving) //On a bougé
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

    for (int enC = 0; enC < NB_WORLD_ENNEMIES; enC++)
    {
        Tennemy *currentEnnemy = &ENNEMIES[enC];
        if (currentEnnemy->bIsAlive > 127)
        {
            updateEnnemyIA(currentEnnemy);

            if (currentEnnemy->bIsActive)
            {
                //Update velocities
                //Gravity
                currentEnnemy->speed_y = currentEnnemy->speed_y + FALLING_SPEED;

                //Frotements
                /* if (currentEnnemy->bOnGround)
                {
                    if (currentEnnemy->speed_x > 0)
                        currentEnnemy->speed_x--;
                    else if (currentEnnemy->speed_x < 0)
                        currentEnnemy->speed_x++;
                }*/

                if (currentEnnemy->speed_x > MAX_SPEED_X)
                    currentEnnemy->speed_x = MAX_SPEED_X;
                else if (currentEnnemy->speed_x < -MAX_SPEED_X)
                    currentEnnemy->speed_x = -MAX_SPEED_X;

                if (currentEnnemy->speed_y > MAX_SPEED_Y)
                    currentEnnemy->speed_y = MAX_SPEED_Y;
                else if (currentEnnemy->speed_y < -MAX_SPEED_Y)
                    currentEnnemy->speed_y = -MAX_SPEED_Y;

                //update pos
                if (currentEnnemy->speed_x > 0)
                {
                    currentEnnemy->new_x = currentEnnemy->x + currentEnnemy->speed_x;
                    currentEnnemy->bMoving = true;

                    currentEnnemy->new_x = min(((WORLD_WIDTH - 1) * 16), currentEnnemy->new_x);
                }
                else if (currentEnnemy->speed_x < 0)
                {
                    currentEnnemy->new_x = currentEnnemy->x + currentEnnemy->speed_x;
                    currentEnnemy->bMoving = true;

                    currentEnnemy->new_x = max(0, currentEnnemy->new_x);
                }

                currentEnnemy->new_y = currentEnnemy->y + currentEnnemy->speed_y;

                //collisions
                checkEnnemyCollisionsWorld(currentEnnemy);
            }
        }
    }
}

void checkEnnemyCollisionsWorld(Tennemy *currentEnnemy)
{
    //X
    if (currentEnnemy->speed_x <= 0)
    {
        TworldTile tileTL = getWorldAtPix(currentEnnemy->new_x, currentEnnemy->y);
        TworldTile tileBL = getWorldAtPix(currentEnnemy->new_x, currentEnnemy->y + 15);
        if ((tileTL.id != BLOCK_AIR && tileTL.id != BLOCK_UNDERGROUND_AIR) || (tileBL.id != BLOCK_AIR && tileBL.id != BLOCK_UNDERGROUND_AIR))
        {
            currentEnnemy->new_x = (currentEnnemy->new_x - (currentEnnemy->new_x % 16)) + 16;
            currentEnnemy->speed_x = -currentEnnemy->speed_x;
        }
    }
    else
    {
        TworldTile tileTR = getWorldAtPix(currentEnnemy->new_x + 16, currentEnnemy->y);
        TworldTile tileBR = getWorldAtPix(currentEnnemy->new_x + 16, currentEnnemy->y + 15);
        if ((tileTR.id != BLOCK_AIR && tileTR.id != BLOCK_UNDERGROUND_AIR) || (tileBR.id != BLOCK_AIR && tileBR.id != BLOCK_UNDERGROUND_AIR))
        {
            currentEnnemy->new_x = (currentEnnemy->new_x - (currentEnnemy->new_x % 16));
            currentEnnemy->speed_x = -currentEnnemy->speed_x;
        }
    }
    //Y
    currentEnnemy->bOnGround = false;
    currentEnnemy->bJumping = false;
    currentEnnemy->bFalling = false;
    if (currentEnnemy->speed_y <= 0)
    {
        //+5 et +12 au lieu de +0 et +15 pour compenser la boundingbox du sprite => NON car cause bug de saut en diagonale
        TworldTile tileTL = getWorldAtPix(currentEnnemy->new_x + 0, currentEnnemy->new_y);
        TworldTile tileTR = getWorldAtPix(currentEnnemy->new_x + 15, currentEnnemy->new_y);
        if ((tileTL.id != BLOCK_AIR && tileTL.id != BLOCK_UNDERGROUND_AIR) || (tileTR.id != BLOCK_AIR && tileTR.id != BLOCK_UNDERGROUND_AIR))
        {
            currentEnnemy->new_y = (currentEnnemy->new_y - (currentEnnemy->new_y % 16)) + 16;
            currentEnnemy->speed_y = 0;
        }
        else
        {
            currentEnnemy->bJumping = true;
        }
    }
    else
    {
        currentEnnemy->bFalling = true;
        //+5 et +12 au lieu de +0 et +15 pour compenser la boundingbox du sprite => NON car cause bug de saut en diagonale
        TworldTile tileBL = getWorldAtPix(currentEnnemy->new_x + 0, currentEnnemy->new_y + 16);
        TworldTile tileBR = getWorldAtPix(currentEnnemy->new_x + 15, currentEnnemy->new_y + 16);
        if ((tileBL.id != BLOCK_AIR && tileBL.id != BLOCK_UNDERGROUND_AIR) || (tileBR.id != BLOCK_AIR && tileBR.id != BLOCK_UNDERGROUND_AIR))
        {
            currentEnnemy->new_y = (currentEnnemy->new_y - (currentEnnemy->new_y % 16));
            currentEnnemy->bOnGround = true;
            currentEnnemy->bFalling = false;
            currentEnnemy->speed_y = 0;
        }
    }

    currentEnnemy->x = currentEnnemy->new_x;
    currentEnnemy->y = currentEnnemy->new_y;

    currentEnnemy->worldX = currentEnnemy->x / 16;
    currentEnnemy->worldY = currentEnnemy->y / 16;
}

void updateEnnemyIA(Tennemy *currentEnnemy)
{
    if (currentEnnemy->bIsAlive > 127)
    {
        //Si ennemi trop loin on ne fait pas
        int distX = abs(currentEnnemy->worldX - Player.pos.worldX);
        int distY = abs(currentEnnemy->worldY - Player.pos.worldY);
        //if ((currentEnnemy->worldX >= 1) && (currentEnnemy->worldX <= worldMAX_X))
        if ((distX < 30) && (distY < 20))
        {
            currentEnnemy->bIsActive = true;
            //Deplacement
            if (currentEnnemy->type == SPIDER_ENNEMY)
            {
                if ((currentEnnemy->speed_x < 0) && currentEnnemy->x <= 0)
                    currentEnnemy->speed_x = SPIDER_WALKING_SPEED;
                else if ((currentEnnemy->speed_x > 0) && currentEnnemy->x >= WORLD_WIDTH * 16)
                    currentEnnemy->speed_x = -SPIDER_WALKING_SPEED;

                if (distY <= 2)
                {
                    int dist = currentEnnemy->worldX - Player.pos.worldX;
                    if (dist < 0 && dist > -5)
                    {
                        currentEnnemy->speed_x = SPIDER_WALKING_SPEED;
                    }
                    else if (dist > 0 && dist < 5)
                    {
                        currentEnnemy->speed_x = -SPIDER_WALKING_SPEED;
                    }
                }
            }
            else if (currentEnnemy->type == ZOMBI_ENNEMY)
            {
                if ((currentEnnemy->speed_x < 0) && currentEnnemy->x <= 0)
                    currentEnnemy->speed_x = ZOMBI_WALKING_SPEED;
                else if ((currentEnnemy->speed_x > 0) && currentEnnemy->x >= WORLD_WIDTH * 16)
                    currentEnnemy->speed_x = -ZOMBI_WALKING_SPEED;

                if (currentEnnemy->worldY == Player.pos.worldY)
                {
                    int dist = currentEnnemy->worldX - Player.pos.worldX;
                    if (dist < 0 && dist > -5)
                    {
                        currentEnnemy->speed_x = ZOMBI_WALKING_SPEED;
                    }
                    else if (dist > 0 && dist < 5)
                    {
                        currentEnnemy->speed_x = -ZOMBI_WALKING_SPEED;
                    }
                }
            }
            //speed x et speed y (jump)
        }
        else
            currentEnnemy->bIsActive = false;
    }
}

void updateItems()
{
    for (int enC = 0; enC < NB_WORLD_ITEMS; enC++)
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
                //Update velocities
                //Gravity
                currentItem->speed_y = currentItem->speed_y + FALLING_SPEED;

                if (currentItem->speed_y > MAX_SPEED_Y)
                    currentItem->speed_y = MAX_SPEED_Y;
                else if (currentItem->speed_y < -MAX_SPEED_Y)
                    currentItem->speed_y = -MAX_SPEED_Y;

                //update pos
                currentItem->new_y = currentItem->y + currentItem->speed_y;

                //collisions
                checkItemCollisionsWorld(currentItem);
            }

            // else
            // currentItem->bIsActive = false;
        }
    }
}
void checkItemCollisionsWorld(Titem *currentItem)
{
    //Y
    currentItem->bOnGround = false;
    currentItem->bJumping = false;
    currentItem->bFalling = false;
    if (currentItem->speed_y <= 0)
    {
        //+5 et +12 au lieu de +0 et +15 pour compenser la boundingbox du sprite => NON car cause bug de saut en diagonale
        TworldTile tileTL = getWorldAtPix(currentItem->x + 0, currentItem->new_y);
        TworldTile tileTR = getWorldAtPix(currentItem->x + 15, currentItem->new_y);
        if ((tileTL.id != BLOCK_AIR && tileTL.id != BLOCK_UNDERGROUND_AIR) || (tileTR.id != BLOCK_AIR && tileTR.id != BLOCK_UNDERGROUND_AIR))
        {
            currentItem->new_y = (currentItem->new_y - (currentItem->new_y % 16)) + 16;
            currentItem->speed_y = 0;
        }
        else
        {
            currentItem->bJumping = true;
        }
    }
    else
    {
        currentItem->bFalling = true;
        //+5 et +12 au lieu de +0 et +15 pour compenser la boundingbox du sprite => NON car cause bug de saut en diagonale
        TworldTile tileBL = getWorldAtPix(currentItem->x + 0, currentItem->new_y + 16);
        TworldTile tileBR = getWorldAtPix(currentItem->x + 15, currentItem->new_y + 16);
        if ((tileBL.id != BLOCK_AIR && tileBL.id != BLOCK_UNDERGROUND_AIR) || (tileBR.id != BLOCK_AIR && tileBR.id != BLOCK_UNDERGROUND_AIR))
        {
            currentItem->new_y = (currentItem->new_y - (currentItem->new_y % 16));
            currentItem->bOnGround = true;
            currentItem->bFalling = false;
            currentItem->speed_y = 0;
        }
    }

    currentItem->y = currentItem->new_y;

    currentItem->worldX = currentItem->x / 16;
    currentItem->worldY = currentItem->y / 16;
}

void updatePlayerVelocities()
{
    //Gravity
    Player.pos.speedY = Player.pos.speedY + FALLING_SPEED;

    //Frotements
    if (Player.bOnGround)
    {
        if (Player.pos.speedX > 0)
            Player.pos.speedX--;
        else if (Player.pos.speedX < 0)
            Player.pos.speedX++;
    }

    if (Player.pos.speedX > MAX_SPEED_X)
        Player.pos.speedX = MAX_SPEED_X;
    else if (Player.pos.speedX < -MAX_SPEED_X)
        Player.pos.speedX = -MAX_SPEED_X;

    if (Player.pos.speedY > MAX_SPEED_Y)
        Player.pos.speedY = MAX_SPEED_Y;
    else if (Player.pos.speedY < -MAX_SPEED_Y)
        Player.pos.speedY = -MAX_SPEED_Y;
}

void updatePlayerPosition()
{
    //if (Player.bWantWalk)
    {
        //CHECK COLLISION a refaire propre
        if (Player.pos.speedX > 0)
        {
            Player.pos.newX = Player.pos.pX + Player.pos.speedX;
            Player.bMoving = true;

            Player.pos.newX = min(((WORLD_WIDTH - 1) * 16), Player.pos.newX);
        }
        else if (Player.pos.speedX < 0)
        {
            Player.pos.newX = Player.pos.pX + Player.pos.speedX;
            Player.bMoving = true;

            Player.pos.newX = max(0, Player.pos.newX);
        }
    }

    Player.pos.newY = Player.pos.pY + Player.pos.speedY;
}

void checkPlayerCollisionsWorld()
{
    //X
    if (Player.pos.speedX <= 0)
    {
        TworldTile tileTL = getWorldAtPix(Player.pos.newX, Player.pos.pY);
        TworldTile tileBL = getWorldAtPix(Player.pos.newX, Player.pos.pY + 15);
        if ((tileTL.id != BLOCK_AIR && tileTL.id != BLOCK_UNDERGROUND_AIR) || (tileBL.id != BLOCK_AIR && tileBL.id != BLOCK_UNDERGROUND_AIR))
        {
            Player.pos.newX = (Player.pos.newX - (Player.pos.newX % 16)) + 16;
            Player.pos.speedX = 0;
        }
    }
    else
    {
        TworldTile tileTR = getWorldAtPix(Player.pos.newX + 16, Player.pos.pY);
        TworldTile tileBR = getWorldAtPix(Player.pos.newX + 16, Player.pos.pY + 15);
        if ((tileTR.id != BLOCK_AIR && tileTR.id != BLOCK_UNDERGROUND_AIR) || (tileBR.id != BLOCK_AIR && tileBR.id != BLOCK_UNDERGROUND_AIR))
        {
            Player.pos.newX = (Player.pos.newX - (Player.pos.newX % 16));
            Player.pos.speedX = 0;
        }
    }
    //Y
    Player.bOnGround = false;
    Player.bJumping = false;
    Player.bFalling = false;
    if (Player.pos.speedY <= 0)
    {
        //+5 et +12 au lieu de +0 et +15 pour compenser la boundingbox du sprite => NON car cause bug de saut en diagonale
        TworldTile tileTL = getWorldAtPix(Player.pos.newX + 0, Player.pos.newY);
        TworldTile tileTR = getWorldAtPix(Player.pos.newX + 15, Player.pos.newY);
        if ((tileTL.id != BLOCK_AIR && tileTL.id != BLOCK_UNDERGROUND_AIR) || (tileTR.id != BLOCK_AIR && tileTR.id != BLOCK_UNDERGROUND_AIR))
        {
            Player.pos.newY = (Player.pos.newY - (Player.pos.newY % 16)) + 16;
            Player.pos.speedY = 0;
        }
        else
        {
            Player.bJumping = true;
        }
    }
    else
    {
        Player.bFalling = true;
        //+5 et +12 au lieu de +0 et +15 pour compenser la boundingbox du sprite => NON car cause bug de saut en diagonale
        TworldTile tileBL = getWorldAtPix(Player.pos.newX + 0, Player.pos.newY + 16);
        TworldTile tileBR = getWorldAtPix(Player.pos.newX + 15, Player.pos.newY + 16);
        if ((tileBL.id != BLOCK_AIR && tileBL.id != BLOCK_UNDERGROUND_AIR) || (tileBR.id != BLOCK_AIR && tileBR.id != BLOCK_UNDERGROUND_AIR))
        {
            Player.pos.newY = (Player.pos.newY - (Player.pos.newY % 16));
            Player.bOnGround = true;
            Player.bFalling = false;
            Player.pos.speedY = 0;
        }
    }

    Player.pos.pX = Player.pos.newX;
    Player.pos.pY = Player.pos.newY;
}

void checkPlayerInputs()
{
    Player.bWantJump = false;
    Player.bWantDoubleJump = false;
    Player.bWantWalk = false;

    if (just_pressed & ARCADA_BUTTONMASK_SELECT)
    {
        //changement inventaire rapide
        Player.currentItemSelected++;
        if (Player.currentItemSelected == MAX_QUICK_ITEMS)
            Player.currentItemSelected = 0;
    }
    else if (just_pressed & ARCADA_BUTTONMASK_START)
    {
        //Menu personnage
        if (gameState == STATE_GAME_MENU)
            gameState = STATE_PLAYING;
        else if (gameState == STATE_PLAYING)
        {
            gameState = STATE_GAME_MENU;
        }
    }

    if (coolDownActionB <= 0 && (pressed_buttons & ARCADA_BUTTONMASK_B) && Player.bOnGround)
    {
        counterActionB++;

        int lastCibleX = Player.cible_wX;
        int lastCibleY = Player.cible_wY;

        //Ciblage, on laisse un peu de temps pour locker la cible
        if (counterActionB < FRAMES_LOCK_ACTION_B)
        {
            if ((pressed_buttons & ARCADA_BUTTONMASK_LEFT) || (pressed_buttons & ARCADA_BUTTONMASK_RIGHT))
            {
                if (pressed_buttons & ARCADA_BUTTONMASK_DOWN)
                {
                    Player.cible_wX = Player.pos.XFront;
                    Player.cible_wY = Player.pos.YDown;
                }
                else if (pressed_buttons & ARCADA_BUTTONMASK_UP)
                {
                    Player.cible_wX = Player.pos.XFront;
                    Player.cible_wY = Player.pos.YUp;
                }
                else
                {
                    Player.cible_wX = Player.pos.XFront;
                    Player.cible_wY = Player.pos.worldY;
                }
            }
            else
            {
                if (pressed_buttons & ARCADA_BUTTONMASK_UP)
                {
                    Player.cible_wX = Player.pos.worldX;
                    Player.cible_wY = Player.pos.YUp;
                }
                else if (pressed_buttons & ARCADA_BUTTONMASK_DOWN)
                {
                    Player.cible_wX = Player.pos.worldX;
                    Player.cible_wY = Player.pos.YDown;
                }
                else
                {
                    //'Devant'
                    Player.cible_wX = Player.pos.XFront;
                    Player.cible_wY = Player.pos.worldY;
                }
            }
        }

        if ((Player.cible_wX != lastCibleX) || (Player.cible_wY != lastCibleY))
        {
            counterActionB = 0;
            Player.cible_pX = ((Player.cible_wX - worldMIN_X) * 16) - currentOffset_X;
            Player.cible_pY = ((Player.cible_wY - worldMIN_Y) * 16) - currentOffset_Y;
        }

        if (counterActionB >= FRAMES_ACTION_B) //Action (pour le moment minage seulement)
        {
            TworldTile value = getWorldAt(Player.cible_wX, Player.cible_wY);

            counterActionB = 0;
            coolDownActionB = FRAMES_COOLDOWN_B;
            lastCibleX = lastCibleY = 0;

            //Serial.printf("Dig:%d,%d v:%d\n", Player.cible_wX, Player.cible_wY, value);
            if (value.id != BLOCK_AIR && value.id != BLOCK_UNDERGROUND_AIR && Player.pos.YDown < (WORLD_HEIGHT - 1))
            {
                // @todo span item ramassable ? ou tile ramassable ?
                createItem(Player.cible_wX, Player.cible_wY, value.id);

                if (value.id < BLOCK_ROCK)
                    value.id = BLOCK_AIR;
                else
                    value.id = BLOCK_UNDERGROUND_AIR;

                // @todo : tester le type de value
                sndPlayerCanal1.play(AudioSampleRock_break);
                parts.createExplosion(Player.cible_pX + 8, Player.cible_pY + 8, 12); //@todo colorer avec la couleur de la brique en cours de travail

                setWorldAt(Player.cible_wX, Player.cible_wY, value);
                updateHauteurColonne(Player.cible_wX, Player.cible_wY);
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
        if (coolDownActionB > 0)
            coolDownActionB--;

        counterActionB = 0;
        //Player.cible_wX = Player.cible_wY = 0;

        if (just_pressed & ARCADA_BUTTONMASK_A)
        {
            if (Player.pos.speedY == 0 && Player.bOnGround)
            {
                Player.bWantJump = true;
                sndPlayerCanal1.play(AudioSample__Jump);
                //Thrust
                Player.pos.speedY = -JUMP_SPEED;
                //Spawn Effect
                set_double_jump_fx();
            }
            else if (!Player.bOnGround && Player.pos.speedY < FALLING_SPEED)
            {
                // @todo spawn dust double jump
                Player.bWantDoubleJump = true;
                sndPlayerCanal1.play(AudioSample__Jump);
                //Thrust
                Player.pos.speedY = -DOUBLE_JUMP_SPEED;
                //Spawn Effect
                set_double_jump_fx();
            }
        }

        if (pressed_buttons & ARCADA_BUTTONMASK_LEFT)
        {
            Player.pos.direction = -1;
            Player.bWantWalk = true;
            if (Player.bOnGround)
                Player.pos.speedX += -RUNNING_SPEED;
            else
            {
                if (Player.pos.speedY < 0)
                    Player.pos.speedX += -WALKING_SPEED;
                else if (Player.pos.speedX == 0)
                    Player.pos.speedX = -WALKING_SPEED;
            }
        }
        else if (pressed_buttons & ARCADA_BUTTONMASK_RIGHT)
        {
            Player.pos.direction = 1;
            Player.bWantWalk = true;
            if (Player.bOnGround)
                Player.pos.speedX += RUNNING_SPEED;
            else
            {
                if (Player.pos.speedY < 0)
                    Player.pos.speedX += WALKING_SPEED;
                else if (Player.pos.speedX == 0)
                    Player.pos.speedX = WALKING_SPEED;
            }
        }
    }
}

void updateGame()
{
    int lastX = 0;
    Player.bMoving = false;

    if (Player.bDying)
    {
        count_player_die++;
        if (count_player_die >= PLAYER_DIE_FRAMES)
        {
            // @todo : On affiche gameover et on quitte
            initGame();
        }
        updatePlayer();
        updateEnnemies();
        updateItems();
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

        updateEnnemies();
        updateItems();

        lastX = cameraX;
    }

    //Ps
    parts.moveParticles();
}

void initMenu()
{

    arcada.display->setTextColor(ARCADA_GREEN);
}

void updateGameMenu()
{
}

void displayGameMenu()
{
    static int menu_page = 0;

    const char *titles[] = {"Personnage", "Inventaire", "Craft", "Sauv."};
    //Onglets : character, inventaire, crafting
    int ret = 0;
    do
    {
        arcada.display->fillScreen(ARCADA_MAROON);
        switch (menu_page)
        {
        case 0:
        {
            const char *selection[] = {"Force", "Magie"};
            ret = Game_Menu(titles[menu_page], selection, 2, ARCADA_WHITE, ARCADA_BLACK);
            break;
        }
        case 1:
        {
            const char *selection[] = {"Armes", "Armures"};
            ret = Game_Menu(titles[menu_page], selection, 2, ARCADA_WHITE, ARCADA_BLACK);
            break;
        }
        case 2:
        {
            const char *selection[] = {"Armes", "Armures"};
            ret = Game_Menu(titles[menu_page], selection, 2, ARCADA_WHITE, ARCADA_BLACK);
            break;
        }
        case 3:
        {
            const char *selection[] = {"Emplacement 1", "Emplacement 2", "Emplacement 3", "Emplacement 4", "Emplacement 5"};
            ret = Game_Menu(titles[menu_page], selection, 5, ARCADA_WHITE, ARCADA_BLACK);
            if (ret >= 0 && ret <= 4)
            {
                //Save
                if (SaveGame(ret + 1))
                {
                    arcada.infoBox("Sauvegarde OK.", ARCADA_BUTTONMASK_A);
                    ret = 255;
                }
                else
                    arcada.errorBox("Erreur !", ARCADA_BUTTONMASK_A);
            }
            break;
        }
        }
        if (ret == 127)
        {
            menu_page++;
            if (menu_page > 3)
                menu_page = 0;
        }
    } while (ret != 255);

    gameState = STATE_PLAYING;
}

static uint8_t maxCharPerLine, fontSize;
static uint16_t charHeight, charWidth;

void _initAlertFonts(void)
{
    fontSize = 1;
    if (arcada.display->width() > 200)
    {
        fontSize = 2;
    }
    charHeight = 8 * fontSize;
    charWidth = 6 * fontSize;
    maxCharPerLine = arcada.display->width() / (6 * fontSize) - 6;
}

uint8_t Game_Menu(const char *menu_name, const char **menu_strings,
                  uint8_t menu_num, uint16_t boxColor,
                  uint16_t textColor)
{

    _initAlertFonts();
    arcada.display->setTextSize(fontSize);

    uint16_t max_len = 0;
    for (int i = 0; i < menu_num; i++)
    {
        // Serial.printf("#%d '%s' -> %d\n", i, menu_strings[i],
        // strlen(menu_strings[i]));
        max_len = max(max_len, strlen(menu_strings[i]));
    }

    arcada.display->fillRoundRect(2, 2, arcada.display->width() - 4, charHeight + 4, 3, ARCADA_BLUE);
    arcada.display->drawRoundRect(2, 2, arcada.display->width() - 4, charHeight + 4, 3, ARCADA_WHITE);
    int titleWidth = charWidth * (strlen(menu_name));
    arcada.display->setTextColor(ARCADA_WHITE);
    arcada.display->setCursor((arcada.display->width() - titleWidth) / 2, 4);
    arcada.display->print(menu_name);

    arcada.display->fillRoundRect(2, 16, arcada.display->width() - 4, arcada.display->height() - 20, 3, ARCADA_BLUE);
    arcada.display->drawRoundRect(2, 16, arcada.display->width() - 4, arcada.display->height() - 20, 3, ARCADA_WHITE);

    uint16_t boxWidth = (max_len + 4) * charWidth;
    uint16_t boxHeight = (menu_num + 2) * charHeight;
    uint16_t boxX = (arcada.display->width() - boxWidth) / 2;
    uint16_t boxY = (arcada.display->height() - boxHeight) / 2;

    // draw the outline box
    arcada.display->fillRoundRect(boxX, boxY, boxWidth, boxHeight, charWidth, boxColor);
    arcada.display->drawRoundRect(boxX, boxY, boxWidth, boxHeight, charWidth, textColor);

    // Print the selection hint
    const char *buttonString = "A";
    uint16_t fontX =
        boxX + boxWidth - (strlen(buttonString) + 1) * charWidth + 2 * fontSize;
    uint16_t fontY = boxY + boxHeight - charHeight;
    arcada.display->fillRoundRect(fontX, fontY, (strlen(buttonString) + 2) * charWidth,
                                  charHeight * 2, charWidth, textColor);
    arcada.display->drawRoundRect(fontX, fontY, (strlen(buttonString) + 2) * charWidth,
                                  charHeight * 2, charWidth, boxColor);
    arcada.display->setCursor(fontX + charWidth, fontY + charHeight / 2);
    arcada.display->setTextColor(boxColor);
    arcada.display->print(buttonString);

    // draw and select the menu
    int8_t selected = 0;
    fontX = boxX + charWidth / 2;
    fontY = boxY + charHeight;

    // wait for any buttons to be released
    while (arcada.readButtons())
        delay(10);

    while (1)
    {
        for (int i = 0; i < menu_num; i++)
        {
            if (i == selected)
            {
                arcada.display->setTextColor(boxColor, textColor);
            }
            else
            {
                arcada.display->setTextColor(textColor, boxColor);
            }
            arcada.display->setCursor(fontX, fontY + charHeight * i);
            arcada.display->print(" ");
            arcada.display->print(menu_strings[i]);
            for (int j = strlen(menu_strings[i]); j < max_len + 2; j++)
            {
                arcada.display->print(" ");
            }
        }

        while (1)
        {
            delay(10);
            arcada.readButtons();
            uint32_t released = arcada.justReleasedButtons();
            if (released & ARCADA_BUTTONMASK_UP)
            {
                selected--;
                if (selected < 0)
                    selected = menu_num - 1;
                break;
            }
            if (released & ARCADA_BUTTONMASK_DOWN)
            {
                selected++;
                if (selected > menu_num - 1)
                    selected = 0;
                break;
            }
            if (released & ARCADA_BUTTONMASK_A)
            {
                return selected;
            }
            if ((released & ARCADA_BUTTONMASK_B) || (released & ARCADA_BUTTONMASK_START))
            {
                return 255;
            }
            if (released & ARCADA_BUTTONMASK_SELECT)
            {
                return 127;
            }
        }
    }
    return selected;
}

bool LoadGame(uint8_t numEmplacement)
{
    bool ret = false;

    char fileName[80];

    sprintf(fileName, "/saves/LO_%02d.sav", numEmplacement);

    //Lecture du fichier
    File data = arcada.open(fileName, FILE_READ);
    if (data)
    {
        //read player stats
        //read ennemies, items, etc.
        //read world

        data.seek(0);

        data.read(); // [
        for (int wY = 0; wY < WORLD_HEIGHT + 2; wY++)
        {
            for (int wX = 0; wX < WORLD_WIDTH; wX++)
            {
                char buff[5];
                buff[0] = data.read();
                buff[1] = data.read();
                buff[2] = data.read();
                buff[3] = 0;
                data.read(); // ,
                int value = strtol(buff, 0, 10);
                WORLD[wY][wX].id = (uint8_t)value;
            }
        }
        data.read(); //  ]
        //read world infos

        //data.read();
        data.close();

        ret = true;
    }
    return ret;
}

bool SaveGame(uint8_t numEmplacement)
{
    bool ret = false;

    char fileName[80];

    sprintf(fileName, "/saves/LO_%02d.sav", numEmplacement);
    //Enreg du fichier
    File data = arcada.open(fileName, (O_RDWR | O_CREAT | O_TRUNC));
    //read player stats
    //read ennemies, items, etc.
    //read world
    if (data)
    {
        data.write('[');
        for (int wY = 0; wY < WORLD_HEIGHT + 2; wY++)
        {
            for (int wX = 0; wX < WORLD_WIDTH; wX++)
            {
                uint8_t value = WORLD[wY][wX].id;
                char buff[5];
                sprintf(buff, "%03d,", value);
                data.write(buff[0]);
                data.write(buff[1]);
                data.write(buff[2]);
                data.write(buff[3]);
            }
        }
        data.write(']');
        //read world infos

        data.flush();
        data.close();

        ret = true;
    }
    return ret;
}

void updatePauseMenu()
{
}

void displayPauseMenu()
{
    // digital clock display of the time
    arcada.display->fillScreen(ARCADA_BLUE);
    const char *selection[] = {"EN CONSTRUCTION"};
    uint8_t selected = arcada.menu(selection, 1, ARCADA_WHITE, ARCADA_BLACK, true);

    if (selected == 0)
    {
        gameState = STATE_PLAYING;
    }
    else
    {
        gameState = STATE_PLAYING;
    }
}
void updateMainMenu()
{
}

void displayMainMenu()
{
    // digital clock display of the time
    arcada.display->fillScreen(ARCADA_BLUE);

    const char *selection[] = {"Nouvelle partie", "Charger", "Options"};
    uint8_t selected = arcada.menu(selection, 3, ARCADA_WHITE, ARCADA_BLACK);

    if (selected == 0)
    {
        //Create player...etc
        // @todo finir
        initGame();
        gameState = STATE_PLAYING;
    }
    else if (selected == 1)
    {
        // wait for button release
        while (arcada.readButtons())
        {
            delay(10);
        }
        initGame();

        const char *selection_empl[] = {"Emplacement 1", "Emplacement 2", "Emplacement 3", "Emplacement 4", "Emplacement 5"};
        uint8_t selected_empl = arcada.menu(selection_empl, 5, ARCADA_WHITE, ARCADA_BLACK, true);
        if (selected_empl >= 0 && selected_empl <= 4)
            if (LoadGame(selected_empl + 1))
            {
                arcada.infoBox("Chargement OK.", ARCADA_BUTTONMASK_A);
                gameState = STATE_PLAYING;
            }
            else
                arcada.errorBox("Erreur.", ARCADA_BUTTONMASK_A);
    }
}

void readInputs()
{
    pressed_buttons = arcada.readButtons();
    just_pressed = arcada.justPressedButtons();
}

void loop()
{
    long elapsedTime;

    //long now = millis();
    //updateSoundManager(now);

    if (!nextFrame())
        return;

    readInputs();
    if (gameState == STATE_MAIN_MENU)
    {
        updateMainMenu();
        displayMainMenu();
    }
    else if (gameState == STATE_PLAYING)
    {
        updateGame();
        displayGame();
    }
    else if (gameState == STATE_PAUSE_MENU)
    {
        updatePauseMenu();
        displayPauseMenu();
    }
    else if (gameState == STATE_GAME_MENU)
    {
        updateGameMenu();
        displayGameMenu();
    }

    //elapsedTime = millis() - now;

    //Serial.printf("E:%d LFD:%d\n", elapsedTime, lastFrameDurationMs);
}

void anim_player_idle()
{
    if (Player.current_framerate == PLAYER_FRAMERATE_IDLE)
    {
        Player.anim_frame++;
        Player.current_framerate = 0;
    }
    Player.current_framerate++;

    if (Player.anim_frame > PLAYER_FRAME_IDLE_3)
    {
        Player.anim_frame = PLAYER_FRAME_IDLE_1;
    }

    switch (Player.anim_frame)
    {
    case PLAYER_FRAME_IDLE_1:
        drawSprite(Player.pos.pX - cameraX, Player.pos.pY - cameraY, player_idle1.width, player_idle1.height, player_idle1.pixel_data, Player.pos.direction);
        break;
    case PLAYER_FRAME_IDLE_2:
        drawSprite(Player.pos.pX - cameraX, Player.pos.pY - cameraY, player_idle2.width, player_idle2.height, player_idle2.pixel_data, Player.pos.direction);
        break;
    case PLAYER_FRAME_IDLE_3:
        drawSprite(Player.pos.pX - cameraX, Player.pos.pY - cameraY, player_idle3.width, player_idle3.height, player_idle3.pixel_data, Player.pos.direction);
        break;
    }
}

void anim_player_digging()
{
    if (Player.current_framerate == PLAYER_FRAMERATE_ACTION)
    {
        Player.anim_frame++;
        Player.current_framerate = 0;
    }
    Player.current_framerate++;

    if (Player.anim_frame > PLAYER_FRAME_ACTION_4)
    {
        Player.anim_frame = PLAYER_FRAME_ACTION_1;
    }

    switch (Player.anim_frame)
    {
    case PLAYER_FRAME_ACTION_1:
        drawSprite(Player.pos.pX - cameraX, Player.pos.pY - cameraY, player_action1.width, player_action1.height, player_action1.pixel_data, Player.pos.direction);
        break;
    case PLAYER_FRAME_ACTION_2:
        drawSprite(Player.pos.pX - cameraX, Player.pos.pY - cameraY, player_action2.width, player_action2.height, player_action2.pixel_data, Player.pos.direction);

        break;
    case PLAYER_FRAME_ACTION_3:
        drawSprite(Player.pos.pX - cameraX, Player.pos.pY - cameraY, player_action3.width, player_action3.height, player_action3.pixel_data, Player.pos.direction);
        break;
    case PLAYER_FRAME_ACTION_4:
        drawSprite(Player.pos.pX - cameraX, Player.pos.pY - cameraY, player_action4.width, player_action4.height, player_action4.pixel_data, Player.pos.direction);
        break;
    }
}

void anim_player_walk()
{
    if (Player.current_framerate == PLAYER_FRAMERATE_WALK)
    {
        Player.anim_frame++;
        Player.current_framerate = 0;
    }
    Player.current_framerate++;

    if (Player.anim_frame > PLAYER_FRAME_WALK_6)
    {
        Player.anim_frame = PLAYER_FRAME_WALK_1;
    }

    switch (Player.anim_frame)
    {
    case PLAYER_FRAME_WALK_1:
        drawSprite(Player.pos.pX - cameraX, Player.pos.pY - cameraY, player_walk1.width, player_walk1.height, player_walk1.pixel_data, Player.pos.direction);
        break;
    case PLAYER_FRAME_WALK_2:
        drawSprite(Player.pos.pX - cameraX, Player.pos.pY - cameraY, player_walk2.width, player_walk2.height, player_walk2.pixel_data, Player.pos.direction);

        break;
    case PLAYER_FRAME_WALK_3:
        drawSprite(Player.pos.pX - cameraX, Player.pos.pY - cameraY, player_walk3.width, player_walk3.height, player_walk3.pixel_data, Player.pos.direction);
        break;
    case PLAYER_FRAME_WALK_4:
        drawSprite(Player.pos.pX - cameraX, Player.pos.pY - cameraY, player_walk4.width, player_walk4.height, player_walk4.pixel_data, Player.pos.direction);
        break;
    case PLAYER_FRAME_WALK_5:
        drawSprite(Player.pos.pX - cameraX, Player.pos.pY - cameraY, player_walk5.width, player_walk5.height, player_walk5.pixel_data, Player.pos.direction);
        break;
    case PLAYER_FRAME_WALK_6:
        drawSprite(Player.pos.pX - cameraX, Player.pos.pY - cameraY, player_walk6.width, player_walk6.height, player_walk6.pixel_data, Player.pos.direction);
        break;
    }
}

void anim_player_jump()
{
    if (Player.current_framerate == PLAYER_FRAMERATE_JUMP)
    {
        Player.anim_frame++;
        Player.current_framerate = 0;
    }
    Player.current_framerate++;

    if (Player.anim_frame > PLAYER_FRAME_JUMP_5)
    {
        Player.anim_frame = PLAYER_FRAME_JUMP_5; //On reste sur 5
    }

    switch (Player.anim_frame)
    {
    case PLAYER_FRAME_JUMP_1:
        drawSprite(Player.pos.pX - cameraX, Player.pos.pY - cameraY, player_jump1.width, player_jump1.height, player_jump1.pixel_data, Player.pos.direction);
        break;
    case PLAYER_FRAME_JUMP_2:
        drawSprite(Player.pos.pX - cameraX, Player.pos.pY - cameraY, player_jump2.width, player_jump2.height, player_jump2.pixel_data, Player.pos.direction);
        break;
    case PLAYER_FRAME_JUMP_3:
        drawSprite(Player.pos.pX - cameraX, Player.pos.pY - cameraY, player_jump3.width, player_jump3.height, player_jump3.pixel_data, Player.pos.direction);
        break;
    case PLAYER_FRAME_JUMP_4:
        drawSprite(Player.pos.pX - cameraX, Player.pos.pY - cameraY, player_jump4.width, player_jump4.height, player_jump4.pixel_data, Player.pos.direction);
        break;
    case PLAYER_FRAME_JUMP_5:
        drawSprite(Player.pos.pX - cameraX, Player.pos.pY - cameraY, player_jump5.width, player_jump5.height, player_jump5.pixel_data, Player.pos.direction);
        break;
    }
}

void anim_player_falling()
{

    if (Player.current_framerate == PLAYER_FRAMERATE_FALLING)
    {
        Player.anim_frame++;
        Player.current_framerate = 0;
    }
    Player.current_framerate++;

    if (Player.anim_frame > PLAYER_FRAME_FALLING_1)
    {
        Player.anim_frame = PLAYER_FRAME_FALLING_1; //On reste sur 1
    }

    switch (Player.anim_frame)
    {
    case PLAYER_FRAME_FALLING_1:
        drawSprite(Player.pos.pX - cameraX, Player.pos.pY - cameraY, player_falling1.width, player_falling1.height, player_falling1.pixel_data, Player.pos.direction);
        break;
    }
}

void anim_player_mining()
{
}

void anim_player_dying()
{

    if (Player.current_framerate == PLAYER_FRAMERATE_DIE)
    {
        Player.anim_frame++;
        Player.current_framerate = 0;
    }
    Player.current_framerate++;

    if (Player.anim_frame > PLAYER_FRAME_DIE_1)
    {
        Player.anim_frame = PLAYER_FRAME_DIE_1;
    }
    /*
  switch (Player.anim_frame)
  {
  case PLAYER_FRAME_DIE_1:
    //    drawSprite(Player.pos.pX - cameraX, Player.pos.pY - cameraY, player_width, player_height, player_die_bits, Player.pos.direction);
    drawSprite(Player.pos.pX - cameraX, Player.pos.pY - cameraY, player_die.width, player_die.height, player_die.pixel_data, Player.pos.direction);
    break;
  }*/
}

void anim_player_wining()
{

    if (Player.current_framerate == PLAYER_FRAMERATE_WIN)
    {
        Player.anim_frame++;
        Player.current_framerate = 0;
        if (random(5) == 0)
            parts.createExplosion(random(10, SCREEN_WIDTH - 10), 10 + (rand() % 2 * SCREEN_HEIGHT / 3), 100 + rand() % 50, random(65535));
    }
    Player.current_framerate++;

    if (Player.anim_frame > PLAYER_FRAME_WIN_3)
    {
        Player.anim_frame = PLAYER_FRAME_WIN_1;
    }
    /*
  switch (Player.anim_frame)
  {
  case PLAYER_FRAME_WIN_1:
    //    drawSprite(Player.pos.pX - cameraX, Player.pos.pY - cameraY, player_width, player_height, player4, Player.pos.direction);
      drawSprite(Player.pos.pX - cameraX, Player.pos.pY - cameraY, player_idle1.width, player_idle1.height, player_idle1.pixel_data, Player.pos.direction);
    break;
  case PLAYER_FRAME_WIN_2:
    //drawSprite(Player.pos.pX - cameraX, Player.pos.pY - cameraY, player_width, player_height, player2, Player.pos.direction);
      drawSprite(Player.pos.pX - cameraX, Player.pos.pY - cameraY, player_run1.width, player_run1.height, player_run1.pixel_data, Player.pos.direction);

    break;
  case PLAYER_FRAME_WIN_3:
    //    drawSprite(Player.pos.pX - cameraX, Player.pos.pY - cameraY, player_width, player_height, player3, Player.pos.direction);
      drawSprite(Player.pos.pX - cameraX, Player.pos.pY - cameraY, player_run2.width, player_run2.height, player_run2.pixel_data, Player.pos.direction);
    break;
  }*/
}

void anim_player_fx_double_jump()
{
    if (Player.FX.current_framerate == FX_FRAMERATE_DOUBLE_JUMP)
    {
        Player.FX.anim_frame++;
        Player.FX.current_framerate = 0;
    }
    Player.FX.current_framerate++;

    if (Player.FX.anim_frame > FX_FRAME_DOUBLE_JUMP_5)
    {
        //Annulation de l'anim
        Player.FX.stateAnim = FX_NONE;
        return;
    }

    switch (Player.FX.anim_frame)
    {
    case FX_FRAME_DOUBLE_JUMP_1:
        drawSprite(Player.FX.pX - cameraX, Player.FX.pY - cameraY, fx_jump1.width, fx_jump1.height, fx_jump1.pixel_data, Player.FX.direction);
        break;
    case FX_FRAME_DOUBLE_JUMP_2:
        drawSprite(Player.FX.pX - cameraX, Player.FX.pY - cameraY, fx_jump2.width, fx_jump2.height, fx_jump2.pixel_data, Player.FX.direction);
        break;
    case FX_FRAME_DOUBLE_JUMP_3:
        drawSprite(Player.FX.pX - cameraX, Player.FX.pY - cameraY, fx_jump3.width, fx_jump3.height, fx_jump3.pixel_data, Player.FX.direction);
        break;
    case FX_FRAME_DOUBLE_JUMP_4:
        drawSprite(Player.FX.pX - cameraX, Player.FX.pY - cameraY, fx_jump4.width, fx_jump4.height, fx_jump4.pixel_data, Player.FX.direction);
        break;
    case FX_FRAME_DOUBLE_JUMP_5:
        drawSprite(Player.FX.pX - cameraX, Player.FX.pY - cameraY, fx_jump5.width, fx_jump5.height, fx_jump5.pixel_data, Player.FX.direction);
        break;
    }
}

void displayGame()
{
    // digital clock display of the time
    canvas->fillScreen(0x867D); //84ceef

    drawWorld();

    //arcada.display->display();
    arcada.blitFrameBuffer(0, 0, false, false);
}
