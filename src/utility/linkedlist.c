#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "utility/linkedlist.h"

LinkedList* new_linkedlist() {
  LinkedList* l = malloc(sizeof(LinkedList));
  l->numItems = 0;
  l->head = NULL;
  l->tail = NULL;

  return l;
}

void free_linkedlist(LinkedList* l, void (*cleanup)(void*)) {
  if (l == NULL) return;

  if (l->head != NULL) {
    ListItem* li = l->head;
    while (li != NULL) {
      ListItem* curr = li;
      if (*cleanup != NULL) {
        (*cleanup)(li->ptr);
      }
      li = curr->next;
      free(curr);
    }
  }

  free(l);
}

void linkedlist_append(LinkedList* l, void* ptr) {
  if (l == NULL) {
    printf("Error: tried to append to a NULL LinkedList");
    return;
  }

  ListItem* new = malloc(sizeof(ListItem));
  new->ptr = ptr;
  new->next = NULL;
  new->prev = NULL;

  if (l->numItems == 0) {
    l->head = new;
  } else {
    ListItem* tail = l->tail;
    tail->next = new;
    new->prev = tail;
  }

  l->tail = new;
  l->numItems++;
}

ListItem* linkedlist_search(LinkedList* l, void* ptr, bool (*comparison)(void*, void*)) {
  ListItem* li = l->head;

  while (li != NULL) {
    if ((*comparison)(li->ptr, ptr)) return li;
    li = li->next;
  }

  return NULL;
}