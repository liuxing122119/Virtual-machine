#ifndef SAVE_LOAD_H
#define SAVE_LOAD_H

#include "tetris.h"

// 存档系统接口函数声明

// 保存游戏状态到指定存档槽
int tetris_save_slot(Tetris* t, int slot, char* errbuf, int errlen);
// 从指定存档槽加载游戏状态
int tetris_load_slot(Tetris* t, int slot, char* errbuf, int errlen);
// 读取存档槽信息（分数、等级、修改时间字符串），时间字符串以UTF-8格式填充
int tetris_read_slot_info(int slot, int* out_score, int* out_level, char* timestr, int timestr_len);
// 删除指定存档槽的文件
int tetris_delete_slot(int slot, char* errbuf, int errlen);

#endif 


