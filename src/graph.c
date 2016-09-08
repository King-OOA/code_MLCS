#include "graph.h"
#include <assert.h>

#define HASH_TAB_SIZE 100000000 /* 哈希表大小 */
#define PRE 0

typedef uint64_t Hash_Value_T;
typedef struct PS_Node *PS_Node_T;

struct Node {
    Seq_Len_T path_len; /* 当前最长路径长度 */
    PS_Node_T precursors; /* 极大前驱集 */
    PS_Node_T successors; /* 后继集 */
    struct Node *next;
    Seq_Len_T key[]; /* 坐标向量 */
};

/* 前驱后继节点集中的节点 */
struct PS_Node {
    Node_T node;
    struct PS_Node *next;
};

/* inline static Hash_Value_T */
/* hash(Seq_Len_T *key, Seq_Num_T key_len, uint16_t step, uint32_t tab_size) */
/* { */
/*     Hash_Value_T value = 0; */
/*     /\* 每隔step，选取一个元素参与运算 *\/ */
/*     for (Pat_Len_T *end = key + key_len; key < end; key += step) */
/* 	value = (value << 6) ^ *key; */
    
/*     return value % tab_size; */
/* } */

inline static Hash_Value_T
hash(Seq_Len_T *key, Seq_Num_T key_len, uint16_t step, uint32_t tab_size)
{
    Hash_Value_T value = 50u;
    /* 每隔step，选取一个元素参与运算 */
    for (Pat_Len_T *end = key + key_len; key < end; key += step)
	value ^= (value << 8) + (value >> 2) + *key;
    
    return value % tab_size;
}

/* 当两个key不相等返回true；否则，返回false */
static bool key_not_equal(Seq_Len_T *key1, Seq_Len_T *key2, Seq_Num_T key_len)
{
    while (key_len && *key1 == *key2)
	key_len--, key1++, key2++;
    
    return key_len;
}

/* 如果key存在，则返回对应节点的指针；否则，新建节点，并返回该节点 */
static Node_T
find_and_insert(Seq_Len_T *key, Graph_T graph, int32_t step, bool *key_exist_p)
{
    Seq_Num_T key_len = graph->key_len;
    Hash_Value_T hash_value = hash(key, key_len, step, graph->tab_size);
    Node_T node;

    for (node = graph->tab[hash_value];
	 node && key_not_equal(node->key, key, key_len);
	 node = node->next)
	;

    if (node) /*找到 */
	*key_exist_p = true;
    else { /* 没找到key，则新建节点，并插入到链表头 */
	*key_exist_p = false;
	node = VNEW0(node, key_len, Seq_Len_T);
	memcpy(node->key, key, key_len * sizeof (*key));
	node->next = graph->tab[hash_value];
	graph->tab[hash_value] = node;
	graph->node_num++;
    }

    return node;
}

/* 将node加入到相应节点的前驱后继集中 */
inline static void add_pre_suc(PS_Node_T *ps_node_p, Node_T node)
{
    PS_Node_T ps_node; /* 新前驱后继集节点 */
    NEW(ps_node);

    ps_node->node = node;
    ps_node->next = *ps_node_p;
    *ps_node_p = ps_node;
}

/* 清除node的所有前驱节点 */
inline static void clear_pres(Node_T node)
{
    for (PS_Node_T p_node = node->precursors, next;
	 p_node; p_node = next) {
	next = p_node->next;
	FREE(p_node);
    }
    
    node->precursors = NULL;
}

/* 更新后继节点，及其后续节点 */
inline static void update_suc_nodes(Node_T node, Node_T suc_node)
{
    if (node->path_len + 1 == suc_node->path_len)
	add_pre_suc(&suc_node->precursors, node);
    else if (node->path_len + 1 > suc_node->path_len) {
	clear_pres(suc_node);
	add_pre_suc(&suc_node->precursors, node);
	suc_node->path_len = node->path_len + 1;
	/* 递归地更新suc_node的所有后继节点 */
	for (PS_Node_T s_node = suc_node->successors;
	     s_node; s_node = s_node->next)
	    update_suc_nodes(suc_node, s_node->node);
    }
}

/* 打印每个节点的key向量 */
static void print_key(Node_T node, Seq_Num_T key_len)
{
    putchar('\n'); putchar('(');
    Seq_Num_T i;
    for (i = 0; i < key_len - 1; i++)
	printf("%d, ", node->key[i]);
    printf("%d)", node->key[i]);
}

/* 打印某个节点的前驱/后继节点集 */
static void print_pre_suc(PS_Node_T ps_node, Seq_Num_T key_len)
{
    while (ps_node) {
	print_key(ps_node->node, key_len);
	ps_node = ps_node->next;
    }
}

/* 用于打印图中每个节点及其前驱和后继 */
void print_nodes(Graph_T graph)
{
    Node_T node;

    for (uint32_t i = 0; i < graph->tab_size; i++) {
	if ((node = graph->tab[i]))
	    while (node) {
		printf("\n\nNode:");
		print_key(node, graph->key_len);
		printf("\nPrcursors:");
		print_pre_suc(node->precursors, graph->key_len);
		printf("\nSuccessors:");
		print_pre_suc(node->successors, graph->key_len);
		node = node->next;
	    }
    }
}

/* 计算哈希链表的长度 */
static int64_t get_hash_list_len(Node_T list_head)
{
    int64_t list_len = 0;

    while (list_head) {
	list_len++;
	list_head = list_head->next;
    }

    return list_len;
}

/* 打印图的统计信息，这里图是哈希表 */
void print_graph_info(Graph_T graph)
{
    printf("\nNodes: %ld, Redundants: %ld\n",
	   graph->node_num, graph->redundants);

   /* 空槽数量 */
    uint32_t empty_slot = 0;
    for (uint32_t i = 0; i < graph->tab_size; i++)
	if (graph->tab[i] == NULL)
	    empty_slot++;
   
    /*平均链表长 */
    double mean = (double) graph->node_num /
	(graph->tab_size - empty_slot);
    
    /* 最大链长，最小链长，链长标准差,链长分布 */
    uint32_t max = 0;
    uint32_t min = UINT32_MAX;
    double sd = 0.0;
    uint32_t len_tab[10000] = {0}; /*假设链长最大为10000  */
    
    for (uint32_t i = 0; i < graph->tab_size; i++) {
	uint32_t len;
	if ((len = get_hash_list_len((graph->tab)[i])) == 0)
	    continue;
	len_tab[len]++;
	if (len > max)
	    max = len;
	else if (len < min)
	    min = len;
	sd += pow((double) len - mean, 2.0);
    }
    
    sd = sqrt(sd / (graph->tab_size - empty_slot));
    
    printf("\nMin list len: %ld\n"
	   "Max list len: %ld\n"
	   "Mean list len: %.2f\n"
	   "SD: %.2f\n"
	   "Empty slots: %ld\n",
	   min, max, mean, sd, empty_slot);
    /* 打印链长分布 */
    printf("List len distribution:\n");
    
    
}

/* 从终止节点回溯，寻找所有MLCS*/
static void back_tracing(Node_T node, char *first_seq, char *mlcs_buf, List_T mlcs_list)
{
    if (node->path_len == 0) /* 回溯到源节点 */
	list_push_front(mlcs_list, str_dup(mlcs_buf));
    else {
	Pat_Len_T i = node->key[0];
	if (i) mlcs_buf[node->path_len-1] = first_seq[i-1];
	for (PS_Node_T p_node = node->precursors;
	     p_node; p_node = p_node->next)
	    back_tracing(p_node->node, first_seq, mlcs_buf, mlcs_list);
    }
}

/* 构建mlcs链表 */
MLCS_Set_T collect_mlcs(Graph_T graph, char *first_seq)
{
    /* 每个mlcs的长度 */
   Seq_Len_T len = graph->dest_node->path_len - 1;

    /* 收集到的mlcs保存在list中 */
    MLCS_Set_T mlcs_set;
    NEW(mlcs_set);
    mlcs_set->mlcs_list = list_new(NULL);
    mlcs_set->len = len;
    
    /* 存放每条产生的MLCS */
    char mlcs_buf[len+1];
    memset(mlcs_buf, 0, len+1);

    /* 从终止节点开始回溯,每个收集到的mlcs保存在链表里 */
    back_tracing(graph->dest_node, first_seq, mlcs_buf, mlcs_set->mlcs_list);

    return mlcs_set;
}

/* 构造前驱后继图,返回终止节点。当前为哈希表实现 */
Graph_T build_graph(Suc_Tabs_T suc_tabs)
{
    Seq_Num_T key_len = suc_tabs->tab_num;
    int32_t step = 1; /* 每隔step，取一个元素参与hash计算 */
    
    Graph_T graph = VNEW0(graph, HASH_TAB_SIZE, Node_T);
    graph->tab_size = HASH_TAB_SIZE;
    graph->key_len = key_len;
    
    /* 创建源节点和终止节点,并不放入图中 */
    Node_T source_node, dest_node;
    VNEW0(source_node, key_len, Seq_Len_T);
    VNEW0(dest_node, key_len, Seq_Len_T);
    graph->dest_node = dest_node;
    
    /* 源节点放入队列 */
    Queue_T q = queue_new();
    queue_push(q, source_node);

    /* 除源节点和终止节点外，其余节点均放入图（哈希表）中 */
    while (!queue_empty(q)) {
	Node_T node = queue_pop(q);
	bool no_successors = true;
	Seq_Len_T key[key_len];
	/* 依次产生node的后继节点所对应的key */
	for (Char_T ch = 0; ch < SIGMA; ch++) {
	    if (!get_suc_key(suc_tabs, node->key, ch, key))
		continue;
	    no_successors = false; /* 至少有一个后继节点 */
	    bool key_exist = false;
	    Node_T suc_node = find_and_insert(key, graph, step, &key_exist);
#if PRE
	    /* 无论suc_node是否存在，都将其加入node的后继集 */
	    add_pre_suc(&node->successors, suc_node);
#endif 
	    if (!key_exist) {	/*suc_node不存在 */
		graph->node_num++;
#if PRE
		add_pre_suc(&suc_node->precursors, node);
		suc_node->path_len = node->path_len + 1;
#endif 		
		queue_push(q, suc_node);
	    }
#if PRE
	    else {/* 后继节点存在 */
		update_suc_nodes(node, suc_node);
		graph->redundants++;
	    }
#endif
	}
	
	/* 如果没有任何后继，则将终止节点作为其后继 */
	if (no_successors) {
	    add_pre_suc(&node->successors, dest_node);
#if PRE
	    update_suc_nodes(node, dest_node);
#endif 
	}
    }
    
    return graph;
}
