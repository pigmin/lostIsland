#ifndef LI_TYPES_H
#define LI_TYPES_H

#define MAX_QUICK_ITEMS 8

#define MAX_BACKPACK_ITEMS 32

//Bande morte sur l'axe X (ignorée par les tests de collision)
#define PLAYER_X_BDM  3
//Idem axe Y mais seulement en haut (les pieds sont sur le sol)
#define PLAYER_Y_BDM  4

typedef struct Vector2
{
  float x;
  float y;
} Vector2;

typedef struct TPos
{
  float pX, pY;
  int worldX, worldY;
  float speedX, speedY;

  float newX, newY;

  int8_t direction;

  int XFront;
  int YDown;
  int YUp;
} ;

typedef struct TInventoryItem
{
  uint8_t typeItem;
  uint8_t nbStack;
  uint8_t life;
};

typedef struct TPlayer
{
  TPos pos;

  uint8_t anim_frame;
  uint8_t current_framerate;
  uint8_t stateAnim;

  struct {
    uint8_t anim_frame;
    uint8_t current_framerate;
    uint8_t stateAnim;
    int8_t direction;
    uint8_t    waterLevel;
    int     pX;
    int     pY;
    int     speedX;
    int     speedY;
  } FX;

  bool bCanMove;
  bool bCanFlip;
  bool bHasWallJumped;

  bool bJumping;
  float jumpTimer;
  bool bDoubleJumping;
  bool bWallJumping;

  float wallJumpTimer;
  int8_t lastWallJumpDirection;

  bool bOnGround;
  float onGroundTimer;

  bool bTouchingWall;
  bool bTouchingLedge;
  bool bLedgeDetected;
  bool bClimbingLedge;
  //Pour monter sur les bords
  Vector2 ledgePosTop;
  Vector2 ledgePos1;
  Vector2 ledgePos2;

  float turnTimer;

  bool bDying;
  bool bWallSliding;
  bool bWallClimbing;
  bool bFalling;
  bool bWalking;
  bool bMovingUpDown;
  bool bLanding; //On vient de tomber a pleine vitesse de falling
  bool bTouched;
  float fTouchCountDown;

  uint8_t waterLevel;
  bool bSplashIn;
  bool bSplashOut;
  bool bUnderWater;

  int  wantedHorizontalDirection;
  int  wantedVerticalDirection;
  bool bWantWalk;
  bool bWantJump;
  bool bWantDoubleJump;

  int cible_wX, cible_wY;
  int cible_pX, cible_pY;

  uint8_t  currentItemSelected;
  TInventoryItem  quick_inventory[MAX_QUICK_ITEMS];
  TInventoryItem  inventory[MAX_BACKPACK_ITEMS];

  unsigned char state;
  unsigned char action, action_cooldown, action_perf;
  unsigned char depth;
  int health;
  int max_health;
  unsigned char level;
  unsigned char stars_kills;
  unsigned short armour_weapon;

} TPlayer;

typedef struct Enemy
{
  TPos pos;

  int type;

  int anim_frame;
  int current_framerate;
  int stateAnim;

  int cible_wX, cible_wY;
  int cible_pX, cible_pY;

  bool bDying;
  bool bJumping;
  bool bFalling;
  bool bWalking;
  bool bOnGround;
  bool bTouched;
  int fTouchCountDown;

  bool bIsActive;
  int bIsAlive;   //de 0 a 127 : zombi, apres 127 vivant

  unsigned char state;
  unsigned char action, action_cooldown, action_perf;
  unsigned char health;
  unsigned char level;
} Enemy;

#define  BLOCK_LIFE_NA     0  //Aucune ou infinie
#define  BLOCK_LIFE_1     1
#define  BLOCK_LIFE_2     2
#define  BLOCK_LIFE_3     3
#define  BLOCK_LIFE_4     4
#define  BLOCK_LIFE_5     5
#define  BLOCK_LIFE_6     6
#define  BLOCK_LIFE_7     7

//On multiplie la life par x pour plus de precision lors du minage
#define TILE_LIFE_PRECISION 10


      
typedef struct {
        uint8_t id;

        uint8_t traversable:1;
        uint8_t opaque:1;
        //uint8_t underground:1; //pour le moment on stocke le underground par rapport a la hauteur originelle..a voir..
        //uint8_t animated:1;
        uint8_t life:4;

        //Water simu
        uint8_t Direction:2;
        uint8_t Level:3;

        //1 si en cours de minage, pour effets divers
        uint8_t hit:1;
        //permet de changer l'apparence en gardant le meme type (arbres)
        uint8_t spriteVariation:2;

        uint8_t contour:4;
#ifdef USE_FOV
        uint8_t light_hit:1;
#endif

      //AU TOTAL ON PEDS DEUX BITS....apres l'alignement nous fait perdre 1 byte...

} TworldTile;


#endif
