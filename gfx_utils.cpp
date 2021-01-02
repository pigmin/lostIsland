#include <Adafruit_Arcada.h>
#include <Adafruit_GFX.h>

#include "gfx_utils.h"

extern GFXcanvas16 *canvas;

/*
void PixelGameEngine::DrawSprite(int32_t x, int32_t y, Sprite* sprite, uint32_t scale, uint8_t flip)
	{
		if (sprite == nullptr)
			return;

		int32_t fxs = 0, fxm = 1, fx = 0;
		int32_t fys = 0, fym = 1, fy = 0;
		if (flip & olc::Sprite::Flip::HORIZ) { fxs = sprite->width - 1; fxm = -1; }
		if (flip & olc::Sprite::Flip::VERT) { fys = sprite->height - 1; fym = -1; }

		if (scale > 1)
		{
			fx = fxs;
			for (int32_t i = 0; i < sprite->width; i++, fx += fxm)
			{
				fy = fys;
				for (int32_t j = 0; j < sprite->height; j++, fy += fym)
					for (uint32_t is = 0; is < scale; is++)
						for (uint32_t js = 0; js < scale; js++)
							Draw(x + (i * scale) + is, y + (j * scale) + js, sprite->GetPixel(fx, fy));
			}
		}
		else
		{
			fx = fxs;
			for (int32_t i = 0; i < sprite->width; i++, fx += fxm)
			{
				fy = fys;
				for (int32_t j = 0; j < sprite->height; j++, fy += fym)
					Draw(x + i, y + j, sprite->GetPixel(fx, fy));
			}
		}
	}
  */
void drawSprite(int16_t xMove, int16_t yMove, int16_t width, int16_t height, const unsigned char *bitmap, int8_t DIR, int light)
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
            canvas->drawPixel(xMove + i, yMove, lightBlendRGB565(value, light));
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
            canvas->drawPixel((xMove + width - 1) - i, yMove, lightBlendRGB565(value, light));
          }
        }
        //    Serial.println("");
      }
    }
  }

  //  Serial.println("");
}

void drawImage(int16_t xMove, int16_t yMove, int16_t width, int16_t height, const unsigned char *bitmap)
{
  uint16_t idx = 0;

  for (int16_t j = 0; j < height; j++, yMove++)
  {
    idx = (width * j) << 1;
    for (int16_t i = 0; i < width; i++)
    {
      uint16_t value = (bitmap[idx++]) + (bitmap[idx++] << 8);
      canvas->drawPixel(xMove + i, yMove, value);
    }
  }
}


void drawBackgroundImage(int16_t srcx, int16_t srcy, int16_t srcw, int16_t srch, const unsigned char *bitmap)
{
  uint16_t idx = 0;

  uint16_t x = min(srcx, srcw - ARCADA_TFT_WIDTH);
  uint16_t y = min(srcy, srch - ARCADA_TFT_HEIGHT);

  uint16_t width = min(srcw - x, ARCADA_TFT_WIDTH);
  uint16_t height = min(srch - y, ARCADA_TFT_HEIGHT);
  
  for (int16_t j = 0; j < height; j++, y++)
  {
    idx = ((srcw * y)  + x) << 1;
    for (int16_t i = 0; i < width; i++)
    {
      uint16_t value = (bitmap[idx++]) + (bitmap[idx++] << 8);
      canvas->drawPixel(i, j, value);
    }
  }
}

void drawWaterTile(int16_t xMove, int16_t yMove, const unsigned char *bitmap, int light, uint8_t waterLevel, bool bOnSurface)
{
  uint16_t waterColor = 0;
  uint8_t valAlpha = 0;
  switch (waterLevel)
  {
  case 7:
    waterColor = RGBConvert(0, 56, 114);
    valAlpha = ALPHA_25;
    break;
  case 6:
    waterColor = RGBConvert(0, 105, 212);
    valAlpha = ALPHA_37;
    break;
  case 5:
    waterColor = RGBConvert(6, 130, 255);
    valAlpha = ALPHA_50;
    break;
  case 4:
    waterColor = RGBConvert(55, 154, 255);
    valAlpha = ALPHA_60;
    break;
  case 3:
    waterColor = RGBConvert(104, 179, 255);
    valAlpha = ALPHA_66;
    break;
  case 2:
    waterColor = RGBConvert(153, 203, 255);
    valAlpha = ALPHA_75;
    break;
  case 1:
    waterColor = RGBConvert(201, 228, 254);
    valAlpha = ALPHA_75;
    break;

  default:
    return;
  }
  if (bitmap)
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
            if ((bOnSurface && j > 8) || (!bOnSurface))
              value = alphaBlendRGB565(value, waterColor, valAlpha);
            canvas->drawPixel(xMove + i, yMove, value);
          }
        }
        //    Serial.println("");
      }
      if (bOnSurface)
        canvas->drawFastHLine(xMove, yMove + 8, 16, ARCADA_WHITE);
    }
    else if (light == 0)
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
            //color blend
            if ((bOnSurface && j > 8) || (!bOnSurface))
              value = alphaBlendRGB565(value, waterColor, valAlpha);
            //light blend
            value = lightBlendRGB565(value, light);
            canvas->drawPixel(xMove + i, yMove, value);
          }
        }
      }
      if (bOnSurface)
        canvas->drawFastHLine(xMove, yMove + 8, 16, lightBlendRGB565(ARCADA_WHITE, light));
    }
  }
  else
  {
    if (light < 0 || light == MAX_LIGHT_INTENSITY)
    {
      if (bOnSurface)
      {
        canvas->fillRect(xMove, yMove + 9, 16, 8, waterColor);
        canvas->drawFastHLine(xMove, yMove + 8, 16, ARCADA_WHITE);
      }
      else
        canvas->fillRect(xMove, yMove, 16, 16, waterColor);
    }
    else if (light == 0)
    {
      canvas->fillRect(xMove, yMove, 16, 16, ARCADA_BLACK);
    }
    else
    {
      if (bOnSurface)
      {
        canvas->fillRect(xMove, yMove + 8, 16, 8, lightBlendRGB565(waterColor, light));
        canvas->drawFastHLine(xMove, yMove + 8, 16, lightBlendRGB565(ARCADA_WHITE, light));
      }
      else
        canvas->fillRect(xMove, yMove, 16, 16, lightBlendRGB565(waterColor, light));
    }
  }
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
    if (light == 0)
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
            canvas->drawPixel(xMove + i, yMove, lightBlendRGB565(value, light));
          }
        }
      }
    }
  }

  //  Serial.println("");
}

void drawTreeTile(int16_t xMove, int16_t yMove, const unsigned char *bitmap, int16_t width, int16_t height, int light)
{
  uint16_t idx = 0;

  xMove = xMove - ((width - 16) >> 1);
  yMove = yMove - (height - 16);

  if (light < 0 || light == MAX_LIGHT_INTENSITY)
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
    if (light == 0)
    {
      for (int16_t j = 0; j < height; j++, yMove++)
      {
        for (int16_t i = 0; i < width; i++)
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

      for (int16_t j = 0; j < height; j++, yMove++)
      {
        for (int16_t i = 0; i < width; i++)
        {
          uint16_t value = (bitmap[idx++]) + (bitmap[idx++] << 8);
          //   Serial.printf("%d,", value);
          if (value != 0xF81F)
          {
            canvas->drawPixel(xMove + i, yMove, lightBlendRGB565(value, light));
          }
        }
      }
    }
  }

  //  Serial.println("");
}
// Fast RGB565 pixel blending
// Found in a pull request for the Adafruit framebuffer library. Clever!
// https://github.com/tricorderproject/arducordermini/pull/1/files#diff-d22a481ade4dbb4e41acc4d7c77f683d
uint16_t alphaBlendRGB565(uint32_t fg, uint32_t bg, uint8_t alpha)
{
  // Alpha converted from [0..255] to [0..31]
  alpha = (alpha + 4) >> 3;

  // Converts  0000000000000000rrrrrggggggbbbbb
  //     into  00000gggggg00000rrrrr000000bbbbb
  // with mask 00000111111000001111100000011111
  // This is useful because it makes space for a parallel fixed-point multiply
  bg = (bg | (bg << 16)) & 0b00000111111000001111100000011111;
  fg = (fg | (fg << 16)) & 0b00000111111000001111100000011111;

  // This implements the linear interpolation formula: result = bg * (1.0 - alpha) + fg * alpha
  // This can be factorized into: result = bg + (fg - bg) * alpha
  // alpha is in Q1.5 format, so 0.0 is represented by 0, and 1.0 is represented by 32
  uint32_t result = (fg - bg) * alpha; // parallel fixed-point multiply of all components
  result >>= 5;
  result += bg;
  result &= 0b00000111111000001111100000011111; // mask out fractional parts
  return (uint16_t)((result >> 16) | result);   // contract result
}

uint16_t lightBlendRGB565(uint32_t fg, uint8_t alpha)
{
  // Alpha converted from [0..255] to [0..31]
  alpha = (alpha + 4) >> 3;

  // Converts  0000000000000000rrrrrggggggbbbbb
  //     into  00000gggggg00000rrrrr000000bbbbb
  // with mask 00000111111000001111100000011111
  // This is useful because it makes space for a parallel fixed-point multiply

  fg = (fg | (fg << 16)) & 0b00000111111000001111100000011111;

  // This implements the linear interpolation formula: result = bg * (1.0 - alpha) + fg * alpha
  // This can be factorized into: result = bg + (fg - bg) * alpha
  // alpha is in Q1.5 format, so 0.0 is represented by 0, and 1.0 is represented by 32
  uint32_t result = (fg)*alpha; // parallel fixed-point multiply of all components
  result >>= 5;
  result &= 0b00000111111000001111100000011111; // mask out fractional parts
  return (uint16_t)((result >> 16) | result);   // contract result
}