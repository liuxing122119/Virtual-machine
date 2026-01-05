#include "ttf_text.h"
#include <stdio.h>
#include <string.h>

// 初始化SDL_ttf库
// - 检查是否已初始化，避免重复初始化
// - 初始化SDL_ttf子系统
int ttf_text_init(void) {
    if (TTF_WasInit() == 0) {
        if (TTF_Init() != 0) {
            return -1;  // 初始化失败
        }
    }
    return 0;  // 成功
}

// 加载TTF字体文件
// - path: 字体文件路径
// - ptsize: 字体像素大小
TTF_Font* ttf_text_load_font(const char* path, int ptsize) {
    if (!path) return NULL;
    TTF_Font* f = TTF_OpenFont(path, ptsize);
    return f;  // 返回字体指针或NULL（失败）
}

// 绘制UTF-8文本到SDL渲染器
// - 使用SDL_ttf渲染混合文本
// - 创建临时纹理进行绘制
// - 自动清理临时资源
void ttf_text_draw(SDL_Renderer* renderer, TTF_Font* font, int x, int y, const char* text, SDL_Color color) {
    if (!renderer || !font || !text) return;
    // 渲染文本到表面
    SDL_Surface* surf = TTF_RenderUTF8_Blended(font, text, color);
    if (!surf) return;
    // 创建纹理
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surf);
    if (!tex) { SDL_FreeSurface(surf); return; }
    // 绘制到屏幕
    SDL_Rect dst = { x, y, surf->w, surf->h };
    SDL_RenderCopy(renderer, tex, NULL, &dst);
    // 清理资源
    SDL_DestroyTexture(tex);
    SDL_FreeSurface(surf);
}

// 释放字体资源
void ttf_text_free_font(TTF_Font* f) {
    if (f) TTF_CloseFont(f);
}

// 退出SDL_ttf库
void ttf_text_quit(void) {
    if (TTF_WasInit()) TTF_Quit();
}

// 测量UTF-8文本尺寸
// - 计算文本渲染后的宽度和高度
// - 用于布局计算和文本截断
int ttf_text_measure(TTF_Font* font, const char* text, int* out_w, int* out_h) {
    if (!font || !text) return -1;
    int w=0, h=0;
    if (TTF_SizeUTF8(font, text, &w, &h) != 0) return -1;  // 测量失败
    if (out_w) *out_w = w;
    if (out_h) *out_h = h;
    return 0;  // 成功
}