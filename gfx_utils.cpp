#include <Adafruit_Arcada.h>
#include <Adafruit_GFX.h>

#include "gfx_utils.h"

extern GFXcanvas16 *canvas;

uint16_t rgbTo565(uint8_t red, uint8_t green, uint8_t blue)
{
  uint16_t Rgb565 = (((red & 0xf8) << 8) + ((green & 0xfc) << 3) + (blue >> 3));

  return Rgb565;
}
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
            if (coeff <= 0.04f)
              canvas->drawPixel(xMove + i, yMove, ARCADA_BLACK);
            else
            {
              uint8_t r = ((((value >> 11) & 0x1F) * 527) + 23) >> 6;
              uint8_t g = ((((value >> 5) & 0x3F) * 259) + 33) >> 6;
              uint8_t b = (((value & 0x1F) * 527) + 23) >> 6;
              value = rgbTo565(int(r * coeff), int(g * coeff), int(b * coeff));
              canvas->drawPixel(xMove + i, yMove, value);
            }
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
            if (coeff <= 0.04f)
              canvas->drawPixel((xMove + width - 1) - i, yMove, ARCADA_BLACK);
            else
            {
              uint8_t r = ((((value >> 11) & 0x1F) * 527) + 23) >> 6;
              uint8_t g = ((((value >> 5) & 0x3F) * 259) + 33) >> 6;
              uint8_t b = (((value & 0x1F) * 527) + 23) >> 6;
              value = rgbTo565(r * coeff, g * coeff, b * coeff);
              canvas->drawPixel((xMove + width - 1) - i, yMove, value);
            }
          }
        }
        //    Serial.println("");
      }
    }
  }

  //  Serial.println("");
}

void drawWaterTile(int16_t px, int16_t py, int waterLevel)
{
  int r, g, b = 0;
  switch (waterLevel)
  {
    case 7:
      r = 5;
      g = 26;
      b = 115;
      break;
    case 6:
      r = 6;
      g = 35;
      b = 131;
      break;
    case 5:
      r = 2;
      g = 51;
      b = 147;
      break;
    case 4:
      r = 0;
      g = 116;
      b = 201;
      break;
    case 3:
      r = 0;
      g = 162;
      b = 249;
      break;
    case 2:
      r = 0;
      g = 196;
      b = 249;
      break;
    case 1:
      r = 22;
      g = 239;
      b = 249;
      break;
    
    default:
      return;
      
  }
  canvas->fillRect(px, py, 16, 16, rgbTo565(r, g, b));  
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
      float coeff = (float)light / MAX_LIGHT_INTENSITY;

      for (int16_t j = 0; j < 16; j++, yMove++)
      {
        for (int16_t i = 0; i < 16; i++)
        {
          uint16_t value = (bitmap[idx++]) + (bitmap[idx++] << 8);
          //   Serial.printf("%d,", value);
          if (value != 0xF81F)
          {
            if (coeff <= 0.04f)
              canvas->drawPixel(xMove + i, yMove, ARCADA_BLACK);
            else
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
  }

  //  Serial.println("");
}

void drawTile2(int16_t xMove, int16_t yMove, const unsigned char *bitmap, int curLight_TL, int curLight_BR, int lightX, int lightY)
{
  uint16_t idx = 0;

  if (curLight_TL < 0 || curLight_TL == MAX_LIGHT_INTENSITY)
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
    float coeff = (float)curLight_TL / MAX_LIGHT_INTENSITY;
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

      float stepY = (curLight_BR - curLight_TL) / 16;

      for (int16_t j = 0; j < 16; j++, yMove++)
      {
        for (int16_t i = 0; i < 16; i++)
        {
          int px = xMove + i;
          int py = yMove;
          
          float curLight = curLight_TL + (stepY * j);

            float distPlayer = sqrt((lightX - px) * (lightX - px) + (lightY - py) * (lightY - py));
            // @todo gerer la non propagassion de la lumiere dans les murs...
            if (distPlayer < 64)
            {
                curLight = curLight + (PLAYER_LIGHT_INTENSITY / distPlayer);
            }
            curLight = min(curLight, MAX_LIGHT_INTENSITY);
            float coeff = (float)curLight / MAX_LIGHT_INTENSITY;

          uint16_t value = (bitmap[idx++]) + (bitmap[idx++] << 8);
          //   Serial.printf("%d,", value);
          if (value != 0xF81F)
          {
            if (coeff == 0)
              canvas->drawPixel(px, py, ARCADA_BLACK);
            else
            {
              uint8_t r = ((((value >> 11) & 0x1F) * 527) + 23) >> 6;
              uint8_t g = ((((value >> 5) & 0x3F) * 259) + 33) >> 6;
              uint8_t b = (((value & 0x1F) * 527) + 23) >> 6;
              value = rgbTo565(int(r * coeff), int(g * coeff), int(b * coeff));
              canvas->drawPixel(px, py, value);
            }
            
          }
        }
        
     }
    }
  }

  //  Serial.println("");
}