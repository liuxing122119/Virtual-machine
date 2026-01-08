#include <SDL.h>
#ifndef SDL_MAIN_HANDLED
#define SDL_MAIN_HANDLED
#endif
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "tetris.h"
#include "loader.h"
#include "save_load.h"
#include "keymap.h"
#include "ttf_text.h"

/* 帮助界面已移除，相关滚动与测量函数不再需要 */

// 游戏状态枚举
// - GAME_STATE_MENU: 主菜单界面
// - GAME_STATE_PLAYING: 游戏进行中
// - GAME_STATE_PAUSED: 游戏暂停
// - GAME_STATE_KEY_CONFIG_UI: 图形界面的按键设置（上下选择、回车修改）
typedef enum {
    GAME_STATE_MENU,
    GAME_STATE_PLAYING,
    GAME_STATE_PAUSED,
    GAME_STATE_KEY_CONFIG_UI
} GameState;

// 主菜单选项枚举（只保留“开始游戏”）
typedef enum {
    MENU_START_GAME, // 开始游戏
    MENU_COUNT
} MenuItem;



// 菜单渲染函数
void render_menu(SDL_Renderer* renderer, Tetris* t, MenuItem selected_item, TTF_Font* font) {
    // 清屏为黑色背景
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    const char* title = "TETRIS";
    int title_cell = 18;
    int title_x = 400 - (title_cell * (int)strlen(title)); 
    int title_y = 100;
    if (font) ttf_text_draw(renderer, font, title_x, title_y, "俄罗斯方块", (SDL_Color){255,255,255,255});
    else tetris_draw_text(t, title_x, title_y, title, title_cell);

    // 菜单项（中文标签与英文回退）
    const char* menu_items[] = { "Start Game" };
    const char* menu_items_cn[] = { "开始游戏" };

    for (int i = 0; i < MENU_COUNT; i++) {
        int item_y = 200 + i * 60;
        int menu_cell = 12;
        int item_x = 400 - (menu_cell * (int)strlen(menu_items[i])); 

        if (i == selected_item) {        
            SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
            int char_w = 2 * menu_cell;
            int char_h = 5 * (menu_cell / 3);
            SDL_Rect highlight = { item_x - 10, item_y - 6, (int)(char_w * strlen(menu_items[i]) + 20), (int)(char_h + 12) };
            SDL_RenderFillRect(renderer, &highlight);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        }

        if (font) {
            int tw, th;
            if (ttf_text_measure(font, menu_items_cn[i], &tw, &th) == 0) {
                int cx = 800/2 - tw/2;
                int cy = item_y;
                if (i == selected_item) {
                    SDL_SetRenderDrawColor(renderer, 255,255,0,255);
                    SDL_Rect hr = { cx - 10, cy - 6, tw + 20, th + 12 };
                    SDL_RenderFillRect(renderer, &hr);
                    ttf_text_draw(renderer, font, cx, cy, menu_items_cn[i], (SDL_Color){0,0,0,255});
                } else {
                    ttf_text_draw(renderer, font, cx, cy, menu_items_cn[i], (SDL_Color){255,255,255,255});
                }
            } else {
                ttf_text_draw(renderer, font, item_x, item_y, menu_items[i], (SDL_Color){255,255,255,255});
            }
        } else {
            tetris_draw_text(t, item_x, item_y, menu_items[i], menu_cell);
        }
    }
}




int main(int argc, char* argv[]) {
    int main(int argc, char* argv[]) {
    (void)argc; (void)argv;
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
        printf("SDL_Init error: %s\n", SDL_GetError());
        return 1;
    }
    SDL_EventState(SDL_DROPFILE, SDL_ENABLE);

    SDL_Window* window = SDL_CreateWindow("Tetris - Test",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        800, 640, SDL_WINDOW_SHOWN);
    if (!window) {
        printf("CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        printf("CreateRenderer failed: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    Tetris game;
    tetris_init(&game, renderer);
    keymap_init();
    if (keymap_load("key_config.txt") != 0) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "Keymap load failed", "Could not load key_config.txt, using defaults", window);
    }

    // 初始化 TTF（TrueType 字体）并加载用于菜单与模态对话框的中文字体
    TTF_Font* menu_font = NULL;
    TTF_Font* modal_font = NULL;
    if (ttf_text_init() == 0) {
        const char* font_path = "assets/fonts/AlimamaShuHeiTi-Bold.ttf";
        menu_font = ttf_text_load_font(font_path, 36);
        modal_font = ttf_text_load_font(font_path, 20);
        if (!menu_font) {
            char warn[256];
            snprintf(warn, sizeof(warn), "Could not load font '%s'. Menu will use pixel font. Please place a suitable TTF at this path.", font_path);
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "Font load failed", warn, window);
        }
    } else {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "SDL_ttf init failed", "SDL_ttf could not be initialized. Menu will use pixel font.", window);
    }
    // 如果加载了 TTF（TrueType 字体），则禁用 tetris 内置的像素数字 HUD（像素 HUD），使用 TTF 绘制更易读的信息面板
    if (menu_font) game.draw_default_hud = 0;

    // 初始化游戏状态机与默认选择
    GameState game_state = GAME_STATE_MENU;
    MenuItem selected_menu_item = MENU_START_GAME;

    int running = 1;
    static int selected_slot = 1;
    // 左侧按钮相关状态（仅用于绘制与键盘选择反馈）
    int left_hover = -1;
    int left_selected = -1;
    const char* left_labels_cn[] = { "保存存档", "读取存档", "按键设置", "关卡选择" };
    int left_count = 4;
    const char* sample_levels[] = { "roms/level_easy.tetrislvl", "roms/level_medium.tetrislvl", "roms/level_hard.tetrislvl" };
    int sample_level_index = 0;
    // 保存/读取模态对话框的运行态变量
    int show_save_dialog = 0;
    int show_load_dialog = 0;
    int dialog_hover_slot = -1;
    int dialog_hover_delete = -1;
    int show_level_dialog = 0;
    const int SAVE_SLOTS = 5;
         int keycfg_selected = 0;
    int keycfg_editing = 0;
    const TetrisAction keycfg_actions[] = { ACTION_MOVE_LEFT, ACTION_MOVE_RIGHT, ACTION_SOFT_DROP, ACTION_ROTATE, ACTION_HARD_DROP, ACTION_SPEED_UP, ACTION_SPEED_DOWN };
    const char* keycfg_labels[] = { "左移", "右移", "向下加速", "旋转", "硬降", "速度增加", "速度减少" };
    const int KEYCFG_COUNT = sizeof(keycfg_actions) / sizeof(keycfg_actions[0]);

    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            switch (game_state) {
                case GAME_STATE_PLAYING:
                    if (sym == SDLK_ESCAPE) {
                        game_state = GAME_STATE_MENU;
                        SDL_SetWindowTitle(window, "Tetris - Menu");
                    } else if (sym == SDLK_SPACE) {
                        // 切换暂停（由空格触发）
                        game_state = GAME_STATE_PAUSED;
                        tetris_set_hud_message(&game, "已暂停", 1200);
                    } else {
                        SDL_Keymod mod = ev.key.keysym.mod;

                        if (sym == SDLK_KP_PLUS || (sym == SDLK_EQUALS && (mod & KMOD_SHIFT))) {
                            float cur = tetris_get_speed_multiplier(&game);
                            cur += 0.1f;
                            if (cur > 2.0f) cur = 2.0f;
                            tetris_set_speed_multiplier(&game, cur);
                            char title[128];
                            snprintf(title, sizeof(title), "Tetris - Speed: %d%%", (int)(cur * 100.0f));
                            SDL_SetWindowTitle(window, title);
                        } else if (sym == SDLK_KP_MINUS || sym == SDLK_MINUS) {
                            float cur = tetris_get_speed_multiplier(&game);
                            cur -= 0.1f;
                            if (cur < 0.5f) cur = 0.5f;
                            tetris_set_speed_multiplier(&game, cur);
                            char title[128];
                            snprintf(title, sizeof(title), "Tetris - Speed: %d%%", (int)(cur * 100.0f));
                            SDL_SetWindowTitle(window, title);
                        } else {
                            // 处理已映射的按键行为（由 keymap 提供）
                            // 包括速度增加/减少两个动作（ACTION_SPEED_UP / ACTION_SPEED_DOWN）
                            TetrisAction act = keymap_get_action(sym);
                            if (act != ACTION_NONE) {
                                if (act == ACTION_SPEED_UP) {
                                    float cur = tetris_get_speed_multiplier(&game);
                                    cur += 0.1f;
                                    if (cur > 2.0f) cur = 2.0f;
                                    tetris_set_speed_multiplier(&game, cur);
                                    char msgsp[64];
                                    snprintf(msgsp, sizeof(msgsp), "速度: %d%%", (int)(cur * 100.0f));
                                    tetris_set_hud_message(&game, msgsp, 1200);
                                } else if (act == ACTION_SPEED_DOWN) {
                                    float cur = tetris_get_speed_multiplier(&game);
                                    cur -= 0.1f;
                                    if (cur < 0.5f) cur = 0.5f;
                                    tetris_set_speed_multiplier(&game, cur);
                                    char msgsp[64];
                                    snprintf(msgsp, sizeof(msgsp), "速度: %d%%", (int)(cur * 100.0f));
                                    tetris_set_hud_message(&game, msgsp, 1200);
                                } else {
                                    tetris_perform_action(&game, (int)act);
                                }
                            }
                        }
                    }
                    break;
            }
        } if (ev.type == SDL_KEYDOWN) {
            if (ev.key.keysym.sym == SDLK_ESCAPE) running = 0;
            // 速度控制：小键盘 +/- 或主键盘 - / =（Shift+= 视为 +）
            SDL_Keycode sym = ev.key.keysym.sym;
            SDL_Keymod mod = ev.key.keysym.mod;

            if (sym == SDLK_KP_PLUS || (sym == SDLK_EQUALS && (mod & KMOD_SHIFT))) {
                float cur = tetris_get_speed_multiplier(&game);
                cur += 0.1f;
                if (cur > 2.0f) cur = 2.0f;
                tetris_set_speed_multiplier(&game, cur);
                char title[128];
                snprintf(title, sizeof(title), "Tetris - Speed: %d%%", (int)(cur * 100.0f));
                SDL_SetWindowTitle(window, title);
            } else if (sym == SDLK_KP_MINUS || sym == SDLK_MINUS) {
                float cur = tetris_get_speed_multiplier(&game);
                cur -= 0.1f;
                if (cur < 0.5f) cur = 0.5f;
                tetris_set_speed_multiplier(&game, cur);
                char title[128];
                snprintf(title, sizeof(title), "Tetris - Speed: %d%%", (int)(cur * 100.0f));
                SDL_SetWindowTitle(window, title);
            }
        }
    }
    SDL_RenderPresent(renderer);
    SDL_Delay(16);
}
 if (menu_font) ttf_text_free_font(menu_font);
    if (modal_font) ttf_text_free_font(modal_font);
    ttf_text_quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

