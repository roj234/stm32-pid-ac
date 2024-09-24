
//#define UI_ENABLE_BCD
#ifdef UI_ENABLE_BCD
void Gui_Num_BCD(u8 x, u8 y, u16 fg, u8 num);
#endif

extern u16 Gui_BgColor;

void Gui_Circle(u8 x, u8 y, u8 radius, u16 color);

void Gui_Line(u8 x0, u8 y0, u8 x1, u8 y1, u16 color);
void Gui_HLine(u8 x, u8 y, u8 len, u16 color);
void Gui_VLine(u8 x, u8 y, u8 len, u16 color);

void Gui_Box(u8 x1, u8 y1, u8 x2, u8 y2, u16 color);
void Gui_Btn(u8 x1, u8 y1, u8 x2, u8 y2, u8 pressed);

//居中
void Gui_Text_XCenter(u8 y, u16 color, char *str);
void Gui_Text(u8 x, u8 y, u16 color, char *str);
//ASCII only
void Gui_Char(u8 x, u8 y, u16 color, u8 chr);
void Gui_Num_Float(u8 x, u8 y, u16 color, int8_t point, int32_t num, int8_t len);
//无小数
__STATIC_INLINE void Gui_Num(u8 x, u8 y, u16 color, int32_t i, int8_t len) {Gui_Num_Float(x,y,color,0,i,len);}
// -24.5
__STATIC_INLINE void Gui_Num_3Temp(u8 x, u8 y, u16 color, int32_t i) {Gui_Num_Float(x,y,color,1,i,4);}
//12:00
void Gui_Num_4Time(u8 x, u8 y, u16 color, u16 num);
