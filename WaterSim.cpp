#include <Arduino.h>
#include "defines.h"
#include "types.h"

#include "WaterSim.h"

extern TworldTile WORLD[WORLD_HEIGHT + 2][WORLD_WIDTH];



int lastCellX = 0, lastCellY = 0;
int currentCellX = 0, currentCellY = 0;



        void WATER_UpdateNextCell()
        {
            WATER_UpdateFluidCell2(currentCellX, currentCellY);

            // increment to next cell
            currentCellX++;

            if (currentCellX >= WORLD_WIDTH)
            {
                currentCellY--;
                currentCellX = 0;
            }

            if (currentCellY < 0)
                currentCellY = WORLD_HEIGHT - 1;

            lastCellX = currentCellX;
            lastCellY = currentCellY;
        }

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
            // loop from bottom to top
            for (int y = WORLD_HEIGHT - 1; y >= 0; y--)
            {
                // loop from right to left
                for (int x = 0; x < WORLD_WIDTH; x++)
                {
                    WORLD[y][x].attr.NoCalc = 0;
                }
            }
        }
#if 0
        public void DebugRender(SpriteBatch sb, SpriteFont font, Texture2D white)
        {
            sb.Begin();

            for (int y = 0; y < WORLD_HEIGHT; y++)
            {
                for (int x = 0; x < WORLD_WIDTH; x++)
                {
                    Cell cell = WORLD[y][x];

                    if (cell->Tile == TileType.Dirt)
                    {
                        sb.Draw(white, new Rectangle(x * Cell->RenderWidth, y * Cell->RenderHeight, Cell->RenderWidth, Cell->RenderHeight), Color.Green);
                    }

                    if (cell->attr.traversable && cell->attr.Level > 0)
                    {
                        var c = ((float)cell->attr.Level / (float)MAX_WATER_LEVEL);
                        sb.Draw(white, new Rectangle(x * Cell->RenderWidth, y * Cell->RenderHeight, Cell->RenderWidth, Cell->RenderHeight), Color.Blue * c);
                    }

                    if (cell->attr.NoCalc)
                    {
                        sb.Draw(white, new Rectangle(x * Cell->RenderWidth, y * Cell->RenderHeight, Cell->RenderWidth, Cell->RenderHeight), Color.Orange * 0.6f);
                    }

                    //var size = font.MeasureString(cell->attr.Level.ToString());
                    //sb.DrawString(font, cell->attr.Level.ToString(), new Vector2(x * Cell->RenderWidth + (Cell->RenderWidth * 0.5f), y * Cell->RenderHeight + (Cell->RenderHeight * 0.5f)), Color.Black, 0, size * 0.5f,
                    //    1, SpriteEffects.None, 0);
                }
            }

            sb.Draw(white, new Rectangle(lastCellX * Cell->RenderWidth, lastCellY * Cell->RenderHeight, Cell->RenderWidth, Cell->RenderHeight), Color.Yellow * 0.5f);

            sb.End();
        }
#endif
 
        void WATER_UpdateFluidCell2(int x, int y)
        {
            // get surrounding 3 cells and input into action matrix
            TworldTile *cell;
            TworldTile *bottomCell;
            TworldTile *leftCell;
            TworldTile *rightCell;
            int matrix = Direction_None;
            int leftOverFluid;

            cell = &WORLD[y][x];
            // if tile type is not == water return or if the no calculate flag is set
            if (!cell->attr.traversable) return;
            // if there is no fluid in this cell return
            if (cell->attr.Level == 0) return;

            // get cell beneath this one
            if (y + 1 < WORLD_HEIGHT)
            {
                bottomCell = &WORLD[y+1][x];
                // for this cell to be considered it must...
                //  be of type Air, not be full of fluid, and have less fluid then the cell we are flowing
                if (bottomCell->attr.traversable && bottomCell->attr.Level < MAX_WATER_LEVEL)
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
                if (leftCell->attr.traversable && leftCell->attr.Level < MAX_WATER_LEVEL && leftCell->attr.Level < cell->attr.Level)
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
                if (rightCell->attr.traversable && rightCell->attr.Level < MAX_WATER_LEVEL && rightCell->attr.Level < cell->attr.Level)
                {
                    matrix |= Direction_Right;
                }
            }

            // we now know what cells we can check
            switch (matrix)
            {
                case Direction_Bottom:
                    cell->attr.Level = FlowBottom(cell, bottomCell);
                    break;
                    
                case Direction_Bottom | Direction_Left:
                    leftOverFluid = FlowBottom(cell, bottomCell);
                    if (leftOverFluid > 0)
                    {
                        FlowLeft(cell, leftCell);
                    }
                    break;

                case Direction_Bottom | Direction_Right:
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

        void FlowRight(TworldTile* cell, TworldTile* rightCell)
        {
            int amountToSpread = (rightCell->attr.Level + cell->attr.Level) / 2;
            int remainder = (rightCell->attr.Level + cell->attr.Level) % 2;

            rightCell->attr.Level = amountToSpread + remainder;
            rightCell->attr.NoCalc = 1;
            rightCell->attr.Direction = Direction_Right;
            cell->attr.Level = amountToSpread;
            cell->attr.NoCalc = 1;
        }

        void FlowLeft(TworldTile* cell, TworldTile* leftCell)
        {
            int amountToSpread = (leftCell->attr.Level + cell->attr.Level) / 2;
            int remainder = (leftCell->attr.Level + cell->attr.Level) % 2;

            leftCell->attr.Level = amountToSpread + remainder;
            leftCell->attr.NoCalc = 1;
            leftCell->attr.Direction = Direction_Left;
            cell->attr.Level = amountToSpread;
            cell->attr.NoCalc = 1;
        }

        void FlowLeftRight(TworldTile* cell, TworldTile* leftCell, TworldTile* rightCell)
        {
            int amountToSpread = (leftCell->attr.Level + rightCell->attr.Level + cell->attr.Level) / 3;
            int remainder = (leftCell->attr.Level + rightCell->attr.Level + cell->attr.Level) % 3;
            // if we have a remainder...
            if (remainder > 0)
            {
                // 
                if (cell->attr.Direction == Direction_Left)
                {
                    leftCell->attr.Level = amountToSpread + remainder;
                    leftCell->attr.Direction = Direction_Left;
                    rightCell->attr.Level = amountToSpread;
                }
                else
                {
                    leftCell->attr.Level = amountToSpread;
                    rightCell->attr.Level = amountToSpread + remainder;
                    rightCell->attr.Direction = Direction_Right;
                }

            }
            else
            {
                // otherwise it's an even split
                leftCell->attr.Level = amountToSpread;
                leftCell->attr.Direction = Direction_None;
                rightCell->attr.Level = amountToSpread;
                rightCell->attr.Direction = Direction_None;
            }

            cell->attr.Level = amountToSpread;
            cell->attr.NoCalc = 1;
            cell->attr.Direction = Direction_None;
            leftCell->attr.NoCalc = 1;
            rightCell->attr.NoCalc = 1;
        }

        int FlowBottom(TworldTile* cell, TworldTile* bottomCell)
        {
            // check to see how much fluid can fall down
            int spaceAvailable = MAX_WATER_LEVEL - bottomCell->attr.Level;
            int amountToMove = spaceAvailable;
            if (amountToMove > cell->attr.Level)
                amountToMove = cell->attr.Level;

            // move all fluid that can be moved
            bottomCell->attr.Level += amountToMove;
            bottomCell->attr.NoCalc = 1;
            bottomCell->attr.Direction = Direction_None;
            cell->attr.Level -= amountToMove;
            cell->attr.NoCalc = 1;

            return cell->attr.Level;
        }
