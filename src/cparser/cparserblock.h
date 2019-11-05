/*
 * cparsercomment.h
 *
 *  Created on: 5 nov. 2019
 *      Author: iso9660
 */

#ifndef CPARSER_CPARSERBLOCK_H_
#define CPARSER_CPARSERBLOCK_H_


namespace cparser {

class cparser_block
{

private:
	// Source info
	FILE *file;
	const uint8_t *filename;
	uint32_t row;
	uint32_t column;

	const uint8_t **terminators;

	// Data
	uint8_t *data;
	uint32_t data_size;
	uint32_t data_count;

	// Blocks
	cparser_block **blocks;
	uint32_t blocks_size;
	uint32_t blocks_count;

	void AppendDataByte(uint8_t b);
	void AppendBlock(cparser_block *b);
	bool ByteIn(uint8_t b, const uint8_t *set);
	bool DigestByte(uint8_t b);

public:
	cparser_block(FILE *f, const uint8_t *file, uint32_t row, uint32_t column, const uint8_t **terminators);
	virtual ~cparser_block();


};

} /* namespace cgl */

#endif /* CPARSER_CPARSERBLOCK_H_ */
