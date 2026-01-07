#ifndef TETRIS_LOADER_H
#define TETRIS_LOADER_H

#include "tetris.h"

// 加载关卡文件（.tetrislvl或其他支持的类型）
// 功能点：
// - 解析关卡文件格式
// - 设置游戏网格状态
// - 成功返回0，失败返回非零值并在errbuf中写入简短错误消息
int tetris_load_level_file(const char* path, Tetris* t, char* errbuf, int errlen);

#endif // TETRIS_LOADER_H


