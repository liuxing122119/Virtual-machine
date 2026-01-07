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

