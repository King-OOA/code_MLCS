#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>

#include "common.h"
#include "mem.h"
#include "adt.h"
#include "bits.h"
#include "patset.h"


typedef char Char_T; /* 字符类型 */
typedef uint16_t Seq_Num_T; /* 序列数量类型 */
typedef uint8_t Seq_Len_T; /* 序列长度类型 */
typedef uint8_t CC_Num_T; /* 字符集大小类型 */
typedef uint64_t Hash_Value_T;

typedef struct Suc_Tab {
    char *seq; /* 对应序列 */
    Seq_Len_T seq_len; /* 后继表列数，即序列长度 */
    void *tab; /* 后继表，为[SIGMA][seq_len]的二维数组 */
} *Suc_Tab_T;

typedef struct PS_Node *PS_Node_T;

typedef struct Node {
    Seq_Len_T path_len; /* 当前最长路径长度 */
    PS_Node_T precursors; /* 极大前驱集 */
    PS_Node_T successors; /* 后继集 */
    struct Node *next;
    Seq_Len_T key[]; /* 坐标向量 */
} *Node_T;

/* 前驱后继节点集中的节点 */
struct PS_Node {
    Node_T node;
    struct PS_Node *next;
};

typedef struct Hash_Tab {
    uint32_t tab_size;
    uint64_t node_num;
    Node_T tab[];
} *Hash_Tab_T;

#define SIGMA 4  /* 字符集大小 */
#define HASH_TAB_SIZE 10000 	/* 哈希表大小 */

/* 不同字符所对应后继表的行号 */
char char_to_row[] = {['A'] = 0, ['C'] = 1, ['G'] = 2, ['T'] = 3};

/* 构造后继表 */
void build_suc_tab(void **seq_p, void *suc_tab_p)
{
    Suc_Tab_T suc_tab =  *((Suc_Tab_T *) suc_tab_p);
    char *seq = (char *) *seq_p;

    suc_tab->seq = seq;
    suc_tab->seq_len = strlen(seq);
    Seq_Len_T seq_len = suc_tab->seq_len;
    suc_tab->tab = CALLOC(SIGMA, Seq_Len_T [seq_len+1]);
    Seq_Len_T (*tab)[seq_len+1] = suc_tab->tab;
    
    /* 构建当前序列的后继表 */
    for (Seq_Len_T i = 0; i < seq_len; i++) {
	CC_Num_T row = char_to_row[seq[i]];
	for (int16_t col = i, v = col + 1;
	     col >= 0 && tab[row][col] == 0; col--)
	    tab[row][col] = v;
    }

    /* 指向下一个后继表 */
    (*((Suc_Tab_T *) suc_tab_p))++;
}

/* 打印后继表 */
void print_suc_tabs(Suc_Tab_T suc_tab, Seq_Num_T seq_num)
{
    for (Suc_Tab_T end = suc_tab + seq_num; suc_tab < end; suc_tab++) {
	printf("\n %s\n",(suc_tab->seq));
	Pat_Len_T (*tab)[suc_tab->seq_len+1] = suc_tab->tab;
	for (uint8_t i = 0; i < SIGMA; i++) {
	    for (Seq_Len_T j = 0; j < suc_tab->seq_len+1; j++)
		printf("%2d ", tab[i][j]);
	    putchar('\n');
	}
    }
}

static Hash_Value_T hash(Seq_Len_T *key, Seq_Num_T key_len, int8_t step, uint32_t hash_tab_size)
{
    Hash_Value_T hash_value = 0;
    /* 每隔step，选取一个元素参与运算 */
    for (Pat_Len_T *end = key + key_len; key < end; key += step)
	hash_value = (hash_value << 5) ^ *key;
    
    return hash_value % hash_tab_size;
}

static bool key_not_equal(Seq_Len_T *key1, Seq_Len_T *key2, Seq_Num_T key_len)
{
    while (key_len && *key1 == *key2)
	key_len--, key1++, key2++;
    
    return key_len;
}

/* 如果存在，则返回对应节点的指针；否则，新建节点并插入，返回该节点 */
Node_T find_and_insert(Seq_Len_T *key, Seq_Num_T key_len, Hash_Tab_T hash_tab, bool *found_p)
{
    Hash_Value_T hash_value = hash(key, key_len, 1, hash_tab->tab_size);
    Node_T node;

    for (node = hash_tab->tab[hash_value];
	 node && key_not_equal(node->key, key, key_len);
	 node = node->next)
	;

    if (node) /*找到 */
	*found_p = true;
    else { /* 没找到，则新建节点，并插入到链表头 */
	*found_p = false;
	node = VNEW0(node, key_len, Seq_Len_T);
	memcpy(node->key, key, key_len * sizeof (*key));
	node->next = hash_tab->tab[hash_value];
	hash_tab->tab[hash_value] = node;
    }

    return node;
}

/* 如果成功产生后继，则返回true；否则，返回false */
bool gen_suc_key(Suc_Tab_T suc_tab, Seq_Num_T tab_num, Seq_Len_T *cur_key, Char_T ch, Seq_Len_T *suc_key)
{
    Suc_Tab_T end;
    Seq_Len_T v;

    /* v不为0 */
    for (end = suc_tab + tab_num; suc_tab < end &&
	     (v = ((Seq_Len_T (*)[suc_tab->seq_len+1]) suc_tab->tab)[ch][*cur_key]);
	 suc_tab++) {
	*suc_key++ = v;
	cur_key++;
    }
    
    return suc_tab == end ? true : false;
}

/* 将suc_node加入到node的后继集中 */
inline static void add_to_suc(Node_T node, Node_T suc_node)
{
    PS_Node_T s_node;
    NEW(s_node);

    s_node->node = suc_node;
    s_node->next = node->successors;
    node->successors = s_node;
}

/* 将pre_node加入到node的前驱集中 */
inline static void add_to_pre(Node_T node, Node_T pre_node)
{
    PS_Node_T p_node;
    NEW(p_node);

    p_node->node = pre_node;
    p_node->next = node->precursors;
    node->precursors = p_node;
}

/* 释放node的所有前驱节点 */
inline static void clear_pre(Node_T node)
{
    for (PS_Node_T p_node = node->precursors, next;
	 p_node; p_node = next) {
	next = p_node->next;
	FREE(p_node);
    }
}

inline static void update_nodes(Node_T node, Node_T suc_node)
{
    if (node->path_len + 1 == suc_node->path_len)
	add_to_pre(suc_node, node);
    else if (node->path_len + 1 > suc_node->path_len) {
	clear_pre(suc_node);
	add_to_pre(suc_node, node);
	suc_node->path_len = node->path_len + 1;
	/* 递归地更新suc_node的所有后继节点 */
	for (PS_Node_T s_node = suc_node->successors;
	     s_node; s_node = s_node->next)
	    update_nodes(suc_node, s_node);
    }
}

int main(int32_t argc, char **argv)
{
    Patset_T sequences = patset_new(argv[1]);
    Seq_Num_T seq_num = sequences->pat_num;

    /* 创建后继表数组 */
    struct Suc_Tab suc_tabs[seq_num];
    /* 跟踪当前处理的后继表 */
    Suc_Tab_T cur_tab = suc_tabs;
    /* 为每一个序列构造后继表 */
    list_traverse(sequences->pat_list, build_suc_tab, &cur_tab);
    print_suc_tabs(suc_tabs, seq_num);
    
    
    Seq_Len_T key0[10] = {0}; 
    
    Seq_Len_T key1[seq_num];
    putchar('\n');
    for (Char_T ch = 0; ch < SIGMA; ch++)
	if (gen_suc_key(suc_tabs, seq_num, key0, ch, key1)) {
	    for (Seq_Len_T i = 0; i < seq_num; i++)
		printf("%d", key1[i]);
	    putchar('\n');
	}
    
    exit(0);
    
    uint32_t hash_tab_size = HASH_TAB_SIZE;
    Hash_Tab_T hash_tab = VNEW0(hash_tab, hash_tab_size, Node_T);

    Seq_Len_T key[seq_num]; 
    memset(key, 0, sizeof(key));/* 初始为源节点，全0向量 */
    bool found = false;
    Node_T source_node = find_and_insert(key, seq_num, hash_tab, &found);
    Node_T dest_node;
    VNEW0(dest_node, seq_num, Seq_Len_T);
    Queue_T q = queue_new();
    queue_push(q, source_node);

    while (!queue_empty(q)) {
	Node_T node = queue_pop(q);
	bool no_successors = true;
	/* 依次产生node的后继节点所对应的key */
	for (Char_T ch = 0; ch < SIGMA; ch++) {
	    if (!gen_suc_key(suc_tabs, seq_num, node->key, ch, key))
		continue;
	    no_successors = false; /* 至少有一个后继节点 */
	    bool key_exists = false;
	    Node_T suc_node = find_and_insert(key, seq_num, hash_tab, &key_exists);
	    /* 无论是否存在，都加入后继集 */
	    add_to_suc(node, suc_node);
	    if (!key_exists) {	/* 后继节点不存在 */
		add_to_pre(suc_node, node);
		suc_node->path_len = node->path_len + 1;
		queue_push(q, suc_node);
	    } else /* 后继节点存在 */
		update_nodes(node, suc_node);
	}
	/* 如果没有任何后继，则将终止节点作为其后继 */
	if (no_successors) {
	    add_to_suc(node, dest_node);
	    update_nodes(node, dest_node);
	}
    }
}
