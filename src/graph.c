#include <assert.h>
#include "graph.h"

#define HASH_TAB_SIZE 100000000 /* 哈希表大小 */
#define PRE 1

typedef uint64_t Hash_Value_T;

uint64_t max_nodes;
uint64_t graph_nodes;
uint64_t freed_ps_nodes;
uint64_t freed_nodes;

#define MAX_STAGED_PS 10000 /* 最大暂存ps_node数量 */

/* 存放被free掉的ps_node */
static List_T free_ps_nodes_list;

/* 分配ps_node (Check!) */
static PS_Node_T alloc_ps_node(void)
{
    if (!list_empty(free_ps_nodes_list)) /* 先在空闲链表里面分配 */
	return list_pop_front(free_ps_nodes_list);

    PS_Node_T ps_node;
    NEW(ps_node); /* 在add_ps_node中初始化 */

    return ps_node;
}

/* 释放ps_node (Check!)*/
static void free_ps_node(PS_Node_T ps_node)
{
    freed_ps_nodes++;
 
    if (list_size(free_ps_nodes_list) > MAX_STAGED_PS)
	FREE(ps_node); /* 超过容量则让系统释放 */
    else /* 否则暂存 */
	list_push_front(free_ps_nodes_list, ps_node);
}

#define MAX_STAGED_NODE 10000 /* 最大暂存node数量 */

/* 存放被free掉的node */
static List_T free_nodes_list;

/* 分配node (Check!) */
static Node_T alloc_node(Seq_Num_T key_len)
{
    if (++graph_nodes > max_nodes)
	max_nodes = graph_nodes;
  
    /* 先在空闲链表里面分配 */
    if (!list_empty(free_nodes_list)) 
	return list_pop_front(free_nodes_list);

    Node_T node; /* 在find_and_insert中初始化 */
    VNEW(node, key_len, Seq_Len_T);

    return node;
}

/* 释放node (Check!) */
static void free_node(Node_T node)
{
    graph_nodes--;
    freed_nodes++;
 
    if (list_size(free_nodes_list) > MAX_STAGED_NODE)
	FREE(node); /* 超过容量则让系统释放 */
    else /* 否则暂存 */
	list_push_front(free_nodes_list, node);
}

inline static Hash_Value_T
hash(Seq_Len_T *key, Seq_Num_T key_len, uint16_t step, uint32_t tab_size)
{
    Hash_Value_T value = 0;
    uint32_t b = 378551, a = 63689;

    /* 每隔step，选取一个元素参与运算 */
    for (Pat_Len_T *end = key + key_len; key < end; key += step) {
	value = value * a + *key;
	a = a * b;
    }
    
    return value % tab_size;
}

static bool key_not_equal(Seq_Len_T *key1, Seq_Len_T *key2, Seq_Num_T key_len)
{
    while (key_len && *key1 == *key2)
	key_len--, key1++, key2++;
    
    return key_len;
}

/* 如果key存在, 则返回对应节点的指针; 否则, 新建节点, 并返回该节点 */
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
    else { /* 没找到key, 则新建节点, 并插入到哈希链表头 */
	*key_exist_p = false;
	node = alloc_node(key_len);
	memcpy(node->key, key, key_len * sizeof (*key));
	Node_T first_node = graph->tab[hash_value];
	node->next = first_node;
	if (first_node) first_node->prev_next = &node->next;
	graph->tab[hash_value] = node;
	node->prev_next = graph->tab + hash_value;
	node->precursors = node->successors = NULL;
    }

    return node;
}

/* 将node插入到相应节点的前驱/后继链表首 (Check!)*/
inline static void add_ps_node(PS_Node_T *ps_node_p, Node_T node)
{
    PS_Node_T ps_node = alloc_ps_node(); /* 新前驱后继集节点 */

    ps_node->node = node;
    ps_node->next = *ps_node_p;
    *ps_node_p = ps_node;
}

/* 在前驱/后继集中删除node (Check!)*/
inline static void del_ps_node(PS_Node_T *ps_node_p, Node_T node)
{
    PS_Node_T ps_node;

    for (ps_node = *ps_node_p; ps_node->node != node; ps_node = *ps_node_p)
	ps_node_p = &ps_node->next;

    *ps_node_p = ps_node->next;

    free_ps_node(ps_node);
}

/* 删除末端节点node及其前驱 */
static inline void backward_delete(Node_T node)
{
    PS_Node_T next_p_node;

    for (PS_Node_T p_node = node->precursors;
	 p_node; p_node = next_p_node)
    {
	Node_T pre_node = p_node->node;
	next_p_node = p_node->next;
	free_ps_node(p_node);

	del_ps_node(&pre_node->successors, node);
	if (pre_node->successors == NULL)
	    backward_delete(pre_node);
    }
    
    *(node->prev_next) = node->next;
    if (node->next) node->next->prev_next = node->prev_next;
    free_node(node);
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
    Seq_Len_T len = graph->end_node->path_len - 1;

    /* 收集到的mlcs保存在list中 */
    MLCS_Set_T mlcs_set;
    NEW(mlcs_set);
    mlcs_set->mlcs_list = list_new(NULL);
    mlcs_set->len = len;
    
    /* 存放每条产生的MLCS */
    char mlcs_buf[len+1];
    memset(mlcs_buf, 0, len+1);

    /* 从终止节点开始回溯,每个收集到的mlcs保存在链表里 */
    back_tracing(graph->end_node, first_seq, mlcs_buf, mlcs_set->mlcs_list);

    return mlcs_set;
}

/* 删除从node到suc_node之外,其它到suc_node的路径 */
inline static void del_other_paths(Node_T node, Node_T suc_node)
{
    PS_Node_T next_p_node;
    /* 遍历suc_node的所有前驱节点 */
    for (PS_Node_T p_node = suc_node->precursors;
	 p_node; p_node = next_p_node)
    {
	Node_T pre_node = p_node->node;
	next_p_node = p_node->next;
	/* 如果前驱节点是node,则跳过 */
	if (pre_node == node) {
	    suc_node->precursors = p_node;
	    p_node->next = NULL;
	    continue;
	}
	free_ps_node(p_node);
        /* 从pre_node的后继集中删除suc_node */
	del_ps_node(&pre_node->successors, suc_node);
	if (pre_node->successors == NULL) /* 末端节点 */
	    backward_delete(pre_node);
    }

    /* 此时suc_node的前驱节点只有node */
    suc_node->path_len = node->path_len + 1;

    /* 递归地更新suc_node的所有后继节点 */
    for (PS_Node_T s_node = suc_node->successors;
	 s_node; s_node = s_node->next)
	del_other_paths(suc_node, s_node->node);
}

/* /\* 建立节点间的前驱后继关系 (Check!)*\/ */
/* void connect(Node_T node, Node_T suc_node) */
/* { */
/*     add_ps_node(&node->successors, suc_node); */
/*     add_ps_node(&suc_node->precursors, node); */
/* } */


/* 建立节点间的前驱后继关系 */
#define connect(node, suc_node)				\
    {							\
      add_ps_node(&((node)->successors), (suc_node));	\
      add_ps_node(&((suc_node)->precursors), (node));	\
    }


/* 依据路径长度,做出不同操作 */
inline static void try_a_new_path(Node_T node, Node_T suc_node)
{
    if (node->path_len + 1 == suc_node->path_len)
	connect(node, suc_node)	/*connect是宏 此处无';' */
    else if (node->path_len + 1 > suc_node->path_len) {
	connect(node, suc_node);
	del_other_paths(node, suc_node);
    }
}

/* 构造前驱后继图, 哈希表实现 */
Graph_T build_graph(Suc_Tabs_T suc_tabs)
{
    Seq_Num_T key_len = suc_tabs->tab_num;
    int32_t step = 1; /* 每隔step, 取一个元素参与hash计算 */
    
    /* 创建图(哈希表) */
    Graph_T graph = VNEW0(graph, HASH_TAB_SIZE, Node_T); /* 初始化为空 */
    graph->tab_size = HASH_TAB_SIZE;
    graph->key_len = key_len;
   
    /* 创建源节点和终止节点,并不放入图中 */
    Node_T source_node, end_node;
    VNEW0(source_node, key_len, Seq_Len_T);
    VNEW0(end_node, key_len, Seq_Len_T);
    graph->end_node = end_node;
    
    /* 存放被free掉的node和ps_node */
    free_ps_nodes_list = list_new(NULL);
    free_nodes_list = list_new(NULL);

    /* 源节点放入队列 */
    Queue_T q = queue_new();
    queue_push(q, source_node);

    /* 除源节点和终止节点外,其余节点均放入图（哈希表）中 */
    while (!queue_empty(q)) {
	Node_T node = queue_pop(q); /* 当前待扩展节点 */
	bool no_successors = true; /* 标记node是否有后继节点 */
	Seq_Len_T key[key_len];	 /* 保存生成的node的后继节点的key */
	/* 依次产生node的后继节点所对应的key */
	for (Char_T ch = 0; ch < SIGMA; ch++) {
	    if (!get_suc_key(suc_tabs, node->key, ch, key))
		continue; /* 当前字符没有对应的key */
	    no_successors = false; /* 至少有一个后继节点 */
	    bool key_exist; /* 该key是否已经存在于图中 */
	    Node_T suc_node = find_and_insert(key, graph, step, &key_exist);
	    
	    if (!key_exist) { /*若后继节点不存在,则suc_node为新建节点 */
		connect(node, suc_node);
		suc_node->path_len = node->path_len + 1;
		queue_push(q, suc_node);
	    } else /* 后继节点存在 */
		try_a_new_path(node, suc_node);
	}

	/* 如果没有任何后继，则将终止节点作为其后继 */
	if (no_successors) 
	    try_a_new_path(node, end_node);
	/* 如果仍然没有任何后继，则删除该节点,同时前向删除 */
	if (node->successors == NULL)
	    backward_delete(node);
    }
    
    return graph;
}



