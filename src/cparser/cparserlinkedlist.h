/*
 * cparserlinkedlist.h
 *
 *  Created on: 9 dic. 2019
 *      Author: iso9660
 */

#ifndef CPARSER_CPARSERLINKEDLIST_H_
#define CPARSER_CPARSERLINKEDLIST_H_


struct cparserlinkedlist_s;
typedef struct cparserlinkedlist_s cparserlinkedlist_t;


cparserlinkedlist_t *LinkedListNew(void *item);
void LinkedListDelete(cparserlinkedlist_t *l);
cparserlinkedlist_t *LinkedListFirst(cparserlinkedlist_t *l);
cparserlinkedlist_t *LinkedListLast(cparserlinkedlist_t *l);
cparserlinkedlist_t *LinkedListPrevious(cparserlinkedlist_t *l);
cparserlinkedlist_t *LinkedListNext(cparserlinkedlist_t *l);
cparserlinkedlist_t *LinkedListInsertBefore(cparserlinkedlist_t *l, void *item);
cparserlinkedlist_t *LinkedListInsertAfter(cparserlinkedlist_t *l, void *item);
void *LinkedListGetItem(cparserlinkedlist_t *l);
void *LinkedListUpdateItem(cparserlinkedlist_t *l, void *item);


#endif /* CPARSER_CPARSERLINKEDLIST_H_ */
