
#ifndef _DEFINES_H
#define _DEFINES_H

#undef DEBUG
//#define NO_SOUND
#define FPS 25

// @todo passer en time based et non frame based
// @todo idem pour les speed et anims frames des sprites...
#define FRAMES_LOCK_ACTION_B int(300 / (1000 / FPS))
#define FRAMES_ANIM_ACTION_B int(550 / (1000 / FPS))
#define FRAMES_ACTION_B int(2000 / (1000 / FPS))
#define FRAMES_COOLDOWN_B int(1000 / (1000 / FPS))

#define WORLD_WIDTH 128
#define WORLD_HEIGHT 96

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

#define MAX_SPEED_X 4
#define MAX_SPEED_Y 15

#define JUMP_SPEED 10
#define DOUBLE_JUMP_SPEED 8


#endif