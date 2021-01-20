#include <Adafruit_Arcada.h>
#include <Adafruit_GFX.h>

#include "gfx_utils.h"
#include "WaterSim.h"

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

void drawSpriteSheet(int16_t xMove, int16_t yMove, int16_t width, int16_t height, const unsigned char *bitmap, uint16_t frame, int8_t DIR, int light)
{
  uint32_t idx = 0;

  idx = frame * width * height * 2;
  
  if (light < 0)
  {

    if (DIR > 0)
    {
      for (int16_t j = 0; j < height; j++, yMove++)
      {
        for (int16_t i = 0; i < width; i++)
        {
          uint16_t value = (bitmap[idx++]) | (bitmap[idx++] << 8);
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
          uint16_t value = (bitmap[idx++]) | (bitmap[idx++] << 8);
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
          uint16_t value = (bitmap[idx++]) | (bitmap[idx++] << 8);
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
          uint16_t value = (bitmap[idx++]) | (bitmap[idx++] << 8);
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
          uint16_t value = (bitmap[idx++]) | (bitmap[idx++] << 8);
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
          uint16_t value = (bitmap[idx++]) | (bitmap[idx++] << 8);
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
          uint16_t value = (bitmap[idx++]) | (bitmap[idx++] << 8);
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
          uint16_t value = (bitmap[idx++]) | (bitmap[idx++] << 8);
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
      uint16_t value = (bitmap[idx++]) | (bitmap[idx++] << 8);
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
    idx = ((srcw * y) + x) << 1;
    for (int16_t i = 0; i < width; i++)
    {
      uint16_t value = (bitmap[idx++]) | (bitmap[idx++] << 8);
      canvas->drawPixel(i, j, value);
    }
  }
}

void drawWaterTile(int16_t xMove, int16_t yMove, const unsigned char *bitmap, int light, uint8_t waterLevel, bool bOnSurface)
{
  uint16_t waterColor = 0;
  uint8_t valAlpha = 0;
  uint8_t waterStart;
  uint8_t waterWidth;

  uint8_t origY = yMove;

  if (light == 0)
  {
    canvas->fillRect(xMove, yMove, 16, 16, ARCADA_BLACK);
    return;
  }

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

  if (bOnSurface)
  {
    waterWidth = waterLevel + 2;
    waterStart = 16 - waterWidth;
  }
  else
  {
    waterWidth = 16;
    waterStart = 0;
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
          uint16_t value = (bitmap[idx++]) | (bitmap[idx++] << 8);
          //   Serial.printf("%d,", value);
          if (j >= waterStart)
            value = alphaBlendRGB565(value, waterColor, valAlpha);
          canvas->drawPixel(xMove + i, yMove, value);
        }
        //    Serial.println("");
      }
      if (bOnSurface)
        canvas->drawFastHLine(xMove, origY + waterStart, 16, ARCADA_WHITE);
    }
    else
    {
      for (int16_t j = 0; j < 16; j++, yMove++)
      {
        for (int16_t i = 0; i < 16; i++)
        {
          uint16_t value = (bitmap[idx++]) | (bitmap[idx++] << 8);
          //   Serial.printf("%d,", value);
          //color blend
          if (j >= waterStart)
            value = alphaBlendRGB565(value, waterColor, valAlpha);
          //light blend
          value = lightBlendRGB565(value, light);
          canvas->drawPixel(xMove + i, yMove, value);
        }
      }
      if (bOnSurface)
        canvas->drawFastHLine(xMove, origY + waterStart, 16, lightBlendRGB565(ARCADA_WHITE, light));
    }
  }
  else
  {
    if (light < 0 || light == MAX_LIGHT_INTENSITY)
    {
      yMove += waterStart;
      for (int16_t j = waterStart; j < 16; j++, yMove++)
      {
        for (int16_t i = 0; i < 16; i++)
        {
          uint16_t value = canvas->getPixel(xMove + i, yMove);
          value = alphaBlendRGB565(value, waterColor, valAlpha);
          canvas->drawPixel(xMove + i, yMove, value);
        }
        //    Serial.println("");
      }
      if (bOnSurface)
        canvas->drawFastHLine(xMove, origY + waterStart, 16, ARCADA_WHITE);
    }
    else
    {
      yMove += waterStart;
      for (int16_t j = waterStart; j < 16; j++, yMove++)
      {
        for (int16_t i = 0; i < 16; i++)
        {
          uint16_t value = canvas->getPixel(xMove + i, yMove);
          //color blend
          value = alphaBlendRGB565(value, waterColor, valAlpha);
          //light blend
          value = lightBlendRGB565(value, light);
          canvas->drawPixel(xMove + i, yMove, value);
        }
      }
      if (bOnSurface)
        canvas->drawFastHLine(xMove, origY + waterStart, 16, lightBlendRGB565(ARCADA_WHITE, light));
    }
  }
}


 /**
  * 
  * MATRIX : 
  * 1 LEFT
  * 2 TOP
  * 4 RIGHT
  * 8 BOTTOM
  * 
  */
 
void drawTile(int16_t xMove, int16_t yMove, const unsigned char *bitmap, int light, uint8_t matrix)
{
  uint16_t idx = 0;

  int xStart = 0;
  int yStart = 0;
  int width = 16;
  int height = 16;
  
  if (light == 0)
  {
    canvas->fillRect(xMove, yMove, 16, 16, ARCADA_BLACK);
  }
  else if (light < 0 || light == MAX_LIGHT_INTENSITY)
  {
    if (matrix & 1)
    {
      xStart = 1;
      canvas->drawFastVLine(xMove, yMove, 16, 0xD652);
    }
    if (matrix & 8)
    {
      canvas->drawFastHLine(xMove, yMove+15, 16, 0xD652);
      height -= 1;
    }
    if (matrix & 2)
    {
      canvas->drawFastHLine(xMove, yMove, 16, 0xD652);
      yStart += 1;
      yMove += 1;
    }
    if (matrix & 4)
    {
      canvas->drawFastVLine(xMove+15, yMove, 16, 0xD652);
      width -= 1;
    }

    for (int16_t j = yStart; j < height; j++, yMove++)
    {
        idx = ((j << 4) + xStart) << 1;
        for (int16_t i = xStart; i < width; i++)
        {
          uint16_t value = (bitmap[idx++]) | (bitmap[idx++] << 8);
          canvas->drawPixel(xMove + i, yMove, value);
        }
    }


  }
  else
  {
    uint16_t borderCol = lightBlendRGB565(0xD652, light);

    if (matrix & 1)
    {
      xStart = 1;
      canvas->drawFastVLine(xMove, yMove, 16, borderCol);
    }
    if (matrix & 8)
    {
      canvas->drawFastHLine(xMove, yMove+15, 16, borderCol);
      height -= 1;
    }
    if (matrix & 2)
    {
      canvas->drawFastHLine(xMove, yMove, 16, borderCol);
      yStart += 1;
      yMove += 1;
    }
    if (matrix & 4)
    {
      canvas->drawFastVLine(xMove+15, yMove, 16, borderCol);
      width -= 1;
    }

    for (int16_t j = yStart; j < height; j++, yMove++)
    {
      idx = ((j << 4) + xStart) << 1;
      for (int16_t i = xStart; i < width; i++)
      {
        uint16_t value = (bitmap[idx++]) | (bitmap[idx++] << 8);
        canvas->drawPixel(xMove + i, yMove, lightBlendRGB565(value, light));
      }
    }
    
  }
}

void drawWaterTileMask(int16_t xMove, int16_t yMove, const unsigned char *bitmap, int light, uint8_t waterLevel, bool bOnSurface)
{
  uint16_t waterColor = 0;
  uint8_t valAlpha = 0;
  uint8_t waterStart;
  uint8_t waterWidth;

  uint8_t origY = yMove;

  if (light == 0)
  {
    canvas->fillRect(xMove, yMove, 16, 16, ARCADA_BLACK);
    return;
  }

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

  if (bOnSurface)
  {
    waterWidth = waterLevel + 2;
    waterStart = 16 - waterWidth;
  }
  else
  {
    waterWidth = 16;
    waterStart = 0;
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
          uint16_t value = (bitmap[idx++]) | (bitmap[idx++] << 8);
          if (value != 0xF81F)
          {
            if (j >= waterStart)
              value = alphaBlendRGB565(value, waterColor, valAlpha);
            canvas->drawPixel(xMove + i, yMove, value);
          }
          else if (j >= waterStart)
            canvas->drawPixel(xMove + i, yMove, waterColor);
        }
      }
      if (bOnSurface)
        canvas->drawFastHLine(xMove, origY + waterStart, 16, ARCADA_WHITE);
    }
    else
    {
      for (int16_t j = 0; j < 16; j++, yMove++)
      {
        for (int16_t i = 0; i < 16; i++)
        {
          uint16_t value = (bitmap[idx++]) | (bitmap[idx++] << 8);
          //   Serial.printf("%d,", value);
          if (value != 0xF81F)
          {
            //color blend
            if (j >= waterStart)
              value = alphaBlendRGB565(value, waterColor, valAlpha);
            //light blend
            value = lightBlendRGB565(value, light);
            canvas->drawPixel(xMove + i, yMove, value);
          }
          else if (j >= waterStart)
          {
            value = lightBlendRGB565(waterColor, light);
            canvas->drawPixel(xMove + i, yMove, value);
          }
        }
      }
      if (bOnSurface)
        canvas->drawFastHLine(xMove, origY + waterStart, 16, lightBlendRGB565(ARCADA_WHITE, light));
    }
  }
  else
  {
    if (light < 0 || light == MAX_LIGHT_INTENSITY)
    {
      yMove += waterStart;
      for (int16_t j = waterStart; j < 16; j++, yMove++)
      {
        for (int16_t i = 0; i < 16; i++)
        {
          uint16_t value = canvas->getPixel(xMove + i, yMove);
          //   Serial.printf("%d,", value);
          value = alphaBlendRGB565(value, waterColor, valAlpha);
          canvas->drawPixel(xMove + i, yMove, value);
        }
        //    Serial.println("");
      }
      if (bOnSurface)
        canvas->drawFastHLine(xMove, origY + waterStart, 16, ARCADA_WHITE);
    }
    else
    {
      yMove += waterStart;
      for (int16_t j = waterStart; j < 16; j++, yMove++)
      {
        for (int16_t i = 0; i < 16; i++)
        {
          uint16_t value = canvas->getPixel(xMove + i, yMove);
          //color blend
          value = alphaBlendRGB565(value, waterColor, valAlpha);
          //light blend
          value = lightBlendRGB565(value, light);
          canvas->drawPixel(xMove + i, yMove, value);
        }
      }
      if (bOnSurface)
        canvas->drawFastHLine(xMove, origY + waterStart, 16, lightBlendRGB565(ARCADA_WHITE, light));
    }
  }
}

void drawTileMask(int16_t xMove, int16_t yMove, const unsigned char *bitmap, int light)
{
  uint16_t idx = 0;

  if (light == 0)
  {
    canvas->fillRect(xMove, yMove, 16, 16, ARCADA_BLACK);
    return;
  }

  if (light < 0 || light == MAX_LIGHT_INTENSITY)
  {

    for (int16_t j = 0; j < 16; j++, yMove++)
    {
      for (int16_t i = 0; i < 16; i++)
      {
        uint16_t value = (bitmap[idx++]) | (bitmap[idx++] << 8);
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
    for (int16_t j = 0; j < 16; j++, yMove++)
    {
      for (int16_t i = 0; i < 16; i++)
      {
        uint16_t value = (bitmap[idx++]) | (bitmap[idx++] << 8);
        //   Serial.printf("%d,", value);
        if (value != 0xF81F)
        {
          canvas->drawPixel(xMove + i, yMove, lightBlendRGB565(value, light));
        }
      }
    }
  }

  //  Serial.println("");
}

void drawTreeTileMask(int16_t xMove, int16_t yMove, const unsigned char *bitmap, int16_t width, int16_t height, int light)
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
        uint16_t value = (bitmap[idx++]) | (bitmap[idx++] << 8);
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
          uint16_t value = (bitmap[idx++]) | (bitmap[idx++] << 8);
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
          uint16_t value = (bitmap[idx++]) | (bitmap[idx++] << 8);
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
