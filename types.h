#ifndef LI_TYPES_H
#define LI_TYPES_H

#define MAX_QUICK_ITEMS 8

#define MAX_ITEMS 32

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

  uint8_t anim;
  uint8_t current_framerate;
  uint8_t stateAnim;

  int cible_wX, cible_wY;
  int cible_pX, cible_pY;

  bool bDying;
  bool bJumping;
  bool bFalling;
  bool bWalking;
  bool bOnGround;
  bool bMoving;
  bool bTouched;
  uint8_t iTouchCountDown;


  uint8_t  currentItemSelected;
  uint8_t  quick_inventory[MAX_QUICK_ITEMS];

  uint8_t  inventory[MAX_ITEMS];

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
