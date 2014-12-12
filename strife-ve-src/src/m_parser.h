//
// Copyright(C) 2007-2014 Samuel Villarreal
// Copyright(C) 2014 Night Dive Studios, Inc.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//

#ifndef __PARSER_H__
#define __PARSER_H__

#include "doomtype.h"
#include "m_qstring.h"

#define MAX_NESTED_PARSERS      128
#define MAX_NESTED_FILENAMES    128

typedef enum
{
    TK_NONE,
    TK_NUMBER,
    TK_STRING,
    TK_POUND,
    TK_COLON,
    TK_SEMICOLON,
    TK_PERIOD,
    TK_QUOTE,
    TK_FORWARDSLASH,
    TK_PLUS,
    TK_MINUS,
    TK_EQUAL,
    TK_LBRACK,
    TK_RBRACK,
    TK_LPAREN,
    TK_RPAREN,
    TK_LSQBRACK,
    TK_RSQBRACK,
    TK_COMMA,
    TK_IDENIFIER,
    TK_EOF
} tokentype_t;

typedef struct
{
    qstring_t   token;
    qstring_t   stringToken;
    char        *buffer;
    char        *pointer_start;
    char        *pointer_end;
    int         linepos;
    int         rowpos;
    int         buffpos;
    int         buffsize;
    int         tokentype;
    const char  *name;
} lexer_t;

void        M_ParserInit(void);
lexer_t    *M_ParserOpen(const char* filename);
void        M_ParserClose(void);
boolean     M_ParserFind(lexer_t *lexer);
void        M_ParserError(const char *msg, ...);
boolean     M_ParserCheckState(lexer_t *lexer);
int         M_ParserGetNumber(lexer_t *lexer);
double      M_ParserGetFloat(lexer_t *lexer);
void        M_ParserGetString(lexer_t *lexer);
void        M_ParserExpectNextToken(lexer_t *lexer, int type);
boolean     M_ParserFind(lexer_t *lexer);
char        M_ParserGetChar(lexer_t *lexer);
boolean     M_ParserMatches(lexer_t *lexer, const char *string);
void        M_ParserRewind(lexer_t *lexer);
void        M_ParserReset(lexer_t *lexer);
void        M_ParserSkipLine(lexer_t *lexer);
const char *M_ParserStringToken(lexer_t *lexer);

#endif
