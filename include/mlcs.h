#pragma once

#include "stdhs.h"

#include "common.h"
#include "mem.h"
#include "adt.h"
#include "bits.h"
#include "patset.h"

typedef char Char_T; /* 字符类型 */
typedef uint16_t Seq_Num_T; /* 序列数量类型 */
typedef uint8_t Seq_Len_T; /* 序列长度类型 */
typedef uint8_t CC_Num_T; /* 字符集大小类型 */

/* 保存生成的所有mlcs */
typedef struct MLCS_Set {
  Seq_Len_T len; /* 某个mlcs的长度 */
  List_T mlcs_list;
} *MLCS_Set_T;


#define DNA 4
#define ABC 26

#define SIGMA DNA   /* 字符集大小 */
