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

//On sauvegarde la hauteur courante dans l'entete de notre colonne de WORLD
void updateHauteurColonne(int x, int y)
{
    //on  cherche la nouvelle hauteur
    int newH = 0;
    for (int wY = 0; wY < WORLD_HEIGHT; wY++)
    {
        if (!WORLD[wY][x].attr.traversable)
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
    Player.pos.pX = Player.pos.worldX * 16;
    Player.pos.pY = Player.pos.worldY * 16;
    Player.pos.speedX = 0;
    Player.pos.speedY = 0;
    Player.pos.newX = Player.pos.pX;
    Player.pos.newY = Player.pos.pY;

    Player.pos.direction = 1;
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

    Player.bDying = false;
    Player.bJumping = false;
    Player.jumpCounter = 0;
    Player.bDoubleJumping = false;
    Player.bFalling = false;
    Player.bWalking = false;
    Player.bTouched = false;
    Player.iTouchCountDown = 0;
    Player.bOnGround = false;
    Player.onGroundCounter = 0;

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

    counterActionB = 0;
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

void set_double_jump_fx(bool bForced = false)
{
    if (Player.FX.stateAnim != FX_DOUBLE_JUMP)
    {
        Player.FX.stateAnim = FX_DOUBLE_JUMP;
        Player.FX.anim_frame = 0;
        //on le met sur le framerate final pour quil apparaissent de suite
        Player.FX.current_framerate = FX_FRAMERATE_DOUBLE_JUMP;
        Player.FX.direction = Player.pos.direction;
        //        Player.FX.speedX = 0;
        //        Player.FX.speedY = 0;
        Player.FX.pX = Player.pos.pX;
        Player.FX.pY = Player.pos.pY;
    }
}

void set_dust_fx(void)
{
    if (Player.FX.stateAnim != FX_DUST)
    {
        Player.FX.stateAnim = FX_DUST;
        Player.FX.anim_frame = 0;
        Player.FX.current_framerate = 0;
        Player.FX.direction = Player.pos.direction;
        Player.FX.speedY = -2;
        Player.FX.speedX = 0;
        Player.FX.pX = Player.pos.pX + 8;
        Player.FX.pY = Player.pos.pY + 15;
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

void postInitWorld()
{
    //Update necessaires au jeu apres init ou load
    //....recalcul hauteur, etc..
    int wY = WORLD_HEIGHT - 1;
    hauteurMaxBackground = 0;
    do
    {
        for (int wX = 0; wX < WORLD_WIDTH; wX++)
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

    
    //Musique
    StopMOD();
    PlayMOD(pmf_ninja);
}

void initWorld()
{
    //MASQUE LUMIERE PLAYER
    int centerX = PLAYER_LIGHT_MASK_WIDTH / 2;
    int centerY = PLAYER_LIGHT_MASK_HEIGHT / 2;

    for (int wY = 0; wY < PLAYER_LIGHT_MASK_HEIGHT; wY++)
    {
        for (int wX = 0; wX < PLAYER_LIGHT_MASK_WIDTH; wX++)
        {
            //Ray cast du player vers decor ?
            float distPlayer = sqrt((centerX - wX) * (centerX - wX) + (centerY - wY) * (centerY - wY));
            //Serial.printf("player:%d,%d tile:%d,%d dist:%f\n", playerLightX, playerLightY, px, py,  distPlayer);
            // @todo gerer la non propagassion de la lumiere dans les murs...
            if (distPlayer > 0)
                playerLightMask[wY][wX] = (PLAYER_LIGHT_INTENSITY /(distPlayer * distPlayer));
            else
                playerLightMask[wY][wX] = PLAYER_LIGHT_INTENSITY;
        }
    }

    //uint8_t zeNoise[WORLD_WIDTH];
    memset(WORLD, 0, sizeof(WORLD));
    SimplexNoise noiseGen;

    int minProfondeur = 999;
    int maxProfondeur = 0;
    for (int wX = 0; wX < WORLD_WIDTH; wX++)
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

        for (int wY = 0; wY < WORLD_HEIGHT; wY++)
        {
            //Le ciel
            if (wY < rowGround)
            {
                WORLD[wY][wX].id = BLOCK_AIR;
                WORLD[wY][wX].attr.traversable = 1;
                WORLD[wY][wX].attr.opaque = 0;
                WORLD[wY][wX].attr.life = BLOCK_LIFE_NA;
            }
            else if (wY == rowGround)
            {
                WORLD[wY][wX].id = BLOCK_GROUND_TOP;
                WORLD[wY][wX].attr.life = BLOCK_LIFE_1;
                WORLD[wY][wX].attr.traversable = 0;
                WORLD[wY][wX].attr.opaque = 1;
            }
            else if (wY <= rowGround + 3) //entre surface et sous sol profond
            {
                WORLD[wY][wX].id = BLOCK_GROUND;
                WORLD[wY][wX].attr.life = BLOCK_LIFE_1;
                WORLD[wY][wX].attr.traversable = 0;
                WORLD[wY][wX].attr.opaque = 1;
            }
            else    //sous sol profond
            {
                WORLD[wY][wX].id = BLOCK_ROCK;
                WORLD[wY][wX].attr.life = BLOCK_LIFE_1;
                WORLD[wY][wX].attr.traversable = 0;
                WORLD[wY][wX].attr.opaque = 1;
            }
        }
    }
    float minN = 999999;
    float maxN = -99999;
    //Update bricks to cool rendering
    for (int wX = 0; wX < WORLD_WIDTH; wX++)
    {
        int curProdondeur = WORLD[HEADER_ROW][wX].id;
        curProdondeur = max(4, curProdondeur);

        for (int wY = curProdondeur - 4; wY < (WORLD_HEIGHT - 1); wY++)
        {
            //            float noise = noiseGen.fractal(8, (float)16*wX / (float)WORLD_WIDTH, (float)16*wY / (float)WORLD_HEIGHT);
            float noise = SimplexNoise::noise((float)16 * wX / (float)WORLD_WIDTH, (float)16 * wY / (float)WORLD_HEIGHT);
            int16_t densite = int(noise * AMPLITUDE_DENSITE);
            if ( wY <= curProdondeur+3 )
            {
                if (densite > MAX_DENSITE)
                {
                    WORLD[wY][wX].id = BLOCK_AIR;
                    WORLD[wY][wX].attr.traversable = 1;
                    WORLD[wY][wX].attr.opaque = 0;
                    WORLD[wY][wX].attr.life = BLOCK_LIFE_NA;
                }
            }
            else
            {
                if (abs(densite) > MAX_DENSITE || densite == 0)
                {
                    if (WORLD[wY][wX].id != BLOCK_AIR)
                    {
                        WORLD[wY][wX].id = BLOCK_UNDERGROUND_AIR;
                        WORLD[wY][wX].attr.traversable = 1;
                        WORLD[wY][wX].attr.opaque = 1;
                        WORLD[wY][wX].attr.life = BLOCK_LIFE_NA;
                    }
                }
                else
                {
                    WORLD[wY][wX].id = BLOCK_ROCK;
                    WORLD[wY][wX].attr.life = BLOCK_ROCK_DENSITY;
                    WORLD[wY][wX].attr.traversable = 0;
                    WORLD[wY][wX].attr.opaque = 1;
                    if (wY >= curProdondeur + 5)
                    {
                        int randSeed = rand() % 100;
                        if (wY <= curProdondeur + 12)
                        {
                            if (randSeed < 2)
                            {
                                WORLD[wY][wX].id = BLOCK_DIAMANT;
                                WORLD[wY][wX].attr.life = BLOCK_DIAMANT_DENSITY;
                                continue;
                            }

                            if (randSeed <= 20)
                            {
                                WORLD[wY][wX].id = BLOCK_REDSTONE;
                                WORLD[wY][wX].attr.life = BLOCK_REDSTONE_DENSITY;
                                continue;
                            }
                        }
                        else if (wY <= curProdondeur + 29)
                        {
                            if (randSeed < 2)
                            {
                                WORLD[wY][wX].id = BLOCK_JADE;
                                WORLD[wY][wX].attr.life = BLOCK_JADE_DENSITY;
                                continue;
                            }
                            else if (randSeed <= 4)
                            {
                                WORLD[wY][wX].id = BLOCK_OR;
                                WORLD[wY][wX].attr.life = BLOCK_OR_DENSITY;
                                continue;
                            }
                            else if (randSeed <= 14)
                            {
                                WORLD[wY][wX].id = BLOCK_CUIVRE;
                                WORLD[wY][wX].attr.life = BLOCK_CUIVRE_DENSITY;
                                continue;
                            }
                        }
                        else if (wY <= curProdondeur + 54)
                        {
                            if (randSeed < 20)
                            {
                                WORLD[wY][wX].id = BLOCK_FER;
                                WORLD[wY][wX].attr.life = BLOCK_FER_DENSITY;
                                continue;
                            }
                            else if (randSeed < 50)
                            {
                                WORLD[wY][wX].id = BLOCK_CHARBON;
                                WORLD[wY][wX].attr.life = BLOCK_CHARBON_DENSITY;
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
            if (!WORLD[wY][wX].attr.traversable)
            {
                //Sauvegarde de la hauteur courante et de la hauteur originelle
                WORLD[HEADER_ROW][wX].id = wY;
                WORLD[REF_ROW][wX].id = wY;
#if 0
                WORLD[wY][wX].id = BLOCK_GROUND_TOP;
                WORLD[wY][wX].attr.traversable = 0;
                WORLD[wY][wX].attr.life = BLOCK_LIFE_1;
                WORLD[wY][wX].attr.opaque = 1;

                //Si possible on change la tile juste en dessous pour du ground
                if (wY + 1 < (WORLD_HEIGHT - 1))
                {
                    WORLD[wY + 1][wX].id = BLOCK_GROUND;
                    WORLD[wY + 1][wX].attr.traversable = 0;
                    WORLD[wY + 1][wX].attr.life = BLOCK_LIFE_1;
                    WORLD[wY + 1][wX].attr.opaque = 1;
                }
                //Et celle du dessus par de l'herbe
                if (wY - 1 >= 0)
                {
                    //12 pourcent de chance de mettre de l'herbe
                    if (rand() % 100 < 12)
                    {
                        WORLD[wY - 1][wX].id = BLOCK_GRASS;
                        WORLD[wY - 1][wX].attr.life = BLOCK_LIFE_NA;
                        WORLD[wY - 1][wX].attr.traversable = 1;
                        WORLD[wY - 1][wX].attr.opaque = 1;
                        WORLD[wY - 1][wX].attr.spriteVariation = rand() % 3;
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
        WORLD[(curP - 1)][4 + nbW].attr.Level = MAX_WATER_LEVEL;
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

        } while (!tile.attr.traversable && !tileG.attr.traversable && !tileD.attr.traversable &&
                 !tile1.attr.traversable && !tileG1.attr.traversable && !tileD1.attr.traversable &&
                 !tile2.attr.traversable && !tileG2.attr.traversable && !tileD2.attr.traversable &&
                 !tile.attr.Level == 0 && !tileG.attr.Level == 0 && !tileD.attr.Level == 0 &&
                 !tileGG.attr.traversable && !tileDD.attr.traversable && nbSearch < 50);

        if (bFound)
        {
            WORLD[hauteur - 1][posX].id = BLOCK_TREE;
            WORLD[hauteur - 1][posX].attr.life = BLOCK_LIFE_1;
            WORLD[hauteur - 1][posX].attr.traversable = 1;
            WORLD[hauteur - 1][posX].attr.opaque = 1;
            WORLD[hauteur - 1][posX].attr.spriteVariation = rand() % 3;
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
        } while (!tile.attr.traversable && !tileG.attr.traversable && !tileD.attr.traversable &&
                 !tile.attr.Level && !tileG.attr.Level && !tileD.attr.Level);

        //Skel
        ENNEMIES[NB_WORLD_ENNEMIES].bIsAlive = 255;
        ENNEMIES[NB_WORLD_ENNEMIES].worldX = posX;
        ENNEMIES[NB_WORLD_ENNEMIES].worldY = hauteur - 1;
        ENNEMIES[NB_WORLD_ENNEMIES].x = ENNEMIES[NB_WORLD_ENNEMIES].worldX * 16;
        ENNEMIES[NB_WORLD_ENNEMIES].y = ENNEMIES[NB_WORLD_ENNEMIES].worldY * 16;
        ENNEMIES[NB_WORLD_ENNEMIES].new_x = 0;
        ENNEMIES[NB_WORLD_ENNEMIES].new_y = 0;
        ENNEMIES[NB_WORLD_ENNEMIES].type = SKEL_ENNEMY;
        ENNEMIES[NB_WORLD_ENNEMIES].speed_x = SKEL_WALKING_SPEED;
        ENNEMIES[NB_WORLD_ENNEMIES].speed_y = 0;
        ENNEMIES[NB_WORLD_ENNEMIES].max_frames = 4;
        ENNEMIES[NB_WORLD_ENNEMIES].nb_anim_frames = 3;

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
        } while (!tile.attr.traversable && !tileG.attr.traversable && !tileD.attr.traversable &&
                 !tile.attr.Level && !tileG.attr.Level && !tileD.attr.Level);

        ENNEMIES[NB_WORLD_ENNEMIES].bIsAlive = 255;
        ENNEMIES[NB_WORLD_ENNEMIES].worldX = posX;
        ENNEMIES[NB_WORLD_ENNEMIES].worldY = hauteur - 1;
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
                    if (WORLD[wY][wX].attr.traversable && WORLD[wY][wX].attr.Level == 0 && WORLD[wY][wX + 1].attr.traversable && WORLD[wY][wX - 1].attr.traversable && WORLD[wY - 1][wX].attr.traversable)
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

    Serial.printf("NB_WORLD_ENNEMIES:%d\n", NB_WORLD_ENNEMIES);
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
    case FX_DUST:
    {
        anim_player_fx_dust();
        break;
    }
    }
}

void createDropFrom(int wX, int wY, uint8_t type)
{
    ITEMS[CURRENT_QUEUE_ITEMS].bIsAlive = 255;
    ITEMS[CURRENT_QUEUE_ITEMS].bIsActive = false;
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

    switch (currentEnnemy->type)
    {
    case SPIDER_ENNEMY:
        parts.createExplosion(px, py, 16, ARCADA_BLACK);
        break;
    case ZOMBI_ENNEMY:
        parts.createExplosion(px, py, 16, ARCADA_DARKGREEN);
        break;
    case SKEL_ENNEMY:
        parts.createExplosion(px, py, 16, ARCADA_WHITE);
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
        case SKEL_ENNEMY:
        {
            int DIR = currentEnnemy->speed_x > 0 ? 1 : -1;
            switch (currentEnnemy->anim_frame)
            {
            case 1:
                drawSprite(px, py, skel_walk1.width, skel_walk1.height, skel_walk1.pixel_data, DIR);
                break;
            case 2:
                drawSprite(px, py, skel_walk2.width, skel_walk2.height, skel_walk2.pixel_data, DIR);
                break;
            case 3:
                drawSprite(px, py, skel_walk3.width, skel_walk3.height, skel_walk3.pixel_data, DIR);
                break;
            }
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
                drawEnnemy(currentEnnemy);
            }
        }
    }
}

const unsigned char *getGroundBlockData(uint8_t value)
{
    const unsigned char *pixel_data = NULL;

    switch(value) {
        case BLOCK_GROUND:
            pixel_data = ground_00.pixel_data;
            break;
        case BLOCK_GROUND+0x01:
            pixel_data = ground_01.pixel_data;
            break;
        case BLOCK_GROUND+0x02:
            pixel_data = ground_02.pixel_data;
            break;
        case BLOCK_GROUND+0x03:
            pixel_data = ground_03.pixel_data;
            break;
        case BLOCK_GROUND+0x04:
            pixel_data = ground_04.pixel_data;
            break;
        case BLOCK_GROUND+0x05:
            pixel_data = ground_05.pixel_data;
            break;
        case BLOCK_GROUND+0x06:
            pixel_data = ground_06.pixel_data;
            break;
        case BLOCK_GROUND+0x07:
            pixel_data = ground_07.pixel_data;
            break;
        case BLOCK_GROUND+0x08:
            pixel_data = ground_08.pixel_data;
            break;
        case BLOCK_GROUND+0x09:
            pixel_data = ground_09.pixel_data;
            break;
        case BLOCK_GROUND+0x0A:
            pixel_data = ground_0A.pixel_data;
            break;
        case BLOCK_GROUND+0x0B:
            pixel_data = ground_0B.pixel_data;
            break;
        case BLOCK_GROUND+0x0C:
            pixel_data = ground_0C.pixel_data;
            break;
        case BLOCK_GROUND+0x0D:
            pixel_data = ground_0D.pixel_data;
            break;
        case BLOCK_GROUND+0x0E:
            pixel_data = ground_0E.pixel_data;
            break;
        case BLOCK_GROUND+0x0F:
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

                    int curLight = MAX_LIGHT_INTENSITY;
                    // @todo gerer la nuit (dans ce cas pas de max light et ground.. ou la lune ?)
                    if (brick->attr.opaque) // || wY > profondeurColonne)
                    {
                        if (profondeurColonne != 0)
                        {
                            int delta = (wY - profondeurColonne) + 1;
                            if (delta > 1)
                                curLight = curLight - (33*delta);

                            curLight = max(curLight, 0);
                        }

                        //if (wY <= (Player.pos.worldY + 2))
                        {
                            if (wX >= playerLightStartX && wX <= playerLightEndX && wY >= playerLightStartY && wY <= playerLightEndY)
                            {
                                //Raycast
                                if (checkCollisionTo(Player.pos.worldX, Player.pos.worldY, wX, wY))
                                    curLight = curLight + playerLightMask[wY - playerLightStartY][wX - playerLightStartX];
                            }
                        }
                        //   curLight = curLight + AMBIENT_LIGHT_INTENSITY;
                        curLight = min(curLight, MAX_LIGHT_INTENSITY);
                    }
#ifdef DEBUG
                    curLight = MAX_LIGHT_INTENSITY;
#endif

                    if (value == BLOCK_AIR)
                    {
                        if (brick->attr.Level > 0)
                        {
                            bool bOnSurface = false;
                            if (wY > 0)
                                bOnSurface = (WORLD[wY - 1][wX].attr.Level == 0); // && WORLD[wY - 1][wX].attr.traversable);

                            drawWaterTile(px, py, NULL, curLight, brick->attr.Level, bOnSurface);
                        }
                        //rien le fond
                    }
                    else if (value == BLOCK_UNDERGROUND_AIR)
                    {
                        //background de terrassement
                        if (brick->attr.Level > 0)
                        {
                            bool bOnSurface = false;
                            if (wY > 0)
                                bOnSurface = (WORLD[wY - 1][wX].attr.Level == 0); // && WORLD[wY - 1][wX].attr.traversable);

                            drawWaterTile(px, py, back_ground.pixel_data, curLight, brick->attr.Level, bOnSurface);
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
                        if (brick->attr.Level > 0)
                        {
                            bool bOnSurface = false;
                            if (wY > 0)
                                bOnSurface = ((WORLD[wY - 1][wX].attr.Level == 0) && WORLD[wY - 1][wX].attr.traversable);
                            
                            if (brick->attr.spriteVariation == 0)
                                drawWaterTileMask(px, py, grass1.pixel_data, curLight, brick->attr.Level, bOnSurface);
                            else if (brick->attr.spriteVariation == 1)
                                drawWaterTileMask(px, py, grass2.pixel_data, curLight, brick->attr.Level, bOnSurface);
                            else
                                drawWaterTileMask(px, py, grass3.pixel_data, curLight, brick->attr.Level, bOnSurface);

                        }
                        else
                        {
                            if (brick->attr.spriteVariation == 0)
                                drawTileMask(px, py, grass1.pixel_data, curLight);
                            else if (brick->attr.spriteVariation == 1)
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
                    else if (value == BLOCK_DIAMANT) //
                    {
                        drawTile(px, py, rock_diamant.pixel_data, curLight);
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

                    int curLight = MAX_LIGHT_INTENSITY;
                    // @todo gerer la nuit (dans ce cas pas de max light et ground.. ou la lune ?)
                    if (brick->attr.opaque) // || wY > profondeurColonne)
                    {
                        if (profondeurColonne != 0)
                        {
                            int delta = (wY - profondeurColonne) + 1;
                            if (delta > 1)
                                curLight = curLight / delta;

                            curLight = max(curLight, 0);
                        }

                        //if (wY <= (Player.pos.worldY + 2))
                        {
                            if (wX >= playerLightStartX && wX <= playerLightEndX && wY >= playerLightStartY && wY <= playerLightEndY)
                            {
                                //Raycast
                                if (checkCollisionTo(Player.pos.worldX, Player.pos.worldY, wX, wY))
                                    curLight = curLight + playerLightMask[wY - playerLightStartY][wX - playerLightStartX];
                            }
                        }
                        //   curLight = curLight + AMBIENT_LIGHT_INTENSITY;
                        curLight = min(curLight, MAX_LIGHT_INTENSITY);
                    }
#ifdef DEBUG
                    curLight = MAX_LIGHT_INTENSITY;
#endif
                 /*   if (value == BLOCK_GRASS) //
                    {                        
                        if (brick->attr.Level > 0)
                        {
                            bool bOnSurface = false;
                            if (wY > 0)
                                bOnSurface = ((WORLD[wY - 1][wX].attr.Level == 0) && WORLD[wY - 1][wX].attr.traversable);
                            
                            if (brick->attr.spriteVariation == 0)
                                drawWaterTileMask(px, py, grass1.pixel_data, curLight, brick->attr.Level, bOnSurface);
                            else if (brick->attr.spriteVariation == 1)
                                drawWaterTileMask(px, py, grass2.pixel_data, curLight, brick->attr.Level, bOnSurface);
                            else
                                drawWaterTileMask(px, py, grass3.pixel_data, curLight, brick->attr.Level, bOnSurface);

                        }
                        else
                        {
                            if (brick->attr.spriteVariation == 0)
                                drawTileMask(px, py, grass1.pixel_data, curLight);
                            else if (brick->attr.spriteVariation == 1)
                                drawTileMask(px, py, grass2.pixel_data, curLight);
                            else
                                drawTileMask(px, py, grass3.pixel_data, curLight);
                        }
                    }
                    else */
                    if (value == BLOCK_TREE) //
                    {
                        int realpx = px;
                        int realpy = py;
                        if (brick->attr.hit)
                        {
                            realpx += random(-itemShakeAmount, itemShakeAmount);
                            realpy += random(-(itemShakeAmount >> 1), itemShakeAmount >> 1);
                        }
                        if (brick->attr.spriteVariation == 0)
                            drawTreeTileMask(realpx, realpy, tree1.pixel_data, tree1.width, tree1.height, curLight);
                        else if (brick->attr.spriteVariation == 1)
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

        int w = parts.particles[i].w;
        int h = parts.particles[i].h;

        if (w == 1 & h == 1)
            canvas->drawPixel(x, y, parts.particles[i].color);
        else
            canvas->fillRect(x, y, w, h, parts.particles[i].color);
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

void drawEffects()
{
    // @todo : pas vraiment ici que ce devrait etre fait...
    if (counterActionB > 0)
    {
        if (currentTileTarget.tile->id != BLOCK_AIR && currentTileTarget.tile->id != BLOCK_UNDERGROUND_AIR && currentTileTarget.pX > -16 && currentTileTarget.pX < ARCADA_TFT_WIDTH && currentTileTarget.pY > -16 && currentTileTarget.pY < ARCADA_TFT_HEIGHT)
        {
            //Test action pour le sprite, pour l'instant la pioche
            // drawSprite(currentTileTarget.pX, currentTileTarget.pY, pioche.width, pioche.height, pioche.pixel_data, Player.pos.direction);
            if ((counterActionB % FRAMES_ANIM_ACTION_B) == 0)
            {
                parts.createExplosion(currentTileTarget.pX + 8, currentTileTarget.pY + 8, 8, currentTileTarget.itemColor); //@todo colorer avec la couleur de la brique en cours de travail
                sndPlayerCanal1.play(AudioSamplePioche);

                currentTileTarget.tile->attr.hit = 1;
                itemShakeAmount = DEFAULT_ITEM_SHAKE_AMOUNT;
            }
        }
    }
    if (itemShakeAmount > 0)
        itemShakeAmount--;

    if (cameraShakeAmount > 0)
    {
        cameraShakeAmount--;
    }
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

    // debugInfos();
}

void pixelToWorld(int *pX, int *pY)
{
    int wX = *pX / 16;
    int wY = *pY / 16;
}

/* Adapted from the code displayed at RogueBasin's "Bresenham's Line
 * Algorithm" article, this function checks for an unobstructed line
 * of sight between two locations using Bresenham's line algorithm to
 * draw a line from one point to the other. Returns true if there is
 * line of sight, false if there is no line of sight. */
#define LOS_DISTANCE 9
#if 0
int los (int los_x_1, int los_y_1, int los_x_2, int
         los_y_2, int level) {
   int delta_x, delta_y, move_x, move_y, error;
 
   /* Calculate deltas. */
   delta_x = abs (los_x_2 - los_x_1) << 1;
   delta_y = abs (los_y_2 - los_y_1) << 1;
 
   /* Calculate signs. */
   move_x = los_x_2 >= los_x_1 ? 1 : -1;
   move_y = los_y_2 >= los_y_1 ? 1 : -1;
 
   /* There is an automatic line of sight, of course, between a
    * location and the same location or directly adjacent
    * locations. */
   if (abs (los_x_2 - los_x_1) < 2 && abs (los_y_2 - los_y_1) < 2) {
      /* Return. */
      return true;
   }
 
   /* Ensure that the line will not extend too long. */
   if (((los_x_2 - los_x_1) * (los_x_2 - los_x_1))
       + ((los_y_2 - los_y_1) * (los_y_2 -
                                 los_y_1)) >
       LOS_DISTANCE * LOS_DISTANCE) {
      /* Return. */
      return false;
   }
 
   /* "Draw" the line, checking for obstructions. */
   if (delta_x >= delta_y) {
      /* Calculate the error factor, which may go below zero. */
      error = delta_y - (delta_x >> 1);
 
      /* Search the line. */
      while (los_x_1 != los_x_2) {
         /* Check for an obstruction. If the obstruction can be "moved
          * around", it isn't really an obstruction. */
         if (feature_data(dungeon (los_x_1, los_y_1, level).feature).obstruction &&
             (((los_y_1 - move_y >= 1
                && los_y_1 - move_y <= DUNGEON_HEIGHT)
               &&
               feature_data (dungeon
                                    (los_x_1, los_y_1 - move_y,
                                     level).feature).obstruction)
              || (los_y_1 != los_y_2 || !(delta_y)))) {
            /* Return. */
            return false;
         }
 
         /* Update values. */
         if (error > 0) {
            if (error || (move_x > 0)) {
               los_y_1 += move_y;
               error -= delta_x;
            }
         }
         los_x_1 += move_x;
         error += delta_y;
      }
   }
   else {
      /* Calculate the error factor, which may go below zero. */
      error = delta_x - (delta_y >> 1);
 
      /* Search the line. */
      while (los_y_1 != los_y_2) {
         /* Check for an obstruction. If the obstruction can be "moved
          * around", it isn't really an obstruction. */
         if (feature_data
             (dungeon (los_x_1, los_y_1, level).feature).obstruction
             &&
             (((los_x_1 - move_x >= 1
                && los_x_1 - move_x <= DUNGEON_WIDTH)
               &&
               feature_data (dungeon
                                    (los_x_1 - move_x, los_y_1,
                                     level).feature).obstruction)
              || (los_x_1 != los_x_2 || !(delta_x)))) {
            /* Return. */
            return false;
         }
 
         /* Update values. */
         if (error > 0) {
            if (error || (move_y > 0)) {
               los_x_1 += move_x;
               error -= delta_y;
            }
         }
         los_y_1 += move_y;
         error += delta_x;
      }
   }
 
   /* Return. */
   return true;
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
bool checkCollisionTo(int origin_x, int origin_y, int dest_x, int dest_y)
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
        } while (WORLD[y][x].attr.traversable == true);

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
        } while (WORLD[y][x].attr.traversable == true);
        return false;
    }
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
    if (cameraShakeAmount > 0)
        cameraX += random(-cameraShakeAmount, cameraShakeAmount);        

    cameraX = max(cameraX, 0);
    cameraX = min(cameraX, ((WORLD_WIDTH - 1) * 16) - ARCADA_TFT_WIDTH);

    cameraY = Player.pos.pY - (ARCADA_TFT_HEIGHT / 2);
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

    //Utile pour dessiner les sprites a l'ecran
    worldOffset_pX = (worldMIN_X * 16) + currentOffset_X;
    worldOffset_pY = (worldMIN_Y * 16) + currentOffset_Y;
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
                        sndPlayerCanal3.play(AudioSample__Kick);
                        SCORE += 5;
                        killEnnemy(currentEnnemy, px, py);
                        cameraShakeAmount = CAMERA_KILL_ENNEMY_AMOUNT;
                        //rebond ?
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

        if ((brique_FRONT.attr.traversable) &&
            ((Player.pos.direction > 0 && (Player.pos.pX < ((WORLD_WIDTH - 1) * 16) - ARCADA_TFT_WIDTH)) ||
             (Player.pos.direction < 0 && (Player.pos.pX >= 2))))
        {
            Player.pos.pX += 1;
            Player.bWalking = true;
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
    //Serial.printf("counterActionB:%d Player.bJumping:%d Player.bFalling:%d bOnGround:%d Player.bWantWalk:%d Player.bWalking:%d\n", counterActionB, Player.bJumping, Player.bFalling, bOnGround, Player.bWantWalk, Player.bWalking);
    if (counterActionB > 0)
    {
        //@todo : Test type action,
        //pour le moment dig
        set_digging();
    }
    else
    {
        if (Player.bJumping || Player.bDoubleJumping)
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
                    if (Player.bWalking) //On a bougé
                    {
                        set_walking();
                        set_dust_fx();
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
                    currentEnnemy->bWalking = true;

                    currentEnnemy->new_x = min(((WORLD_WIDTH - 1) * 16), currentEnnemy->new_x);
                }
                else if (currentEnnemy->speed_x < 0)
                {
                    currentEnnemy->new_x = currentEnnemy->x + currentEnnemy->speed_x;
                    currentEnnemy->bWalking = true;

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
        if ((!tileTL.attr.traversable) || (!tileBL.attr.traversable))
        {
            currentEnnemy->new_x = (currentEnnemy->new_x - (currentEnnemy->new_x % 16)) + 16;
            currentEnnemy->speed_x = -currentEnnemy->speed_x;
        }
    }
    else
    {
        TworldTile tileTR = getWorldAtPix(currentEnnemy->new_x + 16, currentEnnemy->y);
        TworldTile tileBR = getWorldAtPix(currentEnnemy->new_x + 16, currentEnnemy->y + 15);
        if ((!tileTR.attr.traversable) || (!tileBR.attr.traversable))
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
        if ((!tileTL.attr.traversable) || (!tileTR.attr.traversable))
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
        if ((!tileBL.attr.traversable) || (!tileBR.attr.traversable))
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
        if ((distX < WORLD_WIDTH / 2) && (distY < WORLD_HEIGHT / 2))
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
            else if (currentEnnemy->type == SKEL_ENNEMY)
            {
                if ((currentEnnemy->speed_x < 0) && currentEnnemy->x <= 0)
                    currentEnnemy->speed_x = SKEL_WALKING_SPEED;
                else if ((currentEnnemy->speed_x > 0) && currentEnnemy->x >= WORLD_WIDTH * 16)
                    currentEnnemy->speed_x = -SKEL_WALKING_SPEED;

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
                else
                {
                    //On essaie de pas tomber
                    if (currentEnnemy->speed_x <= 0)
                    {
                        TworldTile tileBL = getWorldAtPix((currentEnnemy->x + currentEnnemy->speed_x), currentEnnemy->y + 16);
                        if (tileBL.attr.traversable)
                        {
                            currentEnnemy->speed_x = -currentEnnemy->speed_x;
                        }
                    }
                    else
                    {
                        TworldTile tileBR = getWorldAtPix((currentEnnemy->x + currentEnnemy->speed_x) + 15, currentEnnemy->y + 16);
                        if (tileBR.attr.traversable)
                        {
                            currentEnnemy->speed_x = -currentEnnemy->speed_x;
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
        if ((!tileTL.attr.traversable) || (!tileTR.attr.traversable))
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
        if ((!tileBL.attr.traversable) || (!tileBR.attr.traversable))
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
        TworldTile *tileL = lastColErosion > 0 ? &WORLD[lastRowErosion][lastColErosion-1] : NULL;
        TworldTile *tileR = lastColErosion < (WORLD_WIDTH-1) ? &WORLD[lastRowErosion][lastColErosion+1] : NULL;
        TworldTile *tileU = lastRowErosion > 0 ? &WORLD[lastRowErosion-1][lastColErosion] : NULL;
        TworldTile *tileD = lastColErosion < (WORLD_HEIGHT-1) ? &WORLD[lastRowErosion+1][lastColErosion] : NULL;
        if (tile->attr.traversable)
            continue;

        uint8_t matrix = BLOCK_GROUND;        
        if (tile->id >= BLOCK_GROUND && tile->id <= BLOCK_GROUND_ALL)
        {
            if (tileL && tileL->attr.traversable)
                matrix |= BLOCK_GROUND_LEFT;
            if (tileR && tileR->attr.traversable)
                matrix |= BLOCK_GROUND_RIGHT;
            if (tileU && tileU->attr.traversable && tileU->attr.Level == 0)
                matrix |= BLOCK_GROUND_TOP;
            if (tileD && tileD->attr.traversable)
                matrix |= BLOCK_GROUND_BOTTOM;

            tile->id = matrix;
        }
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
            Player.pos.newX = min(((WORLD_WIDTH - 2) * 16), Player.pos.newX);
        }
        else if (Player.pos.speedX < 0)
        {
            Player.pos.newX = Player.pos.pX + Player.pos.speedX;
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
        // +0 +0
        // +0 +15
        TworldTile tileTL = getWorldAtPix(Player.pos.newX + PLAYER_X_BDM, Player.pos.pY + PLAYER_Y_BDM);
        TworldTile tileBL = getWorldAtPix(Player.pos.newX + PLAYER_X_BDM, Player.pos.pY + 15);
        if (!tileTL.attr.traversable || !tileBL.attr.traversable)
        {
            Player.pos.newX = (Player.pos.newX - (Player.pos.newX % 16)) + (16 - PLAYER_X_BDM);
            Player.pos.speedX = 0;
        }
    }
    else
    {
        // +16 +0
        // +16 +15
        TworldTile tileTR = getWorldAtPix(Player.pos.newX + (16 - PLAYER_X_BDM), Player.pos.pY + PLAYER_Y_BDM);
        TworldTile tileBR = getWorldAtPix(Player.pos.newX + (16 - PLAYER_X_BDM), Player.pos.pY + 15);
        if (!tileTR.attr.traversable || !tileBR.attr.traversable)
        {
            Player.pos.newX = (Player.pos.newX - (Player.pos.newX % 16)) + PLAYER_X_BDM;
            Player.pos.speedX = 0;
        }
    }
    //Y
    Player.bJumping = false;
    Player.bFalling = false;
    Player.bOnGround = false;
    if (Player.pos.speedY <= 0)
    {
        //+5 et +12 au lieu de +0 et +15 pour compenser la boundingbox du sprite => NON car cause bug de saut en diagonale
        // +0 +0
        // +15 +0
        TworldTile tileTL = getWorldAtPix(Player.pos.newX + PLAYER_X_BDM, Player.pos.newY + PLAYER_Y_BDM);
        TworldTile tileTR = getWorldAtPix(Player.pos.newX + (15 - PLAYER_X_BDM), Player.pos.newY + PLAYER_Y_BDM);
        if (!tileTL.attr.traversable || !tileTR.attr.traversable)
        {
            Player.pos.newY = (Player.pos.newY - (Player.pos.newY % 16)) + (16 - PLAYER_Y_BDM); // - PLAYER_Y_BDM ??
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
        // +0 +16
        // +15 +16

        TworldTile tileBL = getWorldAtPix(Player.pos.newX + PLAYER_X_BDM, Player.pos.newY + 16);
        TworldTile tileBR = getWorldAtPix(Player.pos.newX + (15 - PLAYER_X_BDM), Player.pos.newY + 16);
        if (!tileBL.attr.traversable || !tileBR.attr.traversable)
        {
            Player.pos.newY = Player.pos.newY - (Player.pos.newY % 16);
            Player.bOnGround = true;
            Player.bFalling = false;
            //On peut de nouveau realiser un double jump
            Player.bDoubleJumping = false;
            Player.pos.speedY = 0;
        }
    }
    if (Player.onGroundCounter > 0)
        Player.onGroundCounter--;

    if (Player.bOnGround)
        Player.onGroundCounter = FRAMES_GROUND_LATENCY;


    Player.bWalking = false;
    if (Player.pos.pX != Player.pos.newX)
    {
        Player.bWalking = true;
        Player.pos.pX = Player.pos.newX;
    }
    Player.pos.pY = Player.pos.newY;
}

void checkPlayerInputs()
{
    Player.bWantJump = false;
    Player.bWantDoubleJump = false;
    Player.bWantWalk = false;

    if (A_just_pressedCounter > 0)
        A_just_pressedCounter--;
    
    if (Player.jumpCounter > 0)
        Player.jumpCounter--;

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
        counterActionB++;

        int lastCibleX = currentTileTarget.wX;
        int lastCibleY = currentTileTarget.wY;

        //Ciblage, on laisse un peu de temps pour locker la cible
        if (counterActionB < FRAMES_LOCK_ACTION_B)
        {
            if (pressed_buttons & ARCADA_BUTTONMASK_UP)
            {
                currentTileTarget.wX = Player.pos.worldX;
                currentTileTarget.wY = Player.pos.YUp;
            }
            else if (pressed_buttons & ARCADA_BUTTONMASK_DOWN)
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
                counterActionB = 0;
                currentTileTarget.pX = ((currentTileTarget.wX - worldMIN_X) * 16) - currentOffset_X;
                currentTileTarget.pY = ((currentTileTarget.wY - worldMIN_Y) * 16) - currentOffset_Y;
                currentTileTarget.tile = &WORLD[currentTileTarget.wY][currentTileTarget.wX];
                currentTileTarget.currentLife = currentTileTarget.tile->attr.life * TILE_LIFE_PRECISION;
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
            if (counterActionB >= FRAMES_ACTION_B) //Action (pour le moment minage seulement)
            {
                TworldTile *tile = currentTileTarget.tile;
                currentTileTarget.tile->attr.hit = 0;
                counterActionB = 0;
                coolDownActionB = FRAMES_COOLDOWN_B;
                lastCibleX = lastCibleY = 0;
                bool bHit = false;

                //Serial.printf("Dig:%d,%d v:%d\n", currentTileTarget.wX, currentTileTarget.wY, tile);
                if (tile->attr.life != BLOCK_LIFE_NA && Player.pos.YDown < (WORLD_HEIGHT - 1))
                {
                    //on teste si on a creusé dans de la pierre solide et on exclue aussi le ground ( @todo : spawn herbe quand meme )
                    if (tile->id >= BLOCK_ROCK && tile->id <= BLOCK_JADE)
                    {
                        bHit = true;
                        //ROCHER
                        // @todo span item ramassable ? ou tile ramassable ?
                        createDropFrom(currentTileTarget.wX, currentTileTarget.wY, tile->id);

                        tile->id = BLOCK_UNDERGROUND_AIR;
                        tile->attr.life = BLOCK_LIFE_NA;
                        tile->attr.traversable = 1;
                        tile->attr.opaque = 1;
                    }
                    else if (tile->id == BLOCK_TREE)
                    {
                        bHit = true;
                        //On casse tout l'arbre
                        createDropFrom(currentTileTarget.wX, currentTileTarget.wY, tile->id);

                        tile->id = BLOCK_AIR;
                        tile->attr.life = BLOCK_LIFE_NA;
                        tile->attr.traversable = 1;
                        tile->attr.opaque = 0;
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
                            brickUp.attr.life = BLOCK_LIFE_NA;
                            brickUp.attr.traversable = 1;
                            brickUp.attr.opaque = 0;
                            setWorldAt(currentTileTarget.wX, currentTileTarget.wY - 1, brickUp);
                        }
                        tile->id = BLOCK_AIR;
                        tile->attr.life = BLOCK_LIFE_NA;
                        tile->attr.traversable = 1;
                        tile->attr.opaque = 0;
                    }
                    if (bHit)
                    {
                        // @todo : tester le type de tile
                        sndPlayerCanal1.play(AudioSampleRock_break);
                        parts.createExplosion(currentTileTarget.pX + 8, currentTileTarget.pY + 8, 12, currentTileTarget.itemColor); //@todo colorer avec la couleur de la brique en cours de travail

                        updateHauteurColonne(currentTileTarget.wX, currentTileTarget.wY);
                    }
                }
            }
        }
    }
    else
    {
        if (coolDownActionB > 0)
            coolDownActionB--;

        counterActionB = 0;
        if (currentTileTarget.tile != NULL)
            currentTileTarget.tile->attr.hit = 0;

        //currentTileTarget = {0};
        
        if (just_pressed & ARCADA_BUTTONMASK_A)
        {
            A_just_pressedCounter = FRAMES_JUMP_LATENCY;

            if (Player.onGroundCounter > 0)// && Player.pos.speedY == 0)
            {
                A_just_pressedCounter = 0;
                Player.bWantJump = true;
                Player.jumpCounter = FRAMES_DOUBLE_JUMP_DETECTION;
                sndPlayerCanal1.play(AudioSample__Jump);
                //Thrust
                Player.pos.speedY = -JUMP_SPEED;
                //Spawn Effect
                set_double_jump_fx();
            }
            else if (!Player.bOnGround && !Player.bDoubleJumping && Player.jumpCounter > 0)
            {
                Player.jumpCounter = 0;
                A_just_pressedCounter = 0;
                // @todo spawn dust double jump
                Player.bWantDoubleJump = true;
                sndPlayerCanal1.play(AudioSample__Jump);
                //Thrust
                Player.pos.speedY = -DOUBLE_JUMP_SPEED;
                Player.bDoubleJumping = true;
                //Spawn Effect
                set_double_jump_fx(true);
            }
        }
        else
        {
            //On a clique il y a peu de temps et on touche le sol...on saute
            if (A_just_pressedCounter && Player.onGroundCounter > 0)
            {
                A_just_pressedCounter = 0;
                Player.bWantJump = true;
                Player.jumpCounter = FRAMES_DOUBLE_JUMP_DETECTION;
                sndPlayerCanal1.play(AudioSample__Jump);
                //Thrust
                Player.pos.speedY = -JUMP_SPEED;
                //Spawn Effect
                set_double_jump_fx();
            }
            else
            {            
                //Si on relache le bouton on saute moins haut..a tester
                if ((just_released & ARCADA_BUTTONMASK_A) && Player.pos.speedY < 0)
                {
                    Player.pos.speedY = Player.pos.speedY >> 1;
                }
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

        for (int wY = 0; wY < WORLD_HEIGHT + 2; wY++)
        {
            for (int wX = 0; wX < WORLD_WIDTH; wX++)
            {
                uint8_t value = data.read();
                WORLD[wY][wX].id = value;

                uint16_t value16 = (data.read() << 8) | data.read();
                WORLD[wY][wX].attr.RAW = value16;
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
                uint8_t value = WORLD[wY][wX].id;
                data.write(value);
                uint16_t value16 = WORLD[wY][wX].attr.RAW;
                value = (value16 & 0xFF00) >> 8;
                data.write(value);
                value = (value16 & 0x00FF);
                data.write(value);
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

    //if (lastFrameDurationMs > (1000/FPS))
    Serial.printf("LFD:%d\n", lastFrameDurationMs);
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
        if (Player.FX.direction > 0)
            parts.createDust(Player.FX.pX - cameraX, Player.FX.pY - cameraY, 5, Player.FX.speedX, Player.FX.speedY, random(FX_FRAMERATE_DUST + 5, FX_FRAMERATE_DUST << 1));
        else
            parts.createDust(Player.FX.pX - cameraX, Player.FX.pY - cameraY, 5, Player.FX.speedX, Player.FX.speedY, random(FX_FRAMERATE_DUST + 5, FX_FRAMERATE_DUST << 1));
        break;
    }
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
    //canvas->fillScreen(0x867D); //84ceef

    drawWorld();

    //arcada.display->display();
    arcada.blitFrameBuffer(0, 0, false, false);
}
