/*
 * list_sorted.c
 *
 *  Created on: 17 sep 2016
 *      Author: osannolik
 */

#include "list_sorted.h"

void list_sorted_init(list_sorted_t *list)
{
  list->len = 0;
  list->end.value = LIST_END_VALUE;
  list->end.list = list;
  list->end.prev = &(list->end);
  list->end.next = &(list->end);
  list->end.reference = NULL;
  list->iterator = NULL;
}

list_item_t *list_sorted_next_item(list_item_t *item)
{
  list_item_t *next_item = item->next;
  // Will get the next item but exclude the end item
  if (next_item == &(((list_sorted_t *) item->list)->end))
    return next_item->next;
  else
    return next_item;
}

list_item_t *list_sorted_get_iter_item(list_sorted_t *list)
{
  list_item_t *iter_item = NULL;

  if (list->len > 0) {
    iter_item = list->iterator;
    list->iterator = list_sorted_next_item(iter_item);
  }

  return iter_item;
}

void *list_sorted_get_iter_ref(list_sorted_t *list)
{
  list_item_t *next_item = list_sorted_get_iter_item(list);
  
  if (next_item == NULL)
    return NULL;
  else
    return (void *) next_item->reference;
}

uint32_t list_sorted_insert(list_sorted_t *list, list_item_t *item)
{
  list_item_t *insert_at = &(list->end);
  uint32_t input_value = item->value;

  item->list = list;

  if (input_value == LIST_END_VALUE) {
    insert_at = list->end.prev;
  } else {
    // Find last maximum value and insert after that
    while (insert_at->next->value <= input_value)
      insert_at = insert_at->next;
  }

  insert_at->next->prev = item;
  item->next = insert_at->next;
  item->prev = insert_at;
  insert_at->next = item;

  if (list->len == 0)
    list->iterator = item;

  return ++(list->len);
}

uint32_t list_sorted_iter_insert(list_sorted_t *list, list_item_t *item)
{
  // Insert the item to where the iterator points
  // NOTE: It is the user's responsibility to ensure that the position is correct (i.e. sorted)
  list_item_t *insert_at = list->iterator;

  if (list->len == 0) {
    return list_sorted_insert(list, item);
  } else {
    item->list = list;

    insert_at->next->prev = item;
    item->next = insert_at->next;
    item->prev = insert_at;
    insert_at->next = item;

    list->iterator = item;

    return ++(list->len);
  }

}

uint32_t list_sorted_remove(list_item_t *item)
{
  list_sorted_t *from_list = item->list;

  if (from_list != NULL) {
    if (from_list->len == 1) {
      // Removing the last item
      from_list->iterator = NULL;
    } else {
      from_list->iterator = list_sorted_next_item(item);
    }

    item->next->prev = item->prev;
    item->prev->next = item->next;

    (from_list->len)--;
    item->list = NULL;

  }

  return from_list->len;
}