//============================================================================
// Name        : cparser.cpp
// Author      : Daniel Mosquera
// Version     :
// Copyright   : GPLv2
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <stdint.h>
#include <stdio.h>
#include <cparsertools.h>
#include <cparsertoken.h>
#include <cparserpaths.h>
#include <cparser.h>

#include <stdint.h>
#define MAX(A,B)	((A > B) ? A : B)

using namespace cparser;


int main()
{
	uint32_t k = MAX(0, 1);

	cparser_paths *cpaths = new cparser_paths();
	cpaths->AddPath(_T "/usr/include");
	cpaths->AddPath(_T "./src/");
	cpaths->AddPath(_T "./src/cparser");

	cparser::cparser cp(cpaths, _T"project_examples/opengl/main.c");

	cparser::cparser::object_s *oo = cp.Parse(NULL);

	printf("Fin.\r\n");

	return 0;
}

