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
