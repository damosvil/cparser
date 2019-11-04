//============================================================================
// Name        : cparser.cpp
// Author      : Daniel Mosquera
// Version     :
// Copyright   : GPLv2
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <stdint.h>
#include <stdio.h>
#include "cparser/cparsertools.h"
#include "cparser/cparserpaths.h"
#include "cparser/cparser.h"


using namespace cparser;


int main() {
	cparser_paths *cpaths = new cparser_paths();
	cpaths->AddPath(_T "");


//	cparser::cparser cp(_T"src/main.cpp", &cpp);
//	printf("Hola.\r\n");

	return 0;
}
