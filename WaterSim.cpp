#include <Arduino.h>

#include "WaterSim.h"

extern TworldTile WORLD[WORLD_HEIGHT + 2][WORLD_WIDTH];


//TODO faire des update partiels (par troncons de la map ?)
void WATER_Update()
{
    // loop from bottom to top
    for (int y = WORLD_HEIGHT - 1; y >= 0; y--)
    {
        // loop from right to left
        for (int x = 0; x < WORLD_WIDTH; x++)
        {
            WATER_UpdateFluidCell2(x, y);
        }
    }
}

void WATER_UpdateFluidCell2(int x, int y)
{
    // get surrounding 3 cells and input into action matrix
    TworldTile *cell = NULL;
    TworldTile *bottomCell = NULL;
    TworldTile *leftCell = NULL;
    TworldTile *rightCell = NULL;
    uint8_t matrix = Direction_None;
    int leftOverFluid;

    cell = &WORLD[y][x];
    // if tile type is not == water return or if the no calculate flag is set
    if (!cell->traversable)
        return;
    // if there is no fluid in this cell return
    if (cell->Level == 0)
        return;

    // get cell beneath this one
    if (y + 1 < WORLD_HEIGHT)
    {
        bottomCell = &WORLD[y + 1][x];
        // for this cell to be considered it must...
        //  be of type Air, not be full of fluid, and have less fluid then the cell we are flowing
        if (bottomCell->traversable && bottomCell->Level < MAX_WATER_LEVEL)
        {
            matrix |= Direction_Bottom;
        }
    }

    // get cell to the left of this one
    if (x - 1 >= 0)
    {
        leftCell = &WORLD[y][x - 1];
        // for this cell to be considered it must...
        //  be of type Air, not be full of fluid, and have less fluid then the cell we are flowing
        if (leftCell->traversable && leftCell->Level < MAX_WATER_LEVEL && leftCell->Level < cell->Level)
        {
            matrix |= Direction_Left;
        }
    }

    // get cell to the right of this one
    if (x + 1 < WORLD_WIDTH)
    {
        rightCell = &WORLD[y][x + 1];
        // for this cell to be considered it must...
        //  be of type Air, not be full of fluid, and have less fluid then the cell we are flowing
        if (rightCell->traversable && rightCell->Level < MAX_WATER_LEVEL && rightCell->Level < cell->Level)
        {
            matrix |= Direction_Right;
        }
    }

    // we now know what cells we can check
    switch (matrix)
    {
    case Direction_Bottom:
        cell->Level = FlowBottom(cell, bottomCell);
        break;

    case Direction_Bottom_Left:
        leftOverFluid = FlowBottom(cell, bottomCell);
        if (leftOverFluid > 0)
        {
            FlowLeft(cell, leftCell);
        }
        break;

    case Direction_Bottom_Right:
        leftOverFluid = FlowBottom(cell, bottomCell);
        if (leftOverFluid > 0)
        {
            FlowRight(cell, rightCell);
        }
        break;

    case Direction_Bottom | Direction_Right | Direction_Left:
        leftOverFluid = FlowBottom(cell, bottomCell);
        if (leftOverFluid > 0)
        {
            FlowLeftRight(cell, leftCell, rightCell);
        }
        break;

    case Direction_Left:
        FlowLeft(cell, leftCell);
        break;

    case Direction_Right:
        FlowRight(cell, rightCell);
        break;

    case Direction_RightLeft:
        FlowLeftRight(cell, leftCell, rightCell);
        break;
    }
}

void FlowRight(TworldTile *cell, TworldTile *rightCell)
{
    if (!cell || !rightCell)
        return;
    int amountToSpread = (rightCell->Level + cell->Level) / 2;
    int remainder = (rightCell->Level + cell->Level) % 2;

    rightCell->Level = amountToSpread + remainder;
    rightCell->Direction = Direction_Right;
    cell->Level = amountToSpread;
    if (cell->Direction == Direction_Bottom)
        cell->Direction = Direction_Bottom_Right;
    else
        cell->Direction = Direction_Right;
}

void FlowLeft(TworldTile *cell, TworldTile *leftCell)
{
    if (!cell || !leftCell)
        return;
    int amountToSpread = (leftCell->Level + cell->Level) / 2;
    int remainder = (leftCell->Level + cell->Level) % 2;

    leftCell->Level = amountToSpread + remainder;
    leftCell->Direction = Direction_Left;
    cell->Level = amountToSpread;
    if (cell->Direction == Direction_Bottom)
        cell->Direction = Direction_Bottom_Left;
    else
        cell->Direction = Direction_Left;
}

void FlowLeftRight(TworldTile *cell, TworldTile *leftCell, TworldTile *rightCell)
{
    if (!cell || !rightCell || !leftCell)
        return;

    int amountToSpread = (leftCell->Level + rightCell->Level + cell->Level) / 3;
    int remainder = (leftCell->Level + rightCell->Level + cell->Level) % 3;
    // if we have a remainder...
    if (remainder > 0)
    {
        //
        if (cell->Direction == Direction_Left)
        {
            leftCell->Level = amountToSpread + remainder;
            leftCell->Direction = Direction_Left;
            rightCell->Level = amountToSpread;
        }
        else
        {
            leftCell->Level = amountToSpread;
            rightCell->Level = amountToSpread + remainder;
            rightCell->Direction = Direction_Right;
        }
    }
    else
    {
        // otherwise it's an even split
        leftCell->Level = amountToSpread;
        leftCell->Direction = Direction_None;
        rightCell->Level = amountToSpread;
        rightCell->Direction = Direction_None;
    }

    cell->Level = amountToSpread;
    cell->Direction = Direction_None;
}

int FlowBottom(TworldTile *cell, TworldTile *bottomCell)
{
    if (!cell || !bottomCell)
        return 0;

    // check to see how much fluid can fall down
    int spaceAvailable = MAX_WATER_LEVEL - bottomCell->Level;
    int amountToMove = spaceAvailable;
    if (amountToMove > cell->Level)
        amountToMove = cell->Level;

    // move all fluid that can be moved
    bottomCell->Level += amountToMove;
    bottomCell->Direction = Direction_None;
    cell->Level -= amountToMove;
    if (cell->Level > 0)
        cell->Direction = Direction_Bottom;

    return cell->Level;
}
