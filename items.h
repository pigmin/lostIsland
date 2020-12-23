#ifndef ITEMS_H
#define ITEMS_H

#define ITEM_MUSHROOM 1

    typedef struct item {
        int worldX;
        int worldY;
        int type;
        int max_frames;
        int max_anim;

        bool bIsActive;
        int bIsAlive;   //de 0 a 127 : zombi, apres 127 vivant
        int iSpawning;
        int x;
        int y;
        int current_framerate;
        int anim;
        int speed_x;
        int speed_y;
    } Titem;


    #define MAX_ITEMS 64

    Titem ITEMS[MAX_ITEMS];

    
#endif