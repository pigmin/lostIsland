#ifndef ENNEMIES_H
#define ENNEMIES_H

    typedef struct ennemy {
        int worldX;
        int worldY;
        int type;
        int max_frames;
        int nb_anim_frames;

        bool bFalling;
        bool bJumping;
        bool bOnGround;
        bool bWalking;
        
        bool bIsActive;
        int bIsAlive;   //de 0 a 127 : zombi, apres 127 vivant
        int iSpawning;
        int x;
        int y;
        int new_x;
        int new_y;
        int current_framerate;
        int anim_frame;
        int speed_x;
        int speed_y;

        int health;
        int max_health;
    } Tennemy;


    #define MAX_SPIDERS 85
    #define MAX_ZOMBIES 65
    #define MAX_SKELS   50
    #define MAX_ENNEMIES MAX_SPIDERS+MAX_ZOMBIES+MAX_SKELS

    Tennemy ENNEMIES[MAX_ENNEMIES];

    #define SPIDER_ENNEMY           's'
    #define SPIDER_WALKING_SPEED    3
    #define SPIDER_HEALTH       10

    #define ZOMBI_ENNEMY        'z'
    #define ZOMBI_WALKING_SPEED    1
    #define ZOMBI_HEALTH       30

    #define SKEL_ENNEMY         'k'
    #define SKEL_WALKING_SPEED    2
    #define SKEL_HEALTH       60
    
#endif