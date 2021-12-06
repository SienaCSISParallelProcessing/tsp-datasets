/*
  Header file for a singly-linked list structure that holds int values

  Jim Teresco, Siena College, CSIS 330, Spring 2012
  The College of Saint Rose, CSC 381, Fall 2013

  Initial implementation:
  Fri Feb  3 11:02:04 EST 2012

  Modification History
  
  2013-11-10 JDT  Added sll_visit_all
*/

typedef struct sll_node {
  int value;
  struct sll_node *next;
} sll_node;

typedef struct sll {
  struct sll_node *head;
} sll;

extern sll *create_sll();
extern void sll_add_to_tail(sll *q, int value);
extern void sll_add_to_head(sll *q, int value);
extern int sll_get(sll *q, int position);
extern int sll_get_head(sll *q);
extern int sll_get_tail(sll *q);
extern int sll_remove_from_head(sll *q);
extern int sll_remove_from_tail(sll *q);
extern void sll_print_contents(sll *q);
extern int sll_isempty(sll *q);
extern void sll_visit_all(sll *q, void (*callback)(int, void *), void *call_data);
extern void sll_destroy(sll *q);
extern void sll_clear(sll *q);
extern int *sll_to_array(sll *q);
