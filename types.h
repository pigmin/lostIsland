#ifndef LI_TYPES_H
#define LI_TYPES_H

typedef struct TPos
{
  int pX, pY;
  int worldX, worldY;
  int speedX, speedY;

  int newX, newY;

  int8_t direction;

  int XFront;
  int YDown;
  int YUp;
} TPos;

typedef struct TPlayer
{
  TPos pos;

  int anim;
  int current_framerate;
  int stateAnim;

  int cible_wX, cible_wY;
  int cible_pX, cible_pY;

  bool bDying;
  bool bJumping;
  bool bFalling;
  bool bWalking;
  bool bOnGround;
  bool bMoving;
  bool bTouched;
  int iTouchCountDown;


  unsigned char state;
  unsigned char action, action_cooldown, action_perf;
  unsigned char depth;
  unsigned char health;
  unsigned char level;
  unsigned char stars_kills;
  unsigned short armour_weapon;

} TPlayer;

typedef struct Enemy
{
  TPos pos;

  int type;

  int anim;
  int current_framerate;
  int stateAnim;

  int cible_wX, cible_wY;
  int cible_pX, cible_pY;

  bool bDying;
  bool bJumping;
  bool bFalling;
  bool bWalking;
  bool bOnGround;
  bool bMoving;
  bool bTouched;
  int iTouchCountDown;

  bool bIsActive;
  int bIsAlive;   //de 0 a 127 : zombi, apres 127 vivant

  unsigned char state;
  unsigned char action, action_cooldown, action_perf;
  unsigned char health;
  unsigned char level;
} Enemy;

#endif
