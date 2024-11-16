// #include "U8g2lib.h"
#include "u8g2TextBox.h"
u8g2TextBox::u8g2TextBox(U8G2 *Display, u8g2_uint_t X, u8g2_uint_t Y, u8g2_uint_t W, u8g2_uint_t H, const uint8_t *Font, String Header, String Footer, boolean blnDrawBox, u8g2_uint_t Margin, u8g2_uint_t BorderThickness, TextAlignments HorizontalAlignment, TextAlignments VerticalALignment)
{
    u8g2TextBox::Display = Display;
    CalculateTextPositions(X, Y, W, H);
    u8g2TextBox::Header = Header;
    u8g2TextBox::Footer = Footer;
    u8g2TextBox::blnShowBox = blnDrawBox;
    u8g2TextBox::Margin = Margin;
    u8g2TextBox::BorderThickness = BorderThickness;
    u8g2TextBox::HorizontalAlignment = HorizontalAlignment;
    u8g2TextBox::VerticalALignment = VerticalALignment;
    u8g2TextBox::Font = Font;
    u8g2TextBox::Display = Display;
    u8g2TextBox::Display->setFont(Font);
    int Hm = Display->getMaxCharHeight();
    int Dc = Display->getDescent();
    u8g2TextBox::OffsetY = (u8g2TextBox::H - (Hm + Dc)) / 2;
    if (((u8g2TextBox::H - (Hm + Dc)) % 2) > 0)
        u8g2TextBox::OffsetY++; // prefere to put the text above the centre line due to Decent size of text
}
void u8g2TextBox::CalculateTextPositions(u8g2_uint_t X, u8g2_uint_t Y, u8g2_uint_t W, u8g2_uint_t H)
{
    int32_t MaxY = Display->getDisplayHeight() - 1;
    int32_t MaxX = Display->getDisplayWidth() - 1;
    u8g2_uint_t X2=X+W-1;
    u8g2_uint_t Y2=Y+H-1;
    if (X > MaxX)
        X = MaxX;
    if (X2 > MaxX)
        X2 = MaxX;
    if (Y > MaxY)
        Y = MaxY;
    if (Y2 > MaxY)
        Y2 = MaxY;
    if (X < X2)
    {
        u8g2TextBox::X1 = X;
        u8g2TextBox::X2 = X2;
    }
    else
    {
        u8g2TextBox::X1 = X2;
        u8g2TextBox::X2 = X;
    }
    if (Y1 < Y2)
    {
        u8g2TextBox::Y1 = Y;
        u8g2TextBox::Y2 = Y2;
    }
    else
    {
        u8g2TextBox::Y1 = Y2;
        u8g2TextBox::Y2 = Y;
    }
    u8g2TextBox::H = (u8g2TextBox::Y2 - u8g2TextBox::Y1 + 1);
    u8g2TextBox::W = (u8g2TextBox::X2 - u8g2TextBox::X1 + 1);
    u8g2TextBox::Wt = (u8g2TextBox::X2 - u8g2TextBox::X1 + 1) / 8;
    u8g2TextBox::Ht = (u8g2TextBox::Y2 - u8g2TextBox::Y1 + 1) / 8;
    u8g2TextBox::Xt = u8g2TextBox::X1 / 8;
    u8g2TextBox::Yt = u8g2TextBox::Y1 / 8;
}
void u8g2TextBox::ShowData(String Data)
{
    String S = Header + Data + Footer;
    u8g2TextBox::Display->setFont(Font);
    u8g2TextBox::Display->drawStr(X1 + Margin, Y2 - OffsetY, S.c_str()); // write something to the internal memory
}
void u8g2TextBox::u8g2TextBox::Show(String strData)
{
    Display->clearBuffer(); // clear the internal memory
    ShowData(strData);      // Draw the data
    if (blnShowBox)
        ShowBox(); // Draw the box
    u8g2updateDisplay(); // Update the display
}
void u8g2TextBox::u8g2TextBox::Show(int32_t Data)
{
    Show(String(Data)); // Draw the data
}
void u8g2TextBox::u8g2TextBox::Show()
{
    Show(""); // Draw the header and box
}
void u8g2TextBox::Show(double_t Data, int32_t MinWidth, int32_t Precision)
{
    char buff[10];
    dtostrf(Data, MinWidth, Precision, buff);
    Show(buff);
}
// Update the display in tiles (boxes of 8x8 pixels)
void u8g2TextBox::u8g2updateDisplay(u8g2_uint_t X1, u8g2_uint_t Y1, u8g2_uint_t X2, u8g2_uint_t Y2)
{
    u8g2_uint_t Xt = X1 / 8;
    u8g2_uint_t Yt = Y2 / 8;
    u8g2_uint_t Wt = (X2 - X1) / 8;
    u8g2_uint_t Ht = (Y2 - Y1) / 8;
    Display->updateDisplayArea(Xt, Yt, Wt, Ht);
}
void u8g2TextBox::u8g2updateDisplay()
{
    Display->updateDisplayArea(Xt, Yt, Wt, Ht);
}
void u8g2TextBox::ShowBox(u8g2_uint_t X1, u8g2_uint_t Y1, u8g2_uint_t X2, u8g2_uint_t Y2)
{
    Display->drawLine(X1, Y1, X1, Y2);
    Display->drawLine(X1, Y2, X2, Y2);
    Display->drawLine(X2, Y2, X2, Y1);
    Display->drawLine(X2, Y1, X1, Y1);
}
void u8g2TextBox::u8g2TextBox::ShowBox()
{
    for (u8g2_uint_t i = 0; i < BorderThickness; i++)
    {
        ShowBox(X1 + i, Y1 + i, X2 - i, Y2 - i);
    }
}
void u8g2TextBox::ShowAlive()
{
    static char LastChar='|';
    char NextChar;
    switch (LastChar)
    {
        case '|':NextChar='/';break;
        case '/':NextChar='-';break;
        case '-':NextChar='\\';break;
        case '\\':NextChar='|';break;
    }
    LastChar=NextChar;
    String S(NextChar);
    Show(S);
}
