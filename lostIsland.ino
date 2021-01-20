#include <Arduino.h>
#include <Adafruit_Arcada.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SPIFlash.h>
#include <Adafruit_ZeroTimer.h>

#include "gfx_utils.h"
#include "Particle.h"
#include "MySoundManager.h"
#include "pmf_player.h"

#include "SimplexNoise.h"
#include "WaterSim.h"
#include <FastLED.h>

#include "lostIsland.h"

static const uint32_t PROGMEM pmf_ninja[] =
    {
#include "musics/ninja.h"
};

static const uint32_t PROGMEM pmf_aceman[] =
    {
#include "musics/aceman.h"
};

#include "ressources.h"

#if !defined(USE_TINYUSB)
#error("Please select TinyUSB for the USB stack!")
#endif

#ifndef _swap_int16_t
#define _swap_int16_t(a, b) \
    {                       \
        int16_t t = a;      \
        a = b;              \
        b = t;              \
    }
#endif


    const float ledgeClimbXOffset1 = -2.0f;
    const float ledgeClimbYOffset1 = 0.0f;
    const float ledgeClimbXOffset2 = -5.0f;
    const float ledgeClimbYOffset2 = 0.0f;

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
    __now = millis();
    uint8_t remaining;

    // post render
    if (post_render)
    {
        lastFrameDurationMs = __now - lastFrameStart;
        frameCount++;
        post_render = false;
    }

    // if it's not time for the next frame yet
    if (__now < nextFrameStart)
    {
        remaining = nextFrameStart - __now;
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
    nextFrameStart = __now + eachFrameMillis;
    lastFrameStart = __now;
    post_render = true;
    return post_render;
}

inline TworldTile getWorldAtPix(int16_t px, int16_t py)
{
    TworldTile res = {0};

    int16_t x = px / 16;
    int16_t y = py / 16;

    if ((x >= 0 && x < WORLD_WIDTH) && (y >= 0 && y < WORLD_HEIGHT))
        res = WORLD[y][x];

    //Serial.printf("pix:%d,%d  val:%d\n", x, y, res);

    return res;
}

//On sauvegarde la hauteur courante dans l'entete de notre colonne de WORLD
void updateHauteurColonne(int16_t x, int16_t y)
{
    //on  cherche la nouvelle hauteur
    int16_t newH = 0;
    for (int16_t wY = 0; wY < WORLD_HEIGHT; wY++)
    {
        if (!WORLD[wY][x].traversable)
        {
            newH = wY;
            break;
        }
    }
    WORLD[HEADER_ROW][x].id = newH;
}

inline TworldTile getWorldAt(int16_t x, int16_t y)
{
    TworldTile res = {0};

    if ((x >= 0 && x < WORLD_WIDTH) && (y >= 0 && y < WORLD_HEIGHT))
        res = WORLD[y][x];

    return res;
}

inline void setWorldAt(int16_t x, int16_t y, TworldTile val)
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
        int16_t newX = (ARCADA_TFT_WIDTH - w) / 2;
        canvas->setCursor(newX, y);
    }
    else
        canvas->setCursor(x, y);

    canvas->print(text);
}

void setup()
{
    //while (!Serial);
    fElapsedTime = 0.0f;

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

    setup_timer3(100);

    StopMOD();
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

    Player.pos.worldX = BASE_X_PLAYER;
    Player.pos.worldY = (WORLD[HEADER_ROW][BASE_X_PLAYER].id) - 1;
    setPlayerPos(Player.pos.worldX * 16, Player.pos.worldY * 16);
    Player.pos.speedX = 0;
    Player.pos.speedY = 0;

    Player.pos.direction = DIRECTION_RIGHT;
    Player.pos.XFront = 0;
    Player.pos.YDown = 0;
    Player.pos.YUp = 0;

    Player.anim_frame = 0;
    Player.current_framerate = 0;
    Player.stateAnim = PLAYER_STATE_IDLE;

    currentTileTarget = {0};
    /*
  unsigned char state;  
  unsigned char action, action_cooldown, action_perf;
  unsigned char depth;
  unsigned char health;
  unsigned char level;
  unsigned char stars_kills;
  unsigned short armour_weapon;
*/
    Player.bWantJump = false;
    Player.bWantDoubleJump = false;
    Player.bWantWalk = false;

    Player.bCanMove = true;
    Player.bCanFlip = true;
    Player.bHasWallJumped = false;
    Player.lastWallJumpDirection = 0;
    Player.wantedHorizontalDirection = 0;
    Player.wantedVerticalDirection = 0;

    Player.bDying = false;
    Player.bJumping = false;
    Player.jumpTimer = 0;
    Player.wallJumpTimer = 0;
    Player.bDoubleJumping = false;
    Player.bWallJumping = false;
    Player.bWallSliding = false;
    Player.bWallClimbing = false;
    Player.bFalling = false;
    Player.bWalking = false;
    Player.bTouched = false;
    Player.fTouchCountDown = 0;
    Player.bOnGround = false;
    Player.bLanding = false;
    Player.onGroundTimer = 0;
    Player.bTouchingWall = false;
    Player.bTouchingLedge = false;
    Player.bLedgeDetected = false;
    Player.bClimbingLedge = false;
    Player.bUnderWater = false;
    Player.bSplashIn = false;
    Player.bSplashOut = false;

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

    timerActionB = 0;
    coolDownActionB = 0;

    jumpPhase = 0;

    count_player_die = 0;
    count_player_win = 0;

    initWorld();
    postInitWorld();
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

void set_double_jumping()
{
    if (Player.stateAnim != PLAYER_STATE_DOUBLE_JUMP)
    {
        Player.stateAnim = PLAYER_STATE_DOUBLE_JUMP;

        Player.anim_frame = PLAYER_FRAME_SALTO_1;
        Player.current_framerate = 0;
    }
}

void set_wall_jumping()
{
    if (Player.stateAnim != PLAYER_STATE_WALL_JUMP)
    {
        Player.stateAnim = PLAYER_STATE_WALL_JUMP;

        Player.anim_frame = PLAYER_FRAME_WALL_JUMP_1;
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

void set_sliding()
{
    if (Player.stateAnim != PLAYER_STATE_WALL_SLIDING)
    {
        Player.stateAnim = PLAYER_STATE_WALL_SLIDING;

        Player.anim_frame = PLAYER_FRAME_SLIDING_1;
        Player.current_framerate = 0;
    }
}

void set_wall_climbing()
{
    if (Player.stateAnim != PLAYER_STATE_WALL_CLIMBING)
    {
        Player.stateAnim = PLAYER_STATE_WALL_CLIMBING;

        Player.anim_frame = PLAYER_FRAME_WALL_CLIMBING_1;
        Player.current_framerate = 0;
    }
}

void set_walking()
{
    if (Player.stateAnim != PLAYER_STATE_RUN)
    {
        Player.stateAnim = PLAYER_STATE_RUN;

        Player.anim_frame = PLAYER_FRAME_RUN_1;
        Player.current_framerate = 0;
    }
}


void set_ledgeclimbing()
{
    if (Player.stateAnim != PLAYER_STATE_LEDGE_CLIMB)
    {
        Player.stateAnim = PLAYER_STATE_LEDGE_CLIMB;

        Player.anim_frame = PLAYER_FRAME_LEDGE_CLIMB_1;
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
        Player.fTouchCountDown = 0;
        Player.bTouched = false;
        jumpPhase = 0;
        count_player_die = 0;
        Player.anim_frame = PLAYER_FRAME_DIE_1;
        Player.current_framerate = 0;
    }
}

void set_double_jump_fx(bool bForced = false)
{
    if (Player.FX.stateAnim != FX_DOUBLE_JUMP || bForced)
    {
        Player.FX.stateAnim = FX_DOUBLE_JUMP;
        Player.FX.anim_frame = 0;
        //on le met sur le framerate final pour quil apparaissent de suite
        Player.FX.current_framerate = FX_FRAMERATE_DOUBLE_JUMP;
        Player.FX.direction = Player.pos.direction;
        Player.FX.speedX = 1;
        Player.FX.speedY = 1;
        Player.FX.pX = Player.pos.pX;
        Player.FX.pY = Player.pos.pY;
        Player.FX.waterLevel = Player.waterLevel > 0;
    }
}

void set_dust_fx(void)
{
    if (Player.FX.stateAnim != FX_DUST)
    {
        Player.FX.stateAnim = FX_DUST;
        Player.FX.anim_frame = 0;
        Player.FX.current_framerate = FX_FRAMERATE_DUST;
        Player.FX.direction = Player.pos.direction;
        Player.FX.speedX = 1;
        Player.FX.speedY = 1;
        Player.FX.pX = Player.pos.pX;
        Player.FX.pY = Player.pos.pY;
        Player.FX.waterLevel = Player.waterLevel;
    }
}

void set_splash_fx(bool bSplashIn = true)
{
    if (Player.FX.stateAnim != FX_SPLASH)
    {
        Player.FX.stateAnim = FX_SPLASH;
        Player.FX.anim_frame = 0;
        Player.FX.current_framerate = FX_FRAMERATE_SPLASH;
        Player.FX.direction = Player.pos.direction;
        Player.FX.speedX = 1;
        Player.FX.speedY = 1;
        Player.FX.pX = Player.pos.pX;
        Player.FX.pY = Player.pos.pY;
        Player.FX.waterLevel = Player.waterLevel;
    }
}
void set_touched()
{
    // @todo : gerer les type de monstres
    if (Player.fTouchCountDown == 0)
    {
        Player.health -= 10;

        if (Player.health > 0)
        {
            //    sndPlayerCanal3.play(AudioSampleSmb_pipe);
            Player.bTouched = true;
            Player.fTouchCountDown = 2;
            //Player.anim_frame = PLAYER_FRAME_TOUCH_1;
            //Player.current_framerate = 0;
        }
        else if (Player.fTouchCountDown == 0)
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
        Player.fTouchCountDown = 0;
        Player.bTouched = false;
        bWin = true;
        jumpPhase = 0;
        count_player_win = 0;
        Player.anim_frame = PLAYER_FRAME_WIN_1;
        Player.current_framerate = 0;
    }
}

void postInitWorld()
{
    //Update necessaires au jeu apres init ou load
    //....recalcul hauteur, etc..
    int16_t wY = WORLD_HEIGHT - 1;
    hauteurMaxBackground = 0;
    do
    {
        for (int16_t wX = 0; wX < WORLD_WIDTH; wX++)
        {
            if (WORLD[wY][wX].id == BLOCK_AIR)
            {
                hauteurMaxBackground = wY + 1;
                break;
            }
        }
        wY--;
    } while (wY > 0 && hauteurMaxBackground == 0);
    Serial.printf("hauteurMaxBackground:%d\n", hauteurMaxBackground);

    //Reset times
    lastTime = __now = millis();

    //Musique
    StopMOD();
    PlayMOD(pmf_ninja);
}

void initWorld()
{
    //MASQUE LUMIERE PLAYER
    int16_t centerX = PLAYER_LIGHT_MASK_WIDTH / 2;
    int16_t centerY = PLAYER_LIGHT_MASK_HEIGHT / 2;

    for (int16_t wY = 0; wY < PLAYER_LIGHT_MASK_HEIGHT; wY++)
    {
        for (int16_t wX = 0; wX < PLAYER_LIGHT_MASK_WIDTH; wX++)
        {
            //Ray cast du player vers decor ?
            float distPlayer = sqrt((centerX - wX) * (centerX - wX) + (centerY - wY) * (centerY - wY));
            //Serial.printf("player:%d,%d tile:%d,%d dist:%f\n", playerLightX, playerLightY, px, py,  distPlayer);
            // @todo gerer la non propagassion de la lumiere dans les murs...
            if (distPlayer > 0)
                playerLightMask[wY][wX] = (PLAYER_LIGHT_INTENSITY / (distPlayer * distPlayer));
            else
                playerLightMask[wY][wX] = PLAYER_LIGHT_INTENSITY;
        }
    }

    //uint8_t zeNoise[WORLD_WIDTH];
    memset(WORLD, 0, sizeof(WORLD));
    SimplexNoise noiseGen;

    int16_t minProfondeur = 999;
    int16_t maxProfondeur = 0;
    for (int16_t wX = 0; wX < WORLD_WIDTH; wX++)
    {
        //        float noise = SimplexNoise::noise(((float)16 * wX) / (float)WORLD_WIDTH);
        float noise = noiseGen.fractal(4, ((float)16 * wX) / (float)WORLD_WIDTH);

        uint8_t rowGround = (WORLD_HEIGHT - 1) - (int(noise * AMPLITUDE_HAUTEUR) + MEDIUM_HAUTEUR);

        minProfondeur = min(minProfondeur, rowGround);
        maxProfondeur = max(maxProfondeur, rowGround);

        //On sauvegarde la hauteur sol dans la derniere ligne du world (invisible)
        WORLD[HEADER_ROW][wX].id = rowGround;
        //On sauve la hauteur originelle, en dessous on est dans le sol (meme si creusé)
        WORLD[REF_ROW][wX].id = rowGround;

        for (int16_t wY = 0; wY < WORLD_HEIGHT; wY++)
        {
            //Le ciel
            if (wY < rowGround)
            {
                WORLD[wY][wX].id = BLOCK_AIR;
                WORLD[wY][wX].traversable = 1;
                WORLD[wY][wX].opaque = 0;
                WORLD[wY][wX].life = BLOCK_LIFE_NA;
            }
            else if (wY == rowGround)
            {
                WORLD[wY][wX].id = BLOCK_GROUND_TOP;
                WORLD[wY][wX].life = BLOCK_LIFE_1;
                WORLD[wY][wX].traversable = 0;
                WORLD[wY][wX].opaque = 1;
            }
            else if (wY <= rowGround + 3) //entre surface et sous sol profond
            {
                WORLD[wY][wX].id = BLOCK_GROUND;
                WORLD[wY][wX].life = BLOCK_LIFE_1;
                WORLD[wY][wX].traversable = 0;
                WORLD[wY][wX].opaque = 1;
            }
            else //sous sol profond
            {
                WORLD[wY][wX].id = BLOCK_ROCK;
                WORLD[wY][wX].life = BLOCK_LIFE_1;
                WORLD[wY][wX].traversable = 0;
                WORLD[wY][wX].opaque = 1;
            }
        }
    }
    float minN = 999999;
    float maxN = -99999;
    //Update bricks to cool rendering
    for (int16_t wX = 0; wX < WORLD_WIDTH; wX++)
    {
        int16_t curProdondeur = WORLD[HEADER_ROW][wX].id;
        curProdondeur = max(4, curProdondeur);

        for (int16_t wY = curProdondeur - 4; wY < (WORLD_HEIGHT - 1); wY++)
        {
            //            float noise = noiseGen.fractal(8, (float)16*wX / (float)WORLD_WIDTH, (float)16*wY / (float)WORLD_HEIGHT);
            float noise = SimplexNoise::noise((float)16 * wX / (float)WORLD_WIDTH, (float)16 * wY / (float)WORLD_HEIGHT);
            int16_t densite = int(noise * AMPLITUDE_DENSITE);
            if (wY <= curProdondeur + 3)
            {
                if (densite > MAX_DENSITE)
                {
                    WORLD[wY][wX].id = BLOCK_AIR;
                    WORLD[wY][wX].traversable = 1;
                    WORLD[wY][wX].opaque = 0;
                    WORLD[wY][wX].life = BLOCK_LIFE_NA;
                }
            }
            else
            {
                if (abs(densite) > MAX_DENSITE || densite == 0)
                {
                    if (WORLD[wY][wX].id != BLOCK_AIR)
                    {
                        WORLD[wY][wX].id = BLOCK_UNDERGROUND_AIR;
                        WORLD[wY][wX].traversable = 1;
                        WORLD[wY][wX].opaque = 1;
                        WORLD[wY][wX].life = BLOCK_LIFE_NA;
                    }
                }
                else
                {
                    WORLD[wY][wX].id = BLOCK_ROCK;
                    WORLD[wY][wX].life = BLOCK_ROCK_DENSITY;
                    WORLD[wY][wX].traversable = 0;
                    WORLD[wY][wX].opaque = 1;
                    if (wY >= curProdondeur + 5)
                    {
                        int randSeed = rand() % 100;
                        if (wY <= curProdondeur + 12)
                        {
                            if (randSeed < 2)
                            {
                                WORLD[wY][wX].id = BLOCK_DIAMANT;
                                WORLD[wY][wX].life = BLOCK_DIAMANT_DENSITY;
                                continue;
                            }

                            if (randSeed <= 20)
                            {
                                WORLD[wY][wX].id = BLOCK_REDSTONE;
                                WORLD[wY][wX].life = BLOCK_REDSTONE_DENSITY;
                                continue;
                            }
                        }
                        else if (wY <= curProdondeur + 29)
                        {
                            if (randSeed < 2)
                            {
                                WORLD[wY][wX].id = BLOCK_JADE;
                                WORLD[wY][wX].life = BLOCK_JADE_DENSITY;
                                continue;
                            }
                            else if (randSeed <= 4)
                            {
                                WORLD[wY][wX].id = BLOCK_OR;
                                WORLD[wY][wX].life = BLOCK_OR_DENSITY;
                                continue;
                            }
                            else if (randSeed <= 14)
                            {
                                WORLD[wY][wX].id = BLOCK_CUIVRE;
                                WORLD[wY][wX].life = BLOCK_CUIVRE_DENSITY;
                                continue;
                            }
                        }
                        else if (wY <= curProdondeur + 54)
                        {
                            if (randSeed < 20)
                            {
                                WORLD[wY][wX].id = BLOCK_FER;
                                WORLD[wY][wX].life = BLOCK_FER_DENSITY;
                                continue;
                            }
                            else if (randSeed < 50)
                            {
                                WORLD[wY][wX].id = BLOCK_CHARBON;
                                WORLD[wY][wX].life = BLOCK_CHARBON_DENSITY;
                                continue;
                            }
                        }
                    }
                }
            }

            if (noise < minN)
                minN = noise;
            if (noise > maxN)
                maxN = noise;
        }
    }
    for (int wX = 0; wX < WORLD_WIDTH; wX++)
    {
        for (int wY = 0; wY < (WORLD_HEIGHT - 1); wY++)
        {
            TworldTile brick = WORLD[wY][wX];
            if (!WORLD[wY][wX].traversable)
            {
                //Sauvegarde de la hauteur courante et de la hauteur originelle
                WORLD[HEADER_ROW][wX].id = wY;
                WORLD[REF_ROW][wX].id = wY;
#if 0
                WORLD[wY][wX].id = BLOCK_GROUND_TOP;
                WORLD[wY][wX].traversable = 0;
                WORLD[wY][wX].life = BLOCK_LIFE_1;
                WORLD[wY][wX].opaque = 1;

                //Si possible on change la tile juste en dessous pour du ground
                if (wY + 1 < (WORLD_HEIGHT - 1))
                {
                    WORLD[wY + 1][wX].id = BLOCK_GROUND;
                    WORLD[wY + 1][wX].traversable = 0;
                    WORLD[wY + 1][wX].life = BLOCK_LIFE_1;
                    WORLD[wY + 1][wX].opaque = 1;
                }
                //Et celle du dessus par de l'herbe
                if (wY - 1 >= 0)
                {
                    //12 pourcent de chance de mettre de l'herbe
                    if (rand() % 100 < 12)
                    {
                        WORLD[wY - 1][wX].id = BLOCK_GRASS;
                        WORLD[wY - 1][wX].life = BLOCK_LIFE_NA;
                        WORLD[wY - 1][wX].traversable = 1;
                        WORLD[wY - 1][wX].opaque = 1;
                        WORLD[wY - 1][wX].spriteVariation = rand() % 3;
                    }
                }
#endif
                break;
            }
        }
    }

    for (int wY = 0; wY < WORLD_HEIGHT; wY++)
        ErosionUpdate();

    //TEST WATER
    for (int nbW = 0; nbW < WORLD_WIDTH - 4; nbW++)
    {
        int curP = WORLD[REF_ROW][4 + nbW].id;
        WORLD[(curP - 1)][4 + nbW].Level = MAX_WATER_LEVEL;
    }

    //On update en boucle de force pour stabiliser l'eau
    for (int nbWaterUp = 0; nbWaterUp < 500; nbWaterUp++)
        WATER_Update();

    //VEGETATION
    int nbTries = 0;
    for (int nbTrees = 0; nbTrees < MAX_TREES; nbTries < 50)
    {
        int hauteur;
        int posX;
        TworldTile tile = {0};
        TworldTile tileGG = {0};
        TworldTile tileDD = {0};
        TworldTile tile1 = {0};
        TworldTile tile2 = {0};
        TworldTile tileG = {0};
        TworldTile tileG1 = {0};
        TworldTile tileG2 = {0};
        TworldTile tileD = {0};
        TworldTile tileD1 = {0};
        TworldTile tileD2 = {0};
        posX = random(3, WORLD_WIDTH - 3);
        bool bFound = true;
        int nbSearch = 0;
        do
        {
            hauteur = WORLD[HEADER_ROW][posX].id;
            if (hauteur > 2)
            {
                tile = getWorldAt(posX, hauteur - 1);
                tile1 = getWorldAt(posX, hauteur - 2);
                tile2 = getWorldAt(posX, hauteur - 3);
                tileG = getWorldAt(posX - 1, hauteur - 1);
                tileG1 = getWorldAt(posX - 1, hauteur - 2);
                tileG2 = getWorldAt(posX - 1, hauteur - 3);
                tileD = getWorldAt(posX + 1, hauteur - 1);
                tileD1 = getWorldAt(posX + 1, hauteur - 2);
                tileD2 = getWorldAt(posX + 1, hauteur - 3);

                tileGG = getWorldAt(posX - 2, hauteur - 1);
                tileDD = getWorldAt(posX + 2, hauteur - 1);
            }
            nbSearch++;

        } while (!tile.traversable && !tileG.traversable && !tileD.traversable &&
                 !tile1.traversable && !tileG1.traversable && !tileD1.traversable &&
                 !tile2.traversable && !tileG2.traversable && !tileD2.traversable &&
                 !tile.Level == 0 && !tileG.Level == 0 && !tileD.Level == 0 &&
                 !tileGG.traversable && !tileDD.traversable && nbSearch < 50);

        if (bFound)
        {
            WORLD[hauteur - 1][posX].id = BLOCK_TREE;
            WORLD[hauteur - 1][posX].life = BLOCK_LIFE_1;
            WORLD[hauteur - 1][posX].traversable = 1;
            WORLD[hauteur - 1][posX].opaque = 1;
            WORLD[hauteur - 1][posX].spriteVariation = rand() % 3;
            nbTrees++;
        }
        else
        {
            //Echec, on retente 50 fois pas plus
            nbTries++;
        }
    }

    NB_WORLD_ENNEMIES = 0;
    //Skels
    for (int iEnnemy = 0; iEnnemy < MAX_SKELS; iEnnemy++)
    {
        int hauteur;
        int posX;
        TworldTile tile = {0};
        TworldTile tileG = {0};
        TworldTile tileD = {0};
        do
        {
            posX = random(BASE_X_PLAYER + 5, WORLD_WIDTH - 2);
            hauteur = WORLD[HEADER_ROW][posX].id;
            tile = getWorldAt(posX, hauteur - 1);
            tileG = getWorldAt(posX - 1, hauteur - 1);
            tileD = getWorldAt(posX + 1, hauteur - 1);
            // @todo tester que le zombi est pas deja sur un autre ennemi (un marqueur sur la tile ? )
        } while (!tile.traversable && !tileG.traversable && !tileD.traversable &&
                 !tile.Level && !tileG.Level && !tileD.Level);

        //Skel
        ENNEMIES[NB_WORLD_ENNEMIES].bIsAlive = 255;
        ENNEMIES[NB_WORLD_ENNEMIES].worldX = posX;
        ENNEMIES[NB_WORLD_ENNEMIES].worldY = hauteur - 1;
        ENNEMIES[NB_WORLD_ENNEMIES].pX = ENNEMIES[NB_WORLD_ENNEMIES].worldX * 16;
        ENNEMIES[NB_WORLD_ENNEMIES].pY = ENNEMIES[NB_WORLD_ENNEMIES].worldY * 16;
        ENNEMIES[NB_WORLD_ENNEMIES].newX = 0;
        ENNEMIES[NB_WORLD_ENNEMIES].newY = 0;
        ENNEMIES[NB_WORLD_ENNEMIES].type = SKEL_ENNEMY;
        ENNEMIES[NB_WORLD_ENNEMIES].speedX = SKEL_WALKING_SPEED;
        ENNEMIES[NB_WORLD_ENNEMIES].speedY = 0;
        ENNEMIES[NB_WORLD_ENNEMIES].anim_frame = 0;
        ENNEMIES[NB_WORLD_ENNEMIES].max_frames = 5;
        ENNEMIES[NB_WORLD_ENNEMIES].nb_anim_frames = SKEL_WALK_FRAMES;

        ENNEMIES[NB_WORLD_ENNEMIES].bFalling = false;
        ENNEMIES[NB_WORLD_ENNEMIES].bJumping = false;
        ENNEMIES[NB_WORLD_ENNEMIES].bOnGround = false;

        ENNEMIES[NB_WORLD_ENNEMIES].max_health = ZOMBI_HEALTH;
        ENNEMIES[NB_WORLD_ENNEMIES].health = ZOMBI_HEALTH;

        NB_WORLD_SKELS++;
        NB_WORLD_ENNEMIES++;
    }
    for (int iEnnemy = 0; iEnnemy < MAX_ZOMBIES; iEnnemy++)
    {
        int hauteur;
        int posX;
        TworldTile tile = {0};
        TworldTile tileG = {0};
        TworldTile tileD = {0};

        do
        {
            posX = random(BASE_X_PLAYER + 5, WORLD_WIDTH - 2);
            hauteur = WORLD[HEADER_ROW][posX].id;
            tile = getWorldAt(posX, hauteur - 1);
            tileG = getWorldAt(posX - 1, hauteur - 1);
            tileD = getWorldAt(posX + 1, hauteur - 1);
            // @todo tester que le zombi est pas deja sur un autre ennemi (un marqueur sur la tile ? )
        } while (!tile.traversable && !tileG.traversable && !tileD.traversable &&
                 !tile.Level && !tileG.Level && !tileD.Level);

        ENNEMIES[NB_WORLD_ENNEMIES].bIsAlive = 255;
        ENNEMIES[NB_WORLD_ENNEMIES].worldX = posX;
        ENNEMIES[NB_WORLD_ENNEMIES].worldY = hauteur - 1;
        ENNEMIES[NB_WORLD_ENNEMIES].pX = ENNEMIES[NB_WORLD_ENNEMIES].worldX * 16;
        ENNEMIES[NB_WORLD_ENNEMIES].pY = ENNEMIES[NB_WORLD_ENNEMIES].worldY * 16;
        ENNEMIES[NB_WORLD_ENNEMIES].newX = 0;
        ENNEMIES[NB_WORLD_ENNEMIES].newY = 0;
        ENNEMIES[NB_WORLD_ENNEMIES].type = ZOMBI_ENNEMY;
        ENNEMIES[NB_WORLD_ENNEMIES].speedX = ZOMBI_WALKING_SPEED;
        ENNEMIES[NB_WORLD_ENNEMIES].speedY = 0;
        ENNEMIES[NB_WORLD_ENNEMIES].anim_frame = 0;
        ENNEMIES[NB_WORLD_ENNEMIES].max_frames = 5;
        ENNEMIES[NB_WORLD_ENNEMIES].nb_anim_frames = ZOMBI_WALK_FRAMES;

        ENNEMIES[NB_WORLD_ENNEMIES].bFalling = false;
        ENNEMIES[NB_WORLD_ENNEMIES].bJumping = false;
        ENNEMIES[NB_WORLD_ENNEMIES].bOnGround = false;

        ENNEMIES[NB_WORLD_ENNEMIES].max_health = ZOMBI_HEALTH;
        ENNEMIES[NB_WORLD_ENNEMIES].health = ZOMBI_HEALTH;

        NB_WORLD_ENNEMIES++;
        NB_WORLD_ZOMBIES++;
    }

    //Spiders
    for (int wX = BASE_X_PLAYER + 5; wX < WORLD_WIDTH - 1; wX += 3)
    {
        int hauteur = WORLD[HEADER_ROW][wX].id;
        int seed = random(100);

        for (int wY = hauteur + 1; wY < WORLD_HEIGHT - 1; wY++)
        {
            if (NB_WORLD_ENNEMIES < MAX_ENNEMIES)
            {
                if (seed <= 60)
                {
                    if (WORLD[wY][wX].traversable && WORLD[wY][wX].Level == 0 && WORLD[wY][wX + 1].traversable && WORLD[wY][wX - 1].traversable && WORLD[wY - 1][wX].traversable)
                    {
                        //Spiders
                        ENNEMIES[NB_WORLD_ENNEMIES].bIsAlive = 255;
                        ENNEMIES[NB_WORLD_ENNEMIES].worldX = wX;
                        ENNEMIES[NB_WORLD_ENNEMIES].worldY = wY;
                        ENNEMIES[NB_WORLD_ENNEMIES].pX = ENNEMIES[NB_WORLD_ENNEMIES].worldX * 16;
                        ENNEMIES[NB_WORLD_ENNEMIES].pY = ENNEMIES[NB_WORLD_ENNEMIES].worldY * 16;
                        ENNEMIES[NB_WORLD_ENNEMIES].newX = 0;
                        ENNEMIES[NB_WORLD_ENNEMIES].newY = 0;
                        ENNEMIES[NB_WORLD_ENNEMIES].type = SPIDER_ENNEMY;
                        ENNEMIES[NB_WORLD_ENNEMIES].speedX = SPIDER_WALKING_SPEED;
                        ENNEMIES[NB_WORLD_ENNEMIES].speedY = 0;
                        ENNEMIES[NB_WORLD_ENNEMIES].anim_frame = 0;
                        ENNEMIES[NB_WORLD_ENNEMIES].max_frames = 5;
                        ENNEMIES[NB_WORLD_ENNEMIES].nb_anim_frames = SPIDER_WALK_FRAMES;

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

    Serial.printf("NB_WORLD_ENNEMIES:%d\n", NB_WORLD_ENNEMIES);

    Serial.printf("TILE SIZE:%d  WORLD SIZE:%d\n", sizeof(TworldTile), sizeof(WORLD));

    NB_WORLD_ITEMS = 0;
    CURRENT_QUEUE_ITEMS = 0;

    StopMOD();
    PlayMOD(pmf_ninja);
}

void drawPlayer()
{
    if (Player.fTouchCountDown > 0)
    {
        Player.fTouchCountDown -= fElapsedTime;

        //if (fmod(Player.fTouchCountDown, 2) == 0)
        //  return;
    }

    switch (Player.stateAnim)
    {
    case PLAYER_STATE_WALL_CLIMBING:
        anim_player_wall_climbin();
        break;
    case PLAYER_STATE_WALL_SLIDING:
        anim_player_wall_sliding();
        break;
    case PLAYER_STATE_RUN:
        anim_player_walk();
        break;
    case PLAYER_STATE_LEDGE_CLIMB:
        anim_player_ledge_climbing();
        break;
    case PLAYER_STATE_DOUBLE_JUMP:
        anim_player_double_jump();
        break;

    case PLAYER_STATE_WALL_JUMP:
        anim_player_wall_jumping();
        break;
    case PLAYER_STATE_JUMP:
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

    //En plus de l'anim prinipale il peut y avoir des anims supp (effets)
    switch (Player.FX.stateAnim)
    {
    case FX_NONE:

        break;
    case FX_DOUBLE_JUMP:
    {
        anim_player_fx_double_jump();
        break;
    }
    case FX_DUST:
    {
        anim_player_fx_dust();
        break;
    }
    case FX_SPLASH:
    {
        anim_player_fx_splash();
        break;
    }
    }
}

void createDropFrom(int16_t wX, int16_t wY, uint8_t type)
{
    ITEMS[CURRENT_QUEUE_ITEMS].bIsAlive = 255;
    ITEMS[CURRENT_QUEUE_ITEMS].bIsActive = false;
    ITEMS[CURRENT_QUEUE_ITEMS].iSpawning = (FPS >> 2);

    ITEMS[CURRENT_QUEUE_ITEMS].worldX = wX;
    ITEMS[CURRENT_QUEUE_ITEMS].worldY = wY;
    ITEMS[CURRENT_QUEUE_ITEMS].pX = wX * 16;
    ITEMS[CURRENT_QUEUE_ITEMS].pY = wY * 16;
    ITEMS[CURRENT_QUEUE_ITEMS].type = type;
    ITEMS[CURRENT_QUEUE_ITEMS].speedX = 0;
    ITEMS[CURRENT_QUEUE_ITEMS].speedY = 0;
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

    //parts.createExplosion(px, py, 16);
}

void drawItem(Titem *currentItem)
{
    if (currentItem->current_framerate == currentItem->max_frames)
    {
        currentItem->anim_frame++;
        currentItem->current_framerate = 0;
    }
    currentItem->current_framerate++;

    if (currentItem->anim_frame >= currentItem->nb_anim_frames)
    {
        currentItem->anim_frame = 0;
    }
    if (currentItem->bIsAlive < 127)
    {
        //Il va disparaitre apres quelques frames, ca nous laisse le temps de faire un effet de disparition eventuel
        currentItem->bIsAlive -= 4;
    }

    int px = currentItem->pX - cameraX;
    int py = currentItem->pY - cameraY;

    /*
        /*    if (currentItem->iSpawning > 0)
    {
      int tmpH = 16 - (currentItem->iSpawning / 2);
      drawSprite(px, (currentItem->pY ) + (16 - tmpH), mushroom_16x16.width, tmpH, mushroom_16x16.pixel_data, Player.pos.direction);
    }
    else

    */
    //Recentrage
    //px -= 4;
    const int8_t lookUpItems[7] = {-2, -1, 0, 2, 2, 0, -1};
    py = py + lookUpItems[(briquesFRAMES / 4) % 7];
    //Serial.printf("Item:%d %d,%d\n", currentItem->type, px, py);
    if (px < ARCADA_TFT_WIDTH && px > -16 && py < ARCADA_TFT_HEIGHT && py > -16)
    {
        switch (currentItem->type)
        {
        case ITEM_TREE1:
            drawSprite(px, py, item_buche1.width, item_buche1.height, item_buche1.pixel_data, 1);
            break;
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
        case ITEM_DIAMANT:
            drawSprite(px, py, diamant_small.width, diamant_small.height, diamant_small.pixel_data, 1);
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
                //if (rayCastTo(Player.pos.worldX, Player.pos.worldY, currentItem->worldX, currentItem->worldY))
                    drawItem(currentItem);
            }
        }
    }
}

void killEnnemy(Tennemy *currentEnnemy, int px, int py)
{
    currentEnnemy->bIsAlive = 64;

    //px -= cameraX;
    //py -= cameraY;

    switch (currentEnnemy->type)
    {
    case SPIDER_ENNEMY:
        parts.createBodyExplosion(px, py, 16, ARCADA_RED, ARCADA_BLACK);
        break;
    case ZOMBI_ENNEMY:
        parts.createBodyExplosion(px, py, 16, ARCADA_DARKGREEN, ARCADA_BLACK);
        break;
    case SKEL_ENNEMY:
        parts.createBodyExplosion(px, py, 16, ARCADA_WHITE, ARCADA_BLACK);
        break;
    }
}

void drawEnnemy(Tennemy *currentEnnemy)
{

    if (currentEnnemy->current_framerate == currentEnnemy->max_frames)
    {
        currentEnnemy->anim_frame++;
        currentEnnemy->current_framerate = 0;
    }
    currentEnnemy->current_framerate++;

    if (currentEnnemy->anim_frame >= currentEnnemy->nb_anim_frames)
    {
        currentEnnemy->anim_frame = 0;
    }
    if (currentEnnemy->bIsAlive < 127)
    {
        //On fixe a la frames + 1 (dans le case du drraw on dessinera la frame "dying" )
       // currentEnnemy->anim_frame = currentEnnemy->nb_anim_frames + 1;
        currentEnnemy->bIsAlive -= 4;
    }

    int px = currentEnnemy->pX - cameraX;
    int py = currentEnnemy->pY - cameraY;
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
                    drawSprite(px, py, SPIDER_WIDTH, SPIDER_HEIGHT, spider_walk2.pixel_data, 1);
                    break;
                }
                break;
            }
            case ZOMBI_ENNEMY:
            {
                int DIR = currentEnnemy->speedX > 0 ? 1 : -1;
                switch (currentEnnemy->anim_frame)
                {
                case 1:
                    drawSprite(px, py, zombi_walk1.width, zombi_walk1.height, zombi_walk1.pixel_data, DIR);
                    break;
                case 2:
                    drawSprite(px, py, zombi_walk2.width, zombi_walk2.height, zombi_walk2.pixel_data, DIR);
                    break;
                case 3:
                    drawSprite(px, py, ZOMBI_WIDTH, ZOMBI_HEIGHT, zombi_walk3.pixel_data, DIR);
                    break;
                }
                break;
            }
            case SKEL_ENNEMY:
            {
                int DIR = currentEnnemy->speedX > 0 ? 1 : -1;
                drawSpriteSheet(px, py, SKEL_WIDTH, SKEL_HEIGHT, skeleton_sheet.pixel_data, currentEnnemy->anim_frame, DIR);
                break;
            }
        }
        if (currentEnnemy->bIsAlive >= 127)
        {
            int lgHealth = int(14 * currentEnnemy->health / currentEnnemy->max_health);
            canvas->drawFastHLine(px + 1, py - 4, lgHealth, lgHealth < 8 ? 0xC180 : 0xAE6A);
            canvas->drawFastHLine(px + 1, py - 3, lgHealth, lgHealth < 8 ? 0x8940 : 0x5C64);
            canvas->drawRect(px, py - 5, 16, 4, ARCADA_BLACK);
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
                //if (rayCastTo(Player.pos.worldX, Player.pos.worldY, currentEnnemy->worldX, currentEnnemy->worldY))
                    drawEnnemy(currentEnnemy);
            }
        }
    }
}

const unsigned char *getGroundBlockData(uint8_t value)
{
    const unsigned char *pixel_data = NULL;

    switch (value)
    {
    case BLOCK_GROUND:
        pixel_data = ground_00.pixel_data;
        break;
    case BLOCK_GROUND + 0x01:
        pixel_data = ground_01.pixel_data;
        break;
    case BLOCK_GROUND + 0x02:
        pixel_data = ground_02.pixel_data;
        break;
    case BLOCK_GROUND + 0x03:
        pixel_data = ground_03.pixel_data;
        break;
    case BLOCK_GROUND + 0x04:
        pixel_data = ground_04.pixel_data;
        break;
    case BLOCK_GROUND + 0x05:
        pixel_data = ground_05.pixel_data;
        break;
    case BLOCK_GROUND + 0x06:
        pixel_data = ground_06.pixel_data;
        break;
    case BLOCK_GROUND + 0x07:
        pixel_data = ground_07.pixel_data;
        break;
    case BLOCK_GROUND + 0x08:
        pixel_data = ground_08.pixel_data;
        break;
    case BLOCK_GROUND + 0x09:
        pixel_data = ground_09.pixel_data;
        break;
    case BLOCK_GROUND + 0x0A:
        pixel_data = ground_0A.pixel_data;
        break;
    case BLOCK_GROUND + 0x0B:
        pixel_data = ground_0B.pixel_data;
        break;
    case BLOCK_GROUND + 0x0C:
        pixel_data = ground_0C.pixel_data;
        break;
    case BLOCK_GROUND + 0x0D:
        pixel_data = ground_0D.pixel_data;
        break;
    case BLOCK_GROUND + 0x0E:
        pixel_data = ground_0E.pixel_data;
        break;
    case BLOCK_GROUND + 0x0F:
        pixel_data = ground_0F.pixel_data;
        break;
    }

    return (const unsigned char *)pixel_data;
}

void drawTiles()
{
    int playerLightStartX = Player.pos.worldX - (PLAYER_LIGHT_MASK_WIDTH / 2);
    int playerLightStartY = Player.pos.worldY - (PLAYER_LIGHT_MASK_HEIGHT / 2);
    int playerLightEndX = playerLightStartX + (PLAYER_LIGHT_MASK_WIDTH - 1);
    int playerLightEndY = playerLightStartY + (PLAYER_LIGHT_MASK_HEIGHT - 1);

#ifdef USE_FOV
    for (int wX = worldMIN_X; wX < worldMAX_X; wX++)
    {
        for (int wY = worldMIN_Y; wY < worldMAX_Y; wY++)
        {
            TworldTile *brick = &WORLD[wY][wX];
            brick->light_hit = 0;
        }
    }
    do_fov(Player.pos.worldX, Player.pos.worldY, 5);
#endif

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
                    TworldTile *brick = &WORLD[wY][wX];
                    uint8_t value = brick->id;

                    uint8_t light_hit = 0;

                    int curLight = MAX_LIGHT_INTENSITY;
                    // @todo gerer la nuit (dans ce cas pas de max light et ground.. ou la lune ?)
                    if (brick->opaque) // || wY > profondeurColonne)
                    {
                        if (profondeurColonne != 0)
                        {
                            int delta = (wY - profondeurColonne) + 1;
                            if (delta > 1)
                                curLight = curLight - (33 * delta);

                            curLight = max(curLight, 0);
                        }
#ifdef USE_FOV
                        if (WORLD[wY][wX].light_hit)
                        {
                            curLight = curLight + playerLightMask[wY - playerLightStartY][wX - playerLightStartX];
                            if (wX < Player.pos.worldX)
                                light_hit |= 4;
                            else if (wX > Player.pos.worldX)
                                light_hit |= 1;

                            if (wY < Player.pos.worldY)
                                light_hit |= 8;
                            else if (wX < Player.pos.worldX)
                                light_hit |= 2;
                        }
#else
                        if (wX >= playerLightStartX && wX <= playerLightEndX && wY >= playerLightStartY && wY <= playerLightEndY)
                        {
                            //Raycast
                            if (rayCastTo(Player.pos.worldX, Player.pos.worldY, wX, wY))
                            {
                                curLight = curLight + playerLightMask[wY - playerLightStartY][wX - playerLightStartX];
                                if (wX < Player.pos.worldX)
                                    light_hit |= 4;
                                else if (wX > Player.pos.worldX)
                                    light_hit |= 1;

                                if (wY < Player.pos.worldY)
                                    light_hit |= 8;
                                else if (wY > Player.pos.worldY)
                                    light_hit |= 2;
                            }
                        }
#endif
                        //   curLight = curLight + AMBIENT_LIGHT_INTENSITY;
                        curLight = min(curLight, MAX_LIGHT_INTENSITY);
                    }
#ifdef DEBUG
                    curLight = MAX_LIGHT_INTENSITY;
#endif
                    uint8_t contour = (brick->contour & light_hit);

                    if (value == BLOCK_AIR)
                    {
                        if (brick->Level > 0)
                        {
                            bool bOnSurface = false;
                            if (wY > 0)
                                bOnSurface = (WORLD[wY - 1][wX].Level == 0); // && WORLD[wY - 1][wX].traversable);

                            drawWaterTile(px, py, NULL, curLight, brick->Level, bOnSurface);
                        }
                        //rien le fond
                    }
                    else if (value == BLOCK_UNDERGROUND_AIR)
                    {
                        //background de terrassement
                        if (brick->Level > 0)
                        {
                            bool bOnSurface = false;
                            if (wY > 0)
                                bOnSurface = (WORLD[wY - 1][wX].Level == 0); // && WORLD[wY - 1][wX].traversable);

                            drawWaterTile(px, py, back_ground.pixel_data, curLight, brick->Level, bOnSurface);
                        }
                        else
                            drawTile(px, py, back_ground.pixel_data, curLight);
                    }
                    else if (value >= BLOCK_GROUND && value <= BLOCK_GROUND_ALL)
                    {
                        drawTile(px, py, getGroundBlockData(value), curLight);
                    }
                    else if (value == BLOCK_GRASS)
                    {
                        if (brick->Level > 0)
                        {
                            bool bOnSurface = false;
                            if (wY > 0)
                                bOnSurface = ((WORLD[wY - 1][wX].Level == 0) && WORLD[wY - 1][wX].traversable);

                            if (brick->spriteVariation == 0)
                                drawWaterTileMask(px, py, grass1.pixel_data, curLight, brick->Level, bOnSurface);
                            else if (brick->spriteVariation == 1)
                                drawWaterTileMask(px, py, grass2.pixel_data, curLight, brick->Level, bOnSurface);
                            else
                                drawWaterTileMask(px, py, grass3.pixel_data, curLight, brick->Level, bOnSurface);
                        }
                        else
                        {
                            if (brick->spriteVariation == 0)
                                drawTileMask(px, py, grass1.pixel_data, curLight);
                            else if (brick->spriteVariation == 1)
                                drawTileMask(px, py, grass2.pixel_data, curLight);
                            else
                                drawTileMask(px, py, grass3.pixel_data, curLight);
                        }
                    }
                    else if (value == BLOCK_TREE)
                    {
                        //Les tiles avec masking sont dessinées dans drawTilesMasking
                    }
                    else if (value == BLOCK_ROCK)
                    {
                        drawTile(px, py, rock_empty.pixel_data, curLight, contour);
                    }
                    else if (value == BLOCK_CHARBON) //
                    {
                        drawTile(px, py, rock_charbon.pixel_data, curLight, contour);
                    }
                    else if (value == BLOCK_CUIVRE) //
                    {
                        drawTile(px, py, rock_cuivre.pixel_data, curLight, contour);
                    }
                    else if (value == BLOCK_FER) //
                    {
                        drawTile(px, py, rock_fer.pixel_data, curLight, contour);
                    }
                    else if (value == BLOCK_DIAMANT) //
                    {
                        drawTile(px, py, rock_diamant.pixel_data, curLight, contour);
                    }
                    else if (value == BLOCK_JADE) //
                    {
                        drawTile(px, py, rock_jade.pixel_data, curLight, contour);
                    }
                    else if (value == BLOCK_OR) //
                    {
                        drawTile(px, py, rock_or.pixel_data, curLight, contour);
                    }
                    else if (value == BLOCK_REDSTONE) //
                    {
                        drawTile(px, py, rock_redstone.pixel_data, curLight, contour);
                    }                    
                    else
                    {
                        drawTile(px, py, rock_empty.pixel_data, curLight, contour);
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

void drawTilesMasking()
{
    int playerLightStartX = Player.pos.worldX - (PLAYER_LIGHT_MASK_WIDTH / 2);
    int playerLightStartY = Player.pos.worldY - (PLAYER_LIGHT_MASK_HEIGHT / 2);
    int playerLightEndX = playerLightStartX + (PLAYER_LIGHT_MASK_WIDTH - 1);
    int playerLightEndY = playerLightStartY + (PLAYER_LIGHT_MASK_HEIGHT - 1);

    for (int wX = worldMIN_X - 1; wX < worldMAX_X + 1; wX++)
    {
        if (wX < 0 || wX >= WORLD_WIDTH)
            continue;

        int profondeurColonne = WORLD[HEADER_ROW][wX].id;

        int px = ((wX - worldMIN_X) * 16) - currentOffset_X;
        if ((px < ARCADA_TFT_WIDTH + 16) && (px > -16))
        {
            for (int wY = worldMIN_Y - 1; wY < worldMAX_Y + 1; wY++)
            {
                if (wY < 0 || wY >= WORLD_HEIGHT)
                    continue;

                int py = ((wY - worldMIN_Y) * 16) - currentOffset_Y;
                if ((py < ARCADA_TFT_HEIGHT + 16) && (py > -16))
                {
                    TworldTile *brick = &WORLD[wY][wX];
                    uint8_t value = brick->id;

                    uint8_t light_hit = 0;

                    int curLight = MAX_LIGHT_INTENSITY;
                    // @todo gerer la nuit (dans ce cas pas de max light et ground.. ou la lune ?)
                    if (brick->opaque) // || wY > profondeurColonne)
                    {
                        if (profondeurColonne != 0)
                        {
                            int delta = (wY - profondeurColonne) + 1;
                            if (delta > 1)
                                curLight = curLight / delta;

                            curLight = max(curLight, 0);
                        }

                        if (wX >= playerLightStartX && wX <= playerLightEndX && wY >= playerLightStartY && wY <= playerLightEndY)
                        {
                            //Raycast
                            if (rayCastTo(Player.pos.worldX, Player.pos.worldY, wX, wY))
                            {
                                curLight = curLight + playerLightMask[wY - playerLightStartY][wX - playerLightStartX];
                            }
                        }
                        //   curLight = curLight + AMBIENT_LIGHT_INTENSITY;
                        curLight = min(curLight, MAX_LIGHT_INTENSITY);
                    }
#ifdef DEBUG
                    curLight = MAX_LIGHT_INTENSITY;
#endif

                    if (value == BLOCK_TREE) //
                    {
                        int realpx = px;
                        int realpy = py;
                        if (brick->hit)
                        {
                            realpx += random(-itemShakeAmount, itemShakeAmount);
                            realpy += random(-(itemShakeAmount >> 1), itemShakeAmount >> 1);
                        }
                        if (brick->spriteVariation == 0)
                            drawTreeTileMask(realpx, realpy, tree1.pixel_data, tree1.width, tree1.height, curLight);
                        else if (brick->spriteVariation == 1)
                            drawTreeTileMask(realpx, realpy, tree2.pixel_data, tree2.width, tree2.height, curLight);
                        else
                            drawTreeTileMask(realpx, realpy, tree3.pixel_data, tree3.width, tree3.height, curLight);
                    }
                }
            }
        }
    }
}

void drawParticles()
{
    int i;
    for (i = 0; i < parts.getActiveParticles(); i++)
    {
        int x, y, velX, velY;
        x = parts.particles[i].pX - cameraX;
        y = parts.particles[i].pY - cameraY;
        /*  velX = parts.particles[i].velX;
    velY = parts.particles[i].velY;
    
    if (velX > 0) {
      velX++;
    }else {
      velX--;
    }
    if (velY > 0) {
      velY++;
    }else {
      velY--;
    }
*/
        if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT)
            continue;

        int w = parts.particles[i].w;
        int h = parts.particles[i].h;

        if (w == 1 & h == 1)
            canvas->drawPixel(x, y, parts.particles[i].color);
        else
        {
            canvas->fillRect(x, y, w, h, parts.particles[i].color);
            if (parts.particles[i].color2 != 0XFFFF)
                canvas->drawRect(x - 1, y - 1, w + 2, h + 2, parts.particles[i].color2);
        }
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
        Serial.printf("Pix:%f,%f\n", Player.pos.pX, Player.pos.pY);

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

//    canvas->drawRect(debugX, debugY, 16, 16, ARCADA_WHITE);
    canvas->drawRect(debugXF, debugYF, 16, 16, ARCADA_RED);
//    canvas->drawRect(debugXD, debugYD, 16, 16, ARCADA_GREENYELLOW);
//    canvas->drawRect(debugXU, debugYU, 16, 16, ARCADA_CYAN);

    int ledgeF_X = Player.ledgePos2.x  - cameraX;
    int ledgeF_Y = Player.ledgePos2.y  - cameraY;
    int ledgeD_X = Player.ledgePos1.x  - cameraX;
    int ledgeD_Y = Player.ledgePos1.y  - cameraY;
    canvas->drawRect(ledgeF_X, ledgeF_Y, 16, 16, ARCADA_BLUE);
    canvas->drawRect(ledgeD_X, ledgeD_Y, 16, 16, ARCADA_WHITE);

    //   canvas->drawRect((Player.pos.pX - (worldMIN_X * 16)) - currentOffset_X, (Player.pos.pY - (worldMIN_Y * 16)) - currentOffset_Y, 16, 16, ARCADA_BLUE);

    for (int enC = 0; enC < NB_WORLD_ENNEMIES; enC++)
    {
        Tennemy *currentEnnemy = &ENNEMIES[enC];
        if (currentEnnemy->bIsAlive > 0)
        {
            if ((currentEnnemy->worldX >= worldMIN_X - 1) && (currentEnnemy->worldX <= worldMAX_X + 1) && (currentEnnemy->worldY >= worldMIN_Y - 1) && (currentEnnemy->worldY <= worldMAX_Y + 1))
            {

                int px = currentEnnemy->pX - cameraX;
                int py = currentEnnemy->pY - cameraY;

                canvas->drawRect(px, py, 16, 16, ARCADA_RED);
            }
        }
    }
}

void drawEffects()
{
    // @todo : pas vraiment ici que ce devrait etre fait...
    if (timerActionB > 0)
    {
        if (currentTileTarget.tile->id != BLOCK_AIR && currentTileTarget.tile->id != BLOCK_UNDERGROUND_AIR)
        {
            //Test action pour le sprite, pour l'instant la pioche
            // drawSprite(currentTileTarget.pX, currentTileTarget.pY, pioche.width, pioche.height, pioche.pixel_data, Player.pos.direction);
            if (fmod(timerActionB, TIME_ANIM_ACTION_B) == 0)
            {
                uint8_t direction = 0;
                if (currentTileTarget.wY > Player.pos.worldY)
                {
                    direction = Direction_Up | Direction_Left | Direction_Right;
                }
                else if (currentTileTarget.wY < Player.pos.worldY)
                {
                    direction = Direction_Bottom | Direction_Left | Direction_Right;
                }
                else
                {
                    if (Player.pos.direction == DIRECTION_RIGHT)
                        direction = Direction_Left | Direction_Bottom | Direction_Up;
                    else
                        direction = Direction_Right | Direction_Bottom | Direction_Up;
                }
                parts.createDirectionalExplosion(currentTileTarget.pX + 8, currentTileTarget.pY + 8, 8, 3, direction, currentTileTarget.itemColor, ARCADA_BLACK); //@todo colorer avec la couleur de la brique en cours de travail
                sndPlayerCanal1.play(AudioSamplePioche);

                currentTileTarget.tile->hit = 1;
                itemShakeAmount = DEFAULT_ITEM_SHAKE_AMOUNT;
            }
        }
    }
    if (itemShakeAmount > 0)
        itemShakeAmount--;

    if (cameraShakeAmount > 0)
        cameraShakeAmount--;
}

void drawHud()
{
    //Quick items
    canvas->fillRect(HUD_ITEMS_X, HUD_ITEMS_Y, 16 * MAX_QUICK_ITEMS, 16, ARCADA_LIGHTGREY);

    for (uint8_t it = 0; it < MAX_QUICK_ITEMS; it++)
    {
        int px = HUD_ITEMS_X + (it * 16);
        TInventoryItem *item_id = &Player.quick_inventory[it];
        //        if (item_id->typeItem != 0)
        //          canvas->fillRect(px+1, HUD_ITEMS_Y+1, 14, 14, ARCADA_WHITE);

        switch (item_id->typeItem)
        {
        case ITEM_TREE1:
            drawSprite(px, HUD_ITEMS_Y, item_buche1.width, item_buche1.height, item_buche1.pixel_data, 1);
            break;
        case ITEM_ROCK:
            drawSprite(px, HUD_ITEMS_Y, rock_small.width, rock_small.height, rock_small.pixel_data, 1);
            break;
        case ITEM_CHARBON:
            drawSprite(px, HUD_ITEMS_Y, charbon_small.width, charbon_small.height, charbon_small.pixel_data, 1);
            break;
        case ITEM_CUIVRE:
            drawSprite(px, HUD_ITEMS_Y, cuivre_small.width, cuivre_small.height, cuivre_small.pixel_data, 1);
            break;
        case ITEM_FER:
            drawSprite(px, HUD_ITEMS_Y, fer_small.width, fer_small.height, fer_small.pixel_data, 1);
            break;
        case ITEM_DIAMANT:
            drawSprite(px, HUD_ITEMS_Y, diamant_small.width, diamant_small.height, diamant_small.pixel_data, 1);
            break;
        case ITEM_JADE:
            drawSprite(px, HUD_ITEMS_Y, jade_small.width, jade_small.height, jade_small.pixel_data, 1);
            break;
        case ITEM_OR:
            drawSprite(px, HUD_ITEMS_Y, or_small.width, or_small.height, or_small.pixel_data, 1);
            break;
        case ITEM_REDSTONE:
            drawSprite(px, HUD_ITEMS_Y, redstone_small.width, redstone_small.height, redstone_small.pixel_data, 1);
            break;
        }
        if (it == Player.currentItemSelected)
            canvas->drawRect(px, HUD_ITEMS_Y, 16, 16, ARCADA_MAGENTA);
        else
            canvas->drawRect(px, HUD_ITEMS_Y, 16, 16, ARCADA_WHITE);
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
    //On dessine le background uniquement si on peut le voir, en sous sol on l'ignore, les tiles "background air" etant sans transparence
    if (worldMIN_Y < hauteurMaxBackground)
        drawBackgroundImage(0, 0, background_day.width, background_day.height, background_day.pixel_data);
    else
        canvas->fillScreen(ARCADA_BLACK);

    //           uint32_t now = micros();
    drawTiles();

    drawTilesMasking();
    briquesFRAMES++;
    //              Serial.printf("tiles:%d\n", micros() - now);

    drawEnnemies();

    drawItems();

    drawPlayer();

    drawParticles();

    drawEffects();

    drawHud();

    //debugInfos();
}

void pixelToWorld(int *pX, int *pY)
{
    int wX = *pX / 16;
    int wY = *pY / 16;
}

#ifdef USE_FOV
static int multipliers[4][8] = {
    {1, 0, 0, -1, -1, 0, 0, 1},
    {0, 1, -1, 0, 0, -1, 1, 0},
    {0, 1, 1, 0, 0, -1, -1, 0},
    {1, 0, 0, 1, -1, 0, 0, -1}};

inline void cast_light(int x, int y, int radius, int row, float start_slope, float end_slope, int xx, int xy, int yx, int yy)
{
    if (start_slope < end_slope)
    {
        return;
    }
    float next_start_slope = start_slope;
    int radius2 = radius * radius;
    for (int i = row; i <= radius; i++)
    {
        bool blocked = false;
        for (int dx = -i, dy = -i; dx <= 0; dx++)
        {
            float l_slope = (dx - 0.5) / (dy + 0.5);
            float r_slope = (dx + 0.5) / (dy - 0.5);
            if (start_slope < r_slope)
            {
                continue;
            }
            else if (end_slope > l_slope)
            {
                break;
            }

            int sax = dx * xx + dy * xy;
            int say = dx * yx + dy * yy;
            if ((sax < 0 && (int)abs(sax) > x) ||
                (say < 0 && (int)abs(say) > y))
            {
                continue;
            }
            int ax = x + sax;
            int ay = y + say;
            if (ax >= WORLD_WIDTH || ay >= WORLD_HEIGHT)
            {
                continue;
            }

            if ((int)(dx * dx + dy * dy) < radius2)
            {
                WORLD[ay][ax].light_hit = 1;
            }

            if (blocked)
            {
                if (!WORLD[ay][ax].traversable)
                {
                    next_start_slope = r_slope;
                    continue;
                }
                else
                {
                    blocked = false;
                    start_slope = next_start_slope;
                }
            }
            else if (!WORLD[ay][ax].traversable)
            {
                blocked = true;
                next_start_slope = r_slope;
                cast_light(x, y, radius, i + 1, start_slope, l_slope, xx, xy, yx, yy);
            }
        }
        if (blocked)
        {
            break;
        }
    }
}

void do_fov(int x, int y, int radius)
{
    for (int i = 0; i < 8; i++)
    {
        cast_light(x, y, radius, 1, 1.0, 0.0, multipliers[0][i],
                   multipliers[1][i], multipliers[2][i], multipliers[3][i]);
    }
}
#endif

/* Line of sight code         *
 * this is a Boolean function *
 * that returns FALSE if the  *
 * monster cannot see the     *
 * player and TRUE if it can  *
 *                            *
 * It has the monsters x and y*
 * coords as parameters       */
inline bool rayCastTo(int origin_x, int origin_y, int dest_x, int dest_y)
{
    int t, x, y, abs_delta_x, abs_delta_y, sign_x, sign_y, delta_x, delta_y;

    if (origin_x == dest_x && origin_y == dest_y)
        return true;
    /* Delta x is the players x minus the monsters x    *
    * d is my dungeon structure and px is the players  *
    * x position. origin_x is the monsters x position passed *
    * to the function.                                 */
    delta_x = dest_x - origin_x;

    /* delta_y is the same as delta_x using the y coordinates */
    delta_y = dest_y - origin_y;

    /* abs_delta_x & abs_delta_y: these are the absolute values of delta_x & delta_y */
    abs_delta_x = abs(delta_x);
    abs_delta_y = abs(delta_y);

    /* sign_x & sign_y: these are the signs of delta_x & delta_y */
    delta_x > 0 ? sign_x = 1 : sign_x = -1;
    delta_y > 0 ? sign_y = 1 : sign_y = -1;

    /* x & y: these are the monster's x & y coords */
    x = origin_x;
    y = origin_y;

    /* The following if statement checks to see if the line *
    * is x dominate or y dominate and loops accordingly    */
    if (abs_delta_x > abs_delta_y)
    {
        /* X dominate loop */
        /* t = twice the absolute of y minus the absolute of x*/
        t = abs_delta_y * 2 - abs_delta_x;
        do
        {
            if (t >= 0)
            {
                /* if t is greater than or equal to zero then *
             * add the sign of delta_y to y                    *
             * subtract twice the absolute of delta_x from t   */
                y += sign_y;
                t -= abs_delta_x * 2;
            }

            /* add the sign of delta_x to x      *
          * add twice the adsolute of delta_y to t  */
            x += sign_x;
            t += abs_delta_y * 2;

            /* check to see if we are at the player's position */
            if (x == dest_x && y == dest_y)
            {
                /* return that the monster can see the player */
                return true;
            }
            /* keep looping until the monster's sight is blocked *
       * by an object at the updated x,y coord             */
        } while (WORLD[y][x].traversable);

        /* NOTE: sight_blocked is a function that returns true      *
       * if an object at the x,y coord. would block the monster's *
       * sight                                                    */

        /* the loop was exited because the monster's sight was blocked *
       * return FALSE: the monster cannot see the player             */
        return false;
    }
    else
    {
        /* Y dominate loop, this loop is basically the same as the x loop */
        t = abs_delta_x * 2 - abs_delta_y;
        do
        {
            if (t >= 0)
            {
                x += sign_x;
                t -= abs_delta_y * 2;
            }
            y += sign_y;
            t += abs_delta_x * 2;
            if (x == dest_x && y == dest_y)
            {
                return true;
            }
        } while (WORLD[y][x].traversable);
        return false;
    }
}

void updatePlayerWorldCoords()
{
    Player.pos.worldX = (Player.pos.pX + 8) / 16;
    Player.pos.worldY = (Player.pos.pY + 8) / 16;

    if (Player.pos.direction == DIRECTION_LEFT)
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
    Player.pos.YUp = max(Player.pos.YUp, 0);
}

void updateCamera()
{
    int16_t currentCameraX = cameraX;
    int16_t currentCameraY = cameraY;
    int16_t cameraAheadX = Player.pos.direction * CAMERA_AHEAD_AMOUNT;
    //cameraX et cameraY sont la position en world pixels du coin en haut a gauche, qui suit le player
    cameraX = (Player.pos.pX - HALF_SCREEN_WIDTH) + cameraAheadX;

    //LERP
    cameraX = lerp15by8(currentCameraX, cameraX, int(CAMERA_FOLLOW_X_SPEED * fElapsedTime));

    //SHAKE
    if (cameraShakeAmount > 0)
        cameraX += random(-cameraShakeAmount, cameraShakeAmount);

    cameraX = max(cameraX, 0);
    cameraX = min(cameraX, ((WORLD_WIDTH - 1) * 16) - ARCADA_TFT_WIDTH);

    cameraY = Player.pos.pY - HALF_SCREEN_HEIGHT;
    //LERP
    //cameraY = lerp15by8(currentCameraY, cameraY, CAMERA_FOLLOW_Y_SPEED*fElapsedTime);

    //SHAKE
    if (cameraShakeAmount > 0)
        cameraY += random(-cameraShakeAmount, cameraShakeAmount);

    cameraY = max(cameraY, 0);
    cameraY = min(cameraY, ((WORLD_HEIGHT - 1) * 16) - ARCADA_TFT_HEIGHT);

    worldX = cameraX / 16;
    currentOffset_X = cameraX % 16;
    worldMIN_X = min(worldX, (WORLD_WIDTH - 1) - (ARCADA_TFT_WIDTH / 16));
    worldMIN_X = max(worldMIN_X, 0);
    worldMAX_X = worldMIN_X + (ARCADA_TFT_WIDTH / 16);
    worldMAX_X = min(worldMAX_X + 1, (WORLD_WIDTH - 1));

    worldY = cameraY / 16;
    currentOffset_Y = cameraY % 16;
    worldMIN_Y = min(worldY, (WORLD_HEIGHT - 1) - (ARCADA_TFT_HEIGHT / 16));
    worldMIN_Y = max(worldMIN_Y, 0);
    worldMAX_Y = worldMIN_Y + (ARCADA_TFT_HEIGHT / 16);
    worldMAX_Y = min(worldMAX_Y + 1, (WORLD_HEIGHT - 1));
}

bool playerPickupItem(Titem *currentItem)
{
    bool ret = false;

    //inventaire ?
    int index_stack = -1;
    int index_new_quick = -1;
    int index_new = -1;
    for (int inv = 0; inv < MAX_QUICK_ITEMS; inv++)
    {
        if (Player.quick_inventory[inv].typeItem == currentItem->type && Player.quick_inventory[inv].nbStack < MAX_STACK_ITEMS)
        {
            index_stack = inv;
            break;
        }
        else if (Player.quick_inventory[inv].typeItem == 0 && index_new_quick == -1)
            index_new_quick = inv;
    }
    if (index_stack != -1)
    {
        Player.quick_inventory[index_stack].typeItem = currentItem->type;
        Player.quick_inventory[index_stack].nbStack++;
        ret = true;
    }
    else
    {
        for (int inv = 0; inv < MAX_BACKPACK_ITEMS; inv++)
        {
            if (Player.inventory[inv].typeItem == currentItem->type && Player.inventory[inv].nbStack < MAX_STACK_ITEMS)
            {
                index_stack = inv;
                break;
            }
            else if (Player.inventory[inv].typeItem == 0 && index_new == -1)
                index_new = inv;
        }
        if (index_stack != -1)
        {
            Player.inventory[index_stack].typeItem = currentItem->type;
            Player.inventory[index_stack].nbStack++;
            ret = true;
        }
        else if (index_new_quick != -1)
        {
            Player.quick_inventory[index_new_quick].typeItem = currentItem->type;
            Player.quick_inventory[index_new_quick].nbStack++;
            ret = true;
        }
        else if (index_new != -1)
        {
            Player.inventory[index_new].typeItem = currentItem->type;
            Player.inventory[index_new].nbStack++;
            ret = true;
        }
    }

    return ret;
}

inline void setPlayerPos(float x, float y)
{
    Player.pos.pX = Player.pos.newX = x;
    Player.pos.pY = Player.pos.newY = y;
}

void checkPlayerState()
{
    brique_PLAYER_TL = getWorldAtPix(Player.pos.pX + PLAYER_X_BDM, Player.pos.pY);
    brique_PLAYER_BL = getWorldAtPix(Player.pos.pX + PLAYER_X_BDM, Player.pos.pY + 16);
    brique_PLAYER_TR = getWorldAtPix(Player.pos.pX + (15 - PLAYER_X_BDM), Player.pos.pY);
    brique_PLAYER_BR = getWorldAtPix(Player.pos.pX + (15 - PLAYER_X_BDM), Player.pos.pY + 16);

    TworldTile briqueUpAndHalf = getWorldAtPix(Player.pos.pX + 8, Player.pos.pY - 8);
    TworldTile briqueDownAndHalf = getWorldAtPix(Player.pos.pX + 8, Player.pos.pY + 24);
    //Au dessus
    brique_UP = getWorldAt(Player.pos.worldX, Player.pos.YUp);

    //En dessous
    brique_DOWN = getWorldAt(Player.pos.worldX, Player.pos.YDown);
    //  brique_DOWN_FRONT = getWorldAt(Player.pos.worldX, Player.pos.worldY + 1);

    //Devant
    brique_FRONT = getWorldAt(Player.pos.XFront, Player.pos.worldY);

    //Pour detection des bords
    if (Player.pos.direction == DIRECTION_RIGHT)
        brique_LEDGE = getWorldAtPix(Player.pos.pX + 16, Player.pos.pY);
    else
        brique_LEDGE = getWorldAtPix(Player.pos.pX - 1, Player.pos.pY);

    uint8_t oldWater = Player.waterLevel;
    Player.waterLevel = max(brique_PLAYER_BL.Level, brique_PLAYER_BR.Level);

    Player.bSplashIn = false;
    Player.bSplashOut = false;
    Player.bUnderWater = false;

    if (brique_PLAYER_TL.Level >= 2 || brique_PLAYER_TR.Level >= 2)
        Player.bUnderWater = true;

    if (oldWater >= 5 && Player.waterLevel <= 2)
    {
        Player.bSplashOut = true;
    }
    else if (oldWater <= 2 && Player.waterLevel >= 5)
    {
        Player.bSplashIn = true;
    }

    if (!brique_PLAYER_BL.traversable || !brique_PLAYER_BR.traversable)
        Player.bOnGround = true;
    else
        Player.bOnGround = false;

    //On suppose qu'on ne saute plus
    if (Player.pos.speedY >= 0)
    {
        Player.bJumping = false;
        Player.bDoubleJumping = false;
        Player.bWallJumping = false;
    }

    //Ground
    if (Player.onGroundTimer > 0)
        Player.onGroundTimer -= fElapsedTime;
    if (Player.bOnGround)
        Player.onGroundTimer = TIME_GROUND_LATENCY;

    if (Player.wallJumpTimer > 0)
    {
        Player.wallJumpTimer -= fElapsedTime;
        if (Player.bHasWallJumped && Player.wantedHorizontalDirection == -Player.lastWallJumpDirection)
        {
            Player.wallJumpTimer = 0;
            Player.pos.speedY = 0;
            Player.bHasWallJumped = false;
        }
    }
    else
        Player.bHasWallJumped = false;

    //    Serial.printf("Player: onGround:%d Falling:%d speedY:%f\n", Player.bOnGround, Player.bFalling, Player.pos.speedY);

    Player.bTouchingWall = (!brique_FRONT.traversable);
    Player.bTouchingLedge = (!brique_LEDGE.traversable);

    //On slide que sur les mur "hauts" qu'on touche et si on descend
    Player.bWallClimbing = false;
    Player.bWallSliding = false;
    if (Player.bTouchingWall && !Player.bClimbingLedge)
    {
        //Sliding et climbing
        if (Player.pos.speedY >= 0 && briqueDownAndHalf.traversable)
        {
            Player.bFalling = false;
            Player.bJumping = false;
            Player.bDoubleJumping = false;
            Player.bWallJumping = false;
            Player.bWallClimbing = false;

            Player.bWallSliding = true;
        }
        else if (Player.pos.speedY < 0 && Player.wantedVerticalDirection == DIRECTION_UP && !Player.bJumping)
        {
            Player.bFalling = false;
            Player.bJumping = false;
            Player.bDoubleJumping = false;
            Player.bWallJumping = false;
            Player.bWallSliding = false;

            Player.bWallClimbing = true;
        }
    
        //Ledge..
        if (Player.bWallClimbing && !Player.bTouchingLedge && !Player.bLedgeDetected)
        {            
            Player.bLedgeDetected = true;
   
            if (Player.pos.direction == DIRECTION_RIGHT)
                Player.ledgePosTop.x = Player.pos.XFront * 16;
            else
                Player.ledgePosTop.x = (Player.pos.XFront * 16) + 15;

            Player.ledgePosTop.y = Player.pos.worldY * 16;
        }
    }

    //Ledge, peut forcer la position du joueur le temps de l'anim
    CheckLedgeClimb();
}

void CheckLedgeClimb()
{
    if (Player.bLedgeDetected && !Player.bClimbingLedge)
    {
        Player.bClimbingLedge = true;
        Player.bFalling = false;
        Player.bJumping = false;
        Player.bDoubleJumping = false;
        Player.bWallJumping = false;
        Player.bWallSliding = false;
        Player.bWallClimbing = false;

        if (Player.pos.direction == DIRECTION_RIGHT)
        {
            Player.ledgePos1.x = (Player.ledgePosTop.x - 16) - ledgeClimbXOffset1;
            Player.ledgePos1.y = (Player.ledgePosTop.y) - 0 - ledgeClimbYOffset1;

            Player.ledgePos2.x = (Player.ledgePosTop.x + 0) + ledgeClimbXOffset2;
            Player.ledgePos2.y = (Player.ledgePosTop.y) - 16 - ledgeClimbYOffset2;
        }
        else
        {
            Player.ledgePos1.x = (Player.ledgePosTop.x + 1) + ledgeClimbXOffset1;
            Player.ledgePos1.y = (Player.ledgePosTop.y) - 0 - ledgeClimbYOffset1;

            Player.ledgePos2.x = (Player.ledgePosTop.x - 15) - ledgeClimbXOffset2;
            Player.ledgePos2.y = (Player.ledgePosTop.y) - 16 - ledgeClimbYOffset2;
        }

        Player.bCanMove = false;
        Player.bCanFlip = false;
    }
    //On force la position (anim de montee)
    if (Player.bClimbingLedge)
    {
        setPlayerPos(Player.ledgePos1.x, Player.ledgePos1.y);
    }
}

void checkPlayerCollisionsEntities()
{
    //Collisions items
    for (int itC = 0; itC < NB_WORLD_ITEMS; itC++)
    {
        Titem *currentItem = &ITEMS[itC];
        if ((currentItem->bIsAlive > 127) && currentItem->bIsActive)
        {
            //       if (((currentItem->worldX >= (worldMIN_X - 1)) && (currentItem->worldX <= (worldMAX_X + 1))) &&
            //           ((currentItem->worldY >= (worldMIN_Y - 1)) && (currentItem->worldY <= (worldMAX_Y + 1))))
            {

                int px = currentItem->pX;
                int py = currentItem->pY;

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

                int px = currentEnnemy->pX;
                int py = currentEnnemy->pY;

                //BBox
                if (px < Player.pos.pX + 14 &&
                    px + 14 > Player.pos.pX &&
                    py < Player.pos.pY + 14 &&
                    py + 14 > Player.pos.pY)
                {
                    //  Serial.println("Collision.");
                    if (Player.bFalling)
                    {
                        sndPlayerCanal3.play(AudioSample__Kick);
                        SCORE += 5;
                        killEnnemy(currentEnnemy, px, py);
                        cameraShakeAmount = CAMERA_KILL_ENNEMY_AMOUNT;
                        //rebond ?
                        Player.pos.speedY = JUMP_FORCE / 3;
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
        if ((brique_FRONT.traversable) &&
            ((Player.pos.direction == DIRECTION_RIGHT && (Player.pos.pX < ((WORLD_WIDTH - 1) * 16) - ARCADA_TFT_WIDTH)) ||
             (Player.pos.direction == DIRECTION_LEFT && (Player.pos.pX >= 2))))
        {
            Player.pos.pX += 1;
            Player.bWalking = true;
        }

        //Particules feu artifice ?
        Player.pos.pY = Player.pos.pY + FALLING_SPEED;

        updatePlayerWorldCoords();
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
        applyMovements();
        checkMovementDirection();
        updatePlayerVelocities();
        updatePlayerPosition();

        checkPlayerCollisionsWorld();
        updatePlayerWorldCoords();
        checkPlayerState();
        checkPlayerCollisionsEntities();
#endif

        //Position player
        updateCamera();
        computePlayerAnimations();
    }
}

void computePlayerAnimations()
{
    if (timerActionB > 0)
    {
        //@todo : Test type action,
        //pour le moment dig
        set_digging();
    }
    else
    {
        if (Player.bClimbingLedge)
            set_ledgeclimbing();
        else if (Player.bWallClimbing)
            set_wall_climbing();
        else if (Player.bWallSliding)
            set_sliding();
        else if (Player.bWallJumping)
            set_wall_jumping();
        else if (Player.bJumping)
        {
            if (Player.bDoubleJumping)
                set_double_jumping();
            else
                set_jumping();
        }
        else if (Player.bFalling)
            set_falling();
        else
        {
            //    if (Player.bOnGround)

            if (Player.bLanding)
            {
                //Anim aterrissage ?
                set_dust_fx();
            }

            if (Player.bWantWalk) // demannde de deplacement)
            {
                if (Player.bWalking) //On a bougé
                {
                    set_walking();
                    //set_dust_fx();
                }
                else // @todo push
                    set_idle();
            }
            else
            {
                set_idle();
            }
        }
        if (Player.bSplashIn)
        {
            set_splash_fx();
        }
        else if (Player.bSplashOut)
        {
            set_splash_fx();
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
                currentEnnemy->speedY = currentEnnemy->speedY + (FALLING_SPEED * fElapsedTime);

                //Frotements
                /* if (currentEnnemy->bOnGround)
                {
                    if (currentEnnemy->speedX > 0)
                        currentEnnemy->speedX--;
                    else if (currentEnnemy->speedX < 0)
                        currentEnnemy->speedX++;
                }*/

                if (currentEnnemy->speedX > MAX_SPEED_X)
                    currentEnnemy->speedX = MAX_SPEED_X;
                else if (currentEnnemy->speedX < -MAX_SPEED_X)
                    currentEnnemy->speedX = -MAX_SPEED_X;

                if (currentEnnemy->speedY > MAX_SPEED_Y)
                    currentEnnemy->speedY = MAX_SPEED_Y;
                else if (currentEnnemy->speedY < -MAX_SPEED_Y)
                    currentEnnemy->speedY = -MAX_SPEED_Y;

                //update pos
                if (currentEnnemy->speedX > 0)
                {
                    currentEnnemy->newX = currentEnnemy->pX + (currentEnnemy->speedX * fElapsedTime);
                    currentEnnemy->bWalking = true;

                    currentEnnemy->newX = min(((WORLD_WIDTH - 1) * 16), currentEnnemy->newX);
                }
                else if (currentEnnemy->speedX < 0)
                {
                    currentEnnemy->newX = currentEnnemy->pX + (currentEnnemy->speedX * fElapsedTime);
                    currentEnnemy->bWalking = true;

                    currentEnnemy->newX = max(0, currentEnnemy->newX);
                }

                currentEnnemy->newY = currentEnnemy->pY + (currentEnnemy->speedY * fElapsedTime);

                //collisions
                checkEnnemyCollisionsWorld(currentEnnemy);
            }
        }
    }
}

void checkEnnemyCollisionsWorld(Tennemy *currentEnnemy)
{
    //X
    if (currentEnnemy->speedX <= 0)
    {
        TworldTile tileTL = getWorldAtPix(currentEnnemy->newX, currentEnnemy->pY);
        TworldTile tileBL = getWorldAtPix(currentEnnemy->newX, currentEnnemy->pY + 15);
        if ((!tileTL.traversable) || (!tileBL.traversable))
        {
            currentEnnemy->newX = (currentEnnemy->newX - fmod(currentEnnemy->newX, 16)) + 16;
            currentEnnemy->speedX = -currentEnnemy->speedX;
        }
    }
    else
    {
        TworldTile tileTR = getWorldAtPix(currentEnnemy->newX + 16, currentEnnemy->pY);
        TworldTile tileBR = getWorldAtPix(currentEnnemy->newX + 16, currentEnnemy->pY + 15);
        if ((!tileTR.traversable) || (!tileBR.traversable))
        {
            currentEnnemy->newX = (currentEnnemy->newX - fmod(currentEnnemy->newX, 16));
            currentEnnemy->speedX = -currentEnnemy->speedX;
        }
    }
    //Y
    currentEnnemy->bOnGround = false;
    currentEnnemy->bJumping = false;
    currentEnnemy->bFalling = false;
    if (currentEnnemy->speedY <= 0)
    {
        //+5 et +12 au lieu de +0 et +15 pour compenser la boundingbox du sprite => NON car cause bug de saut en diagonale
        TworldTile tileTL = getWorldAtPix(currentEnnemy->newX + 0, currentEnnemy->newY);
        TworldTile tileTR = getWorldAtPix(currentEnnemy->newX + 15, currentEnnemy->newY);
        if ((!tileTL.traversable) || (!tileTR.traversable))
        {
            currentEnnemy->newY = (currentEnnemy->newY - fmod(currentEnnemy->newY, 16)) + 16;
            currentEnnemy->speedY = 0;
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
        TworldTile tileBL = getWorldAtPix(currentEnnemy->newX + 0, currentEnnemy->newY + 16);
        TworldTile tileBR = getWorldAtPix(currentEnnemy->newX + 15, currentEnnemy->newY + 16);
        if ((!tileBL.traversable) || (!tileBR.traversable))
        {
            currentEnnemy->newY = (currentEnnemy->newY - fmod(currentEnnemy->newY, 16));
            currentEnnemy->bOnGround = true;
            currentEnnemy->bFalling = false;
            currentEnnemy->speedY = 0;
        }
    }

    currentEnnemy->pX = currentEnnemy->newX;
    currentEnnemy->pY = currentEnnemy->newY;

    currentEnnemy->worldX = currentEnnemy->pX / 16;
    currentEnnemy->worldY = currentEnnemy->pY / 16;
}

void updateEnnemyIA(Tennemy *currentEnnemy)
{
    if (currentEnnemy->bIsAlive > 127)
    {
        //Si ennemi trop loin on ne fait pas
        int distX = abs(currentEnnemy->worldX - Player.pos.worldX);
        int distY = abs(currentEnnemy->worldY - Player.pos.worldY);
        //if ((currentEnnemy->worldX >= 1) && (currentEnnemy->worldX <= worldMAX_X))
        if ((distX < WORLD_WIDTH / 2) && (distY < WORLD_HEIGHT / 2))
        {
            currentEnnemy->bIsActive = true;
            //Deplacement
            if (currentEnnemy->type == SPIDER_ENNEMY)
            {
                if ((currentEnnemy->speedX < 0) && currentEnnemy->pX <= 0)
                    currentEnnemy->speedX = SPIDER_WALKING_SPEED;
                else if ((currentEnnemy->speedX > 0) && currentEnnemy->pX >= WORLD_WIDTH * 16)
                    currentEnnemy->speedX = -SPIDER_WALKING_SPEED;

                if (distY <= 2)
                {
                    int dist = currentEnnemy->worldX - Player.pos.worldX;
                    if (dist < 0 && dist > -5)
                    {
                        currentEnnemy->speedX = SPIDER_WALKING_SPEED;
                    }
                    else if (dist > 0 && dist < 5)
                    {
                        currentEnnemy->speedX = -SPIDER_WALKING_SPEED;
                    }
                }
            }
            else if (currentEnnemy->type == ZOMBI_ENNEMY)
            {
                if ((currentEnnemy->speedX < 0) && currentEnnemy->pX <= 0)
                    currentEnnemy->speedX = ZOMBI_WALKING_SPEED;
                else if ((currentEnnemy->speedX > 0) && currentEnnemy->pX >= WORLD_WIDTH * 16)
                    currentEnnemy->speedX = -ZOMBI_WALKING_SPEED;

                if (currentEnnemy->worldY == Player.pos.worldY)
                {
                    int dist = currentEnnemy->worldX - Player.pos.worldX;
                    if (dist < 0 && dist > -5)
                    {
                        currentEnnemy->speedX = ZOMBI_WALKING_SPEED;
                    }
                    else if (dist > 0 && dist < 5)
                    {
                        currentEnnemy->speedX = -ZOMBI_WALKING_SPEED;
                    }
                }
            }
            else if (currentEnnemy->type == SKEL_ENNEMY)
            {
                if ((currentEnnemy->speedX < 0) && currentEnnemy->pX <= 0)
                    currentEnnemy->speedX = SKEL_WALKING_SPEED;
                else if ((currentEnnemy->speedX > 0) && currentEnnemy->pX >= WORLD_WIDTH * 16)
                    currentEnnemy->speedX = -SKEL_WALKING_SPEED;

                if (currentEnnemy->worldY == Player.pos.worldY)
                {
                    int dist = currentEnnemy->worldX - Player.pos.worldX;
                    if (dist < 0 && dist > -5)
                    {
                        currentEnnemy->speedX = ZOMBI_WALKING_SPEED;
                    }
                    else if (dist > 0 && dist < 5)
                    {
                        currentEnnemy->speedX = -ZOMBI_WALKING_SPEED;
                    }
                }
                else
                {
                    //On essaie de pas tomber
                    if (currentEnnemy->speedX <= 0)
                    {
                        TworldTile tileBL = getWorldAtPix((currentEnnemy->pX + currentEnnemy->speedX), currentEnnemy->pY + 16);
                        if (tileBL.traversable)
                        {
                            currentEnnemy->speedX = -currentEnnemy->speedX;
                        }
                    }
                    else
                    {
                        TworldTile tileBR = getWorldAtPix((currentEnnemy->pX + currentEnnemy->speedX) + 15, currentEnnemy->pY + 16);
                        if (tileBR.traversable)
                        {
                            currentEnnemy->speedX = -currentEnnemy->speedX;
                        }
                    }
                }

            } //speed x et speed y (jump)
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
                currentItem->speedY = currentItem->speedY + FALLING_SPEED * fElapsedTime;

                if (currentItem->speedY > MAX_SPEED_Y)
                    currentItem->speedY = MAX_SPEED_Y;
                else if (currentItem->speedY < -MAX_SPEED_Y)
                    currentItem->speedY = -MAX_SPEED_Y;

                //update pos
                currentItem->newY = currentItem->pY + currentItem->speedY * fElapsedTime;

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
    if (currentItem->speedY <= 0)
    {
        //+5 et +12 au lieu de +0 et +15 pour compenser la boundingbox du sprite => NON car cause bug de saut en diagonale
        TworldTile tileTL = getWorldAtPix(currentItem->pX + 0, currentItem->newY);
        TworldTile tileTR = getWorldAtPix(currentItem->pX + 15, currentItem->newY);
        if ((!tileTL.traversable) || (!tileTR.traversable))
        {
            currentItem->newY = (currentItem->newY - fmod(currentItem->newY, 16)) + 16;
            currentItem->speedY = 0;
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
        TworldTile tileBL = getWorldAtPix(currentItem->pX + 0, currentItem->newY + 16);
        TworldTile tileBR = getWorldAtPix(currentItem->pX + 15, currentItem->newY + 16);
        if ((!tileBL.traversable) || (!tileBR.traversable))
        {
            currentItem->newY = (currentItem->newY - fmod(currentItem->newY, 16));
            currentItem->bOnGround = true;
            currentItem->bFalling = false;
            currentItem->speedY = 0;
        }
    }

    currentItem->pY = currentItem->newY;

    currentItem->worldX = currentItem->pX / 16;
    currentItem->worldY = currentItem->pY / 16;
}

void updateWorld()
{
    //on fait les verifications sur les briques, les anims, etc, repousser l'herbe sur les hauteur (ground top et herbes)

    //gestion de l'eau...
    if (everyXFrames(2))
    {
        //        uint32_t now = micros();
        WATER_Update();
        //        Serial.printf("WatUpd:%d\n", micros() - now);
    }
    else
    {
        //Update alternatif (lava ? anims ?)
        FoliageUpdate();
    }

    //Erosion
    //un ligne a la fois
    ErosionUpdate();
}

void ErosionUpdate()
{
    for (lastColErosion = 0; lastColErosion < WORLD_WIDTH; lastColErosion++)
    {
        TworldTile *tile = &WORLD[lastRowErosion][lastColErosion];
        TworldTile *tileL = lastColErosion > 0 ? &WORLD[lastRowErosion][lastColErosion - 1] : NULL;
        TworldTile *tileR = lastColErosion < (WORLD_WIDTH - 1) ? &WORLD[lastRowErosion][lastColErosion + 1] : NULL;
        TworldTile *tileU = lastRowErosion > 0 ? &WORLD[lastRowErosion - 1][lastColErosion] : NULL;
        TworldTile *tileD = lastColErosion < (WORLD_HEIGHT - 1) ? &WORLD[lastRowErosion + 1][lastColErosion] : NULL;
        if (tile->traversable)
            continue;

        uint8_t matrix = 0;
        if (!tile->traversable)
        {
            if (tileL && tileL->traversable)
                matrix |= 1;
            if (tileR && tileR->traversable)
                matrix |= 4;
            if (tileU && tileU->traversable && tileU->Level == 0)
                matrix |= 2;
            if (tileD && tileD->traversable)
                matrix |= 8;

            tile->contour = matrix;
        }

        if (tile->id >= BLOCK_GROUND && tile->id <= BLOCK_GROUND_ALL)
            tile->id = (matrix | BLOCK_GROUND);
    }
    lastRowErosion++;
    if (lastRowErosion == WORLD_HEIGHT)
        lastRowErosion = 0;
}
void FoliageUpdate()
{
    for (int wX = 0; wX < WORLD_WIDTH; wX++)
    {
        int hauteur = WORLD[HEADER_ROW][wX].id;
        TworldTile tile = getWorldAt(wX, hauteur);
        TworldTile tileUp = getWorldAt(wX, hauteur - 1);
        int randSeed = random(0, 1000);

        //        if (tileUp.id == BLOCK_TREE &&)
    }
}

void updatePlayerVelocities()
{
    if (Player.bUnderWater) //Tete sous l'eau
    {
        //Gravity
        Player.pos.speedY = Player.pos.speedY + (FALLING_SPEED / 5) * fElapsedTime;

        Player.pos.speedX += -2.0f * Player.pos.speedX * fElapsedTime;
        if (fabs(Player.pos.speedX) < 0.01f)
            Player.pos.speedX = 0.0f;

        if (Player.pos.speedX > (MAX_SPEED_X >> 1))
            Player.pos.speedX = (MAX_SPEED_X >> 1);
        else if (Player.pos.speedX < -(MAX_SPEED_X >> 1))
            Player.pos.speedX = -(MAX_SPEED_X >> 1);

        if (Player.pos.speedY > (MAX_SPEED_Y >> 1))
            Player.pos.speedY = (MAX_SPEED_Y >> 1);
        else if (Player.pos.speedY < -(MAX_SPEED_Y >> 1))
            Player.pos.speedY = -(MAX_SPEED_Y >> 1);
    }
    else
    {
        if (Player.bWallSliding)
        {
            Player.pos.speedY = Player.pos.speedY + (WALL_SLIDE_SPEED * fElapsedTime);

            if (Player.pos.speedY > MAX_SPEED_Y_SLIDING)
                Player.pos.speedY = MAX_SPEED_Y_SLIDING;
            else if (Player.pos.speedY < -MAX_SPEED_Y)
                Player.pos.speedY = -MAX_SPEED_Y;
        }
        else if (Player.bWallClimbing)
        {
            //Player.pos.speedY = Player.pos.speedY + (WALL_SLIDE_SPEED * fElapsedTime);

            if (Player.pos.speedY > MAX_SPEED_Y_CLIMBING)
                Player.pos.speedY = MAX_SPEED_Y_CLIMBING;
            else if (Player.pos.speedY < -MAX_SPEED_Y_CLIMBING)
                Player.pos.speedY = -MAX_SPEED_Y_CLIMBING;
        }
        else 
        {
            //Gravity
            //if (!Player.bOnGround)
            Player.pos.speedY = Player.pos.speedY + (FALLING_SPEED * fElapsedTime);

            //Frotements, a revoir
            if (Player.bOnGround)
            {
                Player.pos.speedX += -10.0f * Player.pos.speedX * fElapsedTime;
                if (fabs(Player.pos.speedX) < 0.01f)
                    Player.pos.speedX = 0.0f;
            }
            else
            {
                //Air frotements
                Player.pos.speedX += -3.0f * Player.pos.speedX * fElapsedTime;
                if (fabs(Player.pos.speedX) < 0.01f)
                    Player.pos.speedX = 0.0f;
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
    }
}

inline void Flip()
{
    if (!Player.bWallSliding && !Player.bWallClimbing && Player.bCanFlip) // && !knockback)
    {
        Player.pos.direction = -Player.pos.direction;
    }
}

inline void updatePlayerPosition()
{
    //if (Player.bWantWalk)
    {
        //CHECK COLLISION a refaire propre
        if (Player.pos.speedX > 0)
        {
            Player.pos.newX = Player.pos.pX + (Player.pos.speedX * fElapsedTime);
            Player.pos.newX = min(((WORLD_WIDTH - 2) * 16), Player.pos.newX);
        }
        else if (Player.pos.speedX < 0)
        {
            Player.pos.newX = Player.pos.pX + (Player.pos.speedX * fElapsedTime);
            Player.pos.newX = max(0, Player.pos.newX);
        }
    }

    Player.pos.newY = Player.pos.pY + (Player.pos.speedY * fElapsedTime);
}

void checkPlayerCollisionsWorld()
{
    bool bWasOnGround = Player.bOnGround;

    //X
    if (Player.pos.speedX > 0)
    {
        // +16 +0
        // +16 +15
        TworldTile tileTR = getWorldAtPix(Player.pos.newX + (16 - PLAYER_X_BDM), Player.pos.pY + PLAYER_Y_BDM);
        TworldTile tileBR = getWorldAtPix(Player.pos.newX + (16 - PLAYER_X_BDM), Player.pos.pY + 15);
        if (!tileTR.traversable || !tileBR.traversable)
        {
            Player.pos.newX = (Player.pos.newX - fmod(Player.pos.newX, 16)) + PLAYER_X_BDM;
            Player.pos.speedX = 0;
        }
    }
    else if (Player.pos.speedX < 0)
    {
        // +0 +0
        // +0 +15
        TworldTile tileTL = getWorldAtPix(Player.pos.newX + PLAYER_X_BDM, Player.pos.pY + PLAYER_Y_BDM);
        TworldTile tileBL = getWorldAtPix(Player.pos.newX + PLAYER_X_BDM, Player.pos.pY + 15);
        if (!tileTL.traversable || !tileBL.traversable)
        {
            Player.pos.newX = (Player.pos.newX - fmod(Player.pos.newX, 16)) + (16 - PLAYER_X_BDM);
            Player.pos.speedX = 0;
        }
    }
    
    //Y
    Player.bFalling = false;
    Player.bOnGround = false;
    Player.bLanding = false;

    if (Player.pos.speedY < 0)
    {
        //+5 et +12 au lieu de +0 et +15 pour compenser la boundingbox du sprite => NON car cause bug de saut en diagonale
        // +0 +0
        // +15 +0
        TworldTile tileTL = getWorldAtPix(Player.pos.newX + PLAYER_X_BDM, Player.pos.newY + PLAYER_Y_BDM);
        TworldTile tileTR = getWorldAtPix(Player.pos.newX + (15 - PLAYER_X_BDM), Player.pos.newY + PLAYER_Y_BDM);
        if (!tileTL.traversable || !tileTR.traversable)
        {
            Player.pos.newY = (Player.pos.newY - fmod(Player.pos.newY, 16)) + (16 - PLAYER_Y_BDM); // - PLAYER_Y_BDM ??
            Player.pos.speedY = 0;
        }
    }
    else if (Player.pos.speedY > 0)
    {
        //+5 et +12 au lieu de +0 et +15 pour compenser la boundingbox du sprite => NON car cause bug de saut en diagonale
        // +0 +16
        // +15 +16

        TworldTile tileBL = getWorldAtPix(Player.pos.newX + PLAYER_X_BDM, Player.pos.newY + 16);
        TworldTile tileBR = getWorldAtPix(Player.pos.newX + (15 - PLAYER_X_BDM), Player.pos.newY + 16);
        if (!tileBL.traversable || !tileBR.traversable)
        {
            Player.pos.newY = Player.pos.newY - fmod(Player.pos.newY, 16);
            Player.bOnGround = true;
            Player.bFalling = false;
            //Si la vitesse etait importante on vient d'atterir, spawn effect
            if (!bWasOnGround && Player.pos.speedY >= SPEED_Y_LANDING)
                Player.bLanding = true;

            //On peut de nouveau realiser un double jump
            Player.bDoubleJumping = false;
            Player.pos.speedY = 0;
        }
        else
        {
            Player.bFalling = true;
        }
    }

    //Walking
    Player.bWalking = false;
    if (Player.pos.pX != Player.pos.newX)
    {
        Player.bWalking = true;
        Player.pos.pX = Player.pos.newX;
    }

    Player.bMovingUpDown = false;
    if (Player.pos.pY != Player.pos.newY)
    {
        Player.bMovingUpDown = true;
        Player.pos.pY = Player.pos.newY;
    }
}

void checkPlayerInputs()
{
    Player.bWantJump = false;
    Player.bWantDoubleJump = false;

    if (A_just_pressedTimer > 0)
        A_just_pressedTimer -= fElapsedTime;

    if (Player.jumpTimer > 0)
        Player.jumpTimer -= fElapsedTime;

    /*
    if (Player.turnTimer > 0)
    {
        Player.turnTimer -= fElapsedTime;

        if(Player.turnTimer <= 0)
        {
            Player.bCanMove = true;
            Player.bCanFlip = true;
        }
    }*/

    if (pressed_buttons & ARCADA_BUTTONMASK_UP)
        Player.wantedVerticalDirection = DIRECTION_LEFT;
    else if (pressed_buttons & ARCADA_BUTTONMASK_DOWN)
        Player.wantedVerticalDirection = DIRECTION_RIGHT;
    else
        Player.wantedVerticalDirection = DIRECTION_NONE;

    if (pressed_buttons & ARCADA_BUTTONMASK_LEFT)
        Player.wantedHorizontalDirection = DIRECTION_UP;
    else if (pressed_buttons & ARCADA_BUTTONMASK_RIGHT)
        Player.wantedHorizontalDirection = DIRECTION_DOWN;
    else
        Player.wantedHorizontalDirection = DIRECTION_NONE;

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
    //@todo : virer le test onGround pour attaques aeriennes..
    if (coolDownActionB <= 0 && (pressed_buttons & ARCADA_BUTTONMASK_B) && Player.bOnGround)
    {
        timerActionB += fElapsedTime;

        int lastCibleX = currentTileTarget.wX;
        int lastCibleY = currentTileTarget.wY;

        //Ciblage, on laisse un peu de temps pour locker la cible
        if (timerActionB < TIME_LOCK_ACTION_B)
        {
            if (Player.wantedVerticalDirection == DIRECTION_UP)
            {
                currentTileTarget.wX = Player.pos.worldX;
                currentTileTarget.wY = Player.pos.YUp;
            }
            else if (Player.wantedVerticalDirection == DIRECTION_DOWN)
            {
                currentTileTarget.wX = Player.pos.worldX;
                currentTileTarget.wY = Player.pos.YDown;
            }
            else
            {
                //'Devant'
                currentTileTarget.wX = Player.pos.XFront;
                currentTileTarget.wY = Player.pos.worldY;
            }

            if (currentTileTarget.wX != lastCibleX || currentTileTarget.wY != lastCibleY)
            {
                timerActionB = 0;
                currentTileTarget.pX = currentTileTarget.wX * 16;
                currentTileTarget.pY = currentTileTarget.wY * 16;
                currentTileTarget.tile = &WORLD[currentTileTarget.wY][currentTileTarget.wX];
                currentTileTarget.currentLife = currentTileTarget.tile->life * TILE_LIFE_PRECISION;
                if (currentTileTarget.tile->id >= BLOCK_ROCK && currentTileTarget.tile->id <= BLOCK_JADE)
                {
                    currentTileTarget.itemColor = RGBConvert(147, 122, 73);
                }
                else if (currentTileTarget.tile->id == BLOCK_TREE)
                {
                    currentTileTarget.itemColor = RGBConvert(158, 96, 47);
                }
                else if (currentTileTarget.tile->id == BLOCK_GROUND_TOP)
                {
                    currentTileTarget.itemColor = RGBConvert(62, 137, 72);
                }
                else
                {
                    currentTileTarget.itemColor = RGBConvert(217, 160, 102);
                }
                currentTileTarget.lastHit = millis();
            }
        }
        else
        {
            if (timerActionB >= TIME_ACTION_B) //Action (pour le moment minage seulement)
            {
                TworldTile *tile = currentTileTarget.tile;
                currentTileTarget.tile->hit = 0;
                timerActionB = 0;
                coolDownActionB = TIME_COOLDOWN_B;
                lastCibleX = lastCibleY = 0;
                bool bHit = false;

                //Serial.printf("Dig:%d,%d v:%d\n", currentTileTarget.wX, currentTileTarget.wY, tile);
                if (tile->life != BLOCK_LIFE_NA && Player.pos.YDown < (WORLD_HEIGHT - 1))
                {
                    //on teste si on a creusé dans de la pierre solide et on exclue aussi le ground ( @todo : spawn herbe quand meme )
                    if (tile->id >= BLOCK_ROCK && tile->id <= BLOCK_JADE)
                    {
                        bHit = true;
                        //ROCHER
                        // @todo span item ramassable ? ou tile ramassable ?
                        createDropFrom(currentTileTarget.wX, currentTileTarget.wY, tile->id);

                        tile->id = BLOCK_UNDERGROUND_AIR;
                        tile->life = BLOCK_LIFE_NA;
                        tile->traversable = 1;
                        tile->opaque = 1;
                    }
                    else if (tile->id == BLOCK_TREE)
                    {
                        bHit = true;
                        //On casse tout l'arbre
                        createDropFrom(currentTileTarget.wX, currentTileTarget.wY, tile->id);

                        tile->id = BLOCK_AIR;
                        tile->life = BLOCK_LIFE_NA;
                        tile->traversable = 1;
                        tile->opaque = 0;
                    }
                    else
                    {
                        bHit = true;

                        // @todo span herbe
                        //createDropFrom(currentTileTarget.wX, currentTileTarget.wY, tile.id);

                        //On enleve l'herbe si il y en avait
                        TworldTile brickUp = getWorldAt(currentTileTarget.wX, currentTileTarget.wY - 1);
                        if (brickUp.id == BLOCK_GRASS)
                        {
                            brickUp.id = BLOCK_AIR;
                            brickUp.life = BLOCK_LIFE_NA;
                            brickUp.traversable = 1;
                            brickUp.opaque = 0;
                            setWorldAt(currentTileTarget.wX, currentTileTarget.wY - 1, brickUp);
                        }
                        tile->id = BLOCK_AIR;
                        tile->life = BLOCK_LIFE_NA;
                        tile->traversable = 1;
                        tile->opaque = 0;
                    }
                    if (bHit)
                    {
                        // @todo : tester le type de tile
                        sndPlayerCanal1.play(AudioSampleRock_break);
                        uint8_t direction = 0;
                        if (currentTileTarget.wY > Player.pos.worldY)
                        {
                            direction = Direction_Up | Direction_Left | Direction_Right;
                        }
                        else if (currentTileTarget.wY < Player.pos.worldY)
                        {
                            direction = Direction_Bottom | Direction_Left | Direction_Right;
                        }
                        else
                        {
                            if (Player.pos.direction == DIRECTION_RIGHT)
                                direction = Direction_Left | Direction_Bottom | Direction_Up;
                            else
                                direction = Direction_Right | Direction_Bottom | Direction_Up;
                        }
                        parts.createDirectionalExplosion(currentTileTarget.pX + 8, currentTileTarget.pY + 8, 6, 6, direction, currentTileTarget.itemColor, ARCADA_BLACK); //@todo colorer avec la couleur de la brique en cours de travail
                        cameraShakeAmount = CAMERA_BREAK_ROCK_AMOUNT;
                        //On force l'erosion sur la ligne du dessus ca raff plus rapidement
                        lastRowErosion = currentTileTarget.wY - 1;
                        updateHauteurColonne(currentTileTarget.wX, currentTileTarget.wY);
                    }
                }
            }
        }
    }
    else
    {
        if (coolDownActionB > 0)
            coolDownActionB -= fElapsedTime;

        timerActionB = 0;
        if (currentTileTarget.tile != NULL)
            currentTileTarget.tile->hit = 0;

        //currentTileTarget = {0};

        if ((just_pressed & ARCADA_BUTTONMASK_A) || A_just_pressedTimer > 0)
        {
            if (Player.bWallSliding && !Player.bOnGround && Player.wantedHorizontalDirection == 0) //(Player.onWallCounter > 0) // && Player.pos.speedY == 0)
            {                                                                                      //WALLHOP (LACHE LE MUR)
                A_just_pressedTimer = 0;
                Player.bWantJump = true;
                Player.jumpTimer = 0;

                Player.bWallSliding = false;
                Player.bWallClimbing = false;
                Player.bWallJumping = false;
                Player.turnTimer = 0;
                Player.bCanMove = true;
                Player.bCanFlip = true;
                Player.bHasWallJumped = false;
                Player.wallJumpTimer = 0;
                Player.pos.direction = -Player.pos.direction;
                Player.lastWallJumpDirection = Player.pos.direction;

                sndPlayerCanal1.play(AudioSample__Jump);
                //Thrust
                Player.pos.speedX = Player.lastWallJumpDirection * WALL_HOP_FORCE_X;
                Player.pos.speedY = -WALL_HOP_FORCE_Y;

                //Spawn Effect (wall juump)
                //set_double_jump_fx();
            }
            else if (!Player.bOnGround && Player.bTouchingWall && Player.wantedHorizontalDirection != Player.pos.direction) //(Player.onWallCounter > 0) // && Player.pos.speedY == 0)
            {
                //WALLJUMP
                A_just_pressedTimer = 0;
                Player.bWantJump = true;

                Player.jumpTimer = 0;
                Player.bWallSliding = false;
                Player.bWallClimbing = false;
                Player.turnTimer = 0;
                Player.bCanMove = true;
                Player.bCanFlip = true;
                Player.bHasWallJumped = true;
                Player.bWallJumping = true;

                Player.wallJumpTimer = TIME_WALL_LATENCY;

                //Wall jump en face
                Player.pos.direction = -Player.pos.direction;
                Player.lastWallJumpDirection = Player.pos.direction;

                //Thrust
                Player.pos.speedX = Player.lastWallJumpDirection * WALL_JUMP_FORCE_X;
                Player.pos.speedY = -WALL_JUMP_FORCE_Y;

                sndPlayerCanal1.play(AudioSample__Jump);
                //Spawn Effect (wall juump)
                set_double_jump_fx();
            }
            else if (Player.onGroundTimer > 0) // && Player.pos.speedY == 0)
            {
                Player.bJumping = true;
                Player.bWallJumping = false;
                Player.bDoubleJumping = false;
                Player.onGroundTimer = 0;
                A_just_pressedTimer = 0;
                Player.bWantJump = true;
                Player.jumpTimer = TIME_DOUBLE_JUMP_DETECTION;
                sndPlayerCanal1.play(AudioSample__Jump);
                //Thrust
                Player.pos.speedY = -JUMP_FORCE;
                //Spawn Effect
                set_double_jump_fx();
            }
            else if (!Player.bOnGround && !Player.bDoubleJumping && Player.jumpTimer > 0)
            {
                Player.jumpTimer = 0;
                A_just_pressedTimer = 0;
                // @todo spawn dust double jump
                Player.bWantDoubleJump = true;
                sndPlayerCanal1.play(AudioSample__Jump);
                //Thrust
                Player.pos.speedY = -DOUBLE_JUMP_FORCE;
                Player.bJumping = true;
                Player.bDoubleJumping = true;
                Player.bWallJumping = false;
                //Spawn Effect
                set_double_jump_fx(true);
            }
        }

        //Si on relache le bouton on saute moins haut
        if ((just_released & ARCADA_BUTTONMASK_A) && Player.pos.speedY < 0)
        {
            Player.pos.speedY = Player.pos.speedY / 2;
        }
        /*
        if (Player.bWallSliding && !Player.bOnGround && Player.wantedHorizontalDirection == -Player.pos.direction)
        {
            Player.bCanMove = false;
            Player.bCanFlip = false;

            Player.turnTimer = TURN_TIMER_SET;
        }*/
    }
}

void applyMovements()
{
    if (timerActionB == 0)
    {
        if (!Player.bOnGround)
        {
            if (Player.bTouchingWall && Player.wantedVerticalDirection != 0)
            {
                Player.pos.speedY += (Player.wantedVerticalDirection * CLIMBING_SPEED * fElapsedTime);
                //On ajoute de la speed x pour l'aider a sortir une fois en haut?
//                if (Player.wantedVerticalDirection == DIRECTION_UP)
  //                  Player.pos.speedX = Player.pos.direction * MAX_SPEED_X;
            }
            else if (!Player.bWallSliding && !Player.bWallClimbing)   //On flotte dans l'air
            {
                Player.pos.speedX += (Player.wantedHorizontalDirection * FLOATING_SPEED * fElapsedTime);
            }
        }
        else if (Player.bCanMove)
        {
            Player.pos.speedX += (Player.wantedHorizontalDirection * RUNNING_SPEED * fElapsedTime);
        }
        
/*
        if (Player.wantedVerticalDirection == DIRECTION_UP)
        {
            if (Player.bTouchingWall)
            {
            }
        }
        else if (Player.wantedVerticalDirection == DIRECTION_DOWN)
        {
            if (Player.bTouchingWall)
            {
            }
        }*/
    }
}

void checkMovementDirection()
{
    Player.bWantWalk = false;

    if (Player.wantedHorizontalDirection != 0)
    {
        if (Player.pos.direction != Player.wantedHorizontalDirection)
            Flip();
        Player.bWantWalk = true;
    }
}

void updateGame()
{
    int lastX = 0;

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

        updateWorld();

        lastX = cameraX;
    }

    //Ps
    parts.moveParticles(cameraX, cameraY);
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

        for (int wY = 0; wY < WORLD_HEIGHT + 2; wY++)
        {
            for (int wX = 0; wX < WORLD_WIDTH; wX++)
            {
                uint8_t *buff = (uint8_t *)&WORLD[wY][wX];
                for (int cptBuf = 0; cptBuf < sizeof(WORLD[wY][wX]); cptBuf++)
                    buff[cptBuf] = data.read();
            }
        }

        data.close();

        ret = true;
    }
    //
    postInitWorld();

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
        for (int wY = 0; wY < WORLD_HEIGHT + 2; wY++)
        {
            for (int wX = 0; wX < WORLD_WIDTH; wX++)
            {
                uint8_t *buff = (uint8_t *)&WORLD[wY][wX];
                for (int cptBuf = 0; cptBuf < sizeof(WORLD[wY][wX]); cptBuf++)
                    data.write(buff[cptBuf]);
            }
        }
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
    just_released = arcada.justReleasedButtons();
}

void loop()
{
    static uint8_t _old_lastFrameDurationMs = lastFrameDurationMs;

    //long now = millis();
    //updateSoundManager(now);

    if (!nextFrame())
        return;

    if (lastTime != 0)
        fElapsedTime = (__now - lastTime) / 1000.0f;
    lastTime = __now;
    //Serial.printf("Elapsed:%f\n", fElapsedTime);

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

    if (lastFrameDurationMs > 37 && (_old_lastFrameDurationMs != lastFrameDurationMs))
    {
        Serial.printf("LFD:%d\n", lastFrameDurationMs);
        _old_lastFrameDurationMs = lastFrameDurationMs;
    }
}

void FinishLedgeClimb()
{
    Player.bClimbingLedge = false;
    setPlayerPos(Player.ledgePos2.x, Player.ledgePos2.y);

    Player.bCanMove = true;
    Player.bCanFlip = true;
    Player.bLedgeDetected = false;
    set_idle();
}

void anim_player_idle()
{
    if (Player.current_framerate == PLAYER_FRAMERATE_IDLE)
    {
        Player.anim_frame++;
        Player.current_framerate = 0;
    }
    Player.current_framerate++;

    if (Player.anim_frame > PLAYER_FRAME_IDLE_4)
    {
        Player.anim_frame = PLAYER_FRAME_IDLE_1;
    }

    drawSpriteSheet(Player.pos.pX - cameraX + PLAYER_X_OFFSET, Player.pos.pY - cameraY + PLAYER_Y_OFFSET, PLAYER_WIDTH, PLAYER_HEIGHT, player_sheet.pixel_data, Player.anim_frame, Player.pos.direction);
}

void anim_player_digging()
{
    if (Player.current_framerate == PLAYER_FRAMERATE_ACTION)
    {
        Player.anim_frame++;
        Player.current_framerate = 0;
    }
    Player.current_framerate++;

    if (Player.anim_frame > PLAYER_FRAME_ACTION_3)
    {
        Player.anim_frame = PLAYER_FRAME_ACTION_1;
    }

    drawSpriteSheet(Player.pos.pX - cameraX + PLAYER_X_OFFSET, Player.pos.pY - cameraY + PLAYER_Y_OFFSET, PLAYER_WIDTH, PLAYER_HEIGHT, player_sheet.pixel_data, Player.anim_frame, Player.pos.direction);
}

void anim_player_wall_climbin()
{
    if (Player.current_framerate == PLAYER_FRAMERATE_WALL_CLIMBING)
    {
        Player.anim_frame++;
        Player.current_framerate = 0;
    }
    Player.current_framerate++;

    if (Player.anim_frame > PLAYER_FRAME_WALL_CLIMBING_4)
    {
        Player.anim_frame = PLAYER_FRAME_WALL_CLIMBING_1; 
    }

    drawSpriteSheet(Player.pos.pX - cameraX + PLAYER_X_OFFSET, Player.pos.pY - cameraY + PLAYER_Y_OFFSET, PLAYER_WIDTH, PLAYER_HEIGHT, player_sheet.pixel_data, Player.anim_frame, Player.pos.direction);
}

void anim_player_wall_sliding()
{
    if (Player.current_framerate == PLAYER_FRAMERATE_SLIDING)
    {
        Player.anim_frame++;
        Player.current_framerate = 0;
    }
    Player.current_framerate++;

    if (Player.anim_frame > PLAYER_FRAME_SLIDING_2)
    {
        Player.anim_frame = PLAYER_FRAME_SLIDING_1; //On reste sur 1
    }

    drawSpriteSheet(Player.pos.pX - cameraX + PLAYER_X_OFFSET, Player.pos.pY - cameraY + PLAYER_Y_OFFSET, PLAYER_WIDTH, PLAYER_HEIGHT, player_sheet.pixel_data, Player.anim_frame, Player.pos.direction);
}

void anim_player_walk()
{
    if (Player.current_framerate == PLAYER_FRAMERATE_RUN)
    {
        Player.anim_frame++;
        Player.current_framerate = 0;
    }
    Player.current_framerate++;

    if (Player.anim_frame > PLAYER_FRAME_RUN_6)
    {
        Player.anim_frame = PLAYER_FRAME_RUN_1;
    }

    drawSpriteSheet(Player.pos.pX - cameraX + PLAYER_X_OFFSET, Player.pos.pY - cameraY + PLAYER_Y_OFFSET, PLAYER_WIDTH, PLAYER_HEIGHT, player_sheet.pixel_data, Player.anim_frame, Player.pos.direction);
}

void anim_player_ledge_climbing()
{
    if (Player.current_framerate == PLAYER_FRAMERATE_LEDGE_CLIMBING)
    {
        Player.anim_frame++;
        Player.current_framerate = 0;
        //cas special, a partir de la frame 5 on monte de 3 , il reste 5 frames, soit 15 pix.
        // le player est force a ledgePos1 dans checkLedge
        if (Player.anim_frame >= PLAYER_FRAME_LEDGE_CLIMB_5)
        {
            Player.ledgePos1.y -=2;
            Player.ledgePos1.x += 1 * Player.pos.direction;
        }
    }
    Player.current_framerate++;

    if (Player.anim_frame > PLAYER_FRAME_LEDGE_CLIMB_9)
    {
        //On annule tout
        FinishLedgeClimb();
        return;
    }

    drawSpriteSheet(Player.pos.pX - cameraX + PLAYER_X_OFFSET, Player.pos.pY - cameraY + PLAYER_Y_OFFSET, PLAYER_WIDTH, PLAYER_HEIGHT, player_sheet.pixel_data, Player.anim_frame, Player.pos.direction);
}

void anim_player_wall_jumping()
{
    if (Player.current_framerate == PLAYER_FRAMERATE_WALL_JUMP)
    {
        Player.anim_frame++;
        Player.current_framerate = 0;
    }
    Player.current_framerate++;

    if (Player.anim_frame > PLAYER_FRAME_WALL_JUMP_2)
    {
        Player.anim_frame = PLAYER_FRAME_WALL_JUMP_1; 
    }

    drawSpriteSheet(Player.pos.pX - cameraX + PLAYER_X_OFFSET, Player.pos.pY - cameraY + PLAYER_Y_OFFSET, PLAYER_WIDTH, PLAYER_HEIGHT, player_sheet.pixel_data, Player.anim_frame, Player.pos.direction);
}

void anim_player_double_jump()
{
    if (Player.current_framerate == PLAYER_FRAMERATE_DOUBLE_JUMP)
    {
        Player.anim_frame++;
        Player.current_framerate = 0;
    }
    Player.current_framerate++;

    if (Player.anim_frame > PLAYER_FRAME_SALTO_4)
    {
        Player.anim_frame = PLAYER_FRAME_SALTO_1; 
    }

    drawSpriteSheet(Player.pos.pX - cameraX + PLAYER_X_OFFSET, Player.pos.pY - cameraY + PLAYER_Y_OFFSET, PLAYER_WIDTH, PLAYER_HEIGHT, player_sheet.pixel_data, Player.anim_frame, Player.pos.direction);
}

void anim_player_jump()
{
    if (Player.current_framerate == PLAYER_FRAMERATE_JUMP)
    {
        Player.anim_frame++;
        Player.current_framerate = 0;
    }
    Player.current_framerate++;

    if (Player.anim_frame > PLAYER_FRAME_JUMP_4)
    {
        Player.anim_frame = PLAYER_FRAME_JUMP_4; //On reste sur 5
    }

    drawSpriteSheet(Player.pos.pX - cameraX + PLAYER_X_OFFSET, Player.pos.pY - cameraY + PLAYER_Y_OFFSET, PLAYER_WIDTH, PLAYER_HEIGHT, player_sheet.pixel_data, Player.anim_frame, Player.pos.direction);
}

void anim_player_falling()
{

    if (Player.current_framerate == PLAYER_FRAMERATE_FALLING)
    {
        Player.anim_frame++;
        Player.current_framerate = 0;
    }
    Player.current_framerate++;

    if (Player.anim_frame > PLAYER_FRAME_FALLING_2)
    {
        Player.anim_frame = PLAYER_FRAME_FALLING_1;
    }

    drawSpriteSheet(Player.pos.pX - cameraX + PLAYER_X_OFFSET, Player.pos.pY - cameraY + PLAYER_Y_OFFSET, PLAYER_WIDTH, PLAYER_HEIGHT, player_sheet.pixel_data, Player.anim_frame, Player.pos.direction);
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
    //    drawSprite(Player.pos.pX - cameraX, Player.pos.pY - cameraY + PLAYER_Y_OFFSET, PLAYER_WIDTH, player_height, player_die_bits, Player.pos.direction);
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
            parts.createExplosion(random(10, SCREEN_WIDTH - 10) + cameraX, 10 + (rand() % 2 * SCREEN_HEIGHT / 3) + cameraY, 100 + rand() % 50, random(65535));
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
    //    drawSprite(Player.pos.pX - cameraX, Player.pos.pY - cameraY + PLAYER_Y_OFFSET, PLAYER_WIDTH, player_height, player4, Player.pos.direction);
      drawSprite(Player.pos.pX - cameraX, Player.pos.pY - cameraY, player_idle1.width, player_idle1.height, player_idle1.pixel_data, Player.pos.direction);
    break;
  case PLAYER_FRAME_WIN_2:
    //drawSprite(Player.pos.pX - cameraX, Player.pos.pY - cameraY + PLAYER_Y_OFFSET, PLAYER_WIDTH, player_height, player2, Player.pos.direction);
      drawSprite(Player.pos.pX - cameraX, Player.pos.pY - cameraY, player_run1.width, player_run1.height, player_run1.pixel_data, Player.pos.direction);

    break;
  case PLAYER_FRAME_WIN_3:
    //    drawSprite(Player.pos.pX - cameraX, Player.pos.pY - cameraY + PLAYER_Y_OFFSET, PLAYER_WIDTH, player_height, player3, Player.pos.direction);
      drawSprite(Player.pos.pX - cameraX, Player.pos.pY - cameraY, player_run2.width, player_run2.height, player_run2.pixel_data, Player.pos.direction);
    break;
  }*/
}

void anim_player_fx_dust()
{
    if (Player.FX.current_framerate == FX_FRAMERATE_DUST)
    {
        Player.FX.anim_frame++;
        Player.FX.current_framerate = 0;
    }
    Player.FX.current_framerate++;

    if (Player.FX.anim_frame > FX_FRAME_DUST_5)
    {
        //Annulation de l'anim
        Player.FX.stateAnim = FX_NONE;
        return;
    }

    drawSpriteSheet(Player.FX.pX - cameraX, Player.FX.pY - cameraY + (16 - JUMP_DUST_VERTICAL_HEIGHT), JUMP_DUST_VERTICAL_WIDTH, JUMP_DUST_VERTICAL_HEIGHT, jump_dust_vertical.pixel_data, Player.FX.anim_frame, Player.pos.direction);
}

void anim_player_fx_splash()
{
    if (Player.FX.current_framerate == FX_FRAMERATE_SPLASH)
    {
        Player.FX.anim_frame++;
        Player.FX.current_framerate = 0;
    }
    Player.FX.current_framerate++;

    if (Player.FX.anim_frame > FX_FRAME_SPLASH_5)
    {
        //Annulation de l'anim
        Player.FX.stateAnim = FX_NONE;
        return;
    }

    int baseY = (16 - fx_splash1.height);

    switch (Player.FX.anim_frame)
    {
    case FX_FRAME_SPLASH_1:
        drawSprite(Player.FX.pX - cameraX, Player.FX.pY - cameraY + baseY, fx_splash1.width, fx_splash1.height, fx_splash1.pixel_data, Player.FX.direction);
        break;
    case FX_FRAME_SPLASH_2:
        drawSprite(Player.FX.pX - cameraX, Player.FX.pY - cameraY + baseY, fx_splash2.width, fx_splash2.height, fx_splash2.pixel_data, Player.FX.direction);
        break;
    case FX_FRAME_SPLASH_3:
        drawSprite(Player.FX.pX - cameraX, Player.FX.pY - cameraY + baseY, fx_splash3.width, fx_splash3.height, fx_splash3.pixel_data, Player.FX.direction);
        break;
    case FX_FRAME_SPLASH_4:
        drawSprite(Player.FX.pX - cameraX, Player.FX.pY - cameraY + baseY, fx_splash4.width, fx_splash4.height, fx_splash4.pixel_data, Player.FX.direction);
        break;
    case FX_FRAME_SPLASH_5:
        drawSprite(Player.FX.pX - cameraX, Player.FX.pY - cameraY + baseY, fx_splash5.width, fx_splash5.height, fx_splash5.pixel_data, Player.FX.direction);
        break;
    }
}

#if 0

void anim_player_fx_dust()
{
    //Pas vraiment une anim mais des particules, principe identique cependant
    if (Player.FX.current_framerate == FX_FRAMERATE_DUST)
    {
        Player.FX.anim_frame++;
        Player.FX.current_framerate = 0;
    }
    Player.FX.current_framerate++;

    if (Player.FX.anim_frame > FX_FRAME_DUST_1)
    {
        //Annulation de l'anim
        Player.FX.stateAnim = FX_NONE;
        return;
    }

    switch (Player.FX.anim_frame)
    {
    case FX_FRAME_DUST_1:
        //Dust
        parts.createLandingDust(Player.FX.pX, Player.FX.pY, 8, Player.FX.speedX, Player.FX.speedY, FX_FRAMERATE_DUST);
        break;
    }
}

#endif
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

    drawSpriteSheet(Player.FX.pX - cameraX, Player.FX.pY - cameraY + (16 - JUMP_DUST_VERTICAL_HEIGHT), JUMP_DUST_VERTICAL_WIDTH, JUMP_DUST_VERTICAL_HEIGHT, jump_dust_vertical.pixel_data, Player.FX.anim_frame, Player.pos.direction);
}

void displayGame()
{
    //canvas->fillScreen(0x867D); //84ceef

    drawWorld();

    //arcada.display->display();
    arcada.blitFrameBuffer(0, 0, false, false);
}
