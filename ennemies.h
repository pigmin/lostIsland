#ifndef ENNEMIES_H
#define ENNEMIES_H

    typedef struct ennemy {
        int worldX;
        int worldY;
        int type;
        int max_frames;
        int max_anim;

        bool bIsActive;
        int bIsAlive;   //de 0 a 127 : zombi, apres 127 vivant
        int x;
        int y;
        int current_framerate;
        int anim;
        int direction;
    } Tennemy;


    #define MAX_ENNEMIES 64

    Tennemy ENNEMIES[MAX_ENNEMIES];

    
#endif