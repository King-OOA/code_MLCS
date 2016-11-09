#include <assert.h>
#include "graph.h"
#define HASH_TAB_SIZE 20000000 /* 哈希表大小 */
#define PRE 1

typedef uint64_t Hash_Value_T;

uint64_t max_nodes_num;
uint64_t graph_nodes_num;
uint64_t freed_list_nodes_num;
uint64_t freed_nodes_num;
extern Seq_Num_T key_len;

#define MAX_STAGED_LIST_NODE 10000 /* 最大暂存list_node数量 */

static List_Node_T cur_nodes;
static List_Node_T new_nodes;
static List_Node_T old_nodes;
static List_Node_T *old_nodes_tail_p = &old_nodes;
static List_Node_T deg0_nodes; /* 入度为0的节点 */

/* 存放被free掉的list_node */
static List_T freed_list_nodes;

/* 分配指向node的list_node */
static List_Node_T alloc_list_node(Node_T node)
{
    List_Node_T list_node;

    if (!list_empty(freed_list_nodes)) /* 先在空闲链表里面分配 */
	list_node = list_pop_front(freed_list_nodes);
    else 
	NEW(list_node); /*从系统中分配 */

    list_node->node = node; /* next域在使用中赋值 */
    
    return list_node;
}

/* 释放list_node (Check!)*/
static void free_list_node(List_Node_T list_node)
{
    freed_list_nodes_num++;
 
    if (list_size(freed_list_nodes) > MAX_STAGED_LIST_NODE)
	FREE(list_node); /* 超过容量则让系统释放 */
    else /* 否则暂存 */
	list_push_front(freed_list_nodes, list_node);
}

#define MAX_STAGED_NODE 10000 /* 最大暂存node数量 */

/* 存放被free掉的node */
static List_T freed_nodes;

/* 分配node (Check!) */
static Node_T alloc_node(Seq_Len_T *key)
{
    if (++graph_nodes_num > max_nodes_num)
    	max_nodes_num = graph_nodes_num;

    Node_T node;
  
    /* 先在空闲链表里面分配 */
    if (!list_empty(freed_nodes)) {
	node = list_pop_front(freed_nodes);
	memset(node, 0, sizeof(struct Node));
    } else 
	VNEW0(node, key_len, Seq_Len_T);

    memcpy(node->key, key, key_len * sizeof (*key));

    return node;
}

/* 释放node (Check!) */
static void free_node(Node_T node)
{
    graph_nodes_num--;
    freed_nodes_num++;
 
    if (list_size(freed_nodes) > MAX_STAGED_NODE)
	FREE(node); /* 超过容量则让系统释放 */
    else /* 否则暂存 */
	list_push_front(freed_nodes, node);
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
find_and_insert(char ch, Seq_Len_T *key, Graph_T graph, int32_t step, bool *key_exist_p)
{
    Hash_Value_T hash_value = hash(key, key_len, step, graph->tab_size);
    Node_T node, *hash_tab = graph->tabs[(unsigned char) ch];

    for (node = hash_tab[hash_value];
	 node && key_not_equal(node->key, key, key_len);
	 node = node->next)
	;

    if (node) /*找到 */
	*key_exist_p = true;
    else { /* 没找到key, 则新建节点, 并插入到哈希链表头 */
	*key_exist_p = false;
	node = alloc_node(key);
	Node_T first_node = hash_tab[hash_value];
	node->next = first_node;
	if (first_node) first_node->prev_next = &node->next;
	hash_tab[hash_value] = node;
	node->prev_next = hash_tab + hash_value;
    }

    return node;
}

inline static void add_to_list(List_Node_T *list_node_p, Node_T node)
{
    List_Node_T list_node = alloc_list_node(node);

    list_node->next = *list_node_p;
    *list_node_p = list_node;
}

void remove_deg0_nodes(List_Node_T *list_node_p)
{
    List_Node_T list_node;

    while ((list_node = *list_node_p) != NULL) {
	if (list_node->node->indegree == 0) {
	    *list_node_p = list_node->next;
	    list_node->next = deg0_nodes;
	    deg0_nodes = list_node;
	}
	if (*list_node_p)
	    list_node_p = &(*list_node_p)->next;
    }
    /* 最后一个入度非0的节点的next域 */
    old_nodes_tail_p = list_node_p;
}

static char *first_sequence;

/* 将node的prefixes,推送到suc_node中,
   replace表示是否替换suc_node中的prefixes */
void push_prefix(Node_T node, Node_T suc_node, bool replace)
{
    Seq_Len_T pref_len = node->pref_len;
    MLCS_Num_T pref_num = node->pref_num;
    uint32_t suc_pref_size = suc_node->pref_len * suc_node->pref_num;
    
    uint32_t new_pref_size = replace ? (pref_len + 1) * pref_num :
	(pref_len + 1) * pref_num + suc_pref_size;

    suc_node->prefixes = realloc(suc_node->prefixes, new_pref_size);
    char *source = node->prefixes;
    char *dest = replace ? suc_node->prefixes :
	suc_node->prefixes + suc_pref_size;
    char ch = first_sequence[suc_node->key[0]-1];
    
    while (pref_num--) {
	memcpy(dest, source, pref_len);
	dest += pref_len;
	*dest++ = ch;
	source += pref_len;
    }
}

void delete_deg0_nodes(List_Node_T *deg0_nodes_p)
{
    List_Node_T next_list_node;
    
    for (List_Node_T list_node = *deg0_nodes_p;
	 list_node; list_node = next_list_node)
    {
	next_list_node = list_node->next;
	Node_T node = list_node->node;
	for (int8_t i = 0; i < node->suc_num; i++) {
	    Node_T suc_node = node->successors[i];
	    suc_node->indegree--;
	    
	    if (node->pref_len + 1 == suc_node->pref_len) {
		push_prefix(node, suc_node, false);
		suc_node->pref_num += node->pref_num;
	    } else if (node->pref_len + 1 > suc_node->pref_len) {
		push_prefix(node, suc_node, true);
		suc_node->pref_len = node->pref_len + 1;
		suc_node->pref_num = node->pref_num;
	    }
	}
	/* 释放后继指针数组 */
	FREE(node->successors);
	/* 释放prefixes域 */
	FREE(node->prefixes);
	/* 先从哈希表中删除node */
	*(node->prev_next) = node->next;
	if (node->next) node->next->prev_next = node->prev_next;
	/* 再释放node */
	free_node(node);
	free_list_node(list_node);
    }

    *deg0_nodes_p = NULL;
}

Node_T create_source_node(Graph_T graph)
{
    Node_T source_node;
    
    VNEW(source_node, key_len, Seq_Len_T);
    source_node->suc_num = 0;
    source_node->pref_len = 0;
    source_node->pref_num = 1;
    source_node->indegree = 0;
    source_node->prefixes = CALLOC(1, char);    /* 任一非Null地址 */
    source_node->successors = NULL;
    /* 插入到第一个哈希表第0项 */
    source_node->next = NULL;
    source_node->prev_next = graph->tabs[0];
    *(graph->tabs[0]) = source_node;

    return source_node;
}

#define add_successors(node, suc_node)					\
  { (node)->successors =						\
      realloc((node)->successors, ++(node)->suc_num * sizeof(Node_T));	\
    (node)->successors[(node)->suc_num-1] = (suc_node);			\
  }

/* 逐层构造图,哈希表实现 */
Node_T build_graph(Suc_Tabs_T suc_tabs)
{
    key_len = suc_tabs->tab_num;
    first_sequence = (suc_tabs->tabs[0]).seq;
    Seq_Len_T first_seq_len = strlen(first_sequence);
    first_sequence[first_seq_len] = '\n';
    
    int32_t step = 1; /* 每隔step, 取一个元素参与hash计算 */
    
    /* 创建图(哈希表) */
    Graph_T graph; NEW(graph);
    for (int32_t i = 0; i < SIGMA; i++)
	graph->tabs[i] = CALLOC(HASH_TAB_SIZE, Node_T); /* 初始化为空 */
    graph->tab_size = HASH_TAB_SIZE;

    /* 创建源节点,放入图中 */
    Node_T source_node, end_node;
    source_node = create_source_node(graph);
    /* 创建终止节点,不放入图中,终止节点对应字符'\n' */
    VNEW0(end_node, key_len, Seq_Len_T);
    end_node->key[0] = first_seq_len+1;
    
    /* 存放被free掉的node和list_node */
    freed_list_nodes = list_new(NULL);
    freed_nodes = list_new(NULL);

    /* 源节点放入新节点链表 */
    add_to_list(&new_nodes, source_node);
    
    while (new_nodes || old_nodes) {
	cur_nodes = new_nodes;
	new_nodes = NULL;
	/* 扩展所有待扩展节点 */
	for (List_Node_T list_node = cur_nodes;
	     list_node != NULL; list_node = list_node->next)
	{
	    Node_T node = list_node->node; /* 当前待扩展节点 */
	    bool no_successors = true; /* 标记node是否有后继节点 */
	    Seq_Len_T key[key_len];	 /* 保存生成的node的后继节点的key */
	    /* 依次产生node的后继节点所对应的key */
	    for (Char_T ch = 0; ch < SIGMA; ch++) {
		if (!get_suc_key(suc_tabs, node->key, ch, key))
		    continue; /* 当前字符没有对应的key */
		no_successors = false; /* 至少有一个后继节点 */
		bool key_exist; /* 该key是否已经存在于图中 */
		Node_T suc_node = find_and_insert(ch, key, graph, step, &key_exist);

		add_successors(node, suc_node);
		suc_node->indegree++;
		if (!key_exist)  /*若后继节点不存在,则加入到新节点链表中 */
		    add_to_list(&new_nodes, suc_node);
	    }

	    if (no_successors)
	      add_successors(node, end_node);
	}

	*old_nodes_tail_p = cur_nodes;	/* 连接old_nodes_list和cur_nodes_list */
	remove_deg0_nodes(&old_nodes); /* 将入度为0的节点移入deg0_nodes */
	delete_deg0_nodes(&deg0_nodes);	/* 删除入度为0的节点,并做相应的处理 */
    }
    
    return end_node;
}
