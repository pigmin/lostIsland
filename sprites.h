#ifndef SPRITES_H
#define SPRITES_H


   #define PLAYER_STATE_IDLE              0
   #define PLAYER_STATE_WALK              1
   #define PLAYER_STATE_JUMP              2
   #define PLAYER_STATE_DOUBLE_JUMP       4
   #define PLAYER_STATE_FALL              8
   #define PLAYER_STATE_DIG               16

   #define PLAYER_STATE_DIE               64   
   #define PLAYER_STATE_WIN               128


   //Idle  
   #define PLAYER_FRAME_IDLE_1    0
   #define PLAYER_FRAME_IDLE_2    1
   #define PLAYER_FRAME_IDLE_3    2

   //Jump

   #define PLAYER_FRAME_FALLING_1  0

   //Run
   #define PLAYER_FRAME_WALK_1  0
   #define PLAYER_FRAME_WALK_2  1
   #define PLAYER_FRAME_WALK_3  2
   #define PLAYER_FRAME_WALK_4  3
   #define PLAYER_FRAME_WALK_5  4
   #define PLAYER_FRAME_WALK_6  5
   //Jump
   #define PLAYER_FRAME_JUMP_1  0
   #define PLAYER_FRAME_JUMP_2  1
   #define PLAYER_FRAME_JUMP_3  2
   #define PLAYER_FRAME_JUMP_4  3
   #define PLAYER_FRAME_JUMP_5  4

   //Action
   #define PLAYER_FRAME_ACTION_1  0
   #define PLAYER_FRAME_ACTION_2  1
   #define PLAYER_FRAME_ACTION_3  2
   #define PLAYER_FRAME_ACTION_4  3

   //Die
   #define PLAYER_FRAME_DIE_1   0

   #define PLAYER_FRAME_WIN_1   0
   #define PLAYER_FRAME_WIN_2   1
   #define PLAYER_FRAME_WIN_3   2

   //Die
   #define PLAYER_FRAME_TOUCH_1   0
   #define PLAYER_FRAME_TOUCH_2   1

   //Die
   #define PLAYER_FRAME_GROW_1   0
   #define PLAYER_FRAME_GROW_2   1

   const int PLAYER_FRAMERATE_IDLE = 4;
   const int PLAYER_FRAMERATE_FALLING = 1;
   const int PLAYER_FRAMERATE_WALK = 2;
   const int PLAYER_FRAMERATE_JUMP = 2;
   const int PLAYER_FRAMERATE_ACTION = 3;
   const int PLAYER_FRAMERATE_DIE = 1;
   const int PLAYER_FRAMERATE_TOUCH = 2;
   const int PLAYER_FRAMERATE_GROW = 2;
   const int PLAYER_FRAMERATE_WIN = 2;

   const int PLAYER_DIE_FRAMES = FPS*3; 
   const int PLAYER_WIN_FRAMES = FPS*8; 
   const int PLAYER_GROW_FRAMES = FPS*2; 
   const int PLAYER_TOUCH_FRAMES = FPS*2; 


#define  FX_NONE           0
#define  FX_DOUBLE_JUMP    1
#define  FX_DUST           2
#define  FX_SPLASH         3

#define FX_FRAME_DOUBLE_JUMP_1  0
#define FX_FRAME_DOUBLE_JUMP_2  1
#define FX_FRAME_DOUBLE_JUMP_3  2
#define FX_FRAME_DOUBLE_JUMP_4  3
#define FX_FRAME_DOUBLE_JUMP_5  4
const int FX_FRAMERATE_DOUBLE_JUMP = 2;


#define FX_FRAME_DUST_1  0
#define FX_FRAME_DUST_2  1
#define FX_FRAME_DUST_3  2
#define FX_FRAME_DUST_4  3
#define FX_FRAME_DUST_5  4
const int FX_FRAMERATE_DUST = 1;


#define FX_FRAME_SPLASH_1  0
#define FX_FRAME_SPLASH_2  1
#define FX_FRAME_SPLASH_3  2
#define FX_FRAME_SPLASH_4  3
#define FX_FRAME_SPLASH_5  4
const int FX_FRAMERATE_SPLASH = 1;


#endif