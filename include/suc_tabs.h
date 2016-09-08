#pragma once

#include "mlcs.h"

typedef struct Suc_Tab {
    char *seq; /* 对应序列 */
    Seq_Len_T seq_len; /* 后继表列数，即序列长度 */
    void *tab; /* 后继表，为[SIGMA][seq_len]的二维数组 */
} *Suc_Tab_T;

typedef struct Suc_Tabs {
  Seq_Num_T tab_num;
  struct Suc_Tab tabs[];
} *Suc_Tabs_T;


Suc_Tabs_T build_suc_tabs(Patset_T sequences);
bool get_suc_key(Suc_Tabs_T suc_tabs,
		 Seq_Len_T *key, Char_T ch, Seq_Len_T *suc_key);
void print_suc_tabs(Suc_Tabs_T suc_tabs);
