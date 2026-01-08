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
