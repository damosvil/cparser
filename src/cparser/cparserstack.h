/*
 * cparserstack.h
 *
 *  Created on: 2 dic. 2019
 *      Author: iso9660
 */

#ifndef CPARSER_CPARSERSTACK_H_
#define CPARSER_CPARSERSTACK_H_


struct cparserstack_s;
typedef struct cparserstack_s cparserstack_t;


cparserstack_t *StackNew(size_t item_size);
void StackDelete(cparserstack_t *s);
void StackPush(cparserstack_t *s, const void *data);
bool StackPop(cparserstack_t *s, void *data);


#endif /* CPARSER_CPARSERSTACK_H_ */
