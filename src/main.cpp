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
#include <cparserpaths.h>
#include <cparsertoken.h>
#include <cparserobject.h>
#include <cparser.h>


using namespace cparser;


int main()
{
	static const long *i = 0;

	cparser_paths *cpaths = new cparser_paths();
	cpaths->AddPath(_T "/usr/include");
	cpaths->AddPath(_T "./src/");
	cpaths->AddPath(_T "./src/cparser");

	cparser::cparser cp(cpaths, _T"project_examples/opengl/main.c");

	object_s *oo = cp.Parse(NULL);

	printf("Fin.\r\n");

	return 0;
}

