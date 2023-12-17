#ifndef LINKEDLIST_H
#define LINKEDLIST_H

typedef struct ListItem {
  void* ptr;
  ListItem* prev;
  ListItem* next;
} ListItem;

typedef struct LinkedList {
  int numItems;
  ListItem* head;
  ListItem* tail;
} LinkedList;

LinkedList* new_linkedlist();
void free_linkedlist(LinkedList* l);

void linkedlist_append(LinkedList* l, void* ptr);

#endif /* LINKEDLIST_H */