#include "keymap.h"
#include <string.h>
#include <stdio.h>

static SDL_Keycode mapping[ACTION_COUNT];
static int configuring_index = -1;

// 初始化按键映射系统
void keymap_init(void) {
    keymap_set_default();  // 设置默认映射
    configuring_index = -1;  // 重置配置状态
}
// 设置默认按键映射
// 功能点：
// - 使用方向键控制基本移动和旋转
// - 空格键用于硬降
// - +/-键用于速度调节
void keymap_set_default(void) {
    // 默认映射：方向键 + 空格/上键
    mapping[ACTION_MOVE_LEFT] = SDLK_LEFT;
    mapping[ACTION_MOVE_RIGHT] = SDLK_RIGHT;
    mapping[ACTION_ROTATE] = SDLK_UP;
    mapping[ACTION_SOFT_DROP] = SDLK_DOWN;
    mapping[ACTION_HARD_DROP] = SDLK_SPACE;
    mapping[ACTION_SPEED_UP] = SDLK_EQUALS; 
    mapping[ACTION_SPEED_DOWN] = SDLK_MINUS; 
}

// 从文件加载按键映射配置

int keymap_load(const char* path) {
    if (!path) path = "key_config.txt";
    FILE* f = fopen(path, "r");
    if (!f) return -1;  

    char line[128];
    // 逐行读取并解析配置
    while (fgets(line, sizeof(line), f)) {
        char keyname[64];
        int val = 0;
       
#ifdef _MSC_VER
        if (sscanf_s(line, "%63[^=]=%d", keyname, (unsigned)sizeof(keyname), &val) == 2) {
#else
        if (sscanf(line, "%63[^=]=%d", keyname, &val) == 2) {
#endif
            // 根据动作名称设置映射
            if (strcmp(keyname, "MOVE_LEFT") == 0) mapping[ACTION_MOVE_LEFT] = (SDL_Keycode)val;
            else if (strcmp(keyname, "MOVE_RIGHT") == 0) mapping[ACTION_MOVE_RIGHT] = (SDL_Keycode)val;
            else if (strcmp(keyname, "ROTATE") == 0) mapping[ACTION_ROTATE] = (SDL_Keycode)val;
            else if (strcmp(keyname, "SOFT_DROP") == 0) mapping[ACTION_SOFT_DROP] = (SDL_Keycode)val;
            else if (strcmp(keyname, "HARD_DROP") == 0) mapping[ACTION_HARD_DROP] = (SDL_Keycode)val;
            else if (strcmp(keyname, "SPEED_UP") == 0) mapping[ACTION_SPEED_UP] = (SDL_Keycode)val;
            else if (strcmp(keyname, "SPEED_DOWN") == 0) mapping[ACTION_SPEED_DOWN] = (SDL_Keycode)val;
        }
    }
    fclose(f);
    return 0;
}

// 保存按键映射配置到文件
int keymap_save(const char* path) {
    if (!path) path = "key_config.txt";
    FILE* f = fopen(path, "w");
    if (!f) return -1;  

    // 写入所有动作的按键映射
    fprintf(f, "MOVE_LEFT=%d\n", (int)mapping[ACTION_MOVE_LEFT]);
    fprintf(f, "MOVE_RIGHT=%d\n", (int)mapping[ACTION_MOVE_RIGHT]);
    fprintf(f, "ROTATE=%d\n", (int)mapping[ACTION_ROTATE]);
    fprintf(f, "SOFT_DROP=%d\n", (int)mapping[ACTION_SOFT_DROP]);
    fprintf(f, "HARD_DROP=%d\n", (int)mapping[ACTION_HARD_DROP]);
    fprintf(f, "SPEED_UP=%d\n", (int)mapping[ACTION_SPEED_UP]);
    fprintf(f, "SPEED_DOWN=%d\n", (int)mapping[ACTION_SPEED_DOWN]);

    fclose(f);
    return 0;
}

// 根据SDL按键码获取对应的游戏动作
TetrisAction keymap_get_action(SDL_Keycode key) {
    
    for (int a = 1; a < ACTION_COUNT; ++a) {
        if (mapping[a] == key) return (TetrisAction)a;
    }
    return ACTION_NONE;  
}

// 获取指定动作当前绑定的SDL按键码
SDL_Keycode keymap_get_binding(TetrisAction action) {
    if (action <= ACTION_NONE || action >= ACTION_COUNT) return (SDL_Keycode)0;
    return mapping[action];
}

// 为指定动作设置新的按键绑定
int keymap_set_binding(TetrisAction action, SDL_Keycode key) {
    if (action <= ACTION_NONE || action >= ACTION_COUNT) return -1;  // 无效动作
    mapping[action] = key;
    return 0;
}

// 开始交互式按键配置流程

void keymap_start_config(void) {
    configuring_index = 1; 
}

// 检查是否正在进行按键配置
int keymap_is_configuring(void) {
    return configuring_index >= 1 && configuring_index < ACTION_COUNT;
}

// 在配置过程中处理按键输入
// 返回值：1表示配置完成，0表示继续配置
int keymap_handle_config_key(SDL_Keycode key) {
    if (!keymap_is_configuring()) return 1;  
    // 为当前动作设置按键绑定
    mapping[configuring_index] = key;
    configuring_index++;
    // 检查是否配置完成
    if (!keymap_is_configuring()) {
        configuring_index = -1;  
        return 1;  // 配置完成
    }
    return 0;  
}

// 获取当前正在配置的动作名称（用于界面提示）
const char* keymap_get_current_config_action_name(void) {
    if (!keymap_is_configuring()) return "";  // 不在配置状态
    switch (configuring_index) {
        case ACTION_MOVE_LEFT: return "左移";
        case ACTION_MOVE_RIGHT: return "右移";
        case ACTION_ROTATE: return "旋转";
        case ACTION_SOFT_DROP: return "软降";
        case ACTION_HARD_DROP: return "硬降";
        case ACTION_SPEED_UP: return "速度增加";
        case ACTION_SPEED_DOWN: return "速度减少";
        default: return "未知动作";
    }
}


