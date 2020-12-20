#ifndef SPRITES_H
#define SPRITES_H

   #define PLAYER_STATE_IDLE     0
   #define PLAYER_STATE_WALK     1
   #define PLAYER_STATE_JUMP     2
   #define PLAYER_STATE_FALL     4
   #define PLAYER_STATE_DIG      8

   #define PLAYER_STATE_DIE      64   
   #define PLAYER_STATE_WIN      128


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

   //Action
   #define PLAYER_FRAME_ACTION_1  0
   #define PLAYER_FRAME_ACTION_2  1
   #define PLAYER_FRAME_ACTION_3  2
   #define PLAYER_FRAME_ACTION_4  3
   #define PLAYER_FRAME_ACTION_5  4
   #define PLAYER_FRAME_ACTION_6  5

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

   const int PLAYER_FRAMERATE_IDLE = 6;
   const int PLAYER_FRAMERATE_FALLING = 1;
   const int PLAYER_FRAMERATE_WALK = 2;
   const int PLAYER_FRAMERATE_JUMP = 2;
   const int PLAYER_FRAMERATE_ACTION = 4;
   const int PLAYER_FRAMERATE_DIE = 1;
   const int PLAYER_FRAMERATE_TOUCH = 2;
   const int PLAYER_FRAMERATE_GROW = 2;
   const int PLAYER_FRAMERATE_WIN = 2;

   const int PLAYER_DIE_FRAMES = FPS*3; 
   const int PLAYER_WIN_FRAMES = FPS*8; 
   const int PLAYER_GROW_FRAMES = FPS*2; 
   const int PLAYER_TOUCH_FRAMES = FPS*2; 


#endif