#ifndef CHIP8_H
#define CHIP8_H

#include <stdint.h>
#include <stdbool.h>

// CHIP-8 硬件规格常量
#define CHIP8_MEMORY_SIZE      4096  // 4KB内存
#define CHIP8_STACK_SIZE       16    // 16层调用栈
#define CHIP8_DISPLAY_WIDTH    64    // 显示宽度
#define CHIP8_DISPLAY_HEIGHT   32    // 显示高度
#define CHIP8_KEY_COUNT        16    // 16个按键
#define CHIP8_FONTSET_SIZE     80    // 字体数据大小 (16字符 x 5字节)

// CHIP-8 虚拟机结构体
typedef struct {
    // CPU 寄存器
    uint8_t  V[16];              // 通用寄存器 V0-VF
    uint16_t I;                  // 索引寄存器
    uint16_t PC;                 // 程序计数器
    uint8_t  SP;                 // 栈指针

    // 内存和栈
    uint8_t  memory[CHIP8_MEMORY_SIZE];     // 主内存
    uint16_t stack[CHIP8_STACK_SIZE];       // 调用栈

    // 显示系统
    uint8_t display[CHIP8_DISPLAY_WIDTH * CHIP8_DISPLAY_HEIGHT];  // 显示缓冲区
    uint8_t draw_flag;          // 屏幕更新标志

    // 定时器系统
    uint8_t delay_timer;        // 延时定时器
    uint8_t sound_timer;        // 声音定时器

    // 输入系统
    uint8_t keys[CHIP8_KEY_COUNT];  // 按键状态

    // 指令执行控制
    uint16_t opcode;            // 当前指令
    uint8_t  halted;            // 虚拟机暂停标志

    // 扩展功能状态 (Day 2实现)
    double speed_multiplier;    // 速度调节倍数
    char current_rom_path[256]; // 当前ROM路径
} Chip8;

// CHIP-8 默认字体数据 (0-F的5x8像素位图)
extern const uint8_t chip8_fontset[CHIP8_FONTSET_SIZE];

// 函数声明

// 核心功能
void chip8_initialize(Chip8* chip8);
void chip8_load_fontset(Chip8* chip8);
void chip8_reset(Chip8* chip8);
int chip8_load_rom(Chip8* chip8, const char* filename);

// 指令执行
void chip8_emulate_cycle(Chip8* chip8);
void chip8_decode_execute(Chip8* chip8, uint16_t opcode);

// 定时器和更新
void chip8_update_timers(Chip8* chip8);

// 调试和状态查询
void chip8_print_registers(const Chip8* chip8);
void chip8_print_display(const Chip8* chip8);
void chip8_print_memory(const Chip8* chip8, uint16_t start, uint16_t end);
uint16_t chip8_get_rom_size(const Chip8* chip8);

// 键盘输入
void chip8_set_key(Chip8* chip8, uint8_t key, uint8_t state);
uint8_t chip8_is_key_pressed(const Chip8* chip8, uint8_t key);

// 显示相关
uint8_t chip8_draw_sprite(Chip8* chip8, uint8_t x, uint8_t y, uint8_t height);
void chip8_clear_display(Chip8* chip8);

// 工具函数
uint16_t chip8_fetch_opcode(const Chip8* chip8);
uint8_t chip8_get_random_byte(void);

#endif // CHIP8_H