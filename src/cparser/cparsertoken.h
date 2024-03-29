/*
 * cparsertoken.h
 *
 *  Created on: 12/11/2019
 *      Author: blue
 */

#ifndef CPARSERTOKEN_H_
#define CPARSERTOKEN_H_


#define CPARSER_TOKEN_FLAG_PARSE_INCLUDE_FILENAME		1
#define CPARSER_TOKEN_FLAG_PARSE_PREPROCESSOR_LITERAL			2
#define CPARSER_TOKEN_FLAG_PARSE_DEFINE_IDENTIFIER		4


// Token type
typedef enum token_type_e
{
	CPARSER_TOKEN_TYPE_SINGLE_CHAR = 0,
	CPARSER_TOKEN_TYPE_IDENTIFIER,
	CPARSER_TOKEN_TYPE_NUMBER_LITERAL,
	CPARSER_TOKEN_TYPE_STRING_LITERAL,
	CPARSER_TOKEN_TYPE_INCLUDE_LITERAL,
	CPARSER_TOKEN_TYPE_DEFINE_LITERAL,
	CPARSER_TOKEN_TYPE_CHAR_LITERAL,
	CPARSER_TOKEN_TYPE_OPERATOR,
	CPARSER_TOKEN_TYPE_C_COMMENT,
	CPARSER_TOKEN_TYPE_CPP_COMMENT,
	CPARSER_TOKEN_TYPE_BACKSLASH,
	CPARSER_TOKEN_TYPE_INVALID
} token_type_t;

// Parse token
typedef struct token_s
{
	token_type_t type;
	bool first_token_in_line;
	uint32_t row;
	uint32_t column;
	uint8_t *str;
} token_t;

// Source read callback function
// returns: last byte read or EOF if no more bytes avaliable
// parameters: from: data source for this callback function
typedef int (*read_callback_t)(void *from);

// Token source
typedef struct token_source_s
{
	void *from;					// Data source that TokenNext will use to read bytes from with "read" callback function
	int16_t last_char;			// Last char read
	uint32_t row;				// Row
	uint32_t column;			// Column
	read_callback_t read;		// Callback to read data source
} token_source_t;


void TokenSourceInit(token_source_t *source, void *from, read_callback_t read);

token_t *TokenNew(void);
void TokenDelete(token_t *tt);
bool TokenNext(token_t *tt, token_source_t *source, uint32_t flags);


#endif /* CPARSERTOKEN_H_ */
