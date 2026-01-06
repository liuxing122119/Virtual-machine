#include "save_load.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

// 确保存档目录存在
static int ensure_save_dir(void) {
    const char* dir = "saves";
#ifdef _WIN32
    if (_mkdir(dir) != 0) {
        
    }
#else
    mkdir(dir, 0755);  
#endif
    return 0;
}

// 生成指定存档槽的文件路径
static const char* slot_path(int slot) {
    static char path[256];
    // 格式：saves/slot{slot}.tetrissav
    snprintf(path, sizeof(path), "saves/slot%d.tetrissav", slot);
    return path;
}

// 保存游戏状态到指定存档槽

int tetris_save_slot(Tetris* t, int slot, char* errbuf, int errlen) {
    // 参数验证
    if (!t || slot < 1 || slot > 5) {
        if (errbuf) {
#ifdef _MSC_VER
            strncpy_s(errbuf, errlen, "无效参数", _TRUNCATE);
#else
            strncpy(errbuf, "无效参数", errlen);
            errbuf[errlen > 0 ? errlen-1 : 0] = '\0';
#endif
        }
        return -1;
    }
    // 确保存档目录存在
    ensure_save_dir();
    const char* path = slot_path(slot);
    FILE* f = fopen(path, "wb");
    if (!f) {
        if (errbuf) snprintf(errbuf, errlen, "无法打开 %s 进行写入", path);
        return -2;
    }
    // 写入文件头
    fwrite("TSAV", 1, 4, f);  // 魔数标识
    uint8_t version = 1;
    fwrite(&version, 1, 1, f);  // 版本号
    // 写入游戏网格
    for (int y = 0; y < TETRIS_HEIGHT; y++) {
        fwrite(t->grid[y], 1, TETRIS_WIDTH, f);
    }
    // 写入方块袋序列
    fwrite(t->bag, 1, 7, f);
    uint8_t bag_index = (uint8_t)t->bag_index;
    fwrite(&bag_index, 1, 1, f);
    // 写入当前方块状态
    uint8_t current_id = (uint8_t)t->current_id;
    fwrite(&current_id, 1, 1, f);
    int32_t current_rot = t->current_rot;
    fwrite(&current_rot, sizeof(current_rot), 1, f);
    // 写入当前方块位置
    int32_t cx = t->current_x, cy = t->current_y;
    fwrite(&cx, sizeof(cx), 1, f);
    fwrite(&cy, sizeof(cy), 1, f);
    // 写入游戏参数
    uint32_t base_int = t->base_drop_interval_ms;
    fwrite(&base_int, sizeof(base_int), 1, f);
    float speed = t->speed_multiplier;
    fwrite(&speed, sizeof(speed), 1, f);
    // 写入统计数据
    int32_t score = t->score;
    fwrite(&score, sizeof(score), 1, f);
    int32_t level = t->level;
    fwrite(&level, sizeof(level), 1, f);
    int32_t lines = t->lines_cleared;
    fwrite(&lines, sizeof(lines), 1, f);

    fclose(f);
    return 0;
}

// 从指定存档槽加载游戏状态
int tetris_load_slot(Tetris* t, int slot, char* errbuf, int errlen) {
    // 参数验证
    if (!t || slot < 1 || slot > 5) {
        if (errbuf) {
#ifdef _MSC_VER
            strncpy_s(errbuf, errlen, "无效参数", _TRUNCATE);
#else
            strncpy(errbuf, "无效参数", errlen);
            errbuf[errlen > 0 ? errlen-1 : 0] = '\0';
#endif
        }
        return -1;
    }
    const char* path = slot_path(slot);
    FILE* f = fopen(path, "rb");
    if (!f) {
        if (errbuf) snprintf(errbuf, errlen, "无法打开 %s 进行读取", path);
        return -2;
    }
    // 验证文件头
    char magic[4];
    if (fread(magic, 1, 4, f) != 4) {
        fclose(f);
        if (errbuf) snprintf(errbuf, errlen, "文件无效");
        return -3;
    }
    if (memcmp(magic, "TSAV", 4) != 0) {
        fclose(f);
        if (errbuf) snprintf(errbuf, errlen, "文件格式错误");
        return -4;
    }
    // 检查版本
    uint8_t version = 0;
    fread(&version, 1, 1, f);
    if (version != 1) {
        fclose(f);
        if (errbuf) snprintf(errbuf, errlen, "不支持的版本");
        return -5;
    }
    // 读取游戏网格
    for (int y = 0; y < TETRIS_HEIGHT; y++) {
        if (fread(t->grid[y], 1, TETRIS_WIDTH, f) != TETRIS_WIDTH) {
            fclose(f);
            if (errbuf) snprintf(errbuf, errlen, "文件不完整");
            return -6;
        }
    }
    // 读取方块袋
    if (fread(t->bag, 1, 7, f) != 7) {
        fclose(f);
        if (errbuf) snprintf(errbuf, errlen, "文件不完整");
        return -7;
    }
    // 读取方块袋状态
    uint8_t bag_index = 0;
    fread(&bag_index, 1, 1, f);
    t->bag_index = bag_index;
    // 读取当前方块
    uint8_t current_id = 0;
    fread(&current_id, 1, 1, f);
    t->current_id = (TetrominoId)current_id;
    int32_t current_rot = 0;
    fread(&current_rot, sizeof(current_rot), 1, f);
    t->current_rot = current_rot;
    // 读取当前方块位置
    int32_t cx = 0, cy = 0;
    fread(&cx, sizeof(cx), 1, f);
    fread(&cy, sizeof(cy), 1, f);
    t->current_x = cx; t->current_y = cy;
    // 读取游戏参数
    uint32_t base_int = 0;
    fread(&base_int, sizeof(base_int), 1, f);
    t->base_drop_interval_ms = base_int;
    float speed = 1.0f;
    fread(&speed, sizeof(speed), 1, f);
    t->speed_multiplier = speed;
    // 读取统计数据
    int32_t score = 0;
    fread(&score, sizeof(score), 1, f);
    t->score = score;
    int32_t level = 1;
    fread(&level, sizeof(level), 1, f);
    t->level = level;
    int32_t lines = 0;
    fread(&lines, sizeof(lines), 1, f);
    t->lines_cleared = lines;
    // 重新计算下落间隔
    if (t->speed_multiplier <= 0.0f) t->speed_multiplier = 1.0f;
    t->drop_interval_ms = (uint32_t)(t->base_drop_interval_ms / t->speed_multiplier);

    fclose(f);
    return 0;
}

// 读取存档槽的基本信息（不加载到游戏中）
int tetris_read_slot_info(int slot, int* out_score, int* out_level, char* timestr, int timestr_len) {
    if (slot < 1 || slot > 5) return -1;
    const char* path = slot_path(slot);
    FILE* f = fopen(path, "rb");
    if (!f) return -2;
    
    long offset = 234; 
    if (fseek(f, offset, SEEK_SET) != 0) { fclose(f); return -3; }
    // 读取分数和等级
    int32_t score = 0, level = 0;
    if (fread(&score, sizeof(score), 1, f) != 1) { fclose(f); return -4; }
    if (fread(&level, sizeof(level), 1, f) != 1) { fclose(f); return -5; }
    fclose(f);
    // 输出结果
    if (out_score) *out_score = score;
    if (out_level) *out_level = level;
    // 获取文件修改时间
#ifdef _WIN32
    // Windows使用_stat
    struct _stat st;
    if (_stat(path, &st) == 0) {
        time_t m = st.st_mtime;
        struct tm tbuf;
        localtime_s(&tbuf, &m);
        if (timestr && timestr_len > 0) {
            strftime(timestr, timestr_len, "%Y-%m-%d %H:%M:%S", &tbuf);
        }
    } else {
        if (timestr && timestr_len > 0) timestr[0] = '\0';
    }
#else
    // Unix/Linux使用stat
    struct stat st;
    if (stat(path, &st) == 0) {
        time_t m = st.st_mtime;
        struct tm tbuf;
        localtime_r(&m, &tbuf);
        if (timestr && timestr_len > 0) {
            strftime(timestr, timestr_len, "%Y-%m-%d %H:%M:%S", &tbuf);
        }
    } else {
        if (timestr && timestr_len > 0) timestr[0] = '\0';
    }
#endif
    return 0;
}

// 删除指定的存档槽文件
int tetris_delete_slot(int slot, char* errbuf, int errlen) {
    if (slot < 1 || slot > 5) {
        if (errbuf) {
#ifdef _MSC_VER
            strncpy_s(errbuf, errlen, "无效槽位", _TRUNCATE);
#else
            strncpy(errbuf, "无效槽位", errlen);
            errbuf[errlen > 0 ? errlen-1 : 0] = '\0';
#endif
        }
        return -1;
    }
    const char* path = slot_path(slot);
    // 尝试删除文件
    if (remove(path) == 0) return 0;
    // 删除失败，提供错误信息
    if (errbuf) {
#ifdef _MSC_VER
        snprintf(errbuf, errlen, "无法删除 %s", path);
#else
        snprintf(errbuf, errlen, "无法删除 %s", path);
#endif
    }
    return -2;
}


