/*
 * C/C++ Image Blend Mode Library (using MACRO)
 * Luceefer 'at' AwpSpace
 * original source: http://inlandstudios.com/en/?p=851
 * convert to pointer by withlovei 'at' AwpSpace
 * ported to Qt4.7.4
 */
#ifndef BLENDMODE_H
#define BLENDMODE_H

#include <cmath>

/*
 * Separate channel blending with modes
 * params: B - Base Layer color (int), L - Blended Layer color (int)
 * return: result's channel color (int)
 */
#define ChannelBlend_Normal(B,L)     ((int)(B))
#define ChannelBlend_Lighten(B,L)    ((int)((L > B) ? L:B))
#define ChannelBlend_Darken(B,L)     ((int)((L > B) ? B:L))
#define ChannelBlend_Multiply(B,L)   ((int)((B * L) / 255))
#define ChannelBlend_Average(B,L)    ((int)((B + L) / 2))
#define ChannelBlend_Add(B,L)        ((int)(qMin(255, (B + L))))

//rewrite subtract mode
//#define ChannelBlend_Subtract(B,L)   ((int)((B + L < 255) ? 0:(B + L - 255)))
#define ChannelBlend_Subtract(B,L)   ((int)(qMax(0, (L-B))))
#define ChannelBlend_Difference(B,L) ((int)(qAbs(B - L)))
#define ChannelBlend_Negation(B,L)   ((int)(255 - abs(255 - B - L)))
#define ChannelBlend_Screen(B,L)     ((int)(255 - (((255 - B) * (255 - L)) >> 8)))
#define ChannelBlend_Exclusion(B,L)  ((int)(B + L - 2 * B * L / 255))
#define ChannelBlend_Overlay(B,L)    ((int)((B < 128) ? (2 * B * L / 255):(255 - 2 * (255 - B) * (255 - L) / 255)))

//rewrite softlight mode (there is no reliable documentation)
//#define ChannelBlend_SoftLight(B,L)  ((int)((L < 128)?(2*((B>>1)+64))*((float)L/255):(255-(2*(255-((B>>1)+64))*(float)(255-L)/255))))
//#define ChannelBlend_SoftLight(B,L)  ((int)((B > 127)?((L)+(255-L)*((B-127.5)/127.5)*(0.5-qAbs(L-127.5)/255)):(L-L*((127.5-B)/127.5)*(0.5-qAbs(L-127.5)/255))))
#define ChannelBlend_SoftLight(B,L)  ((int)((L < 128)?(B*(L + 128)/255):(255-(255-B)*(255-(L-128))/255)))
#define ChannelBlend_HardLight(B,L)  (ChannelBlend_Overlay(L,B))
#define ChannelBlend_ColorDodge(B,L) ((int)((L == 255) ? L:qMin(255, ((B << 8 ) / (255 - L)))))
#define ChannelBlend_ColorBurn(B,L)  ((int)((L == 0) ? L:qMax(0, (255 - ((255 - B) << 8 ) / L))))
#define ChannelBlend_LinearDodge(B,L)(ChannelBlend_Add(B,L))

//rewrite linearburn mode
//#define ChannelBlend_LinearBurn(B,L) (ChannelBlend_Subtract(B,L))
#define ChannelBlend_LinearBurn(B,L) ((int)(qMax(0, (B + L - 255))))
#define ChannelBlend_LinearLight(B,L)((int)(L < 128)?ChannelBlend_LinearBurn(B,(2 * L)):ChannelBlend_LinearDodge(B,(2 * (L - 128))))
#define ChannelBlend_VividLight(B,L) ((int)(L < 128)?ChannelBlend_ColorBurn(B,(2 * L)):ChannelBlend_ColorDodge(B,(2 * (L - 128))))
#define ChannelBlend_PinLight(B,L)   ((int)(L < 128)?ChannelBlend_Darken(B,(2 * L)):ChannelBlend_Lighten(B,(2 * (L - 128))))
#define ChannelBlend_HardMix(B,L)    ((int)((ChannelBlend_VividLight(B,L) < 128) ? 0:255))
#define ChannelBlend_Reflect(B,L)    ((int)((L == 255) ? L:qMin(255, (B * B / (255 - L)))))
#define ChannelBlend_Glow(B,L)       (ChannelBlend_Reflect(L,B))
#define ChannelBlend_Phoenix(B,L)    ((int)(qMin(B,L) - max(B,L) + 255))

//Add alpha parameter (opacity: float from 0.0 - 1.0
#define ChannelBlend_Alpha(B,L,O)    ((int)(O * B + (1 - O) * L))
#define ChannelBlend_AlphaF(B,L,F,O) (ChannelBlend_Alpha(F(B,L),B,O))

/*
 * Make a solid for blending
 * params B - base layer, C - color code, O - opacity, F - blend mode
 */
#define makeSolid(B,C,O,F) {                                \
    QImage solid = QImage(B.size(), B.format());            \
    QPainter p(&solid); p.fillRect(B.rect(), C); p.end();   \
    int r1, g1, b1, r2, g2, b2;                             \
    int *ptr1 = (int*)B.bits();                             \
    int *ptr2 = (int*)solid.bits();                         \
    int *end = ptr1 + B.width() * B.height();               \
    while(ptr1 != end) {                                    \
        r1 = (*ptr1 & 0xff);                                \
        g1 = ((*ptr1 >> 8) & 0xff);                         \
        b1 = ((*ptr1 >> 16) & 0xff);                        \
        r2 = (*ptr2 & 0xff);                                \
        g2 = ((*ptr2 >> 8) & 0xff);                         \
        b2 = ((*ptr2 >> 16) & 0xff);                        \
        r1 = ChannelBlend_AlphaF(r1, r2, F, O);             \
        g1 = ChannelBlend_AlphaF(g1, g2, F, O);             \
        b1 = ChannelBlend_AlphaF(b1, b2, F, O);             \
        *ptr1 = r1 | g1 << 8 | b1 << 16 | 0xFF000000;       \
        ptr1++; ptr2++;}}

/*
 * Make a gradient for blending
 * params B - base layer, G - gradient, O - opacity, F - blend mode
 */
#define makeGradient(B,G,O,F) {                                             \
    QImage imgGradient = QImage(B.size(), B.format());                      \
    QPainter p(&imgGradient); p.fillRect(imgGradient.rect(), G); p.end();   \
    int r1, g1, b1, r2, g2, b2;                                             \
    int *ptr1 = (int*)B.bits();                                             \
    int *ptr2 = (int*)imgGradient.bits();                                   \
    int *end = ptr1 + B.width() * B.height();                               \
    while(ptr1 != end) {                                                    \
        r1 = (*ptr1 & 0xff);                                                \
        g1 = ((*ptr1 >> 8) & 0xff);                                         \
        b1 = ((*ptr1 >> 16) & 0xff);                                        \
        r2 = (*ptr2 & 0xff);                                                \
        g2 = ((*ptr2 >> 8) & 0xff);                                         \
        b2 = ((*ptr2 >> 16) & 0xff);                                        \
        r1 = ChannelBlend_AlphaF(r1, r2, F, O);                             \
        g1 = ChannelBlend_AlphaF(g1, g2, F, O);                             \
        b1 = ChannelBlend_AlphaF(b1, b2, F, O);                             \
        *ptr1 = r1 | g1 << 8 | b1 << 16 | 0xFF000000;                       \
        ptr1++; ptr2++;}}

/*
 * Blend two layers B and L with mode F, opacity O
 */
#define makeBlend(B,L,O,F) {                            \
    int r1, g1, b1, r2, g2, b2;                         \
    int *ptr1 = (int*)B.bits();                         \
    int *ptr2 = (int*)L.bits();                         \
    int *end = ptr1 + B.width() * B.height();           \
    while(ptr1 != end) {                                \
        r1 = (*ptr1 & 0xff);                            \
        g1 = ((*ptr1 >> 8) & 0xff);                     \
        b1 = ((*ptr1 >> 16) & 0xff);                    \
        r2 = (*ptr2 & 0xff);                            \
        g2 = ((*ptr2 >> 8) & 0xff);                     \
        b2 = ((*ptr2 >> 16) & 0xff);                    \
        r1 = ChannelBlend_AlphaF(r1, r2, F, O);         \
        g1 = ChannelBlend_AlphaF(g1, g2, F, O);         \
        b1 = ChannelBlend_AlphaF(b1, b2, F, O);         \
        *ptr1 = r1 | g1 << 8 | b1 << 16 | 0xFF000000;   \
        ptr1++; ptr2++;}}

/*
 * To use the blending along with opacity, use the following.
 * Target[i] = ChannelBlend_AlphaF(Base[i], Blend[i], Blend_Subtract, 0.5F)
 * Color blending
 * To add certain blend modes that utilize hue, luminosity, and saturation
 * Construct a per-color interface instead of per-channel interface
 * For these macros assume that A and B are buffer pointers
 * and they point to bytes with channels red, green, and blue in that order
 * @fixed in Qt, change parameter to QColor, using source API setRed, setBlue and setGreen
 */

//Buffer for below MACROs
//T - destination, B - base, L - blend, M - mode (QColor)
#define ColorBlend_Buffer(T,B,L,M) {                            \
    (T).setRed(ChannelBlend_##M((B).red(), (L).red()));         \
    (T).setGreen(ChannelBlend_##M((B).green(), (L).green()));   \
    (T).setBlue(ChannelBlend_##M((B).blue(), (L).blue()));}

//call buffered macro
#define ColorBlend_Normal(T,B,L)        (ColorBlend_Buffer(T,B,L,Normal))
#define ColorBlend_Lighten(T,B,L)       (ColorBlend_Buffer(T,B,L,Lighten))
#define ColorBlend_Darken(T,B,L)        (ColorBlend_Buffer(T,B,L,Darken))
#define ColorBlend_Multiply(T,B,L)      (ColorBlend_Buffer(T,B,L,Multiply))
#define ColorBlend_Average(T,B,L)       (ColorBlend_Buffer(T,B,L,Average))
#define ColorBlend_Add(T,B,L)           (ColorBlend_Buffer(T,B,L,Add))
#define ColorBlend_Subtract(T,B,L)      (ColorBlend_Buffer(T,B,L,Subtract))
#define ColorBlend_Difference(T,B,L)    (ColorBlend_Buffer(T,B,L,Difference))
#define ColorBlend_Negation(T,B,L)      (ColorBlend_Buffer(T,B,L,Negation))
#define ColorBlend_Screen(T,B,L)        (ColorBlend_Buffer(T,B,L,Screen))
#define ColorBlend_Exclusion(T,B,L)     (ColorBlend_Buffer(T,B,L,Exclusion))
#define ColorBlend_Overlay(T,B,L)       (ColorBlend_Buffer(T,B,L,Overlay))
#define ColorBlend_SoftLight(T,B,L)     (ColorBlend_Buffer(T,B,L,SoftLight))
#define ColorBlend_HardLight(T,B,L)     (ColorBlend_Buffer(T,B,L,HardLight))
#define ColorBlend_ColorDodge(T,B,L)    (ColorBlend_Buffer(T,B,L,ColorDodge))
#define ColorBlend_ColorBurn(T,B,L)     (ColorBlend_Buffer(T,B,L,ColorBurn))
#define ColorBlend_LinearDodge(T,B,L)   (ColorBlend_Buffer(T,B,L,LinearDodge))
#define ColorBlend_LinearBurn(T,B,L)    (ColorBlend_Buffer(T,B,L,LinearBurn))
#define ColorBlend_LinearLight(T,B,L)   (ColorBlend_Buffer(T,B,L,LinearLight))
#define ColorBlend_VividLight(T,B,L)    (ColorBlend_Buffer(T,B,L,VividLight))
#define ColorBlend_PinLight(T,B,L)      (ColorBlend_Buffer(T,B,L,PinLight))
#define ColorBlend_HardMix(T,B,L)       (ColorBlend_Buffer(T,B,L,HardMix))
#define ColorBlend_Reflect(T,B,L)       (ColorBlend_Buffer(T,B,L,Reflect))
#define ColorBlend_Glow(T,B,L)          (ColorBlend_Buffer(T,B,L,Glow))
#define ColorBlend_Phoenix(T,B,L)       (ColorBlend_Buffer(T,B,L,Phoenix))

//must use HSL color model
#define ColorBlend_Hue(T,B,L)            ColorBlend_Hls(T,B,L,HueL,LuminationB,SaturationB)
#define ColorBlend_Saturation(T,B,L)     ColorBlend_Hls(T,B,L,HueB,LuminationB,SaturationL)
#define ColorBlend_Color(T,B,L)          ColorBlend_Hls(T,B,L,HueL,LuminationB,SaturationL)
#define ColorBlend_Luminosity(T,B,L)     ColorBlend_Hls(T,B,L,HueB,LuminationL,SaturationB)

//buffer for HSL color blend mode
#define ColorBlend_Hls(T,B,L,O1,O2,O3) {                                                    \
    float HueB, LuminationB, SaturationB, HueL, LuminationL, SaturationL;                   \
    Color_RgbToHls((B).red(),(B).green(),(B).blue(), &HueB, &LuminationB, &SaturationB);    \
    Color_RgbToHls((L).red(),(L).green(),(L).blue(), &HueL, &LuminationL, &SaturationL);    \
    int x, y, z; Color_HlsToRgb(O1,O2,O3,&x,&y,&z);                                         \
    (T).setRed(x); (T).setGreen(y); (T).setBlue(z);}

//convert color from Hue to RGB
int Color_HueToRgb(float M1, float M2, float Hue, float* Channel) {
    if (Hue < 0.0)
        Hue += 1.0;
    else if (Hue > 1.0)
        Hue -= 1.0;

    if ((6.0 * Hue) < 1.0)
        *Channel = (M1 + (M2 - M1) * Hue * 6.0);
    else if ((2.0 * Hue) < 1.0)
        *Channel = (M2);
    else if ((3.0 * Hue) < 2.0)
        *Channel = (M1 + (M2 - M1) * ((2.0F / 3.0F) - Hue) * 6.0);
    else
        *Channel = (M1);

    return TRUE;
}

//convert color from RGB to HSL
int Color_RgbToHls(int Red, int Green, int Blue, float* Hue, float* Lumination, float* Saturation) {
    float Delta;
    float Max, Min;
    float Redf, Greenf, Bluef;

    Redf    = ((float)Red   / 255.0F);
    Greenf  = ((float)Green / 255.0F);
    Bluef   = ((float)Blue  / 255.0F);

    Max     = qMax(qMax(Redf, Greenf), Bluef);
    Min     = qMin(qMin(Redf, Greenf), Bluef);

    *Hue        = 0;
    *Lumination = (Max + Min) / 2.0F;
    *Saturation = 0;

    if (Max == Min)
        return TRUE;

    Delta = (Max - Min);

    *Saturation = (*Lumination < 0.5) ? (Delta / (Max + Min)) : (Delta / (2.0 - Max - Min));

    if (Redf == Max)
        *Hue = (Greenf - Bluef) / Delta;
    else if (Greenf == Max)
        *Hue = 2.0 + (Bluef - Redf) / Delta;
    else
        *Hue = 4.0 + (Redf - Greenf) / Delta;

    *Hue /= 6.0;

    if (*Hue < 0.0)
        *Hue += 1.0;

    return TRUE;
}

//convert color from HSL to RGB
int Color_HlsToRgb(float Hue, float Lumination, float Saturation, int* Red, int* Green, int* Blue) {
    float M1, M2;
    float Redf, Greenf, Bluef;

    if (Saturation == 0) {
        Redf    = Lumination;
        Greenf  = Lumination;
        Bluef   = Lumination;
    } else {
        if (Lumination <= 0.5)
            M2 = Lumination * (1.0 + Saturation);
        else
            M2 = Lumination + Saturation - Lumination * Saturation;

        M1 = (2.0 * Lumination - M2);

        Color_HueToRgb(M1, M2, Hue + (1.0F / 3.0F), &Redf);
        Color_HueToRgb(M1, M2, Hue, &Greenf);
        Color_HueToRgb(M1, M2, Hue - (1.0F / 3.0F), &Bluef);
    }

    *Red    = (int)(Redf * 255);
    *Blue   = (int)(Bluef * 255);
    *Green  = (int)(Greenf * 255);

    return TRUE;
}

/*
 * Allow use the hue, saturation, color, and luminosity blend modes
 * to use the ColorBlend macros, position bitmap’s pointers to the next RGB iteration and call
 * ColorBlend_Glow(Target + iTarget, Base + iBase, Blend + iBlend);
 *
 * Color macros
 * Some color macros for combining and extracting channels colors out of an integer
 * One macro with a lot of use for is HexToRgb
 * Can copy and paste a hex color value out of the Photoshop color dialog
 * And use the macro to do all the conversion to an rgb value.
*/
#define COLOR_OPAQUE                (0)
#define COLOR_TRANSPARENT           (127)

#define RGB_SIZE                    (3)
#define RGB_BPP                     (24)
#define RGB_MAXRED                  (255)
#define RGB_MAXGREEN                (255)
#define RGB_MAXBLUE                 (255)

#define ARGB_SIZE                   (4)
#define ARGB_BPP                    (32)
#define ARGB_MAXALPHA               (127)
#define ARGB_MAXRED                 (RGB_MAXRED)
#define ARGB_MAXGREEN               (RGB_MAXGREEN)
#define ARGB_MAXBLUE                (RGB_MAXBLUE)

#define Color_GetChannel(c,shift)   ((int)((c) >> (shift)))
#define Color_Reverse(c,bpp)        ((((int)(c) << 24) | ((int)((c) >> 8 ) << 16) | ((int)((c) >> 16) << 8 ) | \
    ((int)((c) >> 24))) >> (32 - (bpp)))

#define Rgb_ByteWidth(width)        ((width) * RGB_SIZE)
#define Rgb_PixelWidth(width)       ((width) / RGB_SIZE)

#define Rgb_GetRed(rgb)             (Color_GetChannel(rgb, 0))
#define Rgb_GetGreen(rgb)           (Color_GetChannel(rgb, 8))
#define Rgb_GetBlue(rgb)            (Color_GetChannel(rgb, 16))

#define Rgba_GetRed(rgba)           (Color_GetChannel(rgba, 24))
#define Rgba_GetGreen(rgba)         (Color_GetChannel(rgba, 16))
#define Rgba_GetBlue(rgba)          (Color_GetChannel(rgba, 8))
#define Rgba_GetAlpha(rgba)         (Color_GetChannel(rgba, 0))

#define Argb_GetAlpha(argb)         (Color_GetChannel(argb, 24))
#define Argb_GetRed(argb)           (Color_GetChannel(argb, 16))
#define Argb_GetGreen(argb)         (Color_GetChannel(argb, 8))
#define Argb_GetBlue(argb)          (Color_GetChannel(argb, 0))

#define MakeRgb(r,g,b)              (((uint)(int)(b) << 16) | ((uint16)(int)(g) << 8 ) | (int)(r))
#define MakeRgba(r,g,b,a)           (((uint)(int)(r) << 24) | ((uint16)(int)(g) << 16) | ((uint16)(int)(b) << 8 ) | (int)(a))
#define MakeArgb(a,r,g,b)           (((uint)(int)(a) << 24) | ((uint)(int)(r) << 16) | ((uint16)(int)(g) << 8 ) | (int)(b))

#define HexToRgb(hex)               (MakeRgb(((hex & 0xFF0000) >> 16), ((hex & 0x00FF00) >> 8 ), (hex & 0xFF)))

#endif // BLENDMODE_H