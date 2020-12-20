
typedef struct Pos {
  unsigned char xpos, ypos, facing;
  char upvel;//?
} Pos;

typedef struct Player {
  Pos p;
  char anim;
  unsigned char state;  
  unsigned char action, action_cooldown, action_perf;
  unsigned char depth;
  unsigned char health;
  unsigned char level;
  unsigned char stars_kills;
  unsigned short armour_weapon;
  
} Player;

typedef struct Enemy {
  Pos p;
  unsigned char state;
  unsigned char health;
} Enemy;

