#pragma once

#include "mlcs.h"
#include "suc_tabs.h"

typedef struct Node *Node_T;

typedef uint64_t Node_Num_T;

typedef struct Hash_Tab {
    uint32_t tab_size; /* 哈希表长 */
    Seq_Num_T key_len; /* 节点中每个key的长度 */
    Node_Num_T node_num; /*节点个数  */
    Node_Num_T redundants; /*冗余节点个数  */
    Node_T dest_node; /* 终止节点 */
    Node_T tab[];
} *Hash_Tab_T;

/* 图为哈希表实现 */
typedef Hash_Tab_T Graph_T;

Graph_T build_graph(Suc_Tabs_T suc_tabs);
void print_graph_info(Graph_T graph);
MLCS_Set_T collect_mlcs(Graph_T graph, char *seq);
void print_nodes(Graph_T graph);

