#include <Adafruit_Arcada.h>
#include <Adafruit_GFX.h>

#include "gfx_utils.h"

extern GFXcanvas16 *canvas;

uint16_t rgbTo565(uint8_t red, uint8_t green, uint8_t blue)
{
  uint16_t Rgb565 = (((red & 0xf8) << 8) + ((green & 0xfc) << 3) + (blue >> 3));

  return Rgb565;
}

void drawSprite(int16_t xMove, int16_t yMove, int16_t width, int16_t height, const unsigned char *bitmap, int DIR, int light)
{
  uint16_t idx = 0;

  if (light < 0)
  {

    if (DIR > 0)
    {
      for (int16_t j = 0; j < height; j++, yMove++)
      {
        for (int16_t i = 0; i < width; i++)
        {
          uint16_t value = (bitmap[idx++]) + (bitmap[idx++] << 8);
          //   Serial.printf("%d,", value);
          if (value != 0xF81F)
          {
            canvas->drawPixel(xMove + i, yMove, value);
          }
        }
        //    Serial.println("");
      }
    }
    else
    {
      for (int16_t j = 0; j < height; j++, yMove++)
      {
        for (int16_t i = 0; i < width; i++)
        {
          uint16_t value = (bitmap[idx++]) + (bitmap[idx++] << 8);
          //        Serial.printf("%0,", value);
          if (value != 0xF81F)
          {
            canvas->drawPixel((xMove + width - 1) - i, yMove, value);
          }
        }
        //    Serial.println("");
      }
    }
  }
  else
  {
    float coeff = (float)light / MAX_LIGHT_INTENSITY;

    if (DIR > 0)
    {
      for (int16_t j = 0; j < height; j++, yMove++)
      {
        for (int16_t i = 0; i < width; i++)
        {
          uint16_t value = (bitmap[idx++]) + (bitmap[idx++] << 8);
          //   Serial.printf("%d,", value);
          if (value != 0xF81F)
          {
            uint8_t r = ((((value >> 11) & 0x1F) * 527) + 23) >> 6;
            uint8_t g = ((((value >> 5) & 0x3F) * 259) + 33) >> 6;
            uint8_t b = (((value & 0x1F) * 527) + 23) >> 6;
            value = rgbTo565(int(r * coeff), int(g * coeff), int(b * coeff));
            canvas->drawPixel(xMove + i, yMove, value);
          }
        }
        //    Serial.println("");
      }
    }
    else
    {
      for (int16_t j = 0; j < height; j++, yMove++)
      {
        for (int16_t i = 0; i < width; i++)
        {
          uint16_t value = (bitmap[idx++]) + (bitmap[idx++] << 8);
          //        Serial.printf("%0,", value);
          if (value != 0xF81F)
          {
            uint8_t r = ((((value >> 11) & 0x1F) * 527) + 23) >> 6;
            uint8_t g = ((((value >> 5) & 0x3F) * 259) + 33) >> 6;
            uint8_t b = (((value & 0x1F) * 527) + 23) >> 6;
            value = rgbTo565(r * coeff, g * coeff, b * coeff);
            canvas->drawPixel((xMove + width - 1) - i, yMove, value);
          }
        }
        //    Serial.println("");
      }
    }
  }

  //  Serial.println("");
}

void drawTile(int16_t xMove, int16_t yMove, const unsigned char *bitmap, int light)
{
  uint16_t idx = 0;

  if (light < 0 || light == MAX_LIGHT_INTENSITY)
  {

    for (int16_t j = 0; j < 16; j++, yMove++)
    {
      for (int16_t i = 0; i < 16; i++)
      {
        uint16_t value = (bitmap[idx++]) + (bitmap[idx++] << 8);
        //   Serial.printf("%d,", value);
        if (value != 0xF81F)
        {
          canvas->drawPixel(xMove + i, yMove, value);
        }
      }
      //    Serial.println("");
    }
  }
  else
  {
    float coeff = (float)light / MAX_LIGHT_INTENSITY;
    if (coeff == 0)
    {
      for (int16_t j = 0; j < 16; j++, yMove++)
      {
        for (int16_t i = 0; i < 16; i++)
        {
          uint16_t value = (bitmap[idx++]) + (bitmap[idx++] << 8);
          //   Serial.printf("%d,", value);
          if (value != 0xF81F)
          {
            canvas->drawPixel(xMove + i, yMove, ARCADA_BLACK);
          }
        }
      }
    }
    else
    {
      for (int16_t j = 0; j < 16; j++, yMove++)
      {
        for (int16_t i = 0; i < 16; i++)
        {
          uint16_t value = (bitmap[idx++]) + (bitmap[idx++] << 8);
          //   Serial.printf("%d,", value);
          if (value != 0xF81F)
          {
            uint8_t r = ((((value >> 11) & 0x1F) * 527) + 23) >> 6;
            uint8_t g = ((((value >> 5) & 0x3F) * 259) + 33) >> 6;
            uint8_t b = (((value & 0x1F) * 527) + 23) >> 6;
            value = rgbTo565(int(r * coeff), int(g * coeff), int(b * coeff));
            canvas->drawPixel(xMove + i, yMove, value);
          }
        }
      }
    }
  }

  //  Serial.println("");
}

void drawTile2(int16_t xMove, int16_t yMove, const unsigned char *bitmap, int light, int curLight_TL, int curLight_TR, int curLight_BL, int curLight_BR)
{
  uint16_t idx = 0;

  if (light < 0 || light == MAX_LIGHT_INTENSITY)
  {

    for (int16_t j = 0; j < 16; j++, yMove++)
    {
      for (int16_t i = 0; i < 16; i++)
      {
        uint16_t value = (bitmap[idx++]) + (bitmap[idx++] << 8);
        //   Serial.printf("%d,", value);
        if (value != 0xF81F)
        {
          canvas->drawPixel(xMove + i, yMove, value);
        }
      }
      //    Serial.println("");
    }
  }
  else
  {
    float coeff = (float)light / MAX_LIGHT_INTENSITY;
    if (coeff == 0)
    {
      for (int16_t j = 0; j < 16; j++, yMove++)
      {
        for (int16_t i = 0; i < 16; i++)
        {
          uint16_t value = (bitmap[idx++]) + (bitmap[idx++] << 8);
          //   Serial.printf("%d,", value);
          if (value != 0xF81F)
          {
            canvas->drawPixel(xMove + i, yMove, ARCADA_BLACK);
          }
        }
      }
    }
    else
    {
      float coeffX = (float)curLight_TL / MAX_LIGHT_INTENSITY;
      float coeffY = (float)curLight_TL / MAX_LIGHT_INTENSITY;
      float stepX = ((curLight_TR - curLight_TL) / MAX_LIGHT_INTENSITY) / 16;
      float stepY = ((curLight_BL - curLight_TL) / MAX_LIGHT_INTENSITY) / 16;
      coeff = coeffY;
      for (int16_t j = 0; j < 16; j++, yMove++)
      {
        coeff = coeffY + (j * stepY);
        for (int16_t i = 0; i < 16; i++)
        {
          uint16_t value = (bitmap[idx++]) + (bitmap[idx++] << 8);
          //   Serial.printf("%d,", value);
          if (value != 0xF81F)
          {
            uint8_t r = ((((value >> 11) & 0x1F) * 527) + 23) >> 6;
            uint8_t g = ((((value >> 5) & 0x3F) * 259) + 33) >> 6;
            uint8_t b = (((value & 0x1F) * 527) + 23) >> 6;
            value = rgbTo565(int(r * coeff), int(g * coeff), int(b * coeff));
            canvas->drawPixel(xMove + i, yMove, value);
          }
          coeff += stepX;
        }
      }
    }
  }

  //  Serial.println("");
}