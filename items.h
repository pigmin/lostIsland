#ifndef ITEMS_H
#define ITEMS_H

#define ITEM_NONE           0

#define ITEM_TREE1           BLOCK_TREE  //6

#define ITEM_ROCK           BLOCK_ROCK  //0x10
#define ITEM_CHARBON        BLOCK_CHARBON
#define ITEM_FER            BLOCK_FER
#define ITEM_CUIVRE         BLOCK_CUIVRE
#define ITEM_OR             BLOCK_OR
#define ITEM_REDSTONE       BLOCK_REDSTONE  //0x17
#define ITEM_DIAMANT        BLOCK_DIAMANT
#define ITEM_JADE           BLOCK_JADE

#define ITEM_PIOCHE_BOIS    0x7f

typedef struct Titem {
    int worldX;
    int worldY;
    uint8_t type;
    int max_frames;
    int nb_anim_frames;

    bool bFalling;
    bool bJumping;
    bool bOnGround;
    bool bMoving;
    
    bool bIsActive;
    int bIsAlive;   //de 0 a 127 : zombi, apres 127 vivant
    int iSpawning;
    float pX;
    float pY;
    float newX;
    float newY;
    float speedX;
    float speedY;

    int current_framerate;
    int anim_frame;
} Titem;


//Nombre max d'items dans le monde
#define MAX_ITEMS 128
Titem ITEMS[MAX_ITEMS];


    
#endif