#include <stdlib.h>
#include <stdio.h>

#include "utility/linkedlist.h"

LinkedList* new_linkedlist() {
  LinkedList* l = malloc(sizeof(LinkedList));
  l->numItems = 0;
  l->head = NULL;
  l->tail = NULL;

  return l;
}

/**
 * Since this is a generic linked list, the caller is responsible for
 * freeing everything contained within the list. This function simply
 * traverses the list and frees the ListItem pointers and the LinkedList
 * itself. 
 */
void free_linkedlist(LinkedList* l) {
  if (l == NULL) return;

  if (l->head != NULL) {
    ListItem* li = l->head;
    while (li != NULL) {
      ListItem* curr = li;
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