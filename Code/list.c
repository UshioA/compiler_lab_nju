#include "list.h"
#include "common.h"

int list_empty(list_entry *head) { return head->next == head; }

void list_init(list_entry *h) {
  h->next = h;
  h->prev = h;
  assert(list_empty(h));
}

/*
 * General helper function for add a node into a list knowing last and next
 */
void __list_add(list_entry *prev, list_entry *next, list_entry *_new) {
  next->prev = _new;
  _new->next = next;
  _new->prev = prev;
  prev->next = _new;
}

list_entry *list_head(list_entry *entry) {
  if (list_empty(entry))
    return NULL;
  return entry->next;
}

list_entry *list_tail(list_entry *entry) {
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
void list_add(list_entry *_new, list_entry *head) {
  __list_add(head, head->next, _new);
}

/**
 * @brief 尾插法
 *
 * @param head: list head to be inserted
 * @param _new: _new node that will be inserted to head
 */
void list_add_tail(list_entry *_new, list_entry *head) {
  __list_add(head->prev, head, _new);
}

/**
 * @brief General helper function for deleting a node knowing last and next
 */
void __list_del(list_entry *prev, list_entry *next) {
  prev->next = next;
  next->prev = prev;
}

/**
 * @brief Delete entry from a list
 *
 * @param entry : trivial.
 */
void list_del(list_entry *entry) {
  __list_del(entry->prev, entry->next);
  entry->prev = entry->next = NULL;
}

/**
 * @brief 从list中删去一个节点并初始化它
 *
 * @param entry : trivial.
 */
void list_del_init(list_entry *entry) {
  __list_del(entry->prev, entry->next);
  list_init(entry);
}

/**
 * @brief 把entry摘下来，然后头插入head
 *
 * @param entry
 * @param head
 */
void list_move(list_entry *entry, list_entry *head) {
  __list_del(entry->prev, entry->next);
  list_add(entry, head);
}

/**
 * @brief 把entry摘下来，然后尾插入head
 *
 * @param entry
 * @param head
 */
void list_move_tail(list_entry *entry, list_entry *head) {
  __list_del(entry->prev, entry->prev);
  list_add_tail(entry, head);
}

void __list_splice(struct list_entry *list, struct list_entry *head) {
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
void list_splice(list_entry *list, list_entry *head) {
  if (!list_empty(list))
    __list_splice(list, head);
}

void list_splice_init(list_entry *list, list_entry *head) {
  if (!list_empty(list)) {
    __list_splice(list, head);
    list_init(list);
  }
}