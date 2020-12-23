#ifndef ENNEMIES_H
#define ENNEMIES_H

    typedef struct ennemy {
        int worldX;
        int worldY;
        int type;
        int max_frames;
        int max_anim;

        bool bFalling;
        bool bJumping;
        bool bOnGround;
        bool bMoving;
        
        bool bIsActive;
        int bIsAlive;   //de 0 a 127 : zombi, apres 127 vivant
        int x;
        int y;
        int new_x;
        int new_y;
        int current_framerate;
        int anim;
        int speed_x;
        int speed_y;
    } Tennemy;


    #define MAX_SPIDERS 100
    #define MAX_ZOMBIES 50
    #define MAX_SKELS   25
    #define MAX_ENNEMIES MAX_SPIDERS+MAX_ZOMBIES+MAX_SKELS

    Tennemy ENNEMIES[MAX_ENNEMIES];

    #define SPIDER_ENNEMY           's'
    #define SPIDER_WALKING_SPEED    3

    #define ZOMBI_ENNEMY        'z'
    #define ZOMBI_WALKING_SPEED    1

    #define SKEL_ENNEMY         'k'
    #define SKEL_WALKING_SPEED    2
    
#endif