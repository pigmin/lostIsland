#ifndef _WATER_SIM_H
#define _WATER_SIM_H


       #define MAX_WATER_LEVEL 7



        #define Direction_None             0
        #define Direction_Bottom           1
        #define Direction_Left             2
        #define Direction_Right            4
        #define Direction_Bottom_Left      Direction_Bottom | Direction_Left
        #define Direction_Bottom_Right     Direction_Bottom | Direction_Right
        #define Direction_RightLeft        Direction_Left | Direction_Right
        #define Direction_All              Direction_Left | Direction_Right | Direction_Bottom

        void WATER_UpdateNextCell();

        void WATER_Update();

        void WATER_UpdateFluidCell2(int x, int y);

        void FlowRight(TworldTile* cell, TworldTile* rightCell);

        void FlowLeft(TworldTile* cell, TworldTile* leftCell);

        void FlowLeftRight(TworldTile* cell, TworldTile* leftCell, TworldTile* rightCell);

        int FlowBottom(TworldTile* cell, TworldTile* bottomCell);

#endif