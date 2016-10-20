#include "suc_tabs.h"
#include "graph.h"
#include "print.h"

extern uint64_t max_nodes_num;
extern uint64_t freed_nodes_num;

Seq_Num_T key_len;

int main(int32_t argc, char **argv)
{
    /* 读取序列,第一个参数表示序列文件 */
    Patset_T sequences = patset_new(argv[1]);
    
    printf("%d\n", sizeof(struct Node));

    /* 构造后继表数组 */
    printf("\nConstructing successor tables ...\n");
    clock_t start = clock();
    Suc_Tabs_T suc_tabs = build_suc_tabs(sequences);
    printf("Done!\n%f\n", (double) (clock() - start) / CLOCKS_PER_SEC);
    
    /* 构造前驱后继图 */
    printf("\nConstructing graph ...\n");
    start = clock();
    Node_T end_node = build_graph(suc_tabs);
    printf("Done!\n%f\n", (double) (clock() - start) / CLOCKS_PER_SEC);
    printf("Num: %d, Len: %d\n", end_node->pref_num, end_node->pref_len-1);
    printf("Max node num: %lu, freed node num: %lu\n", max_nodes_num, freed_nodes_num);
    
/* 输出图的基本信息.和图的具体实现有关 */
//    print_graph_info(graph);
//    print_nodes(graph);

    /* /\* 打印所有mlcs *\/ */
    /* bool output = false; /\*是否打印每个串  *\/ */
    /* if (argc == 3 && strcmp(argv[2], "-o") == 0) */
    bool output = true;

    if (output) {
      uint32_t mlcs_size = end_node->pref_len * end_node->pref_num;
      end_node->prefixes =
	realloc(end_node->prefixes, mlcs_size + 1);
      end_node->prefixes[mlcs_size] = '\0';
      puts(end_node->prefixes);
    }

    FREE(end_node->prefixes); FREE(end_node);
      
}
