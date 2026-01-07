#include "loader.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>


// 字符串修剪辅助函数
// 功能点：
// - 移除尾部换行符、回车符和空格
// - 移除前导空格
static void trim(char* s) {
    // 移除尾部换行符、回车符和空格
    int n = (int)strlen(s);
    while (n > 0 && (s[n-1] == '\n' || s[n-1] == '\r' || isspace((unsigned char)s[n-1]))) {
        s[n-1] = '\0';
        --n;
    }
    // 移除前导空格
    char* p = s;
    while (*p && isspace((unsigned char)*p)) ++p;
    if (p != s) memmove(s, p, strlen(p) + 1);
}

// 加载关卡文件并设置游戏状态
// 功能点：
// - 解析文本格式的关卡文件
// - 设置等级、分数、方块袋和游戏网格
// - 支持版本控制和错误处理
int tetris_load_level_file(const char* path, Tetris* t, char* errbuf, int errlen) {
    // 参数验证
    if (!path || !t) {
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
    // 打开文件
    FILE* f = fopen(path, "r");
    if (!f) {
        if (errbuf) snprintf(errbuf, errlen, "无法打开文件: %s", path);
        return -2;
    }
    char line[512];
    int grid_mode = 0;    // 是否处于网格读取模式
    int grid_row = 0;     // 当前读取的网格行号
    // 从干净状态开始
    tetris_reset(t);
    // 默认保留洗牌后的袋子，但如果提供了bag:则会覆盖
    // 逐行解析文件
    while (fgets(line, sizeof(line), f)) {
        trim(line);
        // 跳过注释行和空行
        if (line[0] == '#' || line[0] == '\0') continue;

        if (!grid_mode) {
            // 解析key:value格式
            char* colon = strchr(line, ':');
            if (!colon) continue;  // 不是key:value格式，跳过
            *colon = '\0';
            char* key = line;
            char* val = colon + 1;
            trim(key); trim(val);

            // 处理不同类型的配置项
            if (strcmp(key, "version") == 0) {
                // 版本信息，暂时忽略
            } else if (strcmp(key, "level") == 0) {
                // 设置等级并重新计算下落速度
                int lv = atoi(val);
                if (lv < 1) lv = 1;
                t->level = lv;
                int new_interval = 1000 - (t->level - 1) * 100;
                if (new_interval < 100) new_interval = 100;  // 最小间隔100ms
                t->base_drop_interval_ms = (uint32_t)new_interval;
                t->drop_interval_ms = (uint32_t)(t->base_drop_interval_ms / t->speed_multiplier);
            } else if (strcmp(key, "score") == 0) {
                // 设置分数
                t->score = atoi(val);
            } else if (strcmp(key, "bag") == 0) {
                // 解析7个数字的CSV格式方块袋
                int idx = 0;
                char* p = val;
                while (p && *p && idx < 7) {
                    // 跳过前导空格
                    while (*p && isspace((unsigned char)*p)) p++;
                    char* end = p;
                    // 找到逗号分隔符
                    while (*end && (*end != ',')) end++;
                    char tmp = *end;
                    *end = '\0';
                    // 解析数字，限制在0-6范围内
                    int v = atoi(p);
                    if (v < 0 || v > 6) v = 0;
                    t->bag[idx++] = (TetrominoId)v;
                    if (tmp == ',') p = end + 1; else break;
                }
                t->bag_index = 0;  // 重置袋子索引
            } else if (strcmp(key, "grid") == 0) {
                // 进入网格读取模式
                grid_mode = 1;
                grid_row = 0;
            }
        } else {
            // 读取网格行数据
            if (grid_row < TETRIS_HEIGHT) {
                int len = (int)strlen(line);
                for (int x = 0; x < TETRIS_WIDTH; x++) {
                    // 超出行长的位置默认为'0'（空）
                    char c = (x < len) ? line[x] : '0';
                    t->grid[grid_row][x] = (c == '1' ? 1 : 0);
                }
            }
            grid_row++;
            if (grid_row >= TETRIS_HEIGHT) {
                // 网格读取完成
                // 继续读取但忽略后续行
            }
        }
    }
    fclose(f);
    return 0;  // 成功
}



