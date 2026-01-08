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
            // 如果当前有模态对话框（保存/读取/关卡弹窗）打开：
            // - 允许按 ESC 取消并关闭对话框（不影响主游戏状态）
            // - 该分支优先处理以避免对话框下方的界面接收按键
            if (ev.type == SDL_KEYDOWN && (show_save_dialog || show_load_dialog || show_level_dialog)) {
                if (ev.key.keysym.sym == SDLK_ESCAPE) {
                    show_save_dialog = 0;
                    show_load_dialog = 0;
                    show_level_dialog = 0;
                    dialog_hover_slot = -1;
                    tetris_set_hud_message(&game, "已取消", 800);
                    continue;
                }
            }
            // 当处于按键编辑模式时接收文本输入事件（字母键通常通过 SDL_TEXTINPUT 到达）
            // 处理流程：
            // - 在进入编辑模式时启用 SDL_StartTextInput()
            // - 在接收到 SDL_TEXTINPUT 后，用 SDL_GetKeyFromName 将文本转换为 SDL_Keycode
            // - 绑定成功后保存配置并关闭编辑模式，调用 SDL_StopTextInput()
            if (ev.type == SDL_TEXTINPUT) {
                if (game_state == GAME_STATE_KEY_CONFIG_UI && keycfg_editing) {
                    const char* txt = ev.text.text;
                    SDL_Keycode kc = SDL_GetKeyFromName(txt);
                    if (kc != SDLK_UNKNOWN) {
                        TetrisAction act = keycfg_actions[keycfg_selected];
                        keymap_set_binding(act, kc);
                        if (keymap_save("key_config.txt") == 0) {
                            char m[128];
                            snprintf(m, sizeof(m), "绑定成功: %s => %s", keycfg_labels[keycfg_selected], SDL_GetKeyName(kc));
                            tetris_set_hud_message(&game, m, 1200);
                        } else {
                            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Save failed", "Could not save key_config.txt", window);
                        }
                    } else {
                        tetris_set_hud_message(&game, "无法识别的按键", 1200);
                    }
                    keycfg_editing = 0;
                    SDL_StopTextInput();
                    continue;
                }
            }
            if (ev.type == SDL_QUIT) {
                running = 0;
            } else if (ev.type == SDL_MOUSEMOTION || ev.type == SDL_MOUSEBUTTONDOWN) {
                // 鼠标移动 / 鼠标按下事件
                // 优先处理模态对话框内的鼠标位置检测（仅用于悬停/高亮；点击已禁用）
                // 这样可以保证在对话框打开时，鼠标不会误触发底层界面元素
                if (show_save_dialog || show_load_dialog || show_level_dialog) {
                    int win_w = 800, win_h = 600;
                    SDL_GetWindowSize(window, &win_w, &win_h);
                    int modal_w = win_w - 100;
                    if (modal_w < 420) modal_w = 420;
                    if (modal_w > win_w - 40) modal_w = win_w - 40;
                    // 测量标题/提示文本高度，用于计算模态框高度，保证 hit-test 与渲染一致
                    const char* raw_title = show_save_dialog ? "选择存档位置" : "选择读取存档";
                    const char* top_hint = "按 ESC 返回";
                    const char* modal_hint = "上/下 选择, Enter 确认, Delete 删除, Esc 取消";
                    int title_h = 20, hint_h = 18, bottom_hint_h = 18;
                    if (menu_font) {
                        int tw, th;
                        if (ttf_text_measure(menu_font, raw_title, &tw, &th) == 0) title_h = th;
                        if (ttf_text_measure(menu_font, top_hint, &tw, &th) == 0) hint_h = th;
                        if (ttf_text_measure(menu_font, modal_hint, &tw, &th) == 0) bottom_hint_h = th;
                    } else if (modal_font) {
                        int tw, th;
                        if (ttf_text_measure(modal_font, raw_title, &tw, &th) == 0) title_h = th;
                        if (ttf_text_measure(modal_font, top_hint, &tw, &th) == 0) hint_h = th;
                        if (ttf_text_measure(modal_font, modal_hint, &tw, &th) == 0) bottom_hint_h = th;
                    }
                    int header_h = 8 + title_h + 6 + hint_h + 8; // 顶部/标题/提示的内边距计算
                    const int slot_step = 64;
                    const int slot_inner_h = 56;
                    int modal_h = header_h + SAVE_SLOTS * slot_step + bottom_hint_h + 20;
                    int mx = (ev.type == SDL_MOUSEMOTION) ? ev.motion.x : ev.button.x;
                    int my = (ev.type == SDL_MOUSEMOTION) ? ev.motion.y : ev.button.y;
                    int modal_x = (win_w - modal_w) / 2;
                    int modal_y = (win_h - modal_h) / 2;
                    int hover_slot = -1;
                    int hover_delete = -1;
                    const int del_w = 72, del_h = 28;
                    int slot_y0 = modal_y + header_h;
                    for (int i = 0; i < SAVE_SLOTS; ++i) {
                        int sx = modal_x + 12;
                        int sy = slot_y0 + i * slot_step;
                        int del_x = modal_x + modal_w - 12 - del_w;
                        int del_y = sy + (slot_inner_h - del_h) / 2;
                        // 先检测删除按钮（使用与绘制时相同的矩形）
                        if (mx >= del_x && mx < del_x + del_w && my >= del_y && my < del_y + del_h) { hover_delete = i; break; }
                        // 主体槽区域（排除删除按钮所在区域）
                        int body_w = modal_w - 24 - del_w - 8;
                        if (mx >= sx && mx < sx + body_w && my >= sy && my < sy + slot_inner_h) { hover_slot = i; break; }
                    }
                    dialog_hover_slot = hover_slot;
                    dialog_hover_delete = hover_delete;
                    // 键盘快捷键与拖放文件交互
                    if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT) {
                        // 消耗事件但不执行任何保存/读取/删除动作
                        dialog_hover_slot = -1;
                        dialog_hover_delete = -1;
                    }
                    // 对话框消费事件（禁用对话框内鼠标点击操作，鼠标点击仅用于探测悬停）
                    continue;
                }
                // 计算整个 UI 组的居中布局（左侧按钮 + 游戏区 + 右侧信息面板）
                // 然后基于计算结果生成左侧按钮的矩形用于悬停/碰撞检测（鼠标仅用于悬停）
                int btn_w = 120, btn_h = 36, btn_gap = 12;
                int win_w = 800, win_h = 600;
                SDL_GetWindowSize(window, &win_w, &win_h);
                int left_w = btn_w;
                int left_total_h = left_count * btn_h + (left_count - 1) * btn_gap;
                int play_cell = 24;
                int play_w = TETRIS_WIDTH * play_cell;
                int play_h = TETRIS_HEIGHT * play_cell;
                // 如果可用 TTF（TrueType 字体），测量右侧标签宽度以估算右侧区域宽度
                int max_label_w = 0;
                if (menu_font) {
                    const char* labels_tmp[] = { "下一个", "分数", "等级", "消行", "速度" };
                    for (int i = 0; i < 5; ++i) {
                        int lw, lh;
                        if (ttf_text_measure(menu_font, labels_tmp[i], &lw, &lh) == 0) {
                            if (lw > max_label_w) max_label_w = lw;
                        }
                    }
                } else {
                    max_label_w = 80;
                }
                int value_area_w = 160;
                int right_w = max_label_w + 16 + value_area_w;
                int gap_left_play = 20;
                int gap_play_right = 40;
                int group_w = left_w + gap_left_play + play_w + gap_play_right + right_w;
                int group_left = (win_w - group_w) / 2;
                int group_top = (win_h - play_h) / 2;
                int left_x = group_left;
                int left_top = group_top + (play_h - left_total_h) / 2;
                int mx = 0, my = 0;
                if (ev.type == SDL_MOUSEMOTION) { mx = ev.motion.x; my = ev.motion.y; }
                else { mx = ev.button.x; my = ev.button.y; }
                int found = -1;
                for (int i = 0; i < left_count; ++i) {
                    int by = left_top + i * (btn_h + btn_gap);
                    int tw = 0, th = 0;
                    int draw_w = btn_w;
                    if (menu_font) {
                        if (ttf_text_measure(menu_font, left_labels_cn[i], &tw, &th) == 0) {
                            int padding = 20;
                            draw_w = (tw + padding > btn_w) ? (tw + padding) : btn_w;
                        }
                    } else {
                        // 在无 TTF（TrueType 字体）时估算像素字体宽度（大约每字符 8 像素）
                        int est = (int)strlen(left_labels_cn[i]) * 8;
                        int padding = 20;
                        draw_w = (est + padding > btn_w) ? (est + padding) : btn_w;
                    }
                    int center_x = left_x + btn_w/2;
                    int bx = center_x - draw_w/2;
                    if (mx >= bx && mx < bx + draw_w && my >= by && my < by + btn_h) { found = i; break; }
                }
                left_hover = found;
            } else if (ev.type == SDL_KEYDOWN) {
                SDL_Keycode sym = ev.key.keysym.sym;

                // 如果存在模态对话框，优先处理对话框的键盘导航：
                // - ↑/↓ 切换选中槽
                // - Enter 确认保存/读取
                // - Delete 删除选中槽（带确认）
                if (show_save_dialog || show_load_dialog) {
                    // 确保至少有一个槽被选中以便使用键盘进行上下导航
                    if (dialog_hover_slot < 0) dialog_hover_slot = 0;
                    if (sym == SDLK_UP) {
                        dialog_hover_slot = (dialog_hover_slot - 1 + SAVE_SLOTS) % SAVE_SLOTS;
                        continue;
                    } else if (sym == SDLK_DOWN) {
                        dialog_hover_slot = (dialog_hover_slot + 1) % SAVE_SLOTS;
                        continue;
                    } else if (sym == SDLK_RETURN || sym == SDLK_KP_ENTER) {
                        int slot = dialog_hover_slot + 1;
                        char err[256] = {0};
                        if (show_save_dialog) {
                            if (tetris_save_slot(&game, slot, err, sizeof(err)) == 0) {
                                tetris_set_hud_message(&game, "存档成功", 1200);
                            } else {
                                SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Save failed", err, window);
                            }
                            show_save_dialog = 0;
                        } else if (show_load_dialog) {
                            if (tetris_load_slot(&game, slot, err, sizeof(err)) == 0) {
                                tetris_set_hud_message(&game, "读档成功", 1200);
                            } else {
                                SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Load failed", err, window);
                            }
                            show_load_dialog = 0;
                        }
                        dialog_hover_slot = -1;
                        dialog_hover_delete = -1;
                        continue;
                    } else if (sym == SDLK_DELETE) {
                        if (dialog_hover_slot >= 0) {
                            int slot = dialog_hover_slot + 1;
                            const SDL_MessageBoxButtonData buttons[] = {
                                { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 0, "是" },
                                { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 1, "否" },
                            };
                            SDL_MessageBoxColorScheme colorScheme;
                            SDL_MessageBoxData msgboxdata = {
                                SDL_MESSAGEBOX_WARNING,
                                window,
                                "删除存档",
                                "确定删除该存档吗？此操作不可撤销。",
                                (int)(sizeof(buttons)/sizeof(buttons[0])),
                                buttons,
                                &colorScheme
                            };
                            int buttonid = -1;
                            if (SDL_ShowMessageBox(&msgboxdata, &buttonid) == 0 && buttonid == 0) {
                                char err[256] = {0};
                                if (tetris_delete_slot(slot, err, sizeof(err)) == 0) {
                                    tetris_set_hud_message(&game, "已删除存档", 1200);
                                } else {
                                    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Delete failed", err, window);
                                }
                            }
                        }
                        continue;
                    } else {
                        continue;
                    }
                }

                // 在游戏进行/暂停时，数字键 1-4 用于选择左侧快捷按钮（供键盘操作，不触发鼠标）
                if ((game_state == GAME_STATE_PLAYING || game_state == GAME_STATE_PAUSED)) {
                    if (sym == SDLK_1 || sym == SDLK_KP_1) { left_selected = 0; tetris_set_hud_message(&game, "已选: 保存存档", 800); continue; }
                    if (sym == SDLK_2 || sym == SDLK_KP_2) { left_selected = 1; tetris_set_hud_message(&game, "已选: 读取存档", 800); continue; }
                    if (sym == SDLK_3 || sym == SDLK_KP_3) { left_selected = 2; tetris_set_hud_message(&game, "已选: 按键设置", 800); continue; }
                    if (sym == SDLK_4 || sym == SDLK_KP_4) { left_selected = 3; tetris_set_hud_message(&game, "已选: 关卡选择", 800); continue; }
                    if ((sym == SDLK_RETURN || sym == SDLK_KP_ENTER) && left_selected >= 0) {
                        switch (left_selected) {
                            case 0: { show_save_dialog = 1; show_load_dialog = 0; dialog_hover_slot = 0; break; }
                            case 1: { show_load_dialog = 1; show_save_dialog = 0; dialog_hover_slot = 0; break; }
                            case 2: { game_state = GAME_STATE_KEY_CONFIG_UI; keycfg_selected = 0; keycfg_editing = 0; tetris_set_hud_message(&game, "按键设置：使用 ↑/↓ 选择，回车修改", 1600); break; }
                            case 3: { show_level_dialog = 1; show_save_dialog = 0; show_load_dialog = 0; dialog_hover_slot = -1; break; }
                        }
                        left_selected = -1;
                        continue;
                    }
                }

                switch (game_state) {
                    case GAME_STATE_MENU:
                        if (sym == SDLK_UP) {
                            selected_menu_item = (selected_menu_item - 1 + MENU_COUNT) % MENU_COUNT;
                        } else if (sym == SDLK_DOWN) {
                            selected_menu_item = (selected_menu_item + 1) % MENU_COUNT;
                        } else if (sym == SDLK_RETURN || sym == SDLK_SPACE) {
                            switch (selected_menu_item) {
                                case MENU_START_GAME:
                                    game_state = GAME_STATE_PLAYING;
                                    tetris_reset(&game);
                                    if (menu_font) game.draw_default_hud = 0;
                                    SDL_SetWindowTitle(window, "Tetris");
                                    break;
                            }
                        } else if (sym == SDLK_ESCAPE) {
                            running = 0;
                        }
                        break;
                    case GAME_STATE_KEY_CONFIG_UI: {
                        // 交互式按键设置界面说明：
                        // - 使用 ↑/↓ 选择要修改的动作
                        // - 回车进入编辑模式，按下新键进行绑定
                        // - 绑定后会自动保存到配置文件
                        if (sym == SDLK_ESCAPE) {
                            game_state = GAME_STATE_PLAYING;
                            tetris_set_hud_message(&game, "按键设置已退出", 800);
                        } else {
                            if (keycfg_editing) {
                                // 接收当前按键作为新的绑定（优先使用 keycode，必要时回退到 scancode 转换）
                                TetrisAction act = keycfg_actions[keycfg_selected];
                                SDL_Keycode bound = sym;
                                if (bound == SDLK_UNKNOWN) {
                                    bound = SDL_GetKeyFromScancode(ev.key.keysym.scancode);
                                }
                                keymap_set_binding(act, bound);
                                if (keymap_save("key_config.txt") == 0) {
                                    char m[128];
                                    const char* kn = SDL_GetKeyName(bound);
                                    snprintf(m, sizeof(m), "绑定成功: %s => %s", keycfg_labels[keycfg_selected], kn);
                                    tetris_set_hud_message(&game, m, 1200);
                                } else {
                                    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Save failed", "Could not save key_config.txt", window);
                                }
                                keycfg_editing = 0;
                            } else {
                                int keycfg_total = KEYCFG_COUNT + 1; // 包含额外的重置为默认行
                                if (sym == SDLK_UP) {
                                    keycfg_selected = (keycfg_selected - 1 + keycfg_total) % keycfg_total;
                                } else if (sym == SDLK_DOWN) {
                                    keycfg_selected = (keycfg_selected + 1) % keycfg_total;
                                } else if (sym == SDLK_RETURN || sym == SDLK_KP_ENTER) {
                                    // 如果选中了“重置为默认”行，则恢复默认并保存
                                    if (keycfg_selected == KEYCFG_COUNT) {
                                        keymap_set_default();
                                        if (keymap_save("key_config.txt") == 0) {
                                            tetris_set_hud_message(&game, "按键已重置为默认", 1400);
                                        } else {
                                            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Save failed", "Could not save key_config.txt", window);
                                        }
                                    } else {
                                        keycfg_editing = 1;
                                        tetris_set_hud_message(&game, "按下要绑定的新键...", 3000);
                                        SDL_StartTextInput();
                                    }
                                }
                            }
                        }
                        break;
                    }
                    case GAME_STATE_PAUSED:
                        if (sym == SDLK_SPACE) {
                            game_state = GAME_STATE_PLAYING;
                            game.last_drop_time = SDL_GetTicks();
                            tetris_set_hud_message(&game, "继续", 800);
                        } else if (sym == SDLK_ESCAPE) {
                            game_state = GAME_STATE_MENU;
                        }
                        break;
                    case GAME_STATE_PLAYING:
                        if (sym == SDLK_ESCAPE) {
                            game_state = GAME_STATE_MENU;
                            SDL_SetWindowTitle(window, "Tetris - Menu");
                        } else if (sym == SDLK_SPACE) {
                            // 切换暂停（由空格触发）
                            game_state = GAME_STATE_PAUSED;
                            tetris_set_hud_message(&game, "已暂停", 1200);
                        } else {
                            // 处理游戏内按键逻辑（保存/读取/速度调整/映射执行等）
                            SDL_Keymod mod = ev.key.keysym.mod;

                            if (sym == SDLK_F5) {
                                char err[256] = {0};
                                if (tetris_save_slot(&game, selected_slot, err, sizeof(err)) == 0) {
                                    char title[128];
                                    snprintf(title, sizeof(title), "Tetris - 已保存 存档 %d", selected_slot);
                                    SDL_SetWindowTitle(window, title);
                                    char msg[128];
                                    snprintf(msg, sizeof(msg), "已保存 存档 %d", selected_slot);
                                    tetris_set_hud_message_typed(&game, msg, 2000, 1);
                                } else {
                                    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Save failed", err, window);
                                }
                            } else if (sym == SDLK_F7) {
                                char err[256] = {0};
                                if (tetris_load_slot(&game, selected_slot, err, sizeof(err)) == 0) {
                                    char title[128];
                                    snprintf(title, sizeof(title), "Tetris - 已读取 存档 %d", selected_slot);
                                    SDL_SetWindowTitle(window, title);
                                    char msg[128];
                                    snprintf(msg, sizeof(msg), "已读取 存档 %d", selected_slot);
                                    tetris_set_hud_message_typed(&game, msg, 2000, 1);
                                } else {
                                    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Load failed", err, window);
                                }
                            } else if (sym == SDLK_1 || sym == SDLK_KP_1) {
                                selected_slot = 1;
                                char msg[64]; snprintf(msg, sizeof(msg), "选中 存档 %d", selected_slot);
                                tetris_set_hud_message(&game, msg, 1200);
                            } else if (sym == SDLK_2 || sym == SDLK_KP_2) {
                                selected_slot = 2;
                                char msg[64]; snprintf(msg, sizeof(msg), "选中 存档 %d", selected_slot);
                                tetris_set_hud_message(&game, msg, 1200);
                            } else if (sym == SDLK_3 || sym == SDLK_KP_3) {
                                selected_slot = 3;
                                char msg[64]; snprintf(msg, sizeof(msg), "选中 存档 %d", selected_slot);
                                tetris_set_hud_message(&game, msg, 1200);
                            } else if (sym == SDLK_4 || sym == SDLK_KP_4) {
                                selected_slot = 4;
                                char msg4[64]; snprintf(msg4, sizeof(msg4), "选中 存档 %d", selected_slot);
                                tetris_set_hud_message(&game, msg4, 1200);
                            } else if (sym == SDLK_5 || sym == SDLK_KP_5) {
                                selected_slot = 5;
                                char msg5[64]; snprintf(msg5, sizeof(msg5), "选中 存档 %d", selected_slot);
                                tetris_set_hud_message(&game, msg5, 1200);
                            } else if (sym == SDLK_KP_PLUS || (sym == SDLK_EQUALS && (mod & KMOD_SHIFT))) {
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
                                // 包括新增的速度增加/减少两个动作（ACTION_SPEED_UP / ACTION_SPEED_DOWN）
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
                                } else {
                                    // 严格映射模式：忽略未映射的游戏按键（避免固化的默认控制干扰）
                                    // 允许在 game over 状态下按 'r' 重启游戏（方便测试）
                                    if (game.game_over && sym == SDLK_r) {
                                        tetris_reset(&game);
                                        tetris_set_hud_message(&game, "已重启", 800);
                                    }
                                }
                            }
                        }
                        break;
                }
            } else if (ev.type == SDL_DROPFILE && game_state == GAME_STATE_PLAYING && !show_level_dialog) {
                char* dropped = ev.drop.file;
                char err[256] = {0};
                if (tetris_load_level_file(dropped, &game, err, sizeof(err)) == 0) {
                    char title[128];
                    snprintf(title, sizeof(title), "Tetris - Loaded: %s", dropped);
                    SDL_SetWindowTitle(window, title);
                    char msg[256];
                    snprintf(msg, sizeof(msg), "Loaded level - Score: %d, Level: %d, Lines: %d",
                            game.score, game.level, game.lines_cleared);
                    tetris_set_hud_message_typed(&game, msg, 4000, 1); // 成功提示
                } else {
                    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Load failed", err, window);
                }
                SDL_free(dropped);
            } else if (ev.type == SDL_DROPFILE && (show_level_dialog)) {
                char* dropped = ev.drop.file;
                char err[256] = {0};
                if (tetris_load_level_file(dropped, &game, err, sizeof(err)) == 0) {
                        char title[128];
                        snprintf(title, sizeof(title), "Tetris - Loaded: %s", dropped);
                        SDL_SetWindowTitle(window, title);
                        char msg[256];
                        snprintf(msg, sizeof(msg), "已加载关卡 - 分数: %d, 等级: %d, 消行: %d",
                                game.score, game.level, game.lines_cleared);
                        tetris_set_hud_message_typed(&game, msg, 4000, 1); // 成功提示
                        show_level_dialog = 0;
                        game_state = GAME_STATE_PLAYING;
                } else {
                        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Load failed", err, window);
                    }
                SDL_free(dropped);
            } else if (ev.type == SDL_KEYDOWN) {
                if (ev.key.keysym.sym == SDLK_ESCAPE) running = 0;
                // 速度控制：小键盘 +/- 或主键盘 - / =（Shift+= 视为 +）
                SDL_Keycode sym = ev.key.keysym.sym;
                SDL_Keymod mod = ev.key.keysym.mod;
                // 存档快捷键：F5 保存，F7 读取；数字键选择槽（1..5）
                if (sym == SDLK_F5) {
                    char err[256] = {0};
                    if (tetris_save_slot(&game, selected_slot, err, sizeof(err)) == 0) {
                        char title[128];
                        snprintf(title, sizeof(title), "Tetris - 已保存 存档 %d", selected_slot);
                        SDL_SetWindowTitle(window, title);
                        char msg[128];
                        snprintf(msg, sizeof(msg), "已保存 存档 %d", selected_slot);
                        tetris_set_hud_message_typed(&game, msg, 2000, 1); // 成功提示
                    } else {
                        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Save failed", err, window);
                    }
                    continue;
                } else if (sym == SDLK_F7) {
                    char err[256] = {0};
                    if (tetris_load_slot(&game, selected_slot, err, sizeof(err)) == 0) {
                        char title[128];
                        snprintf(title, sizeof(title), "Tetris - 已读取 存档 %d", selected_slot);
                        SDL_SetWindowTitle(window, title);
                        char msg[128];
                        snprintf(msg, sizeof(msg), "已读取 存档 %d", selected_slot);
                        tetris_set_hud_message_typed(&game, msg, 2000, 1); // 成功提示
                    } else {
                        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Load failed", err, window);
                    }
                    continue;
                } else if (sym == SDLK_1 || sym == SDLK_KP_1) {
                    selected_slot = 1;
                    char title[64]; snprintf(title, sizeof(title), "Tetris - Slot %d selected", selected_slot);
                    SDL_SetWindowTitle(window, title);
                    { char msg[64]; snprintf(msg, sizeof(msg), "Slot %d selected", selected_slot); tetris_set_hud_message(&game, msg, 1200); }
                    continue;
                } else if (sym == SDLK_2 || sym == SDLK_KP_2) {
                    selected_slot = 2;
                    char title[64]; snprintf(title, sizeof(title), "Tetris - Slot %d selected", selected_slot);
                    SDL_SetWindowTitle(window, title);
                    { char msg[64]; snprintf(msg, sizeof(msg), "Slot %d selected", selected_slot); tetris_set_hud_message(&game, msg, 1200); }
                    continue;
                } else if (sym == SDLK_3 || sym == SDLK_KP_3) {
                    selected_slot = 3;
                    char title[64]; snprintf(title, sizeof(title), "Tetris - Slot %d selected", selected_slot);
                    SDL_SetWindowTitle(window, title);
                    { char msg[64]; snprintf(msg, sizeof(msg), "Slot %d selected", selected_slot); tetris_set_hud_message(&game, msg, 1200); }
                    continue;
                }
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
                } else if (sym == SDLK_F8) {
                    keymap_set_default();
                        if (keymap_save("key_config.txt") == 0) {
                            tetris_set_hud_message_typed(&game, "Keymap reset", 1600, 1); // 成功提示
                    } else {
                        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Save failed", "Could not save key_config.txt", window);
                        tetris_set_hud_message_typed(&game, "Keymap reset (not saved)", 1600, 3); // 错误/警告提示
                    }
                } else if (sym == SDLK_F9) {
                    if (keymap_save("key_config.txt") == 0) {
                        tetris_set_hud_message_typed(&game, "Keymap saved", 1600, 1); // 成功提示
                    } else {
                        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Save failed", "Could not save key_config.txt", window);
                    }
                } else {
                    // 如果处于 keymap 的交互式配置模式，则将按键传递给 keymap 进行处理
                    if (keymap_is_configuring()) {
                        int finished = keymap_handle_config_key(ev.key.keysym.sym);
                        if (finished) {
                            if (keymap_save("key_config.txt") == 0) {
                                tetris_set_hud_message_typed(&game, "Keymap saved", 2000, 1); // 成功提示
                            } else {
                                SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Save failed", "Could not save key_config.txt", window);
                                tetris_set_hud_message_typed(&game, "Keymap configured (not saved)", 2000, 3); // 错误/警告提示
                            }
                        } else {
                            // 提示下一个需要配置的动作（用于向用户展示当前待绑定的动作）
                            const char* action_name = keymap_get_current_config_action_name();
                            char msg[128];
                            snprintf(msg, sizeof(msg), "CONFIG: Press key for %s", action_name);
                            tetris_set_hud_message(&game, msg, 4000);
                        }
                    } else {
                        // 将按键映射为动作（通过 keymap 查找），若存在则执行对应动作
                        TetrisAction act = keymap_get_action(ev.key.keysym.sym);
                        if (act != ACTION_NONE) {
                            tetris_perform_action(&game, (int)act);
                        } else {
                            // 严格映射：不回退到硬编码控制。仅在 game over 状态下允许按 'r' 快速重启以便测试。
                            if (game.game_over && ev.key.keysym.sym == SDLK_r) {
                                tetris_reset(&game);
                                tetris_set_hud_message(&game, "已重启", 800);
                            }
                        }
                    }
                }
            }
        }
                // 根据当前游戏状态更新逻辑并执行绘制
        if (game_state == GAME_STATE_PLAYING || game_state == GAME_STATE_PAUSED) {
            uint32_t now = SDL_GetTicks();
            if (game_state == GAME_STATE_PLAYING) tetris_update(&game, now);

            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            // 计算整体居中布局：左侧按钮列 + 中央游戏区域 + 右侧信息面板（用于居中显示整个 UI 组）
            int btn_w = 120, btn_h = 36, btn_gap = 12;
            int win_w = 800, win_h = 600;
            SDL_GetWindowSize(window, &win_w, &win_h);
            int left_w = btn_w;
            int left_total_h = left_count * btn_h + (left_count - 1) * btn_gap;
            int play_cell = 24;
            int play_w = TETRIS_WIDTH * play_cell;
            int play_h = TETRIS_HEIGHT * play_cell;
            int max_label_w = 0;
            if (menu_font) {
                const char* labels_tmp[] = { "下一个", "分数", "等级", "消行", "速度" };
                for (int i = 0; i < 5; ++i) {
                    int lw, lh;
                    if (ttf_text_measure(menu_font, labels_tmp[i], &lw, &lh) == 0) {
                        if (lw > max_label_w) max_label_w = lw;
                    }
                }
            } else {
                max_label_w = 80;
            }
            int value_area_w = 160;
            int right_w = max_label_w + 16 + value_area_w;
            int gap_left_play = 20;
            int gap_play_right = 40;
            int group_w = left_w + gap_left_play + play_w + gap_play_right + right_w;
            int group_left = (win_w - group_w) / 2;
            int group_top = (win_h - play_h) / 2;
            int left_x = group_left;
            int left_top = group_top + (play_h - left_total_h) / 2;
            int play_px = left_x + left_w + gap_left_play;
            int play_py = group_top;
            int sidebar_x = play_px + play_w + gap_play_right;
            // 绘制左侧按钮列（相对于 playfield 在垂直方向上居中）
            for (int i = 0; i < left_count; ++i) {
                int by = left_top + i * (btn_h + btn_gap);
                int tw = 0, th = 0;
                int draw_w = btn_w;
                if (menu_font) {
                    if (ttf_text_measure(menu_font, left_labels_cn[i], &tw, &th) == 0) {
                        int padding = 20;
                        draw_w = (tw + padding > btn_w) ? (tw + padding) : btn_w;
                    }
                } else {
                    int est = (int)strlen(left_labels_cn[i]) * 8;
                    int padding = 20;
                    draw_w = (est + padding > btn_w) ? (est + padding) : btn_w;
                    th = 12;
                }
                int center_x = left_x + btn_w/2;
                int bx = center_x - draw_w/2;
                if (menu_font) {
                    if (i == left_hover || i == left_selected) SDL_SetRenderDrawColor(renderer, 220, 180, 0, 255);
                    else SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255);
                    SDL_Rect br = { bx, by, draw_w, btn_h };
                    SDL_RenderFillRect(renderer, &br);
                    // 将按钮文本标签居中绘制
                    int tx = center_x - tw/2;
                    int ty = by + (btn_h - th) / 2;
                    SDL_Color col = (i == left_hover) ? (SDL_Color){0,0,0,255} : (SDL_Color){255,255,255,255};
                    ttf_text_draw(renderer, menu_font, tx, ty, left_labels_cn[i], col);
                } else {
                    // 无 TTF（TrueType 字体）时的像素字体回退绘制
                    if (i == left_hover || i == left_selected) SDL_SetRenderDrawColor(renderer, 220, 180, 0, 255);
                    else SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255);
                    SDL_Rect br = { bx, by, draw_w, btn_h };
                    SDL_RenderFillRect(renderer, &br);
                    tetris_draw_text(&game, bx + 8, by + 8, left_labels_cn[i], 12);
                }
            }
            tetris_render(&game, play_px, play_py, play_cell);
            // 如果处于暂停态，绘制覆盖层和暂停文字
            if (game_state == GAME_STATE_PAUSED) {
                // 在 playfield 上绘制半透明遮罩（用于暂停状态的视觉提示）
                int px = play_px, py = play_py, cell = play_cell;
                SDL_Rect area = { px, py, TETRIS_WIDTH * cell, TETRIS_HEIGHT * cell };
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 160);
                SDL_RenderFillRect(renderer, &area);
                // 在遮罩中央绘制“暂停”文字（支持 TTF（TrueType 字体）与像素字体回退）
                const char* ptext = "暂停";
                if (menu_font) {
                    int tw, th;
                    if (ttf_text_measure(menu_font, ptext, &tw, &th) == 0) {
                        int cx = px + (area.w - tw) / 2;
                        int cy = py + (area.h - th) / 2;
                        ttf_text_draw(renderer, menu_font, cx, cy, ptext, (SDL_Color){255,255,255,255});
                    } else {
                        tetris_draw_text(&game, px + area.w/2 - 24, py + area.h/2 - 8, "PAUSED", 12);
                    }
                } else {
                    tetris_draw_text(&game, px + area.w/2 - 24, py + area.h/2 - 8, "PAUSED", 12);
                }
            }
            // 绘制右侧信息面板（下一个预览、分数、等级、消行、速度）
            int y = 40;
            char buf[128];
            if (menu_font) {
            // 右侧面板：左侧为标签，右侧为数值或预览
            // 布局规则：将“下一个”预览顶部与 playfield 顶部对齐，速度项与 playfield 底部对齐
            const int play_cell2 = play_cell;
            const int play_h2 = TETRIS_HEIGHT * play_cell2;
            const char* labels[] = { "下一个", "分数", "等级", "消行", "速度" };
            int label_count = 5;
            int max_label_w2 = max_label_w;
            int label_x = sidebar_x;
            int value_x = sidebar_x + max_label_w2 + 16;
            // 下一个方块预览的顶部与 playfield 顶部对齐
            int preview_cell = 16;
            int preview_x = value_x;
            int preview_y = play_py; // 与左侧 playfield 顶部对齐
            ttf_text_draw(renderer, menu_font, label_x, preview_y, labels[0], (SDL_Color){255,255,255,255});
            TetrominoId next_id = TET_NONE;
            if (game.bag_index < 7) next_id = game.bag[game.bag_index];
            if (next_id != TET_NONE) tetris_render_preview(&game, next_id, preview_x, preview_y + 24, preview_cell);
            // 计算预览底部到 playfield 底部之间的可用区域，用于放置中间的统计项（分数/等级/消行）
            int preview_bottom = preview_y + 24 + 4 * preview_cell; // +24 考虑标签文本高度
            int play_bottom = play_py + play_h2;
            // 在预览和底部之间分配中间项目位置，并将速度项放置在靠近底部的位置
            int middle_count = 3;
            int gap_top = preview_bottom + 8;
            int gap_bottom = play_bottom - 40; 
            if (gap_bottom <= gap_top) gap_bottom = gap_top + middle_count * 40;
            int available = gap_bottom - gap_top;
            int spacing = available / (middle_count + 1);
            int pos_y[4];
            // 中间三项（分数、等级、消行）的纵坐标计算
            for (int i = 0; i < middle_count; ++i) pos_y[i] = gap_top + (i + 1) * spacing;
            // 速度项纵坐标（与 playfield 底部对齐）
            pos_y[3] = play_bottom - 36;
            // 绘制 分数/等级/消行/速度：左侧为标签，右侧为数值
            ttf_text_draw(renderer, menu_font, label_x, pos_y[0], labels[1], (SDL_Color){255,255,255,255});
            snprintf(buf, sizeof(buf), "%d", game.score);
            ttf_text_draw(renderer, menu_font, value_x, pos_y[0], buf, (SDL_Color){255,255,255,255});
            ttf_text_draw(renderer, menu_font, label_x, pos_y[1], labels[2], (SDL_Color){255,255,255,255});
            snprintf(buf, sizeof(buf), "%d", game.level);
            ttf_text_draw(renderer, menu_font, value_x, pos_y[1], buf, (SDL_Color){255,255,255,255});
            ttf_text_draw(renderer, menu_font, label_x, pos_y[2], labels[3], (SDL_Color){255,255,255,255});
            snprintf(buf, sizeof(buf), "%d", game.lines_cleared);
            ttf_text_draw(renderer, menu_font, value_x, pos_y[2], buf, (SDL_Color){255,255,255,255});
            ttf_text_draw(renderer, menu_font, label_x, pos_y[3], labels[4], (SDL_Color){255,255,255,255});
            snprintf(buf, sizeof(buf), "%.0f%%", game.speed_multiplier * 100.0f);
            ttf_text_draw(renderer, menu_font, value_x, pos_y[3], buf, (SDL_Color){255,255,255,255});
            } else {
                // 没有 TTF（TrueType 字体）时回退到像素字体显示的布局
                tetris_draw_text(&game, sidebar_x, y, "SCORE", 12);
                snprintf(buf, sizeof(buf), "%d", game.score);
                tetris_draw_text(&game, sidebar_x, y + 36, buf, 10);
                tetris_draw_text(&game, sidebar_x, y + 80, "LEVEL", 12);
                snprintf(buf, sizeof(buf), "%d", game.level);
                tetris_draw_text(&game, sidebar_x, y + 116, buf, 10);
                tetris_draw_text(&game, sidebar_x, y + 160, "LINES", 12);
                snprintf(buf, sizeof(buf), "%d", game.lines_cleared);
                tetris_draw_text(&game, sidebar_x, y + 196, buf, 10);
                tetris_draw_text(&game, sidebar_x, y + 240, "SPEED", 12);
                snprintf(buf, sizeof(buf), "%.0f%%", game.speed_multiplier * 100.0f);
                tetris_draw_text(&game, sidebar_x, y + 276, buf, 10);
            }
            // 当存在模态对话框（保存/读取/关卡）时绘制弹窗 UI
            if (show_save_dialog || show_load_dialog || show_level_dialog) {
                int win_w = 800, win_h = 600;
                SDL_GetWindowSize(window, &win_w, &win_h);
                // 响应式模态框宽度：保证长文本可见，同时离窗口边缘保留一定内边距
                int modal_w = win_w - 100;
                if (modal_w < 420) modal_w = 420;
                if (modal_w > win_w - 40) modal_w = win_w - 40;
                // 测量标题/提示文本以计算模态框所需高度
                const char* raw_title_for_size = show_save_dialog ? "选择存档位置" : "选择读取存档";
                const char* top_hint_for_size = "按 ESC 返回";
                const char* modal_hint_for_size = "上/下 选择, Enter 确认, Delete 删除, Esc 取消";
                int title_h_sz = 20, hint_h_sz = 18, bottom_hint_h_sz = 18;
                if (menu_font) {
                    int tw, th;
                    if (ttf_text_measure(menu_font, raw_title_for_size, &tw, &th) == 0) title_h_sz = th;
                    if (ttf_text_measure(menu_font, top_hint_for_size, &tw, &th) == 0) hint_h_sz = th;
                    if (ttf_text_measure(menu_font, modal_hint_for_size, &tw, &th) == 0) bottom_hint_h_sz = th;
                } else if (modal_font) {
                    int tw, th;
                    if (ttf_text_measure(modal_font, raw_title_for_size, &tw, &th) == 0) title_h_sz = th;
                    if (ttf_text_measure(modal_font, top_hint_for_size, &tw, &th) == 0) hint_h_sz = th;
                    if (ttf_text_measure(modal_font, modal_hint_for_size, &tw, &th) == 0) bottom_hint_h_sz = th;
                }
                int header_h = 8 + title_h_sz + 6 + hint_h_sz + 8;
                const int slot_step = 64;
                const int slot_inner_h = 56;
                int modal_h = header_h + SAVE_SLOTS * slot_step + bottom_hint_h_sz + 20;
                int modal_x = (win_w - modal_w) / 2;
                int modal_y = (win_h - modal_h) / 2;
                // 背景置暗以突出模态对话框
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 180);
                SDL_Rect full = {0,0,win_w,win_h};
                SDL_RenderFillRect(renderer, &full);
                // 对话框容器背景
                SDL_SetRenderDrawColor(renderer, 40, 40, 40, 255);
                SDL_Rect db = { modal_x, modal_y, modal_w, modal_h };
                SDL_RenderFillRect(renderer, &db);
                // 标题单独一行绘制；将右上角的提示绘制在下一行以避免与标题重叠/截断
                const char* raw_title = show_save_dialog ? "选择存档位置" : (show_load_dialog ? "选择读取存档" : "加载关卡 - 拖动文件到此处");
                int title_w = 0, title_h = 0;
                if (menu_font && ttf_text_measure(menu_font, raw_title, &title_w, &title_h) == 0) {
                    // 如果标题宽度超过模态框可用宽度，则进行截断并添加省略号
                    int max_title_w = modal_w - 32;
                    if (title_w <= max_title_w) {
                        ttf_text_draw(renderer, menu_font, modal_x + 16, modal_y + 8, raw_title, (SDL_Color){255,255,255,255});
                    } else {
                        char buf[128];
#ifdef _MSC_VER
                        strncpy_s(buf, sizeof(buf), raw_title, _TRUNCATE);
#else
                        strncpy(buf, raw_title, sizeof(buf)-1);
                        buf[sizeof(buf)-1] = '\0';
#endif
                        int len = (int)strlen(buf);
                        int tw = title_w, th = title_h;
                        while (len > 0) {
                            buf[--len] = '\0';
                            if (ttf_text_measure(menu_font, buf, &tw, &th) != 0) break;
                            if (tw <= max_title_w - 24) break;
                        }
#ifdef _MSC_VER
                        if (len > 3) {
                            buf[len-3] = '\0';
                            strcat_s(buf, sizeof(buf), "...");
                        }
#else
                        if (len > 3) {
                            buf[len-3] = '\0';
                            strncat(buf, "...", sizeof(buf)-strlen(buf)-1);
                        }
#endif
                        ttf_text_draw(renderer, menu_font, modal_x + 16, modal_y + 8, buf, (SDL_Color){255,255,255,255});
                        title_h = th;
                    }
                } else {
                    // 无 TTF（TrueType 字体）时回退至像素字体绘制标题
                    tetris_draw_text(&game, modal_x + 16, modal_y + 8, raw_title, 14);
                    title_h = 18;
                }
                // 标题下方的右上角提示（如 "按 ESC 返回"）
                const char* top_hint = "按 ESC 返回";
                int hint_w = 0, hint_h = 0;
                int hint_y = modal_y + 8 + (title_h > 0 ? title_h : 20) + 6;
                if (modal_font && ttf_text_measure(modal_font, top_hint, &hint_w, &hint_h) == 0) {
                    int hint_x = modal_x + modal_w - 16 - hint_w;
                    ttf_text_draw(renderer, modal_font, hint_x, hint_y, top_hint, (SDL_Color){200,200,200,255});
                } else if (menu_font && ttf_text_measure(menu_font, top_hint, &hint_w, &hint_h) == 0) {
                    int hint_x = modal_x + modal_w - 16 - hint_w;
                    ttf_text_draw(renderer, menu_font, hint_x, hint_y, top_hint, (SDL_Color){200,200,200,255});
                } else {
                    tetris_draw_text(&game, modal_x + modal_w - 120, hint_y, top_hint, 12);
                }
                // 根据渲染出的头部（标题 + 提示）计算槽列表起始 Y，确保头部与列表间有间距
                int slot_y0 = hint_y + hint_h + 8;
                if (show_level_dialog) {
                    // 渲染关卡拖放加载区域（提示用户将 .tetrislvl 文件拖入）
                    int cx = modal_x + 16;
                    int cy = modal_y + 48;
                    int area_w = modal_w - 32;
                    int area_h = modal_h - 120;
                    SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
                    SDL_Rect ar = { cx, cy, area_w, area_h };
                    SDL_RenderFillRect(renderer, &ar);
                    const char* msg = "将关卡文件 (.tetrislvl / .tetrislvl) 拖放到此处以加载";
                    if (modal_font) {
                        int tw, th;
                        if (ttf_text_measure(modal_font, msg, &tw, &th) == 0) {
                            int tx = modal_x + (modal_w - tw) / 2;
                            int ty = cy + (area_h - th) / 2;
                            ttf_text_draw(renderer, modal_font, tx, ty, msg, (SDL_Color){220,220,220,255});
                        } else {
                            ttf_text_draw(renderer, modal_font, cx + 16, cy + area_h/2 - 8, msg, (SDL_Color){220,220,220,255});
                        }
                    } else if (menu_font) {
                        int tw, th;
                        if (ttf_text_measure(menu_font, msg, &tw, &th) == 0) {
                            int tx = modal_x + (modal_w - tw) / 2;
                            int ty = cy + (area_h - th) / 2;
                            ttf_text_draw(renderer, menu_font, tx, ty, msg, (SDL_Color){220,220,220,255});
                        } else {
                            ttf_text_draw(renderer, menu_font, cx + 16, cy + area_h/2 - 8, msg, (SDL_Color){220,220,220,255});
                        }
                    } else {
                        tetris_draw_text(&game, cx + 16, cy + area_h/2 - 8, msg, 12);
                    }
                } else {
                    // 存档槽列表：渲染每个槽的一行摘要，右侧显示时间戳并在最右侧显示“删除”按钮
                    for (int i = 0; i < SAVE_SLOTS; ++i) {
                        int sx = modal_x + 12;
                        int sy = slot_y0 + i * 64;
                        // 槽背景：当鼠标/键盘选中槽体时高亮整行
                        if (i == dialog_hover_slot) SDL_SetRenderDrawColor(renderer, 220, 180, 0, 255);
                        else SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
                        SDL_Rect sr = { sx, sy, modal_w - 24, 56 };
                        SDL_RenderFillRect(renderer, &sr);

                        const int del_w = 72, del_h = 28;
                        int del_x = modal_x + modal_w - 12 - del_w;
                        int del_y = sy + (56 - del_h) / 2;
                        // 绘制删除按钮（当鼠标悬停在删除按钮上时改变高亮颜色）
                        if (i == dialog_hover_delete) SDL_SetRenderDrawColor(renderer, 180, 50, 50, 255);
                        else SDL_SetRenderDrawColor(renderer, 120, 30, 30, 255);
                        SDL_Rect dr = { del_x, del_y, del_w, del_h };
                        SDL_RenderFillRect(renderer, &dr);
                        // 绘制删除按钮的文字
                        const char* del_label = "删除";
                        if (modal_font) {
                            int tw, th;
                            if (ttf_text_measure(modal_font, del_label, &tw, &th) == 0) {
                                int tx = del_x + (del_w - tw) / 2;
                                int ty = del_y + (del_h - th) / 2;
                                ttf_text_draw(renderer, modal_font, tx, ty, del_label, (SDL_Color){255,255,255,255});
                            } else {
                                ttf_text_draw(renderer, modal_font, del_x + 8, del_y + 6, del_label, (SDL_Color){255,255,255,255});
                            }
                        } else if (menu_font) {
                            int tw, th;
                            if (ttf_text_measure(menu_font, del_label, &tw, &th) == 0) {
                                int tx = del_x + (del_w - tw) / 2;
                                int ty = del_y + (del_h - th) / 2;
                                ttf_text_draw(renderer, menu_font, tx, ty, del_label, (SDL_Color){255,255,255,255});
                            } else {
                                ttf_text_draw(renderer, menu_font, del_x + 8, del_y + 6, del_label, (SDL_Color){255,255,255,255});
                            }
                        } else {
                            tetris_draw_text(&game, del_x + 6, del_y + 6, del_label, 10);
                        }

                        // 读取存档信息并绘制摘要文本与右对齐的时间戳（在删除按钮左侧）
                        int score = 0, level = 0;
                        char timestr[64] = "";
                        if (tetris_read_slot_info(i+1, &score, &level, timestr, sizeof(timestr)) == 0) {
                            char line1[160];
                            snprintf(line1, sizeof(line1), "存档 %d — 分数: %d  等级: %d", i+1, score, level);
                            if (modal_font) {
                                int lw, lh;
                                ttf_text_measure(modal_font, line1, &lw, &lh);
                                // 在左侧绘制主摘要（与时间戳保持相同的垂直偏移）
                                ttf_text_draw(renderer, modal_font, sx + 8, sy + 10, line1, (SDL_Color){255,255,255,255});
                                // 将时间戳右对齐绘制（在删除按钮左侧留出足够空间）
                                int tw = 0, th = 0;
                                if (ttf_text_measure(modal_font, timestr, &tw, &th) == 0) {
                                    int time_x = modal_x + modal_w - 12 - del_w - 8 - tw;
                                    int time_y = sy + 10;
                                    ttf_text_draw(renderer, modal_font, time_x, time_y, timestr, (SDL_Color){200,200,200,255});
                                } else {
                                    ttf_text_draw(renderer, modal_font, del_x - 100, sy + 10, timestr, (SDL_Color){200,200,200,255});
                                }
                            } else if (menu_font) {
                                int lw, lh, tw, th;
                                ttf_text_measure(menu_font, line1, &lw, &lh);
                                ttf_text_measure(menu_font, timestr, &tw, &th);
                                ttf_text_draw(renderer, menu_font, sx + 8, sy + 10, line1, (SDL_Color){255,255,255,255});
                                int time_x = modal_x + modal_w - 12 - del_w - 8 - tw;
                                ttf_text_draw(renderer, menu_font, time_x, sy + 10, timestr, (SDL_Color){200,200,200,255});
                            } else {
                                tetris_draw_text(&game, sx + 8, sy + 10, line1, 12);
                                // 在无 TTF（TrueType 字体）时对时间戳进行粗略定位以避免遮挡删除按钮
                                tetris_draw_text(&game, modal_x + modal_w - 24 - del_w - 40, sy + 10, timestr, 10);
                            }
                        } else {
                            // 空存档槽的占位显示（与已填充槽保持文字大小一致以便视觉统一）
                            char info[160];
                            snprintf(info, sizeof(info), "存档 %d — 空", i+1);
                            if (modal_font) {
                                ttf_text_draw(renderer, modal_font, sx + 8, sy + 10, info, (SDL_Color){200,200,200,255});
                            } else if (menu_font) {
                                ttf_text_draw(renderer, menu_font, sx + 8, sy + 10, info, (SDL_Color){200,200,200,255});
                            } else {
                                tetris_draw_text(&game, sx + 8, sy + 10, info, 12);
                            }
                        }
                    }
                }
                // 对话框底部的键盘提示（根据是否为关卡加载弹窗，提示内容会不同）
                {
                    // 如果是关卡加载弹窗，只显示按 ESC 键 返回 的提示；其他 modal 显示完整快捷键提示
                    const char* modal_hint = show_level_dialog ? "按 ESC 返回" : "上/下 选择, Enter 确认, Delete 删除, Esc 取消";
                    if (modal_font) {
                        int tw = 0, th = 0;
                        if (ttf_text_measure(modal_font, modal_hint, &tw, &th) == 0) {
                            int hx = modal_x + (modal_w - tw) / 2;
                            int hy = modal_y + modal_h - th - 10;
                            ttf_text_draw(renderer, modal_font, hx, hy, modal_hint, (SDL_Color){200,200,200,255});
                        } else {
                            ttf_text_draw(renderer, modal_font, modal_x + 16, modal_y + modal_h - 26, modal_hint, (SDL_Color){200,200,200,255});
                        }
                    } else if (menu_font) {
                        int tw = 0, th = 0;
                        if (ttf_text_measure(menu_font, modal_hint, &tw, &th) == 0) {
                            int hx = modal_x + (modal_w - tw) / 2;
                            int hy = modal_y + modal_h - th - 10;
                            ttf_text_draw(renderer, menu_font, hx, hy, modal_hint, (SDL_Color){200,200,200,255});
                        } else {
                            tetris_draw_text(&game, modal_x + 16, modal_y + modal_h - 26, modal_hint, 10);
                        }
                    } else {
                        tetris_draw_text(&game, modal_x + 16, modal_y + modal_h - 26, modal_hint, 10);
                    }
                }
            }
        } else if (game_state == GAME_STATE_MENU) {
            render_menu(renderer, &game, selected_menu_item, menu_font);
        } else if (game_state == GAME_STATE_KEY_CONFIG_UI) {
            // 新版按键设置 UI（图形界面）：
            // - 列表显示可配置的动作及其当前绑定
            // - 支持上下选择、回车进入编辑、按 ESC 键 退出
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
            const char* header = "按键设置";
            // 将标题在窗口水平居中显示
            {
                int win_w2 = 800, win_h2 = 600;
                SDL_GetWindowSize(window, &win_w2, &win_h2);
                if (menu_font) {
                    int thw = 0, thh = 0;
                    if (ttf_text_measure(menu_font, header, &thw, &thh) == 0) {
                        int header_x = (win_w2 - thw) / 2;
                        ttf_text_draw(renderer, menu_font, header_x, 48, header, (SDL_Color){255,255,255,255});
                    } else {
                        ttf_text_draw(renderer, menu_font, 120, 48, header, (SDL_Color){255,255,255,255});
                    }
                } else {
                    tetris_draw_text(&game, 120, 48, header, 18);
                }
            }
            // 绘制按键映射列表（每行显示动作名称与绑定键）
            int list_x = 120;
            int list_y = 120;
            int row_h = 46;
            int keycfg_total = KEYCFG_COUNT + 1; 
            for (int i = 0; i < keycfg_total; ++i) {
                int ry = list_y + i * row_h;
                SDL_Rect r = { list_x - 8, ry - 6, 560, 40 };
                if (i == keycfg_selected) {
                    SDL_SetRenderDrawColor(renderer, 220, 180, 0, 255);
                    SDL_RenderFillRect(renderer, &r);
                }
                // 动作标签列
                if (i < KEYCFG_COUNT) {
                    if (menu_font) ttf_text_draw(renderer, menu_font, list_x, ry, keycfg_labels[i], (SDL_Color){255,255,255,255});
                    else tetris_draw_text(&game, list_x, ry, keycfg_labels[i], 12);
                    // 当前绑定键显示
                    SDL_Keycode kc = keymap_get_binding(keycfg_actions[i]);
                    const char* kn = kc ? SDL_GetKeyName(kc) : "未绑定";
                    if (i == keycfg_selected && keycfg_editing) {
                        if (menu_font) ttf_text_draw(renderer, menu_font, list_x + 300, ry, "按下新键...", (SDL_Color){255,255,255,255});
                        else tetris_draw_text(&game, list_x + 300, ry, "按下新键...", 12);
                    } else {
                        if (menu_font) ttf_text_draw(renderer, menu_font, list_x + 300, ry, kn, (SDL_Color){200,200,200,255});
                        else tetris_draw_text(&game, list_x + 300, ry, kn, 12);
                    }
                } else {
                    // 重置行为行（用于将按键恢复为默认）
                    const char* reset_label = "重置为默认";
                    if (menu_font) ttf_text_draw(renderer, menu_font, list_x, ry, reset_label, (SDL_Color){255,255,255,255});
                    else tetris_draw_text(&game, list_x, ry, reset_label, 12);
                }
            }
            // 操作提示（避免使用箭头字符，改为中文描述以防字体缺失导致方块字符）
            const char* instr = "上/下 选择, Enter 修改, Esc 返回";
            if (menu_font) {
                int iw, ih;
                if (ttf_text_measure(menu_font, instr, &iw, &ih) == 0) {
                    int ix = 400 - iw/2;
                    ttf_text_draw(renderer, menu_font, ix, 520, instr, (SDL_Color){200,200,200,255});
                } else {
                    ttf_text_draw(renderer, menu_font, 200, 520, instr, (SDL_Color){200,200,200,255});
                }
            } else {
                tetris_draw_text(&game, 200, 520, instr, 12);
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

