#ifndef TETRIS_H
#define TETRIS_H

#include <stdint.h>
#include <SDL.h>

// 俄罗斯方块游戏网格尺寸定义
#define TETRIS_WIDTH 10   // 游戏区域宽度（列数）
#define TETRIS_HEIGHT 20  // 游戏区域高度（行数）

// 俄罗斯方块类型枚举（0-6）
// TET_NONE用于表示没有方块
typedef enum {
    TET_I,      // I形方块（条形）
    TET_O,      // O形方块（正方形）
    TET_T,      // T形方块
    TET_S,      // S形方块
    TET_Z,      // Z形方块
    TET_J,      // J形方块
    TET_L,      // L形方块
    TET_NONE = 255  // 无方块标识
} TetrominoId;

// 俄罗斯方块游戏状态结构体
typedef struct {
    // 游戏网格：0表示空，>0表示已填充（颜色ID）
    uint8_t grid[TETRIS_HEIGHT][TETRIS_WIDTH];
    // 方块袋系统（7种方块的随机序列）
    TetrominoId bag[7];
    int bag_index;

    // 当前下落方块状态
    TetrominoId current_id;  // 当前方块类型
    int current_rot;         // 当前旋转状态（0-3）
    int current_x, current_y; // 当前方块原点位置

    // 时间控制系统
    uint32_t drop_interval_ms;      // 当前下落间隔（毫秒）
    uint32_t last_drop_time;        // 上次下落时间
    uint32_t base_drop_interval_ms; // 基础下落间隔（应用速度倍率前）
    float speed_multiplier;         // 速度倍率（0.5-2.0）

    // 游戏统计数据
    int score;         // 当前分数
    int level;         // 当前等级
    int lines_cleared; // 已消行数

    // 游戏状态标志
    int game_over;     // 游戏结束标志（1=已结束）

    // 渲染相关
    SDL_Renderer* renderer;  // SDL渲染器（用于渲染辅助函数）
    // HUD消息系统
    char hud_msg[128];         // HUD消息文本
    uint32_t hud_msg_until_ms; // 消息过期时间
    int hud_msg_type;          // 消息类型（0=信息，1=成功，2=警告，3=错误）
    int draw_default_hud;      // 是否绘制内置数值HUD（右侧）
} Tetris;

// 初始化俄罗斯方块游戏状态，传入SDL渲染器用于渲染
void tetris_init(Tetris* t, SDL_Renderer* renderer);

// 已移除：tetris_handle_key函数声明（已由keymap系统替代）

// 推进游戏状态（检查定时器），定期调用此函数
void tetris_update(Tetris* t, uint32_t now_ms);

// 渲染当前游戏状态（使用SDL渲染器）
void tetris_render(Tetris* t, int px, int py, int cell_size);

// 重置游戏状态
void tetris_reset(Tetris* t);

// 速度控制接口
void tetris_set_speed_multiplier(Tetris* t, float mul);
float tetris_get_speed_multiplier(Tetris* t);

// HUD消息系统
void tetris_set_hud_message(Tetris* t, const char* msg, uint32_t duration_ms);
void tetris_set_hud_message_typed(Tetris* t, const char* msg, uint32_t duration_ms, int type);

// 文本绘制辅助函数（供其他模块使用，封装内部像素字体）
void tetris_draw_text(Tetris* t, int x, int y, const char* text, int cell_size);
// 渲染方块预览（用于UI侧边栏）
void tetris_render_preview(Tetris* t, TetrominoId id, int px, int py, int cell_size);
// 执行高级动作（来自按键映射系统）
void tetris_perform_action(Tetris* t, int action);

#endif // TETRIS_H
