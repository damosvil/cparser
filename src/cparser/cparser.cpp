/*
 * cparser.cpp
 *
 *  Created on: 2/11/2019
 *      Author: blue
 */

#include <stdint.h>
#include <stdio.h>
#include "cparserpaths.h"
#include "cparsertools.h"
#include "cparser.h"


namespace cparser {

cparser::cparser(const uint8_t *filename, const cparser_paths *paths)
{
	FILE *f = fopen(_t filename, "r");
	if (!f)
		return;


}

cparser::~cparser() {
	// TODO Auto-generated destructor stub
}


} /* namespace cparser */
