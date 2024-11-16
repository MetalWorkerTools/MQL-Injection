#pragma once
#include "U8g2lib.h"

enum TextAlignments
{
    AlignmentLeft,
    AlignmentCenter,
    AlignmentRight,
    AlignmentTop,
    AlignmentBottom
};

class u8g2TextBox
{
public:
    U8G2 *Display;
    u8g2_uint_t X1 = 0;
    u8g2_uint_t Y1 = 0;
    u8g2_uint_t X2 = 7;
    u8g2_uint_t Y2 = 7;
    u8g2_uint_t Xt = 0;
    u8g2_uint_t Yt = 0;
    u8g2_uint_t W = 1;
    u8g2_uint_t H = 1;
    u8g2_uint_t Wt = 1;
    u8g2_uint_t Ht = 1;
    u8g2_uint_t Margin = 3;
    u8g2_uint_t OffsetX = 3;
    u8g2_uint_t OffsetY = 3;
    u8g2_uint_t BorderThickness = 1;
    TextAlignments HorizontalAlignment = TextAlignments::AlignmentCenter;
    TextAlignments VerticalALignment = TextAlignments::AlignmentCenter;
    String Header = "";
    String Footer = "";
    boolean blnShowBox = false;
    const uint8_t *Font;
    u8g2TextBox(U8G2 *Display, u8g2_uint_t X, u8g2_uint_t Y, u8g2_uint_t W, u8g2_uint_t H, const uint8_t *Font, String Header, String Footer, boolean DrawBox, u8g2_uint_t Margin, u8g2_uint_t BorderThickness, TextAlignments HorizontalAlignment, TextAlignments VerticalALignment);
    u8g2TextBox(U8G2 *Display, u8g2_uint_t X, u8g2_uint_t Y, u8g2_uint_t W, u8g2_uint_t H, const uint8_t *Font, String Header, String Footer, boolean DrawBox, u8g2_uint_t Margin, u8g2_uint_t BorderThickness) : u8g2TextBox(Display, X, Y, W, H, Font, Header, Footer, DrawBox, Margin, BorderThickness, TextAlignments::AlignmentCenter, TextAlignments::AlignmentCenter) { ; }
    u8g2TextBox(U8G2 *Display, u8g2_uint_t X, u8g2_uint_t Y, u8g2_uint_t W, u8g2_uint_t H, const uint8_t *Font, String Header, String Footer, boolean DrawBox) : u8g2TextBox(Display, X, Y, W, H, Font, Header, Footer, DrawBox, 2, 1) { ; }
    u8g2TextBox(U8G2 *Display, u8g2_uint_t X, u8g2_uint_t Y, u8g2_uint_t W, u8g2_uint_t H) : u8g2TextBox(Display, X, Y, W, H, Font, Header, Footer, blnShowBox) { ; }
    u8g2TextBox(u8g2TextBox *Reference, u8g2_uint_t dX, u8g2_uint_t dY, String Header, String Footer) : u8g2TextBox(Reference->Display, Reference->X1 + dX, Reference->Y1 + dY, Reference->W, Reference->H, Reference->Font, Header, Footer, Reference->blnShowBox, Reference->Margin, Reference->BorderThickness) { ; }
    u8g2TextBox(u8g2TextBox *Reference, u8g2_uint_t dX, u8g2_uint_t dY, u8g2_uint_t W, u8g2_uint_t H,String Header, String Footer) : u8g2TextBox(Reference->Display, Reference->X1 + dX, Reference->Y1 + dY, W, H, Reference->Font, Header, Footer, Reference->blnShowBox, Reference->Margin, Reference->BorderThickness) { ; }
    void CalculateTextPositions(u8g2_uint_t X, u8g2_uint_t Y, u8g2_uint_t W, u8g2_uint_t H);
    void u8g2updateDisplay(u8g2_uint_t X1, u8g2_uint_t Y1, u8g2_uint_t X2, u8g2_uint_t Y2);
    void u8g2updateDisplay();
    void ShowData(String Data);
    void ShowBox(u8g2_uint_t X1, u8g2_uint_t Y1, u8g2_uint_t X2, u8g2_uint_t Y2);
    void ShowBox();
    void Show(String Data);
    void Show(int32_t Data);
    void Show(double_t Data, int32_t MinWidth, int32_t Precision);
    // void Show(double_t Data);
    void Show();
    void ShowAlive();

private:
};