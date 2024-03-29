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
        float pX;
        float pY;
        float newX;
        float newY;
        float speedX;
        float speedY;

        int current_framerate;
        int anim_frame;

        int health;
        int max_health;
    } Tennemy;


    #define MAX_SPIDERS 85
    #define MAX_ZOMBIES 65
    #define MAX_SKELS   50
    #define MAX_ENNEMIES MAX_SPIDERS+MAX_ZOMBIES+MAX_SKELS

    Tennemy ENNEMIES[MAX_ENNEMIES];

    #define SPIDER_ENNEMY           's'
    #define SPIDER_WALKING_SPEED    75
    #define SPIDER_HEALTH       10
    #define SPIDER_WIDTH        16
    #define SPIDER_HEIGHT        16
    #define SPIDER_WALK_FRAMES        2

    #define ZOMBI_ENNEMY        'z'
    #define ZOMBI_WALKING_SPEED    25
    #define ZOMBI_HEALTH       30
    #define ZOMBI_WIDTH        16
    #define ZOMBI_HEIGHT        16
    #define ZOMBI_WALK_FRAMES        3

    #define SKEL_ENNEMY         'k'
    #define SKEL_WALKING_SPEED    50
    #define SKEL_HEALTH       60
    #define SKEL_WIDTH        16
    #define SKEL_HEIGHT        16
    #define SKEL_WALK_FRAMES        3

#endif