#ifndef KEYMAP_H
#define KEYMAP_H

#include <SDL.h>
#include <stdint.h>

// 俄罗斯方块游戏动作枚举

typedef enum {
    ACTION_NONE = 0,
    ACTION_MOVE_LEFT,      // 左移
    ACTION_MOVE_RIGHT,     // 右移
    ACTION_ROTATE,         // 旋转
    ACTION_SOFT_DROP,      // 软降
    ACTION_HARD_DROP,      // 硬降
    ACTION_SPEED_UP,       // 速度增加
    ACTION_SPEED_DOWN,     // 速度减少
    ACTION_COUNT           // 动作总数
} TetrisAction;


void keymap_init(void);
int keymap_load(const char* path);
int keymap_save(const char* path);
void keymap_set_default(void);

// 根据SDL按键码获取对应的游戏动作
TetrisAction keymap_get_action(SDL_Keycode key);

SDL_Keycode keymap_get_binding(TetrisAction action);
// 为指定动作设置新的按键绑定
int keymap_set_binding(TetrisAction action, SDL_Keycode key);

void keymap_start_config(void);

int keymap_is_configuring(void);

int keymap_handle_config_key(SDL_Keycode key);
// 获取当前正在配置的动作名称
const char* keymap_get_current_config_action_name(void);

#endif 


