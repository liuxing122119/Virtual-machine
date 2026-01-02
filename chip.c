#include "chip8.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// CHIP-8 默认字体数据 (0-F的5x8像素位图)
const uint8_t chip8_fontset[CHIP8_FONTSET_SIZE] = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

// 初始化CHIP-8虚拟机
void chip8_initialize(Chip8* chip8) {
    // 初始化随机数种子
    srand((unsigned int)time(NULL));

    // 重置所有状态
    chip8_reset(chip8);
}

// 加载默认字体数据到内存
void chip8_load_fontset(Chip8* chip8) {
    memcpy(chip8->memory, chip8_fontset, CHIP8_FONTSET_SIZE);
}

// 重置CHIP-8虚拟机到初始状态
void chip8_reset(Chip8* chip8) {
    // 清空所有寄存器
    memset(chip8->V, 0, sizeof(chip8->V));
    chip8->I = 0;
    chip8->PC = 0x200;  // 程序起始地址
    chip8->SP = 0;

    // 清空内存
    memset(chip8->memory, 0, CHIP8_MEMORY_SIZE);

    // 清空栈
    memset(chip8->stack, 0, sizeof(chip8->stack));

    // 清空显示缓冲区
    memset(chip8->display, 0, sizeof(chip8->display));
    chip8->draw_flag = 0;

    // 重置定时器
    chip8->delay_timer = 0;
    chip8->sound_timer = 0;

    // 清空按键状态
    memset(chip8->keys, 0, sizeof(chip8->keys));

    // 加载字体数据
    chip8_load_fontset(chip8);

    // 重置指令和状态
    chip8->opcode = 0;
    chip8->halted = 0;

    // 重置扩展功能状态
    chip8->speed_multiplier = 1.0;
    memset(chip8->current_rom_path, 0, sizeof(chip8->current_rom_path));
}

// 从文件加载ROM到内存
int chip8_load_rom(Chip8* chip8, const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) {
        printf("错误: 无法打开ROM文件 %s\n", filename);
        return -1;
    }

    // 获取文件大小
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // 检查文件大小是否超过可用内存
    if (file_size > (CHIP8_MEMORY_SIZE - 0x200)) {
        printf("错误: ROM文件太大 (%ld字节)，超过可用内存\n", file_size);
        fclose(file);
        return -1;
    }

    // 读取ROM数据到内存 (从0x200地址开始)
    size_t bytes_read = fread(&chip8->memory[0x200], 1, file_size, file);
    if (bytes_read != (size_t)file_size) {
        printf("错误: ROM文件读取失败\n");
        fclose(file);
        return -1;
    }

    fclose(file);

    // 保存当前ROM路径（安全拷贝，兼容 MSVC 与其他编译器）
#if defined(_MSC_VER)
    strcpy_s(chip8->current_rom_path, sizeof(chip8->current_rom_path), filename);
#else
    strncpy(chip8->current_rom_path, filename, sizeof(chip8->current_rom_path) - 1);
    chip8->current_rom_path[sizeof(chip8->current_rom_path) - 1] = '\0';
#endif

    printf("成功加载ROM: %s (%ld字节)\n", filename, file_size);
    return 0;
}

// 执行一个完整的CHIP-8指令周期
void chip8_emulate_cycle(Chip8* chip8) {
    // 取指 (fetch)
    chip8->opcode = chip8_fetch_opcode(chip8);

    // 译码和执行 (decode & execute)
    chip8_decode_execute(chip8, chip8->opcode);

    // 更新程序计数器 (除非跳转指令已更新)
    // 注意：某些指令会在执行时更新PC，这里不需要额外更新
}

// 获取当前PC指向的16位指令
uint16_t chip8_fetch_opcode(const Chip8* chip8) {
    // CHIP-8是大端字节序
    return (chip8->memory[chip8->PC] << 8) | chip8->memory[chip8->PC + 1];
}

// 解码并执行指令 (CHIP-8指令集实现)
void chip8_decode_execute(Chip8* chip8, uint16_t opcode) {
    // 解析指令的各个部分
    uint8_t op    = (opcode & 0xF000) >> 12;  // 最高4位操作码
    uint8_t x     = (opcode & 0x0F00) >> 8;   // 第二个4位 (通常是Vx)
    uint8_t y     = (opcode & 0x00F0) >> 4;   // 第三个4位 (通常是Vy)
    uint8_t n     = (opcode & 0x000F);        // 最低4位
    uint8_t nn    = (opcode & 0x00FF);        // 最低8位
    uint16_t nnn  = (opcode & 0x0FFF);        // 最低12位

    // 指令计数器，用于调试
    static uint32_t instruction_count = 0;
    instruction_count++;

    // 根据操作码执行相应指令
    switch (op) {
        case 0x0:
            if (nnn == 0x0E0) {
                // 00E0 - 清屏
                chip8_clear_display(chip8);
            } else if (nnn == 0x0EE) {
                // 00EE - 从子程序返回
                if (chip8->SP > 0) {
                    chip8->SP--;
                    chip8->PC = chip8->stack[chip8->SP];
                }
            } else {
                // 0NNN - 调用机器码子程序 (CHIP-8中未实现)
                // 在现代实现中通常忽略
            }
            chip8->PC += 2;
            break;

        case 0x1:
            // 1NNN - 跳转到地址NNN
            chip8->PC = nnn;
            break;

        case 0x2:
            // 2NNN - 调用子程序
            if (chip8->SP < CHIP8_STACK_SIZE) {
                chip8->stack[chip8->SP] = chip8->PC;
                chip8->SP++;
                chip8->PC = nnn;
            } else {
                printf("错误: 栈溢出\n");
                chip8->halted = 1;
            }
            break;

        case 0x3:
            // 3XNN - 如果Vx == NN，跳过下一条指令
            if (chip8->V[x] == nn) {
                chip8->PC += 2;
            }
            chip8->PC += 2;
            break;

        case 0x4:
            // 4XNN - 如果Vx != NN，跳过下一条指令
            if (chip8->V[x] != nn) {
                chip8->PC += 2;
            }
            chip8->PC += 2;
            break;

        case 0x5:
            if (n == 0) {
                // 5XY0 - 如果Vx == Vy，跳过下一条指令
                if (chip8->V[x] == chip8->V[y]) {
                    chip8->PC += 2;
                }
            } else {
                // 无效指令
                printf("无效指令: 0x%04X\n", opcode);
                chip8->halted = 1;
                return;
            }
            chip8->PC += 2;
            break;

        case 0x6:
            // 6XNN - 设置Vx = NN
            chip8->V[x] = nn;
            chip8->PC += 2;
            break;

        case 0x7:
            // 7XNN - Vx += NN (不影响进位标志)
            chip8->V[x] += nn;
            chip8->PC += 2;
            break;

        case 0x8:
            switch (n) {
                case 0x0:
                    // 8XY0 - Vx = Vy
                    chip8->V[x] = chip8->V[y];
                    break;
                case 0x1:
                    // 8XY1 - Vx |= Vy
                    chip8->V[x] |= chip8->V[y];
                    break;
                case 0x2:
                    // 8XY2 - Vx &= Vy
                    chip8->V[x] &= chip8->V[y];
                    break;
                case 0x3:
                    // 8XY3 - Vx ^= Vy
                    chip8->V[x] ^= chip8->V[y];
                    break;
                case 0x4:
                    // 8XY4 - Vx += Vy (带进位)
                    {
                        uint16_t sum = chip8->V[x] + chip8->V[y];
                        chip8->V[0xF] = (sum > 0xFF) ? 1 : 0;
                        chip8->V[x] = sum & 0xFF;
                    }
                    break;
                case 0x5:
                    // 8XY5 - Vx -= Vy (带借位)
                    chip8->V[0xF] = (chip8->V[x] >= chip8->V[y]) ? 1 : 0;
                    chip8->V[x] -= chip8->V[y];
                    break;
                case 0x6:
                    // 8XY6 - Vx >>= 1 (带进位)
                    chip8->V[0xF] = chip8->V[x] & 0x1;
                    chip8->V[x] >>= 1;
                    break;
                case 0x7:
                    // 8XY7 - Vx = Vy - Vx (带借位)
                    chip8->V[0xF] = (chip8->V[y] >= chip8->V[x]) ? 1 : 0;
                    chip8->V[x] = chip8->V[y] - chip8->V[x];
                    break;
                case 0xE:
                    // 8XYE - Vx <<= 1 (带进位)
                    chip8->V[0xF] = (chip8->V[x] & 0x80) ? 1 : 0;
                    chip8->V[x] <<= 1;
                    break;
                default:
                    printf("无效指令: 0x%04X\n", opcode);
                    chip8->halted = 1;
                    return;
            }
            chip8->PC += 2;
            break;

        case 0x9:
            if (n == 0) {
                // 9XY0 - 如果Vx != Vy，跳过下一条指令
                if (chip8->V[x] != chip8->V[y]) {
                    chip8->PC += 2;
                }
            } else {
                // 无效指令
                printf("无效指令: 0x%04X\n", opcode);
                chip8->halted = 1;
                return;
            }
            chip8->PC += 2;
            break;

        case 0xA:
            // ANNN - 设置I = NNN
            chip8->I = nnn;
            chip8->PC += 2;
            break;

        case 0xB:
            // BNNN - 跳转到NNN + V0
            chip8->PC = nnn + chip8->V[0];
            break;

        case 0xC:
            // CXNN - Vx = 随机数 & NN
            chip8->V[x] = chip8_get_random_byte() & nn;
            chip8->PC += 2;
            break;

        case 0xD:
            // DXYN - 绘制精灵
            chip8->V[0xF] = chip8_draw_sprite(chip8, chip8->V[x], chip8->V[y], n);
            chip8->draw_flag = 1;
            chip8->PC += 2;
            break;

        case 0xE:
            if (nn == 0x9E) {
                // EX9E - 如果按键Vx被按下，跳过下一条指令
                if (chip8_is_key_pressed(chip8, chip8->V[x])) {
                    chip8->PC += 2;
                }
            } else if (nn == 0xA1) {
                // EXA1 - 如果按键Vx未被按下，跳过下一条指令
                if (!chip8_is_key_pressed(chip8, chip8->V[x])) {
                    chip8->PC += 2;
                }
            } else {
                printf("无效指令: 0x%04X\n", opcode);
                chip8->halted = 1;
                return;
            }
            chip8->PC += 2;
            break;

        case 0xF:
            switch (nn) {
                case 0x07:
                    // FX07 - Vx = 延时定时器值
                    chip8->V[x] = chip8->delay_timer;
                    break;
                case 0x0A:
                    // FX0A - 等待按键按下
                    {
                        uint8_t key_pressed = 0;
                        for (uint8_t i = 0; i < CHIP8_KEY_COUNT; i++) {
                            if (chip8->keys[i]) {
                                chip8->V[x] = i;
                                key_pressed = 1;
                                break;
                            }
                        }
                        if (!key_pressed) {
                            // 不增加PC，等待下一周期
                            return;
                        }
                    }
                    break;
                case 0x15:
                    // FX15 - 设置延时定时器 = Vx
                    chip8->delay_timer = chip8->V[x];
                    break;
                case 0x18:
                    // FX18 - 设置声音定时器 = Vx
                    chip8->sound_timer = chip8->V[x];
                    break;
                case 0x1E:
                    // FX1E - I += Vx
                    chip8->I += chip8->V[x];
                    break;
                case 0x29:
                    // FX29 - 设置I为字符Vx的地址
                    chip8->I = chip8->V[x] * 5;  // 每个字符5字节
                    break;
                case 0x33:
                    // FX33 - 将Vx的BCD表示存储到I, I+1, I+2
                    {
                        uint8_t value = chip8->V[x];
                        chip8->memory[chip8->I]     = value / 100;
                        chip8->memory[chip8->I + 1] = (value / 10) % 10;
                        chip8->memory[chip8->I + 2] = value % 10;
                    }
                    break;
                case 0x55:
                    // FX55 - 将V0到Vx存储到内存[I]开始的位置
                    for (uint8_t i = 0; i <= x; i++) {
                        chip8->memory[chip8->I + i] = chip8->V[i];
                    }
                    break;
                case 0x65:
                    // FX65 - 从内存[I]开始的位置读取到V0到Vx
                    for (uint8_t i = 0; i <= x; i++) {
                        chip8->V[i] = chip8->memory[chip8->I + i];
                    }
                    break;
                default:
                    printf("无效指令: 0x%04X\n", opcode);
                    chip8->halted = 1;
                    return;
            }
            chip8->PC += 2;
            break;

        default:
            printf("未知操作码: 0x%04X\n", opcode);
            chip8->halted = 1;
            return;
    }
}
