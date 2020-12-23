#ifndef GFX_UTILS_H
#define GFX_UTILS_H

#define MAX_LIGHT_INTENSITY 7000
#define SUN_LIGHT_INTENSITY 6000
#define PLAYER_LIGHT_INTENSITY 3000
#define AMBIENT_LIGHT_INTENSITY 1000

void drawSprite(int16_t xMove, int16_t yMove, int16_t width, int16_t height, const unsigned char *bitmap, int8_t DIR = 1, int light = -1);
uint16_t rgbTo565(uint8_t red, uint8_t green, uint8_t blue);
void drawTile(int16_t xMove, int16_t yMove, const unsigned char *bitmap, int light = -1);
void drawTile2(int16_t xMove, int16_t yMove, const unsigned char *bitmap, int curLight_TL = -1, int curLight_BR = -1, int lightX = -1, int lightY = -1);


#endif