/*
 * cparserdictionary.h
 *
 *  Created on: 19/11/2019
 *      Author: blue
 */

#ifndef CPARSERDICTIONARY_H_
#define CPARSERDICTIONARY_H_


struct cparserdictionary_s;
typedef struct cparserdictionary_s cparserdictionary_t;


cparserdictionary_t * DictionaryNew(void);
void DictionaryRemoveKey(cparserdictionary_t *d, const uint8_t *key);
void DictionarySetKeyValue(cparserdictionary_t *d, const uint8_t *key, const void *value);
const bool DictionaryExistsKey(cparserdictionary_t *d, const uint8_t *key);
const void * DictionaryGetKeyValue(cparserdictionary_t *d, const uint8_t *key);
uint32_t DictionaryGetKeyCount(cparserdictionary_t *d);
const uint8_t * DictionaryGetKeyByIndex(cparserdictionary_t *d, uint32_t ix);
const void * DictionaryGetValueByIndex(cparserdictionary_t *d, uint32_t ix);
void DictionaryDelete(cparserdictionary_t *d);


#endif /* CPARSERDICTIONARY_H_ */
