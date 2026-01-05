#ifndef TTF_TEXT_H
#define TTF_TEXT_H

#include <SDL.h>
#include <SDL_ttf.h>

// 初始化SDL_ttf库
int ttf_text_init(void);

// 加载指定像素大小的字体文件
TTF_Font* ttf_text_load_font(const char* path, int ptsize);

// 使用TTF在指定渲染器上绘制UTF-8文本到指定位置和颜色
void ttf_text_draw(SDL_Renderer* renderer, TTF_Font* font, int x, int y, const char* text, SDL_Color color);

// 释放字体资源
void ttf_text_free_font(TTF_Font* f);

// 退出SDL_ttf库
void ttf_text_quit(void);

// 测量UTF-8文本的宽度和高度
int ttf_text_measure(TTF_Font* font, const char* text, int* out_w, int* out_h);

#endif // TTF_TEXT_H
