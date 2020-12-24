#ifndef ITEMS_H
#define ITEMS_H


#define ITEM_NONE           0
#define ITEM_GROUND_ROCK    BLOCK_GROUND_ROCK  //0x03
#define ITEM_ROCK           BLOCK_ROCK  //0x10
#define ITEM_CHARBON        BLOCK_CHARBON
#define ITEM_CUIVRE         BLOCK_CUIVRE
#define ITEM_FER            BLOCK_FER
#define ITEM_ARGENT         BLOCK_ARGENT
#define ITEM_JADE           BLOCK_JADE
#define ITEM_OR             BLOCK_OR
#define ITEM_REDSTONE       BLOCK_REDSTONE  //0x17

typedef struct Titem {
    int worldX;
    int worldY;
    uint8_t type;
    int max_frames;
    int max_anim;

    bool bFalling;
    bool bJumping;
    bool bOnGround;
    bool bMoving;
    
    bool bIsActive;
    int bIsAlive;   //de 0 a 127 : zombi, apres 127 vivant
    int iSpawning;
    int x;
    int y;
    int new_x;
    int new_y;
    int current_framerate;
    int anim;
    int speed_x;
    int speed_y;
} Titem;


#define MAX_ITEMS 128

Titem ITEMS[MAX_ITEMS];

    
#endif