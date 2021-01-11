
#ifndef _DEFINES_H
#define _DEFINES_H

#undef DEBUG
//#define NO_SOUND
#define FPS 25
#define FRAME_DURATION (1/FPS)
#define FRAME_DURATION_8B (255/FPS)

#define CAMERA_FOLLOW_X_SPEED   4
#define CAMERA_FOLLOW_Y_SPEED   10

// @todo passer en time based et non frame based
// @todo idem pour les speed et anims frames des sprites...
#define FRAMES_LOCK_ACTION_B int(225 / (1000 / FPS))
#define FRAMES_ANIM_ACTION_B int(412 / (1000 / FPS))
#define FRAMES_ACTION_B int(1500 / (1000 / FPS))
#define FRAMES_COOLDOWN_B int(750 / (1000 / FPS))

#define HALF_SCREEN_WIDTH (ARCADA_TFT_WIDTH>>1)
#define HALF_SCREEN_HEIGHT (ARCADA_TFT_HEIGHT>>1)

#define WORLD_WIDTH 128
#define WORLD_HEIGHT 96

#define WORLD_WIDTH_PX (WORLD_WIDTH*16)
#define WORLD_HEIGHT_PX (WORLD_HEIGHT*16)

#define AMPLITUDE_HAUTEUR 8
#define MEDIUM_HAUTEUR 80

#define MAX_DENSITE 8
#define AMPLITUDE_DENSITE 16

#define MAX_TREES       30

#define HUD_ITEMS_X 16
#define HUD_ITEMS_Y ARCADA_TFT_HEIGHT - 18

#define BASE_X_PLAYER 3

#define FALLING_SPEED 1
#define WALKING_SPEED 2
#define RUNNING_SPEED 2
//Nombre de frames ou on considere le player comme encore sur le sol (pour les air jumps)
#define FRAMES_GROUND_LATENCY   (FPS/6)
//On se rapelle de la demande de saut pendant x frames (pour sauter meme si on touchait pas encore le sol mais juste apres oui...)
#define FRAMES_JUMP_LATENCY   (FPS/6)
//On autorise le double jump apres x frames suivant le jump classique
#define FRAMES_DOUBLE_JUMP_DETECTION    (FPS/2)

//Au dela de cette vitesse de chute on fera de la poussiere au sol
#define SPEED_Y_LANDING   8

#define MAX_SPEED_X 4
#define MAX_SPEED_Y 15

#define JUMP_SPEED 10
#define DOUBLE_JUMP_SPEED 8

#define DEFAULT_ITEM_SHAKE_AMOUNT   6
#define CAMERA_KILL_ENNEMY_AMOUNT   5
#define CAMERA_BREAK_ROCK_AMOUNT    2

//Nb pixels pour cible camera
#define CAMERA_AHEAD_AMOUNT     32

//Nombre max de pile par type d'item
#define MAX_STACK_ITEMS 64

#endif

/*
    scaled = scale8(val, 100);
scales the 0-255 value down to a 0-100 value. There's a variation on scale8 called scale8_video which has the property that if the passed in value is non-zero, the returned value will be non-zero. To account for the fact that these values are linear, while human perception is not, there's functions to adjust the dim/brightness of a value from the 0-255 value, linear, to a more perceptual range:

   val = dim8_raw(val);
   val = dim8_video(val);
   val = brighten8_raw(val);
   val = brighten8_video(val);
Fast random numbers
Sometimes you want a random number, either 8 bit or 16 bit values. The default random functions provided by the arduino library are a bit on the slow side. So we have 6 random value functions, 3 8-bit, and 3 16-bit functions that you can use:

     random8()       == random from 0..255
     random8( n)     == random from 0..(N-1)
     random8( n, m)  == random from N..(M-1)
 
     random16()      == random from 0..65535
     random16( n)    == random from 0..(N-1)
     random16( n, m) == random from N..(M-1)
Easing and Linear Interpolation functions
Fast 8-bit "easing in/out" function.

     ease8InOutCubic(x) == 3(x^i) - 2(x^3)
     ease8InOutApprox(x) == 
       faster, rougher, approximation of cubic easing
Linear interpolation between two values, with the fraction between them expressed as an 8- or 16-bit fixed point fraction (fract8 or fract16).

     lerp8by8(   fromU8, toU8, fract8 )
     lerp16by8(  fromU16, toU16, fract8 )
     lerp15by8(  fromS16, toS16, fract8 )
       == from + (( to - from ) * fract8) / 256)
     lerp16by16( fromU16, toU16, fract16 )
       == from + (( to - from ) * fract16) / 65536)
Trig Functions
The library also has fast 16bit sin/cos functions. The input is an "angle" from 0-65535, and the output is a singed 16 bit number from -32767 to 32767. The 8bit versions take an input "angle" from 0-255, and the output is a unsigned 8 bit number from 0 to 255:

    sin16(x);
    cos16(x);
    sin8(x);
    cos8(x);

*/