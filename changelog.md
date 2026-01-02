# Virtual-machine
CHIP-8虚拟机开发（实训）
-12/29-陈栩姗commit
-12/29-林佳怡commit(讨论使用什么语言以及做哪几个拓展)
-12/29-方冰玉commit(创建github仓库和changelog文件)
-12/30-陈栩姗commit(讨论项目的实现方案）
-12/30-方冰玉commit（查看讲义进行学习）
-12/30 林佳怡commit(讨论学习ing)
-12/31-陈栩姗commit(确定项目的扩展：游戏速度调节、rom拖放加载、存档与读档）
-12/31-方冰玉commit（讨论项目的具体实现相关计划）
-12/31 林佳怡commit(确定三项项目扩展：游戏速度调节、rom拖放加载、存档与读档)
-1/1-陈栩姗commit（讨论扩展的实现）
-1/1-方冰玉commit（讨论扩展的具体实现）
-1/1-林佳怡commit（讨论有关扩展实现具体方法）
-1/2-陈栩姗commit(CHIP-8有关架构实现)
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
    srand((unsigned int)time(NULL));
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
    chip8->PC = 0x200;  
    chip8->SP = 0;
    memset(chip8->memory, 0, CHIP8_MEMORY_SIZE);
    memset(chip8->stack, 0, sizeof(chip8->stack));
    memset(chip8->display, 0, sizeof(chip8->display));
    chip8->draw_flag = 0;
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
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
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
    // 保存当前ROM路径
#if defined(_MSC_VER)
    strcpy_s(chip8->current_rom_path, sizeof(chip8->current_rom_path), filename);
#else
    strncpy(chip8->current_rom_path, filename, sizeof(chip8->current_rom_path) - 1);
    chip8->current_rom_path[sizeof(chip8->current_rom_path) - 1] = '\0';
#endif
    printf("成功加载ROM: %s (%ld字节)\n", filename, file_size);
    return 0;
}
