#pragma once

#include "mlcs.h"
#include "suc_tabs.h"

typedef struct Node *Node_T;

/* 指向图节点的链表节点 */
typedef struct List_Node {
    struct List_Node *next;
    Node_T node;
} *List_Node_T;

struct Node {
  //  unsigned char a;
  Seq_Len_T pref_len; /* 当前mlcs前缀的长度 */
    MLCS_Num_T pref_num; /* 当前mlcs前缀的数量 */
    unsigned indegree; /* 入度 */
    char *prefixes; /* 保存mlcs前缀 */
    List_Node_T successors; /* 后继集 */
    struct Node *next; /* 指向哈希链表下一个节点 */
    struct Node **prev_next; /* 指向哈希链表前一个节点的next域 */
    Seq_Len_T key[]; /* 坐标向量 */
};

typedef uint64_t Node_Num_T;

typedef struct Hash_Tab {
    uint32_t tab_size; /* 哈希表长 */
    Node_T tab[];
} *Hash_Tab_T;

/* 图为哈希表实现 */
typedef Hash_Tab_T Graph_T;

Node_T build_graph(Suc_Tabs_T suc_tabs);
MLCS_Set_T collect_mlcs(Graph_T graph, char *seq);

