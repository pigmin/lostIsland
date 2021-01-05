#define BLOCK_AIR               0
#define BLOCK_UNDERGROUND_AIR   0x7F

#define BLOCK_GROUND            0x10
#define BLOCK_GROUND_LEFT       (BLOCK_GROUND + 1)
#define BLOCK_GROUND_TOP        (BLOCK_GROUND + 2)
#define BLOCK_GROUND_RIGHT      (BLOCK_GROUND + 4)
#define BLOCK_GROUND_BOTTOM     (BLOCK_GROUND + 8)
//,,,
#define BLOCK_GROUND_ALL        (BLOCK_GROUND_LEFT | BLOCK_GROUND_TOP | BLOCK_GROUND_RIGHT | BLOCK_GROUND_BOTTOM) //0x1F


#define BLOCK_GRASS            0x30
#define BLOCK_TREE             0x31


#define BLOCK_ROCK              0x40    //
#define BLOCK_CHARBON           0x41    //  15
#define BLOCK_FER               0x42    //   10
#define BLOCK_CUIVRE            0x43    //   5
#define BLOCK_OR                0x44    //   2
#define BLOCK_REDSTONE          0x45    //    10
#define BLOCK_DIAMANT           0x46    //    1
#define BLOCK_JADE              0x47    //    1
//510

//Densite "outil de pierre", a la main on multiplie par 4
#define BLOCK_ROCK_DENSITY          2
#define BLOCK_CHARBON_DENSITY       3
#define BLOCK_FER_DENSITY           3
#define BLOCK_CUIVRE_DENSITY        4
#define BLOCK_OR_DENSITY            6
#define BLOCK_REDSTONE_DENSITY      8
#define BLOCK_DIAMANT_DENSITY       10
#define BLOCK_JADE_DENSITY          15