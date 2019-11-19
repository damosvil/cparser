/*
 * cparserdictionary.h
 *
 *  Created on: 19/11/2019
 *      Author: blue
 */

#ifndef CPARSERDICTIONARY_H_
#define CPARSERDICTIONARY_H_

namespace cparser
{


struct dictionary;


dictionary * DictionaryInit(void);
void DictionaryRemoveKey(dictionary *d, const uint8_t *key);
void DictionarySetKeyValue(dictionary *d, const uint8_t *key, const void *value);
const void * DictionaryGetKeyValue(dictionary *d, const uint8_t *key);
uint32_t DictionaryGetKeyCount(dictionary *d);
const uint8_t * DictionaryGetKeyByIndex(dictionary *d, uint32_t ix);
const void * DictionaryGetValueByIndex(dictionary *d, uint32_t ix);
void DictionaryDelete(dictionary *d);


}


#endif /* CPARSERDICTIONARY_H_ */
