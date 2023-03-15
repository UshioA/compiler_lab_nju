#ifndef __CMM_LIST_H__
#define __CMM_LIST_H__
#include "common.h"

typedef struct list_entry {
  struct list_entry *next, *prev;
} list_entry;
inline int list_empty(list_entry *head) { return head->next == head; }

inline void list_init(list_entry *h) {
  h->next = h;
  h->prev = h;
  assert(list_empty(h));
}

/*
 * General helper function for add a node into a list knowing last and next
 */
inline void __list_add(list_entry *prev, list_entry *next, list_entry *_new) {
  next->prev = _new;
  _new->next = next;
  _new->prev = prev;
  prev->next = _new;
}

inline list_entry *list_head(list_entry *entry) {
  if (list_empty(entry))
    return NULL;
  return entry->next;
}

inline list_entry *list_tail(list_entry *entry) {
  if (list_empty(entry))
    return NULL;
  return entry->prev;
}

/**
 * @brief 头插法
 *
 * @param head list head to be inserted
 * @param _new _new node that will be inserted to head
 */
inline void list_add(list_entry *_new, list_entry *head) {
  __list_add(head, head->next, _new);
}

/**
 * @brief 尾插法
 *
 * @param head: list head to be inserted
 * @param _new: _new node that will be inserted to head
 */
inline void list_add_tail(list_entry *_new, list_entry *head) {
  __list_add(head->prev, head, _new);
}

/**
 * @brief General helper function for deleting a node knowing last and next
 */
inline void __list_del(list_entry *prev, list_entry *next) {
  prev->next = next;
  next->prev = prev;
}

/**
 * @brief Delete entry from a list
 *
 * @param entry : trivial.
 */
inline void list_del(list_entry *entry) {
  __list_del(entry->prev, entry->next);
  entry->prev = entry->next = NULL;
}

/**
 * @brief 从list中删去一个节点并初始化它
 *
 * @param entry : trivial.
 */
inline void list_del_init(list_entry *entry) {
  __list_del(entry->prev, entry->next);
  list_init(entry);
}

/**
 * @brief 把entry摘下来，然后头插入head
 *
 * @param entry
 * @param head
 */
inline void list_move(list_entry *entry, list_entry *head) {
  __list_del(entry->prev, entry->next);
  list_add(entry, head);
}

/**
 * @brief 把entry摘下来，然后尾插入head
 *
 * @param entry
 * @param head
 */
inline void list_move_tail(list_entry *entry, list_entry *head) {
  __list_del(entry->prev, entry->prev);
  list_add_tail(entry, head);
}

inline void __list_splice(struct list_entry *list, struct list_entry *head) {
  struct list_entry *first = list->next;
  struct list_entry *last = list->prev;
  struct list_entry *at = head->next;
  first->prev = head;
  head->next = first;
  last->next = at;
  at->prev = last;
}

/**
 * @brief list_splice - join two lists
 * @param list the _new list to add.
 * @param head the place to add it in the first list.
 */
inline void list_splice(list_entry *list, list_entry *head) {
  if (!list_empty(list))
    __list_splice(list, head);
}

inline void list_splice_init(list_entry *list, list_entry *head) {
  if (!list_empty(list)) {
    __list_splice(list, head);
    list_init(list);
  }
}

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
  for (pos = le2(typeof(*pos), (head)->next, member); &pos->member != (head);  \
       pos = le2(typeof(*pos), pos->member.next, member))

#define list_for_reserve_item(pos, head, member)                               \
  for (pos = le2(typeof(*pos), (head)->last, member); &pos->member != (head);  \
       pos = le2(typeof(*pos), pos->member.next, member))

#define list_for_each_item_safe(pos, n, head, member)                          \
  for (pos = le2(typeof(*pos), (head)->next, member),                          \
      n = le2(typeof(*pos), pos->member.next, member);                         \
       &pos->member != (head);                                                 \
       pos = n, n = le2(typeof(*pos), n->member.next, member))

#define list_for_reverse_item_safe(pos, n, head, member)                       \
  for (pos = le2(typeof(*pos), (head)->last, member),                          \
      n = le2(typeof(*pos), pos->member.last, member);                         \
       &pos->member != (head);                                                 \
       pos = n, n = le2(typeof(*pos), n->member.last, member))
#endif