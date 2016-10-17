#pragma once

#include "mlcs.h"
#include "suc_tabs.h"

typedef struct Node *Node_T;

/* 前驱后继节点集中的节点 */
typedef struct PS_Node {
    Node_T node;
    struct PS_Node *next;
} *PS_Node_T;

 struct Node {
    Seq_Len_T path_len; /* 当前最长路径长度 */
    PS_Node_T precursors; /* 极大前驱集 */
    PS_Node_T successors; /* 后继集 */
    struct Node *next; /* 指向下一个节点 */
    struct Node **prev_next; /* 指向前一个节点的next域 */
    Seq_Len_T key[]; /* 坐标向量 */
 };

typedef uint64_t Node_Num_T;

typedef struct Hash_Tab {
    uint32_t tab_size; /* 哈希表长 */
    Seq_Num_T key_len; /* 节点中每个key的长度 */
    Node_T end_node; /* 终止节点 */
    Node_T tab[];
} *Hash_Tab_T;

/* 图为哈希表实现 */
typedef Hash_Tab_T Graph_T;

Graph_T build_graph(Suc_Tabs_T suc_tabs);
MLCS_Set_T collect_mlcs(Graph_T graph, char *seq);

