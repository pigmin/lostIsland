
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

//Nombre max de pile par type d'item
#define MAX_STACK_ITEMS 64

#endif