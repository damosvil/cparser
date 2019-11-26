//============================================================================
// Name        : cparser.cpp
// Author      : Daniel Mosquera
// Version     :
// Copyright   : GPLv2
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <cparsertools.h>
#include <cparserpaths.h>
#include <cparsertoken.h>
#include <cparserobject.h>
#include <cparserdictionary.h>
#include <cparser.h>

int main()
{
//	cparserdictionary_t *dd = DictionaryNew();
//	DictionarySetKeyValue(dd, _T "cparsertools.h", NULL);
//	DictionarySetKeyValue(dd, _T "cparserpaths.h", NULL);
//	DictionarySetKeyValue(dd, _T "cparsertoken.h", NULL);
//	DictionarySetKeyValue(dd, _T "cparserobject.h", NULL);
//	DictionarySetKeyValue(dd, _T "cparserdictionary.h", NULL);
//	DictionarySetKeyValue(dd, _T "cparser.h", NULL);
//	DictionarySetKeyValue(dd, _T "1.h", NULL);
//	DictionarySetKeyValue(dd, _T "2.h", NULL);
//	DictionarySetKeyValue(dd, _T "3.h", NULL);
//	DictionarySetKeyValue(dd, _T "4.h", NULL);
//	DictionarySetKeyValue(dd, _T "5.h", NULL);
//	DictionarySetKeyValue(dd, _T "6.h", NULL);
//	DictionarySetKeyValue(dd, _T "0.h", NULL);
//	DictionarySetKeyValue(dd, _T "a.h", NULL);
//	DictionarySetKeyValue(dd, _T "z.h", NULL);
//	DictionaryDelete(dd);

	cparserdictionary_t *defines = DictionaryNew();
	DictionarySetKeyValue(defines, _T "DEBUG", NULL);

	cparserpaths_t *cpaths = CParserPathsNew();
	CParserPathsAddPath(cpaths,_T "/usr/include");
	CParserPathsAddPath(cpaths,_T "./src/");
	CParserPathsAddPath(cpaths,_T "./src/cparser");
	CParserPathsAddPath(cpaths,_T ".");

	object_t *oo = CParserParse(defines, cpaths, _T"project_examples/opengl/main.c");

	printf("Fin.\r\n");

	return 0;
}

