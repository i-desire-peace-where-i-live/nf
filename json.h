/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#ifndef NOTEFINDER_JSON_H
#define NOTEFINDER_JSON_H

#include "map.h"
#include "slice.h"

#define JSON_MAX_STR_LEN 16384
#define JSON_OBJECT_BEGIN '{'
#define JSON_OBJECT_END '}'
#define JSON_ARRAY_BEGIN '['
#define JSON_ARRAY_END ']'
#define JSON_STRING_ENVELOPE '"'
#define JSON_STRING_BEGIN JSON_STRING_ENVELOPE
#define JSON_STRING_END JSON_STRING_ENVELOPE
#define JSON_ATTR_BEGIN JSON_STRING_ENVELOPE
#define JSON_ATTR_END JSON_STRING_ENVELOPE
#define JSON_SEPARATOR ':'
#define JSON_DELIM ','
#define JSON_ESCAPE '\\'

/* FIXME: this is currently taken from:
 * https://gitlab.com/esr/microjson/-/blob/master/mjson.h
 * TODO: remove unnecessary stuff, turn this into an enum
 * with rather high lowest value and use for errno */

#define JSON_ERR_OBSTART 1      /* non-WS when expecting object start */
#define JSON_ERR_ATTRSTART 2    /* non-WS when expecting attrib start */
#define JSON_ERR_BADATTR 3      /* unknown attribute name */
#define JSON_ERR_ATTRLEN 4      /* attribute name too long */
#define JSON_ERR_NOARRAY 5      /* saw [ when not expecting array */
#define JSON_ERR_NOBRAK 6       /* array element specified, but no [ */
#define JSON_ERR_STRLONG 7      /* string value too long */
#define JSON_ERR_TOKLONG 8      /* token value too long */
#define JSON_ERR_BADTRAIL 9     /* garbage while expecting comma or } or ] */
#define JSON_ERR_ARRAYSTART 10  /* didn't find expected array start */
#define JSON_ERR_OBJARR 11      /* error while parsing object array */
#define JSON_ERR_SUBTOOLONG 12  /* too many array elements */
#define JSON_ERR_BADSUBTRAIL 13 /* garbage while expecting array comma */
#define JSON_ERR_SUBTYPE 14     /* unsupported array element type */
#define JSON_ERR_BADSTRING 15   /* error while string parsing */
#define JSON_ERR_CHECKFAIL 16   /* check attribute not matched */
#define JSON_ERR_NOPARSTR 17    /* can't support strings in parallel arrays */
#define JSON_ERR_BADENUM 18     /* invalid enumerated value */
#define JSON_ERR_QNONSTRING 19  /* saw quoted value when expecting nonstring */
#define JSON_ERR_NONQSTRING \
  20                        /* didn't see quoted value when expecting string */
#define JSON_ERR_MISC 21    /* other data conversion error */
#define JSON_ERR_BADNUM 22  /* error while parsing a numerical argument */
#define JSON_ERR_NULLPTR 23 /* unexpected null value or attribute pointer */
#define JSON_ERR_NOCURLY 24 /* object element specified, but no { */
#define JSON_ERR_EMPTY 25   /* input was empty or white-space only */
#define JSON_ERR_NOSEP 26   /* : expected after attribute but not found */

#define REALLY_SIMPLEST_JSON_EXAMPLE \
  "{\"a\": \"0\", \"b\": \"1\", \"c\": \"2\"}"

#define SIMPLEST_JSON_EXAMPLE                                               \
  "{\"a_byte\" : \"97\", \"b_byte\": {\"smth\": \"something\", \"value\": " \
  "\"98\"}, \"c_byte\": \"99\", \"d_byte\": "                               \
  "\"100\"}"

/* from https://json.org/example.html */
// clang-format off
#define JSON_EXAMPLE                                                      \
  "{"                                                                     \
  "    \"glossary\": {"                                                   \
  "        \"title\": \"example glossary\","                              \
  "		\"GlossDiv\": {"                                                    \
  "            \"title\": \"S\","                                         \
  "			\"GlossList\": {"                                                 \
  "                \"GlossEntry\": {"                                     \
  "                    \"ID\": \"SGML\","                                 \
  "					\"SortAs\": \"SGML\","                                        \
  "					\"GlossTerm\": \"Standard "                                   \
  "Generalized Markup Language\","                                        \
  "					\"Acronym\": \"SGML\","                                       \
  "					\"Abbrev\": \"ISO 8879:1986\","                               \
  "					\"GlossDef\": {"                                              \
  "                        \"para\": \"A meta-markup language, used to "  \
  "create markup languages such as DocBook.\","                           \
  "						\"GlossSeeAlso\": [\"GML\", "                               \
  "\"XML\"]"                                                              \
  "                    },"                                                \
  "					\"GlossSee\": \"markup\""                                     \
  "                }"                                                     \
  "            }"                                                         \
  "        }"                                                             \
  "    }"                                                                 \
  "}"
// clang-format on

typedef enum {
  JSON_VALUE_UNKNOWN,
  JSON_VALUE_NULL,
  JSON_VALUE_STRING,
  JSON_VALUE_INT,
  JSON_VALUE_BOOL,
  JSON_VALUE_ARR,
  JSON_VALUE_OBJECT,
} JsonValueType;

typedef struct {
  void* value;
  JsonValueType value_type;
} JsonObject;

typedef enum {
  NONE,
  INIT,
  EXPECT_ATTR,
  IN_ATTR,
  EXPECT_SEP,
  IN_OBJECT,
  EXPECT_VALUE,
  IN_STRING,
  IN_ESCAPE,
  IN_ARRAY,
  GOT_VALUE,
} JsonParserState;

typedef struct {
  char attr[JSON_MAX_STR_LEN];
  char parent_attr[JSON_MAX_STR_LEN];
  unsigned int depth;
} JsonParserRule;

Map* json_parse_object(char**, const char**);

#endif
