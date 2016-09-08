#include "suc_tabs.h"
#include "graph.h"


static void print_str(void **mlcs_p, void *arg)
{
    printf("%s\n", (char *) (*mlcs_p));
}

static void print_mlcs(MLCS_Set_T mlcs_set)
{
    /* 打印MLCS */
    printf("\nThere are %d MLCS of length %d: \n\n",
    	   list_size(mlcs_set->mlcs_list), mlcs_set->len);
    list_traverse(mlcs_set->mlcs_list, print_str, NULL);
}

int main(int32_t argc, char **argv)
{
    /* 读取序列,第一个参数表示序列文件 */
    Patset_T sequences = patset_new(argv[1]);

    /* 构造后继表数组 */
    printf("\nConstructing successor tables ...\n");
    clock_t start = clock();
    Suc_Tabs_T suc_tabs = build_suc_tabs(sequences);
    printf("Done!\n%f\n", (double) (clock() - start) / CLOCKS_PER_SEC);
    
    /* 构造前驱后继图 */
    printf("\nConstructing graph ...\n");
    start = clock();
    Graph_T graph = build_graph(suc_tabs);
    printf("Done!\n%f\n", (double) (clock() - start) / CLOCKS_PER_SEC);
    /* 输出图的基本信息.和图的具体实现有关 */
    print_graph_info(graph);
    //print_nodes(graph);

    /* 收集MLCS */
    printf("\n\nCollecting MLCS ...\n");
    start = clock();
    MLCS_Set_T mlcs_set = collect_mlcs(graph, suc_tabs->tabs[0].seq);
    printf("Done!\n%f\n", (double) (clock() - start) / CLOCKS_PER_SEC);

    /* 打印mlcs信息 */
    print_mlcs(mlcs_set);
}
