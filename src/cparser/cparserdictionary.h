/*
 * cparserdictionary.h
 *
 *  Created on: 19/11/2019
 *      Author: blue
 */

#ifndef CPARSERDICTIONARY_H_
#define CPARSERDICTIONARY_H_


struct dictionary_s;
typedef struct dictionary_s dictionary_t;


dictionary_t * DictionaryNew(void);
void DictionaryRemoveKey(dictionary_t *d, const uint8_t *key);
void DictionarySetKeyValue(dictionary_t *d, const uint8_t *key, const void *value);
const void * DictionaryGetKeyValue(dictionary_t *d, const uint8_t *key);
uint32_t DictionaryGetKeyCount(dictionary_t *d);
const uint8_t * DictionaryGetKeyByIndex(dictionary_t *d, uint32_t ix);
const void * DictionaryGetValueByIndex(dictionary_t *d, uint32_t ix);
void DictionaryDelete(dictionary_t *d);


#endif /* CPARSERDICTIONARY_H_ */
