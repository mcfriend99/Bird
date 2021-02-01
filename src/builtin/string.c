#include "builtin/string.h"
#include "util.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * a Bird regex must always start and end with the same delimiter e.g. /
 *
 * e.g.
 * /\d+/
 *
 * it can be followed by one or more matching fine tuning constants
 *
 * e.g.
 *
 * /\d+.+[a-z]+/sim -> '.' matches all, it's case insensitive and multiline
 * (see the function for list of available options)
 *
 * returns:
 * -1 -> false
 * 0 -> true
 * negative value -> invalid delimiter where abs(value) is the character
 * positive value > 0 ? for compiled delimeters
 */
int is_regex(b_obj_string *string) {
  char start = string->chars[0];
  bool match_found = false;

  uint32_t coptions = 0; // pcre2 options

  int i;
  for (i = 1; i < string->length; i++) {
    if (string->chars[i] == start) {
      match_found = true;
      continue;
    }

    if (match_found) {
      // compile the delimiters
      switch (string->chars[i]) {
      /* Perl compatible options */
      case 'i':
        coptions |= PCRE2_CASELESS;
        break;
      case 'm':
        coptions |= PCRE2_MULTILINE;
        break;
      case 's':
        coptions |= PCRE2_DOTALL;
        break;
      case 'x':
        coptions |= PCRE2_EXTENDED;
        break;

      /* PCRE specific options */
      case 'A':
        coptions |= PCRE2_ANCHORED;
        break;
      case 'D':
        coptions |= PCRE2_DOLLAR_ENDONLY;
        break;
      case 'S': /* Pass. */
        break;
      case 'X': /* Pass. */
        break;
      case 'U':
        coptions |= PCRE2_UNGREEDY;
        break;
      case 'u':
        coptions |= PCRE2_UTF;
        /* In  PCRE,  by  default, \d, \D, \s, \S, \w, and \W recognize only
       ASCII characters, even in UTF-8 mode. However, this can be changed by
       setting the PCRE2_UCP option. */
#ifdef PCRE2_UCP
        coptions |= PCRE2_UCP;
#endif
        break;
      case 'J':
        coptions |= PCRE2_DUPNAMES;
        break;

      case ' ':
      case '\n':
      case '\r':
        break;

      default:
        return coptions = -string->chars[i];
      }
    }
  }

  if (!match_found)
    return -1;
  else
    return coptions;
}

char *remove_regex_delimiter(b_vm *vm, b_obj_string *string) {
  if (string->length == 0)
    return string->chars;

  char start = string->chars[0];
  int i = string->length - 1;
  for (; i > 0; i--) {
    if (string->chars[i] == start)
      break;
  }

  char *str = ALLOCATE(char, i);
  memcpy(str, string->chars + 1, i - 1);
  str[i - 1] = '\0';

  return str;
}

DECLARE_STRING_METHOD(length) {
  ENFORCE_ARG_COUNT(length, 0);
  RETURN_NUMBER(AS_STRING(METHOD_OBJECT)->length);
}

DECLARE_STRING_METHOD(upper) {
  ENFORCE_ARG_COUNT(upper, 0);
  char *string = (char *)AS_CSTRING(METHOD_OBJECT);
  for (char *p = string; *p; p++)
    *p = toupper(*p);
  RETURN_TSTRING(string, AS_STRING(METHOD_OBJECT)->length);
}

DECLARE_STRING_METHOD(lower) {
  ENFORCE_ARG_COUNT(lower, 0);
  char *string = (char *)AS_CSTRING(METHOD_OBJECT);
  for (char *p = string; *p; p++)
    *p = tolower(*p);
  RETURN_TSTRING(string, AS_STRING(METHOD_OBJECT)->length);
}

DECLARE_STRING_METHOD(is_alpha) {
  ENFORCE_ARG_COUNT(is_alpha, 0);
  b_obj_string *string = AS_STRING(METHOD_OBJECT);
  for (int i = 0; i < string->length; i++) {
    if (!isalpha((unsigned char)string->chars[i]))
      RETURN_FALSE;
  }
  RETURN_TRUE;
}

DECLARE_STRING_METHOD(is_alnum) {
  ENFORCE_ARG_COUNT(is_alnum, 0);
  b_obj_string *string = AS_STRING(METHOD_OBJECT);
  for (int i = 0; i < string->length; i++) {
    if (!isalnum((unsigned char)string->chars[i]))
      RETURN_FALSE;
  }
  RETURN_TRUE;
}

DECLARE_STRING_METHOD(is_number) {
  ENFORCE_ARG_COUNT(is_number, 0);
  b_obj_string *string = AS_STRING(METHOD_OBJECT);
  for (int i = 0; i < string->length; i++) {
    if (!isdigit((unsigned char)string->chars[i]))
      RETURN_FALSE;
  }
  RETURN_TRUE;
}

DECLARE_STRING_METHOD(is_lower) {
  ENFORCE_ARG_COUNT(is_lower, 0);
  b_obj_string *string = AS_STRING(METHOD_OBJECT);
  for (int i = 0; i < string->length; i++) {
    if (!islower((unsigned char)string->chars[i]))
      RETURN_FALSE;
  }
  RETURN_TRUE;
}

DECLARE_STRING_METHOD(is_upper) {
  ENFORCE_ARG_COUNT(is_upper, 0);
  b_obj_string *string = AS_STRING(METHOD_OBJECT);
  for (int i = 0; i < string->length; i++) {
    if (!isupper((unsigned char)string->chars[i]))
      RETURN_FALSE;
  }
  RETURN_TRUE;
}

DECLARE_STRING_METHOD(is_space) {
  ENFORCE_ARG_COUNT(is_space, 0);
  b_obj_string *string = AS_STRING(METHOD_OBJECT);
  for (int i = 0; i < string->length; i++) {
    if (!isspace((unsigned char)string->chars[i]))
      RETURN_FALSE;
  }
  RETURN_TRUE;
}

DECLARE_STRING_METHOD(trim) {
  ENFORCE_ARG_RANGE(trim, 0, 1);

  char trimmer = '\0';

  if (arg_count == 1) {
    ENFORCE_ARG_TYPE(trim, 0, IS_CHAR);
    trimmer = (char)AS_STRING(args[0])->chars[0];
  }

  char *string = AS_CSTRING(METHOD_OBJECT);

  char *end;

  // Trim leading space
  if (trimmer == '\0') {
    while (isspace((unsigned char)*string))
      string++;
  } else {
    while (trimmer == *string)
      string++;
  }

  if (*string == 0) // All spaces?
    RETURN_OBJ(copy_string(vm, "", 0));

  // Trim trailing space
  end = string + strlen(string) - 1;
  if (trimmer == '\0') {
    while (end > string && isspace((unsigned char)*end))
      end--;
  } else {
    while (end > string && trimmer == *end)
      end--;
  }

  // Write new null terminator character
  end[1] = '\0';

  RETURN_STRING(string);
}

DECLARE_STRING_METHOD(ltrim) {
  ENFORCE_ARG_RANGE(ltrim, 0, 1);

  char trimmer = '\0';

  if (arg_count == 1) {
    ENFORCE_ARG_TYPE(ltrim, 0, IS_CHAR);
    trimmer = (char)AS_STRING(args[0])->chars[0];
  }

  char *string = AS_CSTRING(METHOD_OBJECT);

  char *end;

  // Trim leading space
  if (trimmer == '\0') {
    while (isspace((unsigned char)*string))
      string++;
  } else {
    while (trimmer == *string)
      string++;
  }

  if (*string == 0) // All spaces?
    RETURN_OBJ(copy_string(vm, "", 0));

  end = string + strlen(string) - 1;

  // Write new null terminator character
  end[1] = '\0';

  RETURN_STRING(string);
}

DECLARE_STRING_METHOD(rtrim) {
  ENFORCE_ARG_RANGE(rtrim, 0, 1);

  char trimmer = '\0';

  if (arg_count == 1) {
    ENFORCE_ARG_TYPE(rtrim, 0, IS_CHAR);
    trimmer = (char)AS_STRING(args[0])->chars[0];
  }

  char *string = AS_CSTRING(METHOD_OBJECT);

  char *end;

  if (*string == 0) // All spaces?
    RETURN_OBJ(copy_string(vm, "", 0));

  end = string + strlen(string) - 1;
  if (trimmer == '\0') {
    while (end > string && isspace((unsigned char)*end))
      end--;
  } else {
    while (end > string && trimmer == *end)
      end--;
  }

  // Write new null terminator character
  end[1] = '\0';

  RETURN_STRING(string);
}

DECLARE_STRING_METHOD(join) {
  ENFORCE_ARG_COUNT(join, 1);
  ENFORCE_ARG_TYPE(join, 0, IS_OBJ);

  b_value argument = args[0];
  int length = 0;
  char **array = NULL;

  if (IS_STRING(argument)) {
    // empty argument
    if (AS_STRING(argument)->length == 0)
      RETURN_VALUE(argument);

    char *string = (char *)AS_CSTRING(argument);
    length = AS_STRING(argument)->length;
    array = (char **)calloc(length, sizeof(char **));

    for (int i = 0; i < length; i++) {
      array[i] = (char *)calloc(2, sizeof(char *));
      char str[2] = {string[i], '\0'};
      strcpy(array[i], str);
    }
  } else if (IS_LIST(argument) || IS_DICT(argument)) {

    length = IS_LIST(argument) ? AS_LIST(argument)->items.count
                               : AS_DICT(argument)->names.count;
    b_value *values = IS_LIST(argument) ? AS_LIST(argument)->items.values
                                        : AS_DICT(argument)->names.values;

    if (length == 0)
      RETURN_VALUE(argument);

    array = (char **)calloc(length, sizeof(char **));

    for (int i = 0; i < length; i++) {
      // get interpreted string here
      array[i] = value_to_string(vm, values[i]);
    }
  } else {
    RETURN_ERROR("join() does not support object of type %s",
                 value_type(argument))
  }

  for (int i = 0; i < length; i++) {
    if (i != 0)
      array[0] = append_strings(array[0], array[1]);

    if (i != length - 1)
      array[0] = append_strings(array[0], AS_CSTRING(METHOD_OBJECT));
  }

  RETURN_STRING(array[0]);
}

DECLARE_STRING_METHOD(split) {
  ENFORCE_ARG_COUNT(split, 1);
  ENFORCE_ARG_TYPE(split, 0, IS_STRING);

  b_obj_list *list = new_list(vm);

  if (AS_STRING(METHOD_OBJECT)->length == 0)
    return OBJ_VAL(list);

  // main work here...
  if (AS_STRING(args[0])->length > 0) {
    char *token, *str, *tofree;

    tofree = str =
        strdup(AS_CSTRING(METHOD_OBJECT)); // We own str's memory now.
    while ((token = strsep(&str, AS_CSTRING(args[0]))))
      write_list(vm, list, OBJ_VAL(copy_string(vm, token, strlen(token))));
    free(tofree);
  } else {
    const char *string = AS_CSTRING(METHOD_OBJECT);
    for (int i = 0; i < (int)strlen(string); i++) {
      write_list(vm, list, OBJ_VAL(copy_string(vm, &string[i], 1)));
    }
  }

  RETURN_OBJ(list);
}

DECLARE_STRING_METHOD(index_of) {
  ENFORCE_ARG_COUNT(index_of, 1);
  ENFORCE_ARG_TYPE(index_of, 0, IS_STRING);

  char *str = AS_CSTRING(METHOD_OBJECT);
  char *result = strstr(str, AS_CSTRING(args[0]));

  RETURN_NUMBER(result - str);
}

DECLARE_STRING_METHOD(starts_with) {
  ENFORCE_ARG_COUNT(starts_with, 1);
  ENFORCE_ARG_TYPE(starts_with, 0, IS_STRING);

  b_obj_string *string = AS_STRING(METHOD_OBJECT);
  b_obj_string *substr = AS_STRING(args[0]);

  if (string->length == 0 || substr->length == 0 ||
      substr->length > string->length)
    RETURN_FALSE;

  RETURN_BOOL(memcmp(substr->chars, string->chars, substr->length) == 0);
}

DECLARE_STRING_METHOD(ends_with) {
  ENFORCE_ARG_COUNT(ends_with, 1);
  ENFORCE_ARG_TYPE(ends_with, 0, IS_STRING);

  b_obj_string *string = AS_STRING(METHOD_OBJECT);
  b_obj_string *substr = AS_STRING(args[0]);

  if (string->length == 0 || substr->length == 0 ||
      substr->length > string->length)
    RETURN_FALSE;

  int difference = string->length - substr->length;

  RETURN_BOOL(
      memcmp(substr->chars, string->chars + difference, substr->length) == 0);
}

DECLARE_STRING_METHOD(count) {
  ENFORCE_ARG_COUNT(count, 1);
  ENFORCE_ARG_TYPE(count, 0, IS_STRING);

  b_obj_string *string = AS_STRING(METHOD_OBJECT);
  b_obj_string *substr = AS_STRING(args[0]);

  if (substr->length == 0 || string->length == 0)
    RETURN_NUMBER(0);

  int count = 0;
  const char *tmp = string->chars;
  while ((tmp = strstr(tmp, substr->chars))) {
    count++;
    tmp++;
  }

  RETURN_NUMBER(count);
}

DECLARE_STRING_METHOD(to_number) {
  ENFORCE_ARG_COUNT(to_number, 0);
  RETURN_NUMBER(strtod(AS_CSTRING(METHOD_OBJECT), NULL));
}

DECLARE_STRING_METHOD(to_list) {
  ENFORCE_ARG_COUNT(to_list, 0);
  b_obj_string *string = AS_STRING(METHOD_OBJECT);
  b_obj_list *list = new_list(vm);

  if (string->length > 0) {

    for (int i = 0; i < string->length; i++) {
      char *ch = &string->chars[i];
      write_list(vm, list, OBJ_VAL(copy_string(vm, ch, 1)));
    }
  }

  RETURN_OBJ(list);
}

DECLARE_STRING_METHOD(lpad) {
  ENFORCE_ARG_RANGE(lpad, 1, 2);
  ENFORCE_ARG_TYPE(lpad, 0, IS_NUMBER);

  b_obj_string *string = AS_STRING(METHOD_OBJECT);
  int width = AS_NUMBER(args[0]);
  char fill_char = ' ';

  if (arg_count == 2) {
    ENFORCE_ARG_TYPE(lpad, 1, IS_CHAR);
    fill_char = AS_CSTRING(args[1])[0];
  }

  if (width <= string->length)
    RETURN_VALUE(METHOD_OBJECT);

  int fill_size = width - string->length;
  char fill[fill_size];

  int i;
  for (i = 0; i < fill_size; i++)
    fill[i] = fill_char;

  char *str = ALLOCATE(char, string->length + fill_size + 1);
  memcpy(str, fill, fill_size);
  memcpy(str + fill_size, string->chars, string->length);
  str[string->length + fill_size] = '\0';

  RETURN_OBJ(copy_string(vm, str, string->length + fill_size));
}

DECLARE_STRING_METHOD(rpad) {
  ENFORCE_ARG_RANGE(rpad, 1, 2);
  ENFORCE_ARG_TYPE(rpad, 0, IS_NUMBER);

  b_obj_string *string = AS_STRING(METHOD_OBJECT);
  int width = AS_NUMBER(args[0]);
  char fill_char = ' ';

  if (arg_count == 2) {
    ENFORCE_ARG_TYPE(rpad, 1, IS_CHAR);
    fill_char = AS_CSTRING(args[1])[0];
  }

  if (width <= string->length)
    RETURN_VALUE(METHOD_OBJECT);

  int fill_size = width - string->length;
  char fill[fill_size];

  int i;
  for (i = 0; i < fill_size; i++)
    fill[i] = fill_char;

  char *str = ALLOCATE(char, string->length + fill_size + 1);
  memcpy(str, string->chars, string->length);
  memcpy(str + string->length, fill, fill_size);
  str[string->length + fill_size] = '\0';

  RETURN_OBJ(copy_string(vm, str, string->length + fill_size));
}

DECLARE_STRING_METHOD(match) {
  ENFORCE_ARG_COUNT(match, 1);
  ENFORCE_ARG_TYPE(match, 0, IS_STRING);

  b_obj_string *string = AS_STRING(METHOD_OBJECT);
  b_obj_string *substr = AS_STRING(args[0]);

  if (string->length == 0 && substr->length == 0) {
    RETURN_TRUE;
  } else if (string->length == 0 || substr->length == 0) {
    RETURN_FALSE;
  }

  GET_REGEX_COMPILE_OPTIONS(match, substr, false);

  if ((int)compile_options < 0) {
    RETURN_BOOL(strstr(string->chars, substr->chars) - string->chars > -1);
  }

  char *real_regex = remove_regex_delimiter(vm, substr);

  int error_number;
  PCRE2_SIZE error_offset;

  PCRE2_SPTR pattern = (PCRE2_SPTR)real_regex;
  PCRE2_SPTR subject = (PCRE2_SPTR)string->chars;
  PCRE2_SIZE subject_length = (PCRE2_SIZE)string->length;

  pcre2_code *re =
      pcre2_compile(pattern, PCRE2_ZERO_TERMINATED, compile_options,
                    &error_number, &error_offset, NULL);

  REGEX_COMPILATION_ERROR(re, error_number, error_offset);

  pcre2_match_data *match_data = pcre2_match_data_create_from_pattern(re, NULL);

  int rc = pcre2_match(re, subject, subject_length, 0, 0, match_data, NULL);

  if (rc < 0) {
    switch (rc) {
    case PCRE2_ERROR_NOMATCH:
      RETURN_FALSE;
      break;

    default:
      REGEX_RC_ERROR();
    }
  }

  PCRE2_SIZE *ovector = pcre2_get_ovector_pointer(match_data);
  uint32_t name_count;
  uint32_t name_entry_size;
  PCRE2_SPTR name_table;

  b_obj_list *result = new_list(vm);
  (void)pcre2_pattern_info(re, PCRE2_INFO_NAMECOUNT, &name_count);

  if (name_count == 0) {
    for (int i = 0; i < rc; i++) {
      PCRE2_SIZE substring_length = ovector[2 * i + 1] - ovector[2 * i];
      if (substring_length > 0) {
        PCRE2_SPTR substring_start = subject + ovector[2 * i];
        write_list(vm, result,
                   OBJ_VAL(copy_string(vm, (char *)substring_start,
                                       (int)substring_length)));
      }
    }
  } else {

    b_obj_list *match_list = new_list(vm);
    b_obj_dict *match_dict = new_dict(vm);

    for (int i = 0; i < rc; i++) {
      PCRE2_SIZE substring_length = ovector[2 * i + 1] - ovector[2 * i];
      if (substring_length > 0) {
        PCRE2_SPTR substring_start = subject + ovector[2 * i];
        write_value_arr(vm, &match_list->items,
                        OBJ_VAL(copy_string(vm, (char *)substring_start,
                                            (int)substring_length)));
      }
    }

    PCRE2_SPTR tabptr;

    (void)pcre2_pattern_info(re, PCRE2_INFO_NAMETABLE, &name_table);
    (void)pcre2_pattern_info(re, PCRE2_INFO_NAMEENTRYSIZE, &name_entry_size);

    tabptr = name_table;

    for (int i = 0; i < (int)name_count; i++) {
      int n = (tabptr[0] << 8) | tabptr[1];

      char *_key = malloc(sizeof(char *));
      char *_val = malloc(sizeof(char *));
      sprintf(_key, "%*s", name_entry_size - 3, tabptr + 2);
      sprintf(_val, "%*s", (int)(ovector[2 * n + 1] - ovector[2 * n]),
              subject + ovector[2 * n]);

      while (isspace((unsigned char)*_key))
        _key++;

      dict_add_entry(
          vm, match_dict, OBJ_VAL(copy_string(vm, _key, name_entry_size - 3)),
          OBJ_VAL(copy_string(vm, _val,
                              (int)(ovector[2 * n + 1] - ovector[2 * n]))));

      tabptr += name_entry_size;
    }

    write_value_arr(vm, &match_list->items, OBJ_VAL(match_dict));
    write_list(vm, result, OBJ_VAL(match_list));
  }

  pcre2_match_data_free(match_data);
  pcre2_code_free(re);

  RETURN_OBJ(result);
}

DECLARE_STRING_METHOD(matches) {
  ENFORCE_ARG_COUNT(matches, 1);
  ENFORCE_ARG_TYPE(matches, 0, IS_STRING);

  b_obj_list *result = new_list(vm);

  b_obj_string *string = AS_STRING(METHOD_OBJECT);
  b_obj_string *substr = AS_STRING(args[0]);

  if (string->length == 0 && substr->length == 0) {
    RETURN_OBJ(result); // empty string matches empty string to empty list
  } else if (string->length == 0 || substr->length == 0) {
    RETURN_FALSE; // if either string or str is empty, return false
  }

  GET_REGEX_COMPILE_OPTIONS(matches, substr, true);

  char *real_regex = remove_regex_delimiter(vm, substr);

  int error_number;
  PCRE2_SIZE error_offset;
  uint32_t option_bits;
  uint32_t newline;
  uint32_t name_count;
  uint32_t name_entry_size;
  PCRE2_SPTR name_table;

  PCRE2_SPTR pattern = (PCRE2_SPTR)real_regex;
  PCRE2_SPTR subject = (PCRE2_SPTR)string->chars;
  PCRE2_SIZE subject_length = (PCRE2_SIZE)string->length;

  pcre2_code *re = pcre2_compile(pattern, PCRE2_ZERO_TERMINATED, 0,
                                 &error_number, &error_offset, NULL);

  REGEX_COMPILATION_ERROR(re, error_number, error_offset);

  pcre2_match_data *match_data = pcre2_match_data_create_from_pattern(re, NULL);

  int rc = pcre2_match(re, subject, subject_length, 0, 0, match_data, NULL);

  if (rc < 0) {
    switch (rc) {
    case PCRE2_ERROR_NOMATCH:
      RETURN_FALSE;
      break;
    default:
      REGEX_RC_ERROR();
    }
  }

  PCRE2_SIZE *ovector = pcre2_get_ovector_pointer(match_data);

  // REGEX_VECTOR_SIZE_WARNING();

  // handle edge cases such as /(?=.\K)/
  REGEX_ASSERTION_ERROR(re, match_data, ovector);

  (void)pcre2_pattern_info(re, PCRE2_INFO_NAMECOUNT, &name_count);

  // add first set of matches to response
  if (name_count == 0) {
    int i;
    for (i = 0; i < rc; i++) {
      PCRE2_SIZE substring_length = ovector[2 * i + 1] - ovector[2 * i];
      if (substring_length > 0) {
        PCRE2_SPTR substring_start = subject + ovector[2 * i];
        write_list(vm, result,
                   OBJ_VAL(copy_string(vm, (char *)substring_start,
                                       (int)substring_length)));
      }
    }
  } else {

    b_obj_list *match_list = new_list(vm);
    b_obj_dict *match_dict = new_dict(vm);

    int i;
    for (i = 0; i < rc; i++) {
      PCRE2_SIZE substring_length = ovector[2 * i + 1] - ovector[2 * i];
      if (substring_length > 0) {
        PCRE2_SPTR substring_start = subject + ovector[2 * i];
        write_value_arr(vm, &match_list->items,
                        OBJ_VAL(copy_string(vm, (char *)substring_start,
                                            (int)substring_length)));
      }
    }

    PCRE2_SPTR tabptr;

    (void)pcre2_pattern_info(re, PCRE2_INFO_NAMETABLE, &name_table);

    (void)pcre2_pattern_info(re, PCRE2_INFO_NAMEENTRYSIZE, &name_entry_size);

    tabptr = name_table;

    for (i = 0; i < (int)name_count; i++) {
      int n = (tabptr[0] << 8) | tabptr[1];

      char *_key = malloc(sizeof(char *));
      char *_val = malloc(sizeof(char *));
      sprintf(_key, "%*s", name_entry_size - 3, tabptr + 2);
      sprintf(_val, "%*s", (int)(ovector[2 * n + 1] - ovector[2 * n]),
              subject + ovector[2 * n]);

      while (isspace((unsigned char)*_key))
        _key++;

      dict_add_entry(
          vm, match_dict, OBJ_VAL(copy_string(vm, _key, name_entry_size - 3)),
          OBJ_VAL(copy_string(vm, _val,
                              (int)(ovector[2 * n + 1] - ovector[2 * n]))));

      tabptr += name_entry_size;
    }

    write_value_arr(vm, &match_list->items, OBJ_VAL(match_dict));
    write_list(vm, result, OBJ_VAL(match_list));
  }

  (void)pcre2_pattern_info(re, PCRE2_INFO_ALLOPTIONS, &option_bits);
  int utf8 = (option_bits & PCRE2_UTF) != 0;

  (void)pcre2_pattern_info(re, PCRE2_INFO_NEWLINE, &newline);
  int crlf_is_newline = newline == PCRE2_NEWLINE_ANY ||
                        newline == PCRE2_NEWLINE_CRLF ||
                        newline == PCRE2_NEWLINE_ANYCRLF;

  // find the other matchs
  for (;;) {
    uint32_t options = 0;
    PCRE2_SIZE start_offset = ovector[1];

    // if the previous match was for an empty string
    if (ovector[0] == ovector[1]) {
      if (ovector[0] == subject_length)
        break;
      options = PCRE2_NOTEMPTY_ATSTART | PCRE2_ANCHORED;
    } else {
      PCRE2_SIZE startchar = pcre2_get_startchar(match_data);
      if (start_offset <= startchar) {
        if (startchar >= subject_length)
          break;
        start_offset = startchar + 1;
        if (utf8) {
          for (; start_offset < subject_length; start_offset++)
            if ((subject[start_offset] & 0xc0) != 0x80)
              break;
        }
      }
    }

    rc = pcre2_match(re, subject, subject_length, start_offset, options,
                     match_data, NULL);

    if (rc == PCRE2_ERROR_NOMATCH) {
      if (options == 0)
        break;
      ovector[1] = start_offset + 1;
      if (crlf_is_newline && start_offset < subject_length - 1 &&
          subject[start_offset] == '\r' && subject[start_offset + 1] == '\n')
        ovector[1] += 1;
      else if (utf8) {
        while (ovector[1] < subject_length) {
          if ((subject[ovector[1]] & 0xc0) != 0x80)
            break;
          ovector[1] += 1;
        }
      }
      continue;
    }

    if (rc < 0) {
      runtime_error("regular expression error %d", rc);
      pcre2_match_data_free(match_data);
      pcre2_code_free(re);
      return EMPTY_VAL;
    }

    // REGEX_VECTOR_SIZE_WARNING();
    REGEX_ASSERTION_ERROR(re, match_data, ovector);

    if (name_count == 0) {
      int i;
      for (i = 0; i < rc; i++) {
        PCRE2_SIZE substring_length = ovector[2 * i + 1] - ovector[2 * i];
        if (substring_length > 0) {
          PCRE2_SPTR substring_start = subject + ovector[2 * i];
          write_list(vm, result,
                     OBJ_VAL(copy_string(vm, (char *)substring_start,
                                         (int)substring_length)));
        }
      }
    } else {

      b_obj_list *match_list = new_list(vm);
      b_obj_dict *match_dict = new_dict(vm);

      int i;
      for (i = 0; i < rc; i++) {
        PCRE2_SIZE substring_length = ovector[2 * i + 1] - ovector[2 * i];
        if (substring_length > 0) {
          PCRE2_SPTR substring_start = subject + ovector[2 * i];
          write_value_arr(vm, &match_list->items,
                          OBJ_VAL(copy_string(vm, (char *)substring_start,
                                              (int)substring_length)));
        }
      }

      PCRE2_SPTR tabptr;

      (void)pcre2_pattern_info(re, PCRE2_INFO_NAMETABLE, &name_table);

      (void)pcre2_pattern_info(re, PCRE2_INFO_NAMEENTRYSIZE, &name_entry_size);

      tabptr = name_table;

      for (i = 0; i < (int)name_count; i++) {
        int n = (tabptr[0] << 8) | tabptr[1];

        char *_key = malloc(sizeof(char *));
        char *_val = malloc(sizeof(char *));
        sprintf(_key, "%*s", name_entry_size - 3, tabptr + 2);
        sprintf(_val, "%*s", (int)(ovector[2 * n + 1] - ovector[2 * n]),
                subject + ovector[2 * n]);

        while (isspace((unsigned char)*_key))
          _key++;

        dict_add_entry(
            vm, match_dict, OBJ_VAL(copy_string(vm, _key, name_entry_size - 3)),
            OBJ_VAL(copy_string(vm, _val,
                                (int)(ovector[2 * n + 1] - ovector[2 * n]))));

        tabptr += name_entry_size;
      }

      write_value_arr(vm, &match_list->items, OBJ_VAL(match_dict));
      write_list(vm, result, OBJ_VAL(match_list));
    }
  }

  pcre2_match_data_free(match_data);
  pcre2_code_free(re);

  RETURN_OBJ(result);
}

DECLARE_STRING_METHOD(replace) {
  ENFORCE_ARG_COUNT(replace, 2);
  ENFORCE_ARG_TYPE(replace, 0, IS_STRING);
  ENFORCE_ARG_TYPE(replace, 1, IS_STRING);

  b_obj_string *string = AS_STRING(METHOD_OBJECT);
  b_obj_string *substr = AS_STRING(args[0]);
  b_obj_string *rep_substr = AS_STRING(args[1]);

  if (string->length == 0 && substr->length == 0) {
    RETURN_TRUE;
  } else if (string->length == 0 || substr->length == 0) {
    RETURN_FALSE;
  }

  GET_REGEX_COMPILE_OPTIONS(replace, substr, false);
  char *real_regex = substr->chars;
  if ((int)compile_options > -1) {
    real_regex = remove_regex_delimiter(vm, substr);
  }

  PCRE2_SPTR input = (PCRE2_SPTR)string->chars;
  PCRE2_SPTR pattern = (PCRE2_SPTR)real_regex;
  PCRE2_SPTR replacement = (PCRE2_SPTR)rep_substr->chars;

  int result, error_number;
  PCRE2_SIZE error_offset;

  pcre2_code *re =
      pcre2_compile(pattern, PCRE2_ZERO_TERMINATED, PCRE2_MULTILINE,
                    &error_number, &error_offset, 0);

  REGEX_COMPILATION_ERROR(re, error_number, error_offset);

  pcre2_match_context *match_context = pcre2_match_context_create(0);

  PCRE2_SIZE output_length = 0;
  result = pcre2_substitute(
      re, input, PCRE2_ZERO_TERMINATED, 0,
      PCRE2_SUBSTITUTE_GLOBAL | PCRE2_SUBSTITUTE_OVERFLOW_LENGTH |
          PCRE2_SUBSTITUTE_EXTENDED,
      0, match_context, replacement, PCRE2_ZERO_TERMINATED, 0, &output_length);

  if (result != PCRE2_ERROR_NOMEMORY) {
    runtime_error("regular expression post-compilation failed for replacement");
    return EMPTY_VAL;
  }

  PCRE2_UCHAR *output_buffer = malloc(output_length * sizeof(PCRE2_UCHAR));

  result = pcre2_substitute(
      re, input, PCRE2_ZERO_TERMINATED, 0,
      PCRE2_SUBSTITUTE_GLOBAL | PCRE2_SUBSTITUTE_EXTENDED, 0, match_context,
      replacement, PCRE2_ZERO_TERMINATED, output_buffer, &output_length);

  if (result < 0) {
    runtime_error("regular expression error at replacement time");
    return EMPTY_VAL;
  }

  b_obj_string *response =
      copy_string(vm, (char *)output_buffer, output_length);

  pcre2_match_context_free(match_context);
  pcre2_code_free(re);

  RETURN_OBJ(response);
}

DECLARE_STRING_METHOD(to_bytes) {
  ENFORCE_ARG_COUNT(to_bytes, 0);
  b_obj_string *string = AS_STRING(METHOD_OBJECT);
  RETURN_OBJ(copy_bytes(vm, (unsigned char *)string->chars, string->length));
}