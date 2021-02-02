#ifndef SPRITES_H
#define SPRITES_H

   #define PLAYER_X_OFFSET -3
   #define PLAYER_Y_OFFSET -3
   #define PLAYER_WIDTH 23
   #define PLAYER_HEIGHT 20


   #define PLAYER_STATE_IDLE              0
   #define PLAYER_STATE_RUN               1
   #define PLAYER_STATE_JUMP              2
   #define PLAYER_STATE_DOUBLE_JUMP       3
   #define PLAYER_STATE_WALL_JUMP         4
   #define PLAYER_STATE_FALL              5
   #define PLAYER_STATE_DIG               6
   #define PLAYER_STATE_WALL_SLIDING      7
   #define PLAYER_STATE_LEDGE_CLIMB       8
   #define PLAYER_STATE_WALL_CLIMBING     9

   #define PLAYER_STATE_DIE               64   
   #define PLAYER_STATE_WIN               128


   //Idle  
   #define PLAYER_FRAME_IDLE_1    0
   #define PLAYER_FRAME_IDLE_2    1
   #define PLAYER_FRAME_IDLE_3    2
   #define PLAYER_FRAME_IDLE_4    3

   //Run
   #define PLAYER_FRAME_RUN_1  8
   #define PLAYER_FRAME_RUN_2  9
   #define PLAYER_FRAME_RUN_3  10
   #define PLAYER_FRAME_RUN_4  11
   #define PLAYER_FRAME_RUN_5  12
   #define PLAYER_FRAME_RUN_6  13

  //Jump
   #define PLAYER_FRAME_JUMP_1  14
   #define PLAYER_FRAME_JUMP_2  15
   #define PLAYER_FRAME_JUMP_3  16
   #define PLAYER_FRAME_JUMP_4  17

  //Double Jump
   #define PLAYER_FRAME_SALTO_1  18
   #define PLAYER_FRAME_SALTO_2  19
   #define PLAYER_FRAME_SALTO_3  20
   #define PLAYER_FRAME_SALTO_4  21

  //Wall jump
   #define PLAYER_FRAME_WALL_JUMP_1  66
   #define PLAYER_FRAME_WALL_JUMP_2  67


   //Fall
   #define PLAYER_FRAME_FALLING_1  22
   #define PLAYER_FRAME_FALLING_2  23

   //Wall sliding
   #define PLAYER_FRAME_SLIDING_1   68
   #define PLAYER_FRAME_SLIDING_2   69

   //Wall climbining
   #define PLAYER_FRAME_WALL_CLIMBING_1   70
   #define PLAYER_FRAME_WALL_CLIMBING_2   71
   #define PLAYER_FRAME_WALL_CLIMBING_3   72
   #define PLAYER_FRAME_WALL_CLIMBING_4   73

   //LEDGE climbing
   #define PLAYER_FRAME_LEDGE_CLIMB_1   24
   #define PLAYER_FRAME_LEDGE_CLIMB_2   25
   #define PLAYER_FRAME_LEDGE_CLIMB_3   26
   #define PLAYER_FRAME_LEDGE_CLIMB_4   27
   #define PLAYER_FRAME_LEDGE_CLIMB_5   28
   #define PLAYER_FRAME_LEDGE_CLIMB_6   29
   #define PLAYER_FRAME_LEDGE_CLIMB_7   30
   #define PLAYER_FRAME_LEDGE_CLIMB_8   31
   #define PLAYER_FRAME_LEDGE_CLIMB_9   32

 
   //Action
   #define PLAYER_FRAME_ACTION_1  82
   #define PLAYER_FRAME_ACTION_2  83
   #define PLAYER_FRAME_ACTION_3  84

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

   #define PLAYER_FRAMERATE_IDLE             5
   #define PLAYER_FRAMERATE_FALLING          2
   #define PLAYER_FRAMERATE_RUN              2
   #define PLAYER_FRAMERATE_JUMP             2
   #define PLAYER_FRAMERATE_ACTION           5
   #define PLAYER_FRAMERATE_DIE              5
   #define PLAYER_FRAMERATE_TOUCH            5
   #define PLAYER_FRAMERATE_GROW             5  
   #define PLAYER_FRAMERATE_WIN              5  
   #define PLAYER_FRAMERATE_SLIDING          2
   #define PLAYER_FRAMERATE_LEDGE_CLIMBING   3
   #define PLAYER_FRAMERATE_WALL_CLIMBING    5
   #define PLAYER_FRAMERATE_DOUBLE_JUMP      2
   #define PLAYER_FRAMERATE_WALL_JUMP        2

   #define PLAYER_DIE_FRAMES  (FPS*3) 
   #define PLAYER_WIN_FRAMES  (FPS*8) 
   #define PLAYER_GROW_FRAMES  (FPS*2) 
   #define PLAYER_TOUCH_FRAMES  (FPS*2) 


const float ledgeClimbXOffset1 = -2.0f;
const float ledgeClimbYOffset1 = 0.0f;
const float ledgeClimbXOffset2 = -5.0f;
const float ledgeClimbYOffset2 = 0.0f;

#define  FX_NONE           0
#define  FX_DOUBLE_JUMP    1
#define  FX_DUST           2
#define  FX_SPLASH         3

#define FX_FRAME_DOUBLE_JUMP_1  0
#define FX_FRAME_DOUBLE_JUMP_2  1
#define FX_FRAME_DOUBLE_JUMP_3  2
#define FX_FRAME_DOUBLE_JUMP_4  3
#define FX_FRAME_DOUBLE_JUMP_5  4
#define FX_FRAMERATE_DOUBLE_JUMP             2



#define JUMP_DUST_VERTICAL_WIDTH    16
#define JUMP_DUST_VERTICAL_HEIGHT   5
#define JUMP_DUST_HORIZONTAL_WIDTH   5
#define JUMP_DUST_HORIZONTAL_HEIGHT   16

#define FX_FRAME_DUST_1  0
#define FX_FRAME_DUST_2  1
#define FX_FRAME_DUST_3  2
#define FX_FRAME_DUST_4  3
#define FX_FRAME_DUST_5  4
#define FX_FRAMERATE_DUST                    1


#define FX_FRAME_SPLASH_1  0
#define FX_FRAME_SPLASH_2  1
#define FX_FRAME_SPLASH_3  2
#define FX_FRAME_SPLASH_4  3
#define FX_FRAME_SPLASH_5  4
#define FX_FRAMERATE_SPLASH                  1


#endif