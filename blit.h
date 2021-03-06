#ifndef __BLIT_H__
#define __BLIT_H__

#include <vitasdkkern.h>

#define COLOR_CYAN 			0x00FFFF00
#define COLOR_MAGENDA 		0x00FF00FF
#define COLOR_YELLOW 		0x0000FFFF
#define COLOR_WHITE 		0x00FFFFFF
#define COLOR_BLUE 			0x00FF0000
#define COLOR_GREEN 		0x0000FF00
#define COLOR_BLACK 		0x00000000
#define COLOR_TRANSPARENT	0xFF000000

#define RGB(R,G,B)    (((B)<<16)|((G)<<8)|(R))
#define RGBT(R,G,B,T) (((T)<<24)|((B)<<16)|((G)<<8)|(R))

void blit_set_color(int fg_col,int bg_col);
int blit_string(int sx,int sy,const char *msg);
int blit_string_ctr(int sy,const char *msg);
int blit_string_center_x(int offset);
int blit_stringf(int sx, int sy, const char *msg, ...);
int blit_set_frame_buf(const SceDisplayFrameBuf *param);

#endif
