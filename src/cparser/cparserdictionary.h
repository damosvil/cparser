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


struct dictionary_s;


dictionary_s * DictionaryNew(void);
void DictionaryRemoveKey(dictionary_s *d, const uint8_t *key);
void DictionarySetKeyValue(dictionary_s *d, const uint8_t *key, const void *value);
const void * DictionaryGetKeyValue(dictionary_s *d, const uint8_t *key);
uint32_t DictionaryGetKeyCount(dictionary_s *d);
const uint8_t * DictionaryGetKeyByIndex(dictionary_s *d, uint32_t ix);
const void * DictionaryGetValueByIndex(dictionary_s *d, uint32_t ix);
void DictionaryDelete(dictionary_s *d);


}


#endif /* CPARSERDICTIONARY_H_ */
