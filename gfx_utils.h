#ifndef GFX_UTILS_H
#define GFX_UTILS_H

#define MAX_LIGHT_INTENSITY_RGB 85

#define MAX_LIGHT_INTENSITY 255
#define SUN_LIGHT_INTENSITY 255
#define PLAYER_LIGHT_INTENSITY 680
#define AMBIENT_LIGHT_INTENSITY 28


void drawSprite(int16_t xMove, int16_t yMove, int16_t width, int16_t height, const unsigned char *bitmap, int8_t DIR = 1, int light = -1);
void drawTile(int16_t xMove, int16_t yMove, const unsigned char *bitmap, int light = -1);
void drawWaterTile(int16_t px, int16_t py, const unsigned char *bitmap, int light = -1, uint8_t waterLevel = 0, bool bOnSurface = false);

void drawTileMask(int16_t xMove, int16_t yMove, const unsigned char *bitmap, int light = -1);
void drawWaterTileMask(int16_t px, int16_t py, const unsigned char *bitmap, int light = -1, uint8_t waterLevel = 0, bool bOnSurface = false);
void drawTreeTileMask(int16_t xMove, int16_t yMove, const unsigned char *bitmap, int16_t width, int16_t height, int light = -1);

void drawImage(int16_t xMove, int16_t yMove, int16_t width, int16_t height, const unsigned char *bitmap);
void drawBackgroundImage(int16_t srcx, int16_t srcy, int16_t srcw, int16_t srch, const unsigned char *bitmap);



#define RGBConvert(red, green, blue)    (uint16_t) ((((uint16_t)(red) & 0xF8) << 8) | (((uint16_t)(green) & 0xFC) << 3) | ((uint16_t)(blue) >> 3))
//Pour le color blending
// exemple : color = ConvertColor50(_color)+ ConvertColor50(bcolor);
#define ConvertColor50(color)  (uint16_t)((color & (0x00FEFEFEul))>>1)
#define ConvertColor25(color)  (uint16_t)((color & (0x00FCFCFCul))>>2)
#define ConvertColor75(color)  (uint16_t)(ConvertColor50(color) + ConvertColor25(color))

#define ALPHA_0      0
#define ALPHA_3      8
#define ALPHA_6      16
#define ALPHA_12     32
#define ALPHA_25     64
#define ALPHA_37     94
#define ALPHA_50    128
#define ALPHA_60    154
#define ALPHA_66    168
#define ALPHA_75    192
#define ALPHA_88    224
#define ALPHA_100   255

uint16_t alphaBlendRGB565( uint32_t fg, uint32_t bg, uint8_t alpha );
uint16_t lightBlendRGB565( uint32_t fg, uint8_t alpha );


 /*
   * For 16bpp pixels we can go a step further: put the middle component
   * in the high 16 bits of a 32 bit word, and process all three RGB
   * components at the same time. Since the smallest gap is here just
   * 5 bits, we have to scale alpha down to 5 bits as well.
   */
 #define ALPHA_BLIT16_565(to, from, length, bpp, alpha)  \
       do {                        \
           int i;                      \
            uint16_t *src = (uint16_t *)(from);         \
            uint16_t *dst = (uint16_t *)(to);           \
            uint32_t ALPHA = alpha >> 3;          \
            for(i = 0; i < (int)(length); i++) {        \
                uint32_t s = *src++;              \
                uint32_t d = *dst;                \
                s = (s | s << 16) & 0x07e0f81f;     \
                d = (d | d << 16) & 0x07e0f81f;     \
                d += (s - d) * ALPHA >> 5;          \
                d &= 0x07e0f81f;                \
                *dst++ = (uint16_t)(d | d >> 16);         \
            }                       \
       } while(0)

/*
    * For 16bpp, we can actually blend two pixels in parallel, if we take
    * care to shift before we add, not after.
    */
   
   /* helper: blend a single 16 bit pixel at 50% */
   #define BLEND16_50(dst, src, mask)          \
       do {                        \
       uint32_t s = *src++;              \
       uint32_t d = *dst;                \
       *dst++ = (uint16_t)((((s & mask) + (d & mask)) >> 1) +    \
                         (s & d & (~mask & 0xffff)));      \
       } while(0)
   
   /* basic 16bpp blender. mask is the pixels to keep when adding. */
   #define ALPHA_BLIT16_50(to, from, length, bpp, alpha, mask)     \
       do {                                \
       unsigned n = (length);                      \
       uint16_t *src = (uint16_t *)(from);                 \
       uint16_t *dst = (uint16_t *)(to);                   \
       if(((uintptr_t)src ^ (uintptr_t)dst) & 3) {         \
           /* source and destination not in phase, blit one by one */  \
           while(n--)                          \
           BLEND16_50(dst, src, mask);             \
       } else {                            \
           if((uintptr_t)src & 3) {                    \
           /* first odd pixel */                   \
           BLEND16_50(dst, src, mask);             \
           n--;                            \
           }                               \
           for(; n > 1; n -= 2) {                  \
           uint32_t s = *(uint32_t *)src;              \
           uint32_t d = *(uint32_t *)dst;              \
           *(uint32_t *)dst = ((s & (mask | mask << 16)) >> 1)   \
                          + ((d & (mask | mask << 16)) >> 1)   \
                          + (s & d & (~(mask | mask << 16)));  \
           src += 2;                       \
           dst += 2;                       \
           }                               \
           if(n)                           \
           BLEND16_50(dst, src, mask); /* last odd pixel */    \
       }                               \
       } while(0)
   
   #define ALPHA_BLIT16_565_50(to, from, length, bpp, alpha)   \
       ALPHA_BLIT16_50(to, from, length, bpp, alpha, 0xf7de)


#endif