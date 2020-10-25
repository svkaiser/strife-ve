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
// DESCRIPTION:
//    Misc Text File Parser
//

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>

#include "m_parser.h"
#include "m_dllist.h"
#include "i_system.h"
#include "w_wad.h"
#include "z_zone.h"

typedef struct
{
    lexer_t         *currentLexer;
    lexer_t         *lexers[MAX_NESTED_PARSERS];
    int             numLexers;
    byte            charcode[256];
    char            *nestedFilenames[MAX_NESTED_FILENAMES];
    int             numNestedFilenames;
    dllistitem_t    *defineList;
} parser_t;

typedef struct
{
    dllistitem_t    link;
    qstring_t       name;
    qstring_t       string;
} defineMacro_t;

static parser_t parser;

//#define SC_DEBUG

#define COMMENT_NONE        0
#define COMMENT_SINGLELINE  1
#define COMMENT_MULTILINE   2

typedef enum
{
    CHAR_NUMBER,
    CHAR_LETTER,
    CHAR_SYMBOL,
    CHAR_QUOTE,
    CHAR_SPECIAL,
    CHAR_EOF
} chartype_t;

#ifdef SC_DEBUG

//
// M_ParserDebugPrintf
//

static void M_ParserDebugPrintf(const char *str, ...)
{
    char buf[1024];
    va_list v;

    if(!verbose)
    {
        return;
    }
    
    va_start(v, str);
    vsprintf(buf, str,v);
    va_end(v);

    fprintf(debugfile, buf);
}
#endif

//
// M_ParserInitLexer
//

static void M_ParserInitLexer(lexer_t *lexer, const char *filename, char *buf, int bufSize)
{
    lexer->buffer           = (char*)Z_Calloc(1, bufSize+1, PU_STATIC, 0);
    memcpy(lexer->buffer, buf, bufSize);

    lexer->buffsize         = bufSize;
    lexer->pointer_start    = lexer->buffer;
    lexer->pointer_end      = lexer->buffer + lexer->buffsize;
    lexer->linepos          = 1;
    lexer->rowpos           = 1;
    lexer->buffpos          = 0;
    lexer->tokentype        = TK_NONE;
    lexer->name             = filename;

    QStrInitCreate(&lexer->token);
    QStrInitCreate(&lexer->stringToken);
}

//
// M_ParserDeleteLexerData
//

static void M_ParserDeleteLexerData(lexer_t *lexer)
{
    if(lexer->buffer)
    {
        Z_Free(lexer->buffer);
    }
    
    lexer->buffer           = NULL;
    lexer->buffsize         = 0;
    lexer->pointer_start    = NULL;
    lexer->pointer_end      = NULL;
    lexer->linepos          = 0;
    lexer->rowpos           = 0;
    lexer->buffpos          = 0;

    QStrFree(&lexer->token);
    QStrFree(&lexer->stringToken);
}

//
// M_ParserCheckState
//

boolean M_ParserCheckState(lexer_t *lexer)
{
#ifdef SC_DEBUG
    M_ParserDebugPrintf("(%s): checking script state: %i : %i\n",
        lexer->name, lexer->buffpos, lexer->buffsize);
#endif

    if(lexer->buffpos < lexer->buffsize && lexer->tokentype != TK_EOF)
    {
        return true;
    }

    return false;
}

//
// M_ParserCompareToken
//

static boolean M_ParserCompareToken(lexer_t *lexer, const char *string)
{
    const byte *us1 = (const byte*)QStrConstPtr(&lexer->token);
    const byte *us2 = (const byte*)string;

    while(tolower(*us1) == tolower(*us2))
    {
        if(*us1++ == '\0')
        {
            return false;
        }

        us2++;
    }

    return (tolower(*us1) - tolower(*us2)) != 0;
}

//
// M_ParserClearToken
//

static void M_ParserClearToken(lexer_t *lexer)
{
    lexer->tokentype = TK_NONE;
    QStrClear(&lexer->token);
}

//
// M_ParserGetNumber
//

int M_ParserGetNumber(lexer_t *lexer)
{
#ifdef SC_DEBUG
    M_ParserDebugPrintf("get number (%s)\n", QStrConstPtr(&lexer->token));
#endif

    M_ParserFind(lexer);

    if(lexer->tokentype != TK_NUMBER)
    {
        M_ParserError("%s is not a number", QStrConstPtr(&lexer->token));
    }

    return QStrAtoi(&lexer->token);
}

//
// M_ParserGetFloat
//

double M_ParserGetFloat(lexer_t *lexer)
{
#ifdef SC_DEBUG
    M_ParserDebugPrintf("get float (%s)\n", QStrConstPtr(&lexer->token));
#endif

    M_ParserFind(lexer);

    if(lexer->tokentype != TK_NUMBER)
    {
        M_ParserError("%s is not a float", QStrConstPtr(&lexer->token));
    }

    return atof(QStrConstPtr(&lexer->token));
}

//
// M_ParserGetString
//

void M_ParserGetString(lexer_t *lexer)
{
    M_ParserExpectNextToken(lexer, TK_STRING);
    QStrQCopy(&lexer->stringToken, &lexer->token);
}

//
// M_ParserStringToken
//
// The actual function that returns the value of stringToken
//

const char *M_ParserStringToken(lexer_t *lexer)
{
    return QStrConstPtr(&lexer->stringToken);
}

//
// M_ParserMustMatchToken
//

static void M_ParserMustMatchToken(lexer_t *lexer, int type)
{
#ifdef SC_DEBUG
    M_ParserDebugPrintf("must match %i\n", type);
    M_ParserDebugPrintf("tokentype %i\n", lexer->tokentype);
#endif

    if(lexer->tokentype != type)
    {
        const char *string;

        switch(type)
        {
            case TK_NUMBER:
                string = "a number";
                break;
            case TK_STRING:
                string = "a string";
                break;
            case TK_POUND:
                string = "a pound sign";
                break;
            case TK_COLON:
                string = "a colon";
                break;
            case TK_SEMICOLON:
                string = "a semicolon";
                break;
            case TK_LBRACK:
                string = "{";
                break;
            case TK_RBRACK:
                string = "}";
                break;
            case TK_LSQBRACK:
                string = "[";
                break;
            case TK_RSQBRACK:
                string = "]";
                break;
            case TK_LPAREN:
                string = "(";
                break;
            case TK_RPAREN:
                string = ")";
                break;
            case TK_COMMA:
                string = "a comma";
                break;
            case TK_PLUS:
                string = "+";
                break;
            case TK_MINUS:
                string = "-";
                break;
            default:
                string = NULL;
                M_ParserError("Invalid token: %s", QStrConstPtr(&lexer->token));
                break;
        }

        M_ParserError("Expected %s, but found: %s (%i : %i)",
            string, QStrConstPtr(&lexer->token), lexer->tokentype, type);
    }
}

//
// M_ParserExpectNextToken
//

void M_ParserExpectNextToken(lexer_t *lexer, int type)
{
#ifdef SC_DEBUG
    M_ParserDebugPrintf("expect %i\n", type);
#endif
    M_ParserFind(lexer);
    M_ParserMustMatchToken(lexer, type);
}

//
// M_ParserGetNumberToken
//

static void M_ParserGetNumberToken(lexer_t *lexer, char initial)
{
    int c = (byte)initial;

    lexer->tokentype = TK_NUMBER;

    while(parser.charcode[c] == CHAR_NUMBER)
    {
        QStrPutc(&lexer->token, c);
        c = M_ParserGetChar(lexer);
    }

#ifdef SC_DEBUG
    M_ParserDebugPrintf("get number (%s)\n", lexer->token);
#endif

    M_ParserRewind(lexer);
}

//
// M_ParserGetLetterToken
//

static void M_ParserGetLetterToken(lexer_t *lexer, char initial)
{
    int c = (byte)initial;
    boolean haschar = false;

    while(parser.charcode[c] == CHAR_LETTER ||
        (haschar && parser.charcode[c] == CHAR_NUMBER))
    {
        QStrPutc(&lexer->token, c);
        c = M_ParserGetChar(lexer);
        haschar = true;
    }

    lexer->tokentype = TK_IDENIFIER;

#ifdef SC_DEBUG
    M_ParserDebugPrintf("get letter (%s)\n", lexer->token);
#endif

    M_ParserRewind(lexer);
}

//
// M_ParserGetSymbolToken
//

static void M_ParserGetSymbolToken(lexer_t *lexer, char c)
{
    switch(c)
    {
        case '#':
            lexer->tokentype = TK_POUND;
            break;
        case ':':
            lexer->tokentype = TK_COLON;
            break;
        case ';':
            lexer->tokentype = TK_SEMICOLON;
            break;
        case '=':
            lexer->tokentype = TK_EQUAL;
            break;
        case '.':
            lexer->tokentype = TK_PERIOD;
            break;
        case '{':
            lexer->tokentype = TK_LBRACK;
            break;
        case '}':
            lexer->tokentype = TK_RBRACK;
            break;
        case '(':
            lexer->tokentype = TK_LPAREN;
            break;
        case ')':
            lexer->tokentype = TK_RPAREN;
            break;
        case '[':
            lexer->tokentype = TK_LSQBRACK;
            break;
        case ']':
            lexer->tokentype = TK_RSQBRACK;
            break;
        case ',':
            lexer->tokentype = TK_COMMA;
            break;
        case '\'':
            lexer->tokentype = TK_QUOTE;
            break;
        case '/':
            lexer->tokentype = TK_FORWARDSLASH;
            break;
        case '+':
            lexer->tokentype = TK_PLUS;
            break;
        case '-':
            lexer->tokentype = TK_MINUS;
            break;
        default:
            M_ParserError("Unknown symbol: %c", c);
            break;
    }

    QStrClear(&lexer->token);
    QStrPutc(&lexer->token, c);

#ifdef SC_DEBUG
    M_ParserDebugPrintf("get symbol (%s)\n", QStrConstPtr(&lexer->token));
#endif
}

//
// M_ParserGetStringToken
//

static void M_ParserGetStringToken(lexer_t *lexer)
{
    char c = (byte)M_ParserGetChar(lexer);

    while(parser.charcode[(byte)c] != CHAR_QUOTE)
    {
        QStrPutc(&lexer->token, c);
        c = M_ParserGetChar(lexer);
    }

    lexer->tokentype = TK_STRING;

#ifdef SC_DEBUG
    M_ParserDebugPrintf("get string (%s)\n", QStrConstPtr(&lexer->token));
#endif
}

//
// M_ParserFind
//

boolean M_ParserFind(lexer_t *lexer)
{
    char c = 0;
    int comment = COMMENT_NONE;

    M_ParserClearToken(lexer);

    while(M_ParserCheckState(lexer))
    {
        c = M_ParserGetChar(lexer);

        if(comment == COMMENT_NONE)
        {
            if(c == '/')
            {
                char gc = M_ParserGetChar(lexer);

                if(gc != '/' && gc != '*')
                {
                    M_ParserRewind(lexer);
                }
                else
                {
                    if(gc == '*')
                    {
                        comment = COMMENT_MULTILINE;
                    }
                    else
                    {
                        comment = COMMENT_SINGLELINE;
                    }
                }
            }
        }
        else if(comment == COMMENT_MULTILINE)
        {
            if(c == '*')
            {
                char gc = M_ParserGetChar(lexer);

                if(gc != '/')
                {
                    M_ParserRewind(lexer);
                }
                else
                {
                    comment = COMMENT_NONE;
                    continue;
                }
            }
        }

        if(comment == COMMENT_NONE)
        {
            byte bc = ((byte)c);

            if(parser.charcode[bc] != CHAR_SPECIAL)
            {
                switch(parser.charcode[bc])
                {
                case CHAR_NUMBER:
                    M_ParserGetNumberToken(lexer, c);
                    return true;
                        
                case CHAR_LETTER:
                    M_ParserGetLetterToken(lexer, c);
                    return true;
                        
                case CHAR_QUOTE:
                    M_ParserGetStringToken(lexer);
                    return true;
                        
                case CHAR_SYMBOL:
                    M_ParserGetSymbolToken(lexer, c);
                    return true;
                        
                case CHAR_EOF:
                    lexer->tokentype = TK_EOF;
#ifdef SC_DEBUG
                    M_ParserDebugPrintf("EOF token\n");
#endif
                    return true;
                        
                default:
                    break;
                }
            }
        }

        if(c == '\n')
        {
            lexer->linepos++;
            lexer->rowpos = 1;

            if(comment == COMMENT_SINGLELINE)
            {
                comment = COMMENT_NONE;
            }
        }
    }

    return false;
}

//
// M_ParserGetChar
//

char M_ParserGetChar(lexer_t *lexer)
{
    int c;

#ifdef SC_DEBUG
    M_ParserDebugPrintf("(%s): get char\n", lexer->name);
#endif

    lexer->rowpos++;
    c = (byte)lexer->buffer[lexer->buffpos++];

    if(parser.charcode[c] == CHAR_EOF)
    {
        c = 0;
    }

#ifdef SC_DEBUG
    M_ParserDebugPrintf("get char: %i\n", c);
#endif

    return c;
}

//
// M_ParserMatches
//

boolean M_ParserMatches(lexer_t *lexer, const char *string)
{
    return !M_ParserCompareToken(lexer, string);
}

//
// M_ParserRewind
//

void M_ParserRewind(lexer_t *lexer)
{
#ifdef SC_DEBUG
    M_ParserDebugPrintf("(%s): rewind\n", lexer->name);
#endif

    lexer->rowpos--;
    lexer->buffpos--;
}

//
// M_ParserReset
//

void M_ParserReset(lexer_t *lexer)
{
#ifdef SC_DEBUG
    M_ParserDebugPrintf("(%s): reset\n", lexer->name);
#endif

    lexer->linepos          = 1;
    lexer->rowpos           = 1;
    lexer->buffpos          = 0;
    lexer->tokentype        = TK_NONE;
}

//
// M_ParserSkipLine
//

void M_ParserSkipLine(lexer_t *lexer)
{
#ifdef SC_DEBUG
    M_ParserDebugPrintf("SkipLine\n");
#endif

    int curline = lexer->linepos;

    while(M_ParserCheckState(lexer))
    {
        M_ParserFind(lexer);

        if(curline != lexer->linepos)
        {
            return;
        }
    }
}

//
// M_ParserInit
//
// Sets up lookup tables
//

void M_ParserInit(void)
{
    int i;

    parser.numNestedFilenames = 0;
    parser.numLexers = 0;

    for(i = 0; i < 256; i++)
    {
        parser.charcode[i] = CHAR_SPECIAL;
    }
    for(i = '!'; i <= '~'; i++)
    {
        parser.charcode[i] = CHAR_SYMBOL;
    }
    for(i = '0'; i <= '9'; i++)
    {
        parser.charcode[i] = CHAR_NUMBER;
    }
    for(i = 'A'; i <= 'Z'; i++)
    {
        parser.charcode[i] = CHAR_LETTER;
    }
    for(i = 'a'; i <= 'z'; i++)
    {
        parser.charcode[i] = CHAR_LETTER;
    }

    parser.charcode['"'] = CHAR_QUOTE;
    parser.charcode['_'] = CHAR_LETTER;
    parser.charcode['-'] = CHAR_NUMBER;
    parser.charcode['.'] = CHAR_NUMBER;
    parser.charcode[127] = CHAR_EOF;
    
    parser.defineList = NULL;
}

//
// M_ParserGetNestedFileName
//

const char *M_ParserGetNestedFileName(void)
{
    if(parser.numNestedFilenames <= 0)
    {
        return NULL;
    }

    return parser.nestedFilenames[parser.numNestedFilenames-1];
}

//
// M_ParserPushFileName
//

void M_ParserPushFileName(const char *name)
{
#ifdef SC_DEBUG
    M_ParserDebugPrintf("push nested file %s\n", name);
#endif
    parser.nestedFilenames[parser.numNestedFilenames++] = strdup(name);
}

//
// M_ParserPopFileName
//

void M_ParserPopFileName(void)
{
#ifdef SC_DEBUG
    M_ParserDebugPrintf("nested file pop\n");
#endif
    free(parser.nestedFilenames[parser.numNestedFilenames-1]);
    parser.nestedFilenames[parser.numNestedFilenames-1] = NULL;
    
    parser.numNestedFilenames--;
}

//
// M_ParserPushLexer
//

void M_ParserPushLexer(const char *filename, char *buf, int bufSize)
{
    if(parser.numLexers >= MAX_NESTED_PARSERS)
    {
        M_ParserError("Reached max number of nested lexers (%i)", parser.numLexers);
    }

    // allocate a new lexer and setup data for it
    parser.lexers[parser.numLexers] = Z_Calloc(1, sizeof(lexer_t), PU_STATIC, 0);
    M_ParserInitLexer(parser.lexers[parser.numLexers], filename, buf, bufSize);
    
    parser.currentLexer = parser.lexers[parser.numLexers];
    parser.numLexers++;
}

//
// M_ParserPopLexer
//

void M_ParserPopLexer(void)
{
    M_ParserDeleteLexerData(parser.lexers[parser.numLexers-1]);
    Z_Free(parser.lexers[--parser.numLexers]);

    if(parser.numLexers <= 0)
    {
        parser.currentLexer = NULL;
    }
    else
    {
        parser.currentLexer = parser.lexers[parser.numLexers - 1];
    }
}

//
// M_ParserError
//

void M_ParserError(const char *msg, ...)
{
    char buf[1024];
    va_list v;
    
    va_start(v,msg);
    vsprintf(buf,msg,v);
    va_end(v);

    I_Error("%s : %s\n(line = %i, pos = %i)",
        M_ParserGetNestedFileName(),
        buf, parser.currentLexer->linepos, parser.currentLexer->rowpos);
}

//
// M_ParserHasScriptFile
//

static boolean M_ParserHasScriptFile(const char *file)
{
    int i;
    
    for(i = 0; i < parser.numNestedFilenames; i++)
    {
        if(!strcmp(parser.nestedFilenames[i], file))
        {
            return true;
        }
    }
    
    return false;
}

//
// M_ParserCheckIncludes
//

static void M_ParserCheckIncludes(lexer_t *lexer)
{
    while(M_ParserCheckState(lexer))
    {
        int oldpos = lexer->buffpos;
        
        M_ParserFind(lexer);
        if(lexer->tokentype == TK_POUND)
        {
            M_ParserFind(lexer);
            if(M_ParserMatches(lexer, "include"))
            {
                const char *nfile = M_ParserStringToken(lexer);
                
                M_ParserGetString(lexer);
                if(!M_ParserHasScriptFile(nfile))
                {
                    lexer_t *newlexer;
                    
                    if((newlexer = M_ParserOpen(nfile)))
                    {
                        int newsize = (oldpos + (lexer->buffsize + newlexer->buffsize)) + 1;
                        char *newbuffer = (char*)Z_Calloc(1, newsize, PU_STATIC, 0);
                        
                        if(oldpos > 0)
                        {
                            strncat(newbuffer, lexer->buffer, oldpos);
                            strcat(newbuffer, "\n");
                        }
                        
                        strncat(newbuffer, newlexer->buffer, newlexer->buffsize);
                        strncat(newbuffer, lexer->buffer + lexer->buffpos, lexer->buffsize);
                        
                        M_ParserDeleteLexerData(lexer);
                        M_ParserInitLexer(lexer, lexer->name, newbuffer, newsize);
                        
                        Z_Free(newbuffer);
                    }
                }
            }
        }
    }
    
    while(parser.currentLexer != lexer)
    {
        M_ParserClose();
    }
    
    M_ParserReset(lexer);
}

//
// M_ParserFindMacroName
//

static defineMacro_t *M_ParserFindMacroName(const char *check)
{
    dllistitem_t *chain;
    
    for(chain = parser.defineList; chain; chain = chain->next)
    {
        defineMacro_t *macro = chain->object;
        
        if(!strcmp(QStrConstPtr(&macro->name), check))
        {
            return macro;
        }
    }
    
    return NULL;
}

//
// M_ParserReplaceTokensWithMacroStrings
//

static void M_ParserReplaceTokensWithMacroStrings(lexer_t *lexer)
{
    char *newbuffer = (char*)Z_Calloc(1, lexer->buffsize, PU_STATIC, 0);
    int pos;
    defineMacro_t *macro;
    int lastpos = 0;
    
    M_ParserFind(lexer);
    
    while(M_ParserCheckState(lexer))
    {
        pos = lexer->buffpos;
        
        // skip lines that defines a macro
        if(lexer->tokentype == TK_POUND)
        {
            M_ParserSkipLine(lexer);
            
            if(lexer->tokentype == TK_IDENIFIER)
            {
                strcat(newbuffer, "\n");
                lastpos = lexer->buffpos - QStrLen(&lexer->token);
            }
            else
            {
                lastpos = lexer->buffpos;
            }

            continue;
        }
        else if(lexer->tokentype == TK_IDENIFIER &&
                ((macro = M_ParserFindMacroName(QStrConstPtr(&lexer->token)))))
        {
            // found a token that maches a define macro name
            int size = (lexer->buffpos - lastpos) - QStrLen(&lexer->token);

            // copy the file over to the new buffer so far and then concat the
            // string contents of the macro
            strncat(newbuffer, lexer->buffer + lastpos, size);
            strncat(newbuffer, QStrConstPtr(&macro->string), QStrLen(&macro->string));
            lastpos = lexer->buffpos;
        }
        else
        {
            int size = lexer->buffpos - lastpos;
            
            // just copy over
            strncat(newbuffer, lexer->buffer + lastpos, size);
            lastpos = lexer->buffpos;
        }
        
        M_ParserFind(lexer);
    }
 
    M_ParserReset(lexer);
    
    M_ParserDeleteLexerData(lexer);
    M_ParserInitLexer(lexer, lexer->name, newbuffer, strlen(newbuffer)+1);
    
    Z_Free(newbuffer);
}

//
// M_ParserCheckDefineMacros
//

static void M_ParserCheckDefineMacros(lexer_t *lexer)
{
    // scan through file and look for any macros that needs to be
    // added to the main list
    while(M_ParserCheckState(lexer))
    {
        M_ParserFind(lexer);
        if(lexer->tokentype == TK_POUND)
        {
            M_ParserFind(lexer);
            if(M_ParserMatches(lexer, "define"))
            {
                defineMacro_t *defineMacro;
                
                M_ParserFind(lexer);
                if(lexer->tokentype != TK_IDENIFIER)
                {
                    M_ParserError("Token (%s) is not an identifier\n", QStrConstPtr(&lexer->token));
                    return;
                }

                if(M_ParserFindMacroName(QStrConstPtr(&lexer->token)))
                {
                    // already has it
                    continue;
                }
                
                // allocate a new define macro
                defineMacro = (defineMacro_t*)Z_Calloc(1, sizeof(defineMacro_t), PU_STATIC, 0);
                
                QStrInitCreate(&defineMacro->name);
                QStrInitCreate(&defineMacro->string);
                
                QStrQCat(&defineMacro->name, &lexer->token);
                M_ParserGetString(lexer);
                QStrCat(&defineMacro->string, M_ParserStringToken(lexer));
                
                M_DLListInsert(&defineMacro->link, defineMacro, &parser.defineList);
            }
        }
    }
    
    M_ParserReset(lexer);
    
    // if we have any define macros in the list, then we need to
    // scan through the file and replace any instances of the macro
    // label with the string contents
    if(parser.defineList)
    {
        M_ParserReplaceTokensWithMacroStrings(lexer);
    }
}

//
// M_ParserOpen
//

lexer_t *M_ParserOpen(const char* filename)
{
    int buffsize;
    byte *buffer;
    int lumpnum;
    
#ifdef SC_DEBUG
    M_ParserDebugPrintf("opening %s\n", filename);
#endif

    lumpnum = W_GetNumForName((char*)filename);
    buffer = W_CacheLumpNum(lumpnum, PU_STATIC);
    buffsize = W_LumpLength(lumpnum);

    if(buffsize <= 0)
    {
        fprintf(stderr, "M_ParserOpen: %s not found\n", filename);
        return NULL;
    }

    // push out a new lexer
    M_ParserPushLexer(filename, (char*)buffer, buffsize);
    M_ParserPushFileName(filename);

    Z_Free(buffer);
    
    M_ParserCheckIncludes(parser.currentLexer);
    M_ParserCheckDefineMacros(parser.currentLexer);
    
    return parser.currentLexer;
}

//
// M_ParserClose
//

void M_ParserClose(void)
{
    M_ParserPopLexer();
    M_ParserPopFileName();
}
