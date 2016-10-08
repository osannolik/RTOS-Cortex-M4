/*
 * list_sorted.h
 *
 *  Created on: 17 sep 2016
 *      Author: osannolik
 */

#ifndef LIST_SORTED_H_
#define LIST_SORTED_H_

#include <stddef.h>
#include <stdint.h>

#define LIST_VALUE_DEPTH (2)

struct item {
  //uint32_t value[LIST_VALUE_DEPTH];
  uint32_t value;
  void *reference;
  void *list;
  struct item *prev;
  struct item *next;
};
typedef struct item list_item_t;

typedef struct {
  uint32_t len;
  list_item_t end;
  list_item_t *iterator;
} list_sorted_t;

#define LIST_ITEM_INIT {0, NULL, NULL, NULL, NULL}
#define LIST_INIT {0, LIST_ITEM_INIT, NULL}

#define LIST_END_VALUE 0xFFFFFFFF

#define LIST_LENGTH(plist)        (((list_sorted_t *) plist)->len)
// These are only valid when there are >0 items in list:
#define LIST_MAX_ITEM(plist)      (((list_sorted_t *) plist)->end.prev)
#define LIST_MIN_ITEM(plist)      (((list_sorted_t *) plist)->end.next)
#define LIST_MAX_VALUE(plist)     (((list_sorted_t *) plist)->end.prev->value)
#define LIST_MIN_VALUE(plist)     (((list_sorted_t *) plist)->end.next->value)
#define LIST_MAX_VALUE_REF(plist) (((list_sorted_t *) plist)->end.prev->reference)
#define LIST_MIN_VALUE_REF(plist) (((list_sorted_t *) plist)->end.next->reference)

#define LIST_FIRST_ITEM LIST_MIN_ITEM
#define LIST_LAST_ITEM  LIST_MAX_ITEM
#define LIST_FIRST_REF  LIST_MIN_VALUE_REF
#define LIST_LAST_REF   LIST_MAX_VALUE_REF

#define LIST_SET_ITERATOR_TO(item) (((list_sorted_t *) ((list_item_t *) item)->list)->iterator = (list_item_t *) item)

void list_sorted_init(list_sorted_t *list);
list_item_t *list_sorted_next_item(list_item_t *item);
list_item_t *list_sorted_get_iter_item(list_sorted_t *list);
void *list_sorted_get_iter_ref(list_sorted_t *list);
// uint32_t list_sorted_insert(list_sorted_t *list, list_item_t *item, uint8_t depth);
uint32_t list_sorted_insert(list_sorted_t *list, list_item_t *item);
uint32_t list_sorted_iter_insert(list_sorted_t *list, list_item_t *item);
uint32_t list_sorted_remove(list_item_t *item);

#endif /* LIST_SORTED_H_ */
