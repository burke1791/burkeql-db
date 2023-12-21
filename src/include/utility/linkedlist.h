#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#pragma pack(push, 1) /* disabling memory alignment because I don't want to deal with it */
typedef struct ListItem {
  void* ptr;
  struct ListItem* prev;
  struct ListItem* next;
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