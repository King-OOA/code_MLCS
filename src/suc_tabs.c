#include "suc_tabs.h"

/*对指定序列构造后继表 */
static void build_each_tab(void **seq_p, void *suc_tab_p)
{
  /* 不同字符所对应后继表的行号 */
  static char char_to_row[] = {['A'] = 0, ['C'] = 1, ['G'] = 2, ['T'] = 3};

  Suc_Tab_T suc_tab =  *((Suc_Tab_T *) suc_tab_p);
  char *seq = (char *) (*seq_p);

  suc_tab->seq = seq;
  Seq_Len_T seq_len = strlen(seq);
  suc_tab->seq_len = seq_len;
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

Suc_Tabs_T build_suc_tabs(Patset_T sequences)
{
    Seq_Num_T seq_num = sequences->pat_num;

    /* 创建后继表数组 */
    Suc_Tabs_T suc_tabs;
    VNEW0(suc_tabs, seq_num, struct Suc_Tab);
    suc_tabs->tab_num = seq_num;

    /* 跟踪当前待建后继表 */
    Suc_Tab_T cur_tab = suc_tabs->tabs;
    /* 为每一个序列构造后继表 */
    list_traverse(sequences->pat_list, build_each_tab, &cur_tab);

    return suc_tabs;
}

/* 根据当前key和对应字符ch，产生后继suc_key*/
bool get_suc_key(Suc_Tabs_T suc_tabs,
		 Seq_Len_T *key, Char_T ch, Seq_Len_T *suc_key)
{
    Suc_Tab_T suc_tab = suc_tabs->tabs;
    Suc_Tab_T end = suc_tab + suc_tabs->tab_num;
    Seq_Len_T v;
    
    /* v不为0 */
    while (suc_tab < end &&
	   (v = ((Seq_Len_T (*)[suc_tab->seq_len+1]) suc_tab->tab)[ch][*key]))
    {	*suc_key++ = v;
	suc_tab++, key++;
    }
    
    return suc_tab == end ? true : false;
}

/* 打印后继表 */
void print_suc_tabs(Suc_Tabs_T suc_tabs)
{
    for (Suc_Tab_T suc_tab = suc_tabs->tabs,
	     end = suc_tab + suc_tabs->tab_num;
	 suc_tab < end; suc_tab++)
    {
	printf("\n %s\n",(suc_tab->seq));
	Pat_Len_T (*tab)[suc_tab->seq_len+1] = suc_tab->tab;
	for (Char_T ch = 0; ch < SIGMA; ch++) {
	    for (Seq_Len_T col = 0; col < suc_tab->seq_len+1; col++)
		printf("%2d ", tab[ch][col]);
	    putchar('\n');
	}
    }
}

