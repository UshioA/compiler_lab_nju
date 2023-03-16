#ifndef __CMM_LIST_H__
#define __CMM_LIST_H__

typedef struct list_entry {
  struct list_entry *next, *prev;
} list_entry;
int list_empty(list_entry *head);

void list_init(list_entry *h);

/*
 * General helper function for add a node into a list knowing last and next
 */
void __list_add(list_entry *prev, list_entry *next, list_entry *_new);

list_entry *list_head(list_entry *entry);
list_entry *list_tail(list_entry *entry);

/**
 * @brief 头插法
 *
 * @param head list head to be inserted
 * @param _new _new node that will be inserted to head
 */
void list_add(list_entry *_new, list_entry *head);

/**
 * @brief 尾插法
 *
 * @param head: list head to be inserted
 * @param _new: _new node that will be inserted to head
 */
void list_add_tail(list_entry *_new, list_entry *head);

/**
 * @brief General helper function for deleting a node knowing last and next
 */
void __list_del(list_entry *prev, list_entry *next);

/**
 * @brief Delete entry from a list
 *
 * @param entry : trivial.
 */
void list_del(list_entry *entry);

/**
 * @brief 从list中删去一个节点并初始化它
 *
 * @param entry : trivial.
 */
void list_del_init(list_entry *entry);

/**
 * @brief 把entry摘下来，然后头插入head
 *
 * @param entry
 * @param head
 */
void list_move(list_entry *entry, list_entry *head);

/**
 * @brief 把entry摘下来，然后尾插入head
 *
 * @param entry
 * @param head
 */
void list_move_tail(list_entry *entry, list_entry *head);

void __list_splice(struct list_entry *list, struct list_entry *head);

/**
 * @brief list_splice - join two lists
 * @param list the _new list to add.
 * @param head the place to add it in the first list.
 */
void list_splice(list_entry *list, list_entry *head);

void list_splice_init(list_entry *list, list_entry *head);

/**
 * @brief Marcos to compute address of struct from the address of list_entry and
 * the name of this entry in that struct.
 * @example : (page*) le2(page, list_entry, link)
 * will get a page* pointed at a struct page that contains list_entry
 */
#define le2(type_name, le, member_name)                                        \
  ((type_name *)le2struct((le), type_name, member_name))

#define __offset(type_name, member_name)                                       \
  ((size_t)(&((type_name *)0)->member_name))

#define le2struct(le, type_name, member_name)                                  \
  ((size_t)((char *)(le)) - __offset(type_name, member_name))

#define list_for_each(pos, head)                                               \
  for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_reverse(pos, head)                                            \
  for (pos = (head)->last; pos != (head); pos = pos->last)

#define list_for_each_safe(pos, head, n)                                       \
  for (pos = (head)->next, n = pos->next; pos != (head); pos = n, n = n->next)

#define list_for_reverse_safe(pos, head, n)                                    \
  for (pos = (head)->last, n = pos->last; pos != (head); pos = n, n = n->last)

/**
 * @param pos : pointer to the struct counter
 * @param head : head of list
 * @param member : name of list in this struct
 */
#define list_for_each_item(pos, head, member)                                  \
  for (pos = le2(__typeof__(*pos), (head)->next, member);                      \
       &pos->member != (head);                                                 \
       pos = le2(__typeof__(*pos), pos->member.next, member))

#define list_for_reserve_item(pos, head, member)                               \
  for (pos = le2(__typeof__(*pos), (head)->last, member);                      \
       &pos->member != (head);                                                 \
       pos = le2(__typeof__(*pos), pos->member.next, member))

#define list_for_each_item_safe(pos, n, head, member)                          \
  for (pos = le2(__typeof__(*pos), (head)->next, member),                      \
      n = le2(__typeof__(*pos), pos->member.next, member);                     \
       &pos->member != (head);                                                 \
       pos = n, n = le2(__typeof__(*pos), n->member.next, member))

#define list_for_reverse_item_safe(pos, n, head, member)                       \
  for (pos = le2(__typeof__(*pos), (head)->last, member),                      \
      n = le2(__typeof__(*pos), pos->member.last, member);                     \
       &pos->member != (head);                                                 \
       pos = n, n = le2(__typeof__(*pos), n->member.last, member))
#endif