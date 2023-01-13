/* Author:			Sergey Simonenko <gforgx@protonmail.com>
 * SPDX-License-Identifier:	0BSD */

#include "json.h"

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "map.h"
#include "slice.h"
#include "util.h"

extern int enable_debug;
static char* json_parser_states[] = {
    "NONE",       "INIT",      "EXPECT_ATTR",  "IN_ATTR",
    "EXPECT_SEP", "IN_OBJECT", "EXPECT_VALUE", "IN_STRING",
    "IN_ESCAPE",  "IN_ARRAY",  "GOT_VALUE"};

/* http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2Float */
#define NEXT_POW_TWO(v) \
  v--;                  \
  v |= v >> 1;          \
  v |= v >> 2;          \
  v |= v >> 4;          \
  v |= v >> 8;          \
  v |= v >> 16;         \
  v++;

// debugf("Skipping garbage");
#define SKIP_GARBAGE(c)                       \
  if (isspace(c) || '\n' == c || '\t' == c) { \
    continue;                                 \
  }

#define SET_ERRNO(code) \
  if (0 == errno) {     \
    errno = code;       \
  }

#define CHANGE_STATE(s) \
  { state = s; }

/*
 * This function is used to parse JSON attributes and string values, and expects
 * NUL-terminated input array not longer than JSON_MAX_STR_LEN, without leading
 * double quotation, and will terminate parsing upon meeting next one unless
 * it's escaped, therefore passing input with leading double quotation will
 * result in JSON_ERR_BADSTRING. The data will be written into outbuf which the
 * caller should allocate, the cursor will be overwritten to new position
 */
static inline int json_parse_string(char** in, char* outbuf) {
  debugf("Entered %s()", __func__);

  char* cp = *in;
  char* nextchar;
  unsigned int idx = 0;
  bool in_escape = false;

  // FIXME: add support for \n, \t, \b (seems to be DONE)
  for (; 0 != *cp; cp++) {
    debugf("Iterating string, saw %c at %p", *cp, cp);
    switch (*cp) {
      case JSON_ESCAPE:
        nextchar = cp + 1;
        if (!in_escape &&
            (*nextchar != 'n' && *nextchar != 'b' && *nextchar != 't')) {
          in_escape = true;
          continue;
        } else {
          if (*nextchar == 'n')
            outbuf[idx++] = '\n';
          else if (*nextchar == 't')
            outbuf[idx++] = '\t';
          else
            outbuf[idx++] = '\b';

          outbuf[idx] = 0;
          cp++;
          break;
        }

      case JSON_STRING_END:
        if (!in_escape) goto out;

      default:
        if (in_escape) in_escape = false;

        if (idx == JSON_MAX_STR_LEN) return JSON_ERR_STRLONG;

        outbuf[idx++] = *cp;
        outbuf[idx] = 0;
        break;
    }
  }

  if (0 == idx || JSON_STRING_END != *cp) return JSON_ERR_BADSTRING;

out:
  *in = cp;

  debugf("Returning from %s()", __func__);
  return 0;
}

typedef struct {
  JsonValueType type;
  union {
    int64_t value_int;
    bool value_bool;
  } value;
} JsonToken;

static inline int json_parse_token(char** in, JsonToken* out) {
  debugf("Entered %s()", __func__);

  char* begin = *in;
  char* cp = *in;
  char* end = NULL;

  for (; 0 != *cp; cp++) {
    debugf("Iterating token, saw %c at %p", *cp, cp);
    if (JSON_DELIM == *cp || JSON_OBJECT_END == *cp || ' ' == *cp ||
        '\t' == *cp || '\n' == *cp) {
      end = cp;
      break;
    }
  }

  // Give up here
  if (!end) {
    debugf("Couldn't find anything like token end");
    return JSON_ERR_TOKLONG;
  }

  size_t len = end - begin;
  debugf("len is: %zu", len);

  out->type = JSON_VALUE_UNKNOWN;

  if (5 == len && begin[0] == 'f' && begin[1] == 'a' && begin[2] == 'l' &&
      begin[3] == 's' && begin[4] == 'e') {
    out->type = JSON_VALUE_BOOL;
    out->value.value_bool = false;
    debugf("Got false token");
    goto out;
  }

  if (4 == len) {
    if (begin[0] == 'n' && begin[1] == 'u' && begin[2] == 'l' &&
        begin[3] == 'l') {
      out->type = JSON_VALUE_NULL;
      debugf("Got null token");
      goto out;
    }

    if (begin[0] == 't' && begin[1] == 'r' && begin[2] == 'u' &&
        begin[3] == 'e') {
      out->type = JSON_VALUE_BOOL;
      out->value.value_bool = true;
      debugf("Got true token");
      goto out;
    }
  }

  char* first_invalid;
  out->value.value_int = strtoll(cp, &first_invalid, 10);

  if (0 != errno || first_invalid != end) return JSON_ERR_BADNUM;

  debugf("Got numeric token");
  out->type = JSON_VALUE_INT;

out:
  *in = end;

  debugf("Returning from %s()", __func__);
  return 0;
}

void* json_token_copy_value(JsonToken* tok) {
  if (JSON_VALUE_BOOL == tok->type) {
    bool* bool_ret = malloc_or_die(sizeof(bool));
    *bool_ret = tok->value.value_bool;
    return (void*)bool_ret;
  }

  if (JSON_VALUE_INT == tok->type) {
    int* int_ret = malloc_or_die(sizeof(int64_t));
    *int_ret = tok->value.value_int;
    return (void*)int_ret;
  }

  return NULL;
}

static int json_parse_array(char** in, Slice* out) {
  JsonValueType last_value_type = JSON_VALUE_UNKNOWN;
  JsonValueType value_type = JSON_VALUE_UNKNOWN;
  char current_value[JSON_MAX_STR_LEN];
  Map* child_map = NULL;
  int err;

  debugf("Entered %s()", __func__);

  if (JSON_ARRAY_BEGIN != **in) return JSON_ERR_NOARRAY;

  char* cp = *in;
  cp++;

  for (; 0 != *cp && 0 != *cp; cp++) {
    if ('\n' == *cp)
      debugf("Iterating array, saw newline at %p", cp);
    else
      debugf("Iterating array, saw %c at %p", *cp, cp);

    SKIP_GARBAGE(*cp);

    /* FIXME: disallow storing different types of array members within array,
     * produce an error until I introduce JsonObject wrapper */
    switch (*cp) {
      case JSON_OBJECT_BEGIN:
        debugf("Have object!");
        child_map = json_parse_object(&cp, NULL);
        slice_append(out, child_map);
        debugf("Resuming parsing array");
        break;
      case JSON_STRING_BEGIN:
        debugf("Have string!");
        current_value[0] = 0;
        cp++;
        if (0 != (err = json_parse_string(&cp, current_value))) {
          err = JSON_ERR_BADSTRING;
          goto error;
        }
        slice_append(out, strdup(current_value));
        debugf("Resuming parsing array, after putting %s", current_value);
        break;
      case JSON_ARRAY_END:
        goto out;

      default:
        break;
    }
  }

out:
  debugf("Collected values:\n");
  for (size_t i = 0; i < out->len; i++)
    debugf("Collected array value: %s", (char*)out->data[i]);

  *in = cp;
  return 0;
error:
  // FIXME: clean all collected resources
  return err;
}

/*
 * This function is implemented as a recursive state-machine and parses the
 * whole JSON document into allocated memory. Auxillary functions to parse
 * strings, tokens and arrays do not perform any memory allocation. Root object
 * returned is a hashmap; arrays are serialized into pointer slices strings, and
 * simpler scalar JSON objects are duplicated or malloc'd. Keys (attributes) are
 * maintained by hashmap and should not be freed manually, the responsibility to
 * free any values is on the caller
 * TODO: simpler interface like json_parse() must be provided as public.
 */
Map* json_parse_object(char** rawdata, const char** collected_attrs) {
  debugf("Entered %s()", __func__);
  JsonParserState state = INIT;
  size_t cap = 0;
  int err;
  char current_attr[JSON_MAX_STR_LEN];
  char current_value[JSON_MAX_STR_LEN];
  Map* this_map = NULL;
  Map* child = NULL;
  void* token_copy;

  if (!rawdata || !(*rawdata)) {
    err = JSON_ERR_NULLPTR;
    goto out;
  }

  if (collected_attrs) {
    const char* p = collected_attrs[0];
    int i = 0;
    while (p) {
      cap++;
      p = collected_attrs[i++];
    }

    NEXT_POW_TWO(cap);
  } else
    cap = 16;

  this_map = map_new(cap);

  char* cp = *rawdata;
  for (; '\0' != *cp; cp++) {
    if ('\n' == *cp)
      debugf("Iterating, state is %s, saw newline at %p",
             json_parser_states[state], cp);
    else
      debugf("Iterating, state is: %s, saw %c at %p", json_parser_states[state],
             *cp, cp);

    switch (state) {
      case INIT:
        SKIP_GARBAGE(*cp);

        if (JSON_OBJECT_BEGIN == *cp)
          state = EXPECT_ATTR;
        else
          goto error;

        break;

      case EXPECT_ATTR:
        debugf("Got back to EXPECT_ATTR");
        SKIP_GARBAGE(*cp);

        if (JSON_DELIM == *cp) {
          debugf("Saw comma, waiting for next attribute");
          continue;
        } else if (JSON_ATTR_BEGIN == *cp) {
          current_attr[0] = 0;
          cp++;
          if (0 != (err = json_parse_string(&cp, current_attr))) {
            err = JSON_ERR_BADATTR;
            goto error;
          }

          debugf("Attribute is: \"%s\"", current_attr);
          state = EXPECT_SEP;
        } else if (JSON_OBJECT_END == *cp) {
          debugf("Going to out!");
          goto out;
        } else {
          err = JSON_ERR_ATTRSTART;
          goto error;
        }

        break;

      case EXPECT_SEP:
        SKIP_GARBAGE(*cp);

        if (JSON_SEPARATOR != *cp) {
          printf("Dying here\n");
          err = JSON_ERR_NOSEP;
          goto error;
        }

        current_value[0] = 0;
        state = EXPECT_VALUE;
        break;

      case EXPECT_VALUE:
        SKIP_GARBAGE(*cp);

        if (JSON_OBJECT_BEGIN == *cp) {
          child = json_parse_object(&cp, NULL);

          if (!child) debugf("Child is NULL!");

          map_put(this_map, current_attr, child);
          debugf("Resuming parsing");
          state = EXPECT_ATTR;

          continue;
        } else if (JSON_STRING_BEGIN == *cp) {
          cp++;
          if (0 != (err = json_parse_string(&cp, current_value))) goto error;

          debugf("Value for \"%s\" is: \"%s\"", current_attr, current_value);
          map_put(this_map, current_attr, strdup(current_value));
          current_value[0] = 0;

          state = EXPECT_ATTR;
        } else if (JSON_ARRAY_BEGIN == *cp) {
          Slice* slice = slice_new(16);
          json_parse_array(&cp, slice);

          map_put(this_map, current_attr, slice);

          CHANGE_STATE(EXPECT_ATTR);
          continue;
        } else {
          JsonToken tok;
          if (0 != (err = json_parse_token(&cp, &tok))) {
            goto error;
          }
          if (!(token_copy = json_token_copy_value(&tok))) goto error;

          map_put(this_map, current_attr, token_copy);
          state = EXPECT_ATTR;
        }

        break;

      default:
        break;
    }
  }

out:
  *rawdata = cp;
  return this_map;
error:
  debugf("Error!");
  return NULL;
}

#undef SKIP_GARBAGE
#undef NEXT_POW_TWO
#undef CHANGE_STATE
#undef SET_ERRNO
