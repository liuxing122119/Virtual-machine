#include "tetris.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <math.h>
#include <SDL.h>
#include "keymap.h"

// 俄罗斯方块形状定义：7种方块×4种旋转状态
// 每个旋转状态用16位二进制表示（按行主序排列的4x4网格）
static const uint16_t tetromino_shapes[7][4] = {
    // I形方块（条形）
    { 0x0F00, 0x2222, 0x00F0, 0x4444 },
    // O形方块（正方形）
    { 0x6600, 0x6600, 0x6600, 0x6600 },
    // T形方块
    { 0x0E40, 0x4C40, 0x04E0, 0x4640 },
    // S形方块
    { 0x06C0, 0x8C40, 0x06C0, 0x8C40 },
    // Z形方块
    { 0x0C60, 0x4C80, 0x0C60, 0x4C80 },
    // J形方块
    { 0x08E0, 0x44C0, 0x0E20, 0xC880 },
    // L形方块
    { 0x02E0, 0x88C0, 0x0E80, 0xC440 }
};

// 俄罗斯方块颜色定义（I、O、T、S、Z、J、L）
static const SDL_Color tetromino_colors[7] = {
    {0, 240, 240, 255},   // I - 青色
    {240, 240, 0, 255},   // O - 黄色
    {160, 0, 240, 255},   // T - 紫色
    {0, 240, 0, 255},     // S - 绿色
    {240, 0, 0, 255},     // Z - 红色
    {0, 0, 240, 255},     // J - 蓝色
    {240, 160, 0, 255}    // L - 橙色
};

// 洗牌袋算法：生成随机方块序列
// 功能点：
// - 填充袋子数组（0-6对应7种方块）
// - 使用Fisher-Yates洗牌算法确保随机性
// - 重置袋子索引为0
static void shuffle_bag(Tetris* t) {
    // 填充袋子数组（7种方块ID：0-6）
    for (int i = 0; i < 7; i++) t->bag[i] = (TetrominoId)i;
    // Fisher-Yates洗牌算法：从后往前随机交换位置
    for (int i = 6; i > 0; i--) {
        int j = rand() % (i + 1);
        TetrominoId tmp = t->bag[i];
        t->bag[i] = t->bag[j];
        t->bag[j] = tmp;
    }
    t->bag_index = 0;
}

// 检查方块形状中的特定单元格是否被占用
// 参数：
// - shape: 16位方块形状数据
// - rx, ry: 相对坐标（0-3范围）
// 返回：1表示该位置有方块，0表示空位
static int shape_cell(uint16_t shape, int rx, int ry) {
    // rx,ry范围：0..3
    // 计算位移：从高位到低位，从右到左
    int bit = (shape >> ((3 - ry) * 4 + (3 - rx))) & 1;
    return bit;
}

// 碰撞检测函数
// 功能点：
// - 检查方块在指定位置和旋转状态下是否会发生碰撞
// - 检测边界碰撞和与其他已放置方块的碰撞
// 参数：
// - id: 方块类型
// - rot: 旋转状态
// - x, y: 方块左上角在游戏网格中的位置
// 返回：1表示碰撞，0表示无碰撞
static int check_collision(Tetris* t, TetrominoId id, int rot, int x, int y) {
    uint16_t shape = tetromino_shapes[id][rot & 3];
    // 遍历4x4方块形状中的每个单元格
    for (int ry = 0; ry < 4; ry++) {
        for (int rx = 0; rx < 4; rx++) {
            if (shape_cell(shape, rx, ry)) {
                // 计算在游戏网格中的绝对坐标
                int gx = x + rx;
                int gy = y + ry;
                // 边界检测：超出左右边界或上下边界
                if (gx < 0 || gx >= TETRIS_WIDTH || gy < 0 || gy >= TETRIS_HEIGHT) return 1;
                // 方块重叠检测：该位置已有其他方块
                if (t->grid[gy][gx]) return 1;
            }
        }
    }
    return 0;
}

// 锁定当前方块到游戏网格中
// 功能点：
// - 将当前下落方块固定到游戏网格上
// - 每个方块ID存储为(id + 1)，避免0值冲突
// - 只锁定在有效边界内的方块单元
static void lock_piece(Tetris* t) {
    TetrominoId id = t->current_id;
    uint16_t shape = tetromino_shapes[id][t->current_rot & 3];
    // 遍历方块形状，将占用的单元格写入网格
    for (int ry = 0; ry < 4; ry++) {
        for (int rx = 0; rx < 4; rx++) {
            if (shape_cell(shape, rx, ry)) {
                // 计算网格坐标
                int gx = t->current_x + rx;
                int gy = t->current_y + ry;
                // 确保在有效边界内才写入
                if (gx >= 0 && gx < TETRIS_WIDTH && gy >= 0 && gy < TETRIS_HEIGHT) {
                    // 存储为(id + 1)，避免与空单元格(0)冲突
                    t->grid[gy][gx] = (uint8_t)(id + 1);
                }
            }
        }
    }
}

// 消行检测和处理函数
// 功能点：
// - 从底部开始逐行检查是否满行
// - 发现满行时，将上方所有行向下移动
// - 清空顶部行，准备新行
// - 重新检查同一行（因为上方行下移后可能又变满）
// 返回：清除的行数
static int clear_lines(Tetris* t) {
    int cleared = 0;
    // 从底部行开始向上检查（便于行下移操作）
    for (int y = TETRIS_HEIGHT - 1; y >= 0; y--) {
        int full = 1;
        // 检查当前行是否每列都有方块
        for (int x = 0; x < TETRIS_WIDTH; x++) {
            if (!t->grid[y][x]) {
                full = 0;
                break;
            }
        }
        if (full) {
            // 找到满行，将上方所有行向下移动一行
            for (int yy = y; yy > 0; yy--) {
                memcpy(t->grid[yy], t->grid[yy-1], TETRIS_WIDTH);
            }
            // 清空最顶行
            memset(t->grid[0], 0, TETRIS_WIDTH);
            cleared++;
            y++; // 重新检查当前行号（因为上方行已下移）
        }
    }
    return cleared;
}

// 消行得分计算辅助函数
// 功能点：
// - 根据一次性消行数量计算基础得分
// - 应用当前等级倍率
// - 标准俄罗斯方块得分规则：1行=100分，2行=300分，3行=500分，4行=800分
static int score_for_cleared(int lines, int level) {
    if (lines <= 0) return 0;
    int base = 0;
    // 根据消行数量确定基础得分
    switch (lines) {
        case 1: base = 100; break;  // 单行消：100分
        case 2: base = 300; break;  // 双行消：300分
        case 3: base = 500; break;  // 三行消：500分
        case 4: base = 800; break;  // 四行消：800分
        default: base = 100 * lines; break;  // 多行消的简单计算
    }
    // 应用等级倍率（至少1级）
    return base * (level > 0 ? level : 1);
}

// --- 音频支持模块（通过SDL_QueueAudio生成简单音调）---
// 全局音频设备和规格变量
static SDL_AudioDeviceID g_audio_dev = 0;
static SDL_AudioSpec g_audio_spec;

// 初始化音频系统
// 功能点：
// - 配置音频参数（44.1kHz，32位浮点，单声道）
// - 打开默认音频设备
// - 音频初始化失败时静默继续（不中断游戏）
static void tetris_audio_init(void) {
    if (g_audio_dev) return;  // 已初始化则跳过
    SDL_zero(g_audio_spec);
    // 配置音频规格：44.1kHz采样率，32位浮点格式，单声道，4096样本缓冲区
    g_audio_spec.freq = 44100;
    g_audio_spec.format = AUDIO_F32SYS;
    g_audio_spec.channels = 1;
    g_audio_spec.samples = 4096;
    g_audio_dev = SDL_OpenAudioDevice(NULL, 0, &g_audio_spec, NULL, 0);
    if (g_audio_dev == 0) {
        // 音频初始化失败；继续运行但无音频
        SDL_Log("SDL_OpenAudioDevice failed: %s", SDL_GetError());
    } else {
        SDL_PauseAudioDevice(g_audio_dev, 0);  // 启动音频播放
    }
}

// 播放指定频率和时长的音调
// 参数：
// - freq_hz: 频率（Hz）
// - ms: 时长（毫秒）
// - volume: 音量（0.0-1.0）
static void tetris_play_tone(float freq_hz, int ms, float volume) {
    if (!g_audio_dev) return;  // 无音频设备则跳过
    // 计算需要的样本数
    int samples = (int)(g_audio_spec.freq * (ms / 1000.0f));
    if (samples <= 0) return;
    // 分配音频缓冲区
    float* buf = (float*)malloc(sizeof(float) * samples);
    if (!buf) return;
    // 生成正弦波音调
    const float two_pi_f = 2.0f * 3.14159265f * freq_hz;
    for (int i = 0; i < samples; i++) {
        float t = (float)i / (float)g_audio_spec.freq;
        // 简单正弦波 + 线性衰减包络
        float env = 1.0f - (float)i / (float)samples;  // 线性衰减
        buf[i] = volume * env * sinf(two_pi_f * t);
    }
    // 队列音频数据并释放缓冲区
    SDL_QueueAudio(g_audio_dev, buf, samples * sizeof(float));
    free(buf);
}

// 播放方块着陆音效
static void tetris_play_landing(void) {
    tetris_play_tone(220.0f, 80, 0.25f);
}

// 播放消行音效序列
static void tetris_play_lineclear(void) {
    // 序列：短促的下降音调
    tetris_play_tone(880.0f, 80, 0.25f);   // 高音
    tetris_play_tone(660.0f, 80, 0.25f);   // 中音
    tetris_play_tone(440.0f, 120, 0.25f);  // 低音（稍长）
}

// 播放升级音效
static void tetris_play_levelup(void) {
    tetris_play_tone(1000.0f, 150, 0.35f);
}

// 使用位图数字绘制小数字的辅助函数
// 注意：像素数字HUD已移除（TTF侧边栏替代）

// 简单5x5像素字体，支持A-Z、数字和空格
static const uint8_t font5x5[36][5] = {
    // A-Z (0..25)
    {0b01110,0b10001,0b11111,0b10001,0b10001}, // A
    {0b11110,0b10001,0b11110,0b10001,0b11110}, // B
    {0b01111,0b10000,0b10000,0b10000,0b01111}, // C
    {0b11110,0b10001,0b10001,0b10001,0b11110}, // D
    {0b11111,0b10000,0b11110,0b10000,0b11111}, // E
    {0b11111,0b10000,0b11110,0b10000,0b10000}, // F
    {0b01111,0b10000,0b10111,0b10001,0b01110}, // G
    {0b10001,0b10001,0b11111,0b10001,0b10001}, // H
    {0b01110,0b00100,0b00100,0b00100,0b01110}, // I
    {0b00111,0b00010,0b00010,0b10010,0b01100}, // J
    {0b10001,0b10010,0b11100,0b10010,0b10001}, // K
    {0b10000,0b10000,0b10000,0b10000,0b11111}, // L
    {0b10001,0b11011,0b10101,0b10001,0b10001}, // M (approx)
    {0b10001,0b11001,0b10101,0b10011,0b10001}, // N
    {0b01110,0b10001,0b10001,0b10001,0b01110}, // O
    {0b11110,0b10001,0b11110,0b10000,0b10000}, // P
    {0b01110,0b10001,0b10001,0b10011,0b01111}, // Q
    {0b11110,0b10001,0b11110,0b10010,0b10001}, // R
    {0b01111,0b10000,0b01110,0b00001,0b11110}, // S
    {0b11111,0b00100,0b00100,0b00100,0b00100}, // T
    {0b10001,0b10001,0b10001,0b10001,0b01110}, // U
    {0b10001,0b10001,0b10001,0b01010,0b00100}, // V
    {0b10001,0b10001,0b10101,0b11011,0b10001}, // W
    {0b10001,0b01010,0b00100,0b01010,0b10001}, // X
    {0b10001,0b01010,0b00100,0b00100,0b00100}, // Y
    {0b11111,0b00010,0b00100,0b01000,0b11111}, // Z
    // 0-9 (26..35)
    {0b01110,0b10101,0b10101,0b10101,0b01110}, // 0
    {0b00100,0b01100,0b00100,0b00100,0b01110}, // 1
    {0b01110,0b10001,0b00010,0b00100,0b11111}, // 2
    {0b01110,0b10001,0b00110,0b10001,0b01110}, // 3
    {0b00010,0b00110,0b01010,0b11111,0b00010}, // 4
    {0b11111,0b10000,0b11110,0b00001,0b11110}, // 5
    {0b01110,0b10000,0b11110,0b10001,0b01110}, // 6
    {0b11111,0b00001,0b00010,0b00100,0b00100}, // 7
    {0b01110,0b10001,0b01110,0b10001,0b01110}, // 8
    {0b01110,0b10001,0b01111,0b00001,0b01110}  // 9
};

// 像素字体文本渲染器
// 功能点：
// - 使用内置的5x5位图字体渲染文本
// - 支持A-Z（自动转为大写）和0-9数字
// - 空格字符特殊处理
static void draw_text_renderer(Tetris* t, int x, int y, const char* text, int cell_size) {
    if (!t || !t->renderer || !text) return;
    // 计算缩放比例（基于cell_size）
    int scale = cell_size / 3;
    if (scale < 2) scale = 2;
    int cx = x;
    // 逐字符渲染
    for (const char* p = text; *p; ++p) {
        char ch = *p;
        // 小写字母转为大写
        if (ch >= 'a' && ch <= 'z') ch = (char)(ch - 'a' + 'A');
        // 空格处理
        if (ch == ' ') {
            cx += 6 * scale;
            continue;
        }
        // 查找字符索引
        int idx = -1;
        if (ch >= 'A' && ch <= 'Z') idx = ch - 'A';
        else if (ch >= '0' && ch <= '9') idx = 26 + (ch - '0');
        // 不支持的字符跳过
        if (idx < 0) { cx += 6 * scale; continue; }
        // 渲染5x5位图字体
        for (int ry = 0; ry < 5; ry++) {
            for (int rx = 0; rx < 5; rx++) {
                if (font5x5[idx][ry] & (1 << (4 - rx))) {
                    SDL_SetRenderDrawColor(t->renderer, 255,255,255,255);
                    SDL_Rect r = { cx + rx*scale, y + ry*scale, scale-1, scale-1 };
                    SDL_RenderFillRect(t->renderer, &r);
                }
            }
        }
        cx += 6 * scale;
    }
}

// 公共接口：供其他模块使用相同的像素字体绘制文本
void tetris_draw_text(Tetris* t, int x, int y, const char* text, int cell_size) {
    draw_text_renderer(t, x, y, text, cell_size);
}

// 渲染方块预览（在4x4区域内居中显示）
// 功能点：
// - 在指定位置渲染指定方块的预览
// - 使用方块的原始旋转状态（rotation 0）
// - 每个单元格按指定大小绘制
void tetris_render_preview(Tetris* t, TetrominoId id, int px, int py, int cell_size) {
    if (!t || !t->renderer) return;
    if (id < 0 || id > TET_L) return;
    // 预览使用旋转状态0
    uint16_t shape = tetromino_shapes[id][0];
    // 渲染4x4预览区域
    for (int ry = 0; ry < 4; ry++) {
        for (int rx = 0; rx < 4; rx++) {
            if (shape_cell(shape, rx, ry)) {
                // 使用方块对应的颜色
                SDL_Color c = tetromino_colors[id];
                SDL_SetRenderDrawColor(t->renderer, c.r, c.g, c.b, c.a);
                SDL_Rect r = { px + rx * cell_size, py + ry * cell_size, cell_size, cell_size };
                SDL_RenderFillRect(t->renderer, &r);
            }
        }
    }
}

// 初始化俄罗斯方块游戏状态
// 功能点：
// - 清零所有游戏数据
// - 设置默认游戏参数（下落间隔、速度倍率等）
// - 初始化音频系统
// - 随机化种子并生成第一个方块
void tetris_init(Tetris* t, SDL_Renderer* renderer) {
    if (!t) return;
    // 清零结构体
    memset(t, 0, sizeof(*t));
    t->renderer = renderer;
    // 设置默认游戏参数
    t->base_drop_interval_ms = 1000;  // 基础下落间隔1秒
    t->speed_multiplier = 1.0f;       // 默认速度倍率
    t->drop_interval_ms = (uint32_t)(t->base_drop_interval_ms / t->speed_multiplier);
    t->score = 0;                     // 初始分数
    t->level = 1;                     // 初始等级
    t->lines_cleared = 0;             // 已消行数
    // 初始化音频（尽力而为，不影响游戏运行）
    tetris_audio_init();
    // 设置随机种子
    srand((unsigned)time(NULL));
    // 生成初始方块序列
    shuffle_bag(t);
    // 生成第一个下落方块
    t->current_id = t->bag[t->bag_index++];
    if (t->bag_index >= 7) shuffle_bag(t);
    t->current_rot = 0;      // 初始旋转状态
    t->current_x = 3;        // 初始X坐标（居中）
    t->current_y = 0;        // 初始Y坐标（顶部）
    t->last_drop_time = SDL_GetTicks();
    // 保留draw_default_hud设置（由main.c控制）
}

// 重置游戏状态（用于重新开始游戏）
// 功能点：
// - 清空游戏网格
// - 重置所有游戏统计数据
// - 重新生成方块序列
// - 生成新的下落方块
void tetris_reset(Tetris* t) {
    // 清空游戏网格
    memset(t->grid, 0, sizeof(t->grid));
    // 重置游戏统计
    t->score = 0;
    t->level = 1;
    t->lines_cleared = 0;
    t->game_over = 0;
    // 重置游戏速度参数
    t->base_drop_interval_ms = 1000;
    t->speed_multiplier = 1.0f;
    t->drop_interval_ms = (uint32_t)(t->base_drop_interval_ms / t->speed_multiplier);
    // 重新生成方块序列
    shuffle_bag(t);
    // 生成新的下落方块
    t->current_id = t->bag[t->bag_index++];
    t->current_rot = 0;
    t->current_x = 3;
    t->current_y = 0;
    t->last_drop_time = SDL_GetTicks();
    // 启用默认HUD绘制
    t->draw_default_hud = 1;
}

// 已移除：tetris_handle_key函数（旧的按键处理逻辑，已由keymap系统完全替代）
// 原函数已删除以减少代码冗余

// 更新游戏状态（处理自动下落和方块锁定）
// 功能点：
// - 根据时间间隔自动下移方块
// - 处理方块着陆、消行、得分计算
// - 检测游戏结束条件
// - 生成新的下落方块
void tetris_update(Tetris* t, uint32_t now_ms) {
    if (!t) return;
    // 检查是否到达下落时间间隔
    if (now_ms - t->last_drop_time >= t->drop_interval_ms) {
        t->last_drop_time = now_ms;
        // 尝试向下移动方块
        if (!check_collision(t, t->current_id, t->current_rot, t->current_x, t->current_y + 1)) {
            t->current_y++;  // 无碰撞则下移
        } else {
            // 方块无法下移：锁定到网格
            lock_piece(t);
            tetris_play_landing();  // 播放着陆音效
            // 处理消行
            int c = clear_lines(t);
            if (c > 0) {
                tetris_play_lineclear();  // 播放消行音效
                // 计算得分
                int gained = score_for_cleared(c, t->level);
                t->score += gained;
                t->lines_cleared += c;
                // 检查等级提升（每10行升一级）
                int new_level = 1 + (t->lines_cleared / 10);
                if (new_level != t->level) {
                    t->level = new_level;
                    // 根据等级调整下落速度
                    int new_interval = 1000 - (t->level - 1) * 100;
                    if (new_interval < 100) new_interval = 100;  // 最小间隔100ms
                    t->base_drop_interval_ms = (uint32_t)new_interval;
                    t->drop_interval_ms = (uint32_t)(t->base_drop_interval_ms / t->speed_multiplier);
                    tetris_play_levelup();  // 播放升级音效
                }
            }
            // 生成下一个方块
            t->current_id = t->bag[t->bag_index++];
            if (t->bag_index >= 7) shuffle_bag(t);  // 袋子用完重新洗牌
            t->current_rot = 0;
            t->current_x = 3;
            t->current_y = 0;
            // 检查新方块是否能生成（碰撞检测）
            if (check_collision(t, t->current_id, t->current_rot, t->current_x, t->current_y)) {
                t->game_over = 1;  // 生成失败，游戏结束
                return; // 不生成方块
            }
        }
    }
}

// 设置游戏速度倍率
// 功能点：
// - 设置方块下落速度倍率（0.5x到2.0x范围）
// - 自动重新计算实际下落间隔
void tetris_set_speed_multiplier(Tetris* t, float mul) {
    if (!t) return;
    // 限制倍率范围
    if (mul < 0.5f) mul = 0.5f;
    if (mul > 2.0f) mul = 2.0f;
    t->speed_multiplier = mul;
    // 重新计算实际下落间隔
    if (t->base_drop_interval_ms == 0) t->base_drop_interval_ms = 1000;
    t->drop_interval_ms = (uint32_t)(t->base_drop_interval_ms / t->speed_multiplier);
}

// 获取当前速度倍率
float tetris_get_speed_multiplier(Tetris* t) {
    if (!t) return 1.0f;
    return t->speed_multiplier;
}

// 设置HUD消息（无类型，默认信息类型）
void tetris_set_hud_message(Tetris* t, const char* msg, uint32_t duration_ms) {
    tetris_set_hud_message_typed(t, msg, duration_ms, 0); // 默认信息类型
}

// 设置带类型的HUD消息
// 功能点：
// - 在游戏界面显示临时消息
// - 支持不同消息类型（信息、成功、警告、错误）
// - 消息会在指定时间后自动消失
void tetris_set_hud_message_typed(Tetris* t, const char* msg, uint32_t duration_ms, int type) {
    if (!t) return;
    if (!msg) {
        // 清空消息
        t->hud_msg[0] = '\0';
        t->hud_msg_until_ms = 0;
        t->hud_msg_type = 0;
        return;
    }
    // 复制消息文本（兼容MSVC和标准C）
#ifdef _MSC_VER
    strncpy_s(t->hud_msg, sizeof(t->hud_msg), msg, _TRUNCATE);
#else
    strncpy(t->hud_msg, msg, sizeof(t->hud_msg)-1);
    t->hud_msg[sizeof(t->hud_msg)-1] = '\0';
#endif
    // 设置消息过期时间和类型
    t->hud_msg_until_ms = SDL_GetTicks() + duration_ms;
    t->hud_msg_type = type;
}

// 执行游戏动作（由keymap系统调用）
// 功能点：
// - 处理所有游戏控制动作（移动、旋转、降落等）
// - 每个动作都包含完整的游戏逻辑（碰撞检测、状态更新、音效播放）
void tetris_perform_action(Tetris* t, int action) {
    if (!t) return;
    switch (action) {
        case ACTION_MOVE_LEFT:
            // 左移：检查碰撞后移动
            if (!check_collision(t, t->current_id, t->current_rot, t->current_x - 1, t->current_y))
                t->current_x--;
            break;
        case ACTION_MOVE_RIGHT:
            // 右移：检查碰撞后移动
            if (!check_collision(t, t->current_id, t->current_rot, t->current_x + 1, t->current_y))
                t->current_x++;
            break;
        case ACTION_SOFT_DROP:
            // 软降：向下移动一行
            if (!check_collision(t, t->current_id, t->current_rot, t->current_x, t->current_y + 1))
                t->current_y++;
            break;
        case ACTION_HARD_DROP:
            // 硬降：直接降到底部
            while (!check_collision(t, t->current_id, t->current_rot, t->current_x, t->current_y + 1))
                t->current_y++;
            // 锁定方块并处理完整游戏逻辑
            lock_piece(t);
            {
                tetris_play_landing();  // 着陆音效
                int c = clear_lines(t);  // 消行处理
                if (c > 0) {
                    tetris_play_lineclear();  // 消行音效
                    // 得分计算
                    int gained = score_for_cleared(c, t->level);
                    t->score += gained;
                    t->lines_cleared += c;
                    // 等级提升检测
                    int new_level = 1 + (t->lines_cleared / 10);
                    if (new_level != t->level) {
                        t->level = new_level;
                        // 调整下落速度
                        int new_interval = 1000 - (t->level - 1) * 100;
                        if (new_interval < 100) new_interval = 100;
                        t->base_drop_interval_ms = (uint32_t)new_interval;
                        t->drop_interval_ms = (uint32_t)(t->base_drop_interval_ms / t->speed_multiplier);
                        tetris_play_levelup();  // 升级音效
                    }
                }
                // 生成下一个方块
                t->current_id = t->bag[t->bag_index++];
                if (t->bag_index >= 7) shuffle_bag(t);
                t->current_rot = 0;
                t->current_x = 3;
                t->current_y = 0;
                // 检查游戏结束条件
                if (check_collision(t, t->current_id, t->current_rot, t->current_x, t->current_y)) {
                    tetris_reset(t);  // 无法生成方块，重启游戏
                }
            }
            break;
        case ACTION_ROTATE:
            // 顺时针旋转
            {
                int newrot = (t->current_rot + 1) & 3;
                if (!check_collision(t, t->current_id, newrot, t->current_x, t->current_y))
                    t->current_rot = newrot;
            }
            break;
        default:
            // 未知动作，忽略
            break;
    }
}

// 渲染俄罗斯方块游戏界面
// 功能点：
// - 绘制游戏网格和已放置的方块
// - 渲染当前下落方块（带高亮效果）
// - 处理游戏结束画面
// - 绘制边框（无内部网格线）
void tetris_render(Tetris* t, int px, int py, int cell_size) {
    if (!t || !t->renderer) return;
    // 清空游戏区域（假设背景已设置）
    SDL_SetRenderDrawColor(t->renderer, 0, 0, 0, 255);
    SDL_Rect area = { px, py, TETRIS_WIDTH * cell_size, TETRIS_HEIGHT * cell_size };
    SDL_RenderFillRect(t->renderer, &area);

    // 游戏结束时渲染结束画面
    if (t->game_over) {
        // 半透明遮罩
        SDL_SetRenderDrawColor(t->renderer, 0, 0, 0, 180);
        SDL_RenderFillRect(t->renderer, &area);

        // "GAME OVER" 文字
        int w = TETRIS_WIDTH * cell_size;
        int center_x = px + w / 2;
        int center_y = py + (TETRIS_HEIGHT * cell_size) / 2;

        const char* game_over_text = "GAME OVER";
        int text_x = center_x - (6 * (cell_size/3) * (int)strlen(game_over_text) / 2);
        int text_y = center_y - (cell_size/2);

        // 文字背景（红色）
        SDL_SetRenderDrawColor(t->renderer, 255, 0, 0, 255);
        SDL_Rect bg = { text_x - 4, text_y - 4, (int)(6 * (cell_size/3) * strlen(game_over_text) + 8), (int)(5 * (cell_size/3) + 8) };
        SDL_RenderFillRect(t->renderer, &bg);

        // 白色文字
        SDL_SetRenderDrawColor(t->renderer, 255, 255, 255, 255);
        draw_text_renderer(t, text_x, text_y, game_over_text, cell_size);

        // 重启提示
        const char* restart_text = "Press R to restart";
        int restart_x = center_x - (6 * (cell_size/3) * (int)strlen(restart_text) / 2);
        int restart_y = center_y + cell_size;
        draw_text_renderer(t, restart_x, restart_y, restart_text, cell_size);

        return; // 游戏结束时不渲染普通游戏元素
    }
    // 绘制游戏区域单一边框（无内部网格线）
    SDL_SetRenderDrawColor(t->renderer, 120, 120, 120, 255);
    SDL_Rect border = area;
    SDL_RenderDrawRect(t->renderer, &border);

    // 绘制已放置的方块
    for (int y = 0; y < TETRIS_HEIGHT; y++) {
        for (int x = 0; x < TETRIS_WIDTH; x++) {
            uint8_t val = t->grid[y][x];
            if (val) {
                // 获取方块颜色索引
                int idx = (int)val - 1;
                if (idx < 0) idx = 0;
                if (idx > 6) idx = 6;
                SDL_Color c = tetromino_colors[idx];
                SDL_SetRenderDrawColor(t->renderer, c.r, c.g, c.b, c.a);
                SDL_Rect r = { px + x * cell_size, py + y * cell_size, cell_size, cell_size };
                SDL_RenderFillRect(t->renderer, &r);
            }
        }
    }

    // 绘制当前下落方块
    if (t->current_id != TET_NONE) {
        uint16_t shape = tetromino_shapes[t->current_id][t->current_rot & 3];
        for (int ry = 0; ry < 4; ry++) {
            for (int rx = 0; rx < 4; rx++) {
                if (shape_cell(shape, rx, ry)) {
                    int gx = t->current_x + rx;
                    int gy = t->current_y + ry;
                    // 确保在有效边界内
                    if (gx >= 0 && gx < TETRIS_WIDTH && gy >= 0 && gy < TETRIS_HEIGHT) {
                        SDL_Color c = tetromino_colors[t->current_id];
                        // 下落方块使用稍亮的颜色
                        SDL_SetRenderDrawColor(t->renderer, (Uint8)min(255, c.r + 40), (Uint8)min(255, c.g + 40), (Uint8)min(255, c.b + 40), c.a);
                        SDL_Rect r = { px + gx * cell_size, py + gy * cell_size, cell_size, cell_size };
                        SDL_RenderFillRect(t->renderer, &r);
                    }
                }
            }
        }
    }
    // 像素HUD已移除；侧边栏（TTF）提供分数/等级显示
    // HUD消息在其他地方绘制（TTF侧边栏）；传统像素HUD已移除
}

// tetris.c 结束