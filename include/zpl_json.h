/*

ZPL - JSON parser module

Usage:
    #define ZPLJ_IMPLEMENTATION exactly in ONE source file right BEFORE including the library, like:

    #define ZPLJ_IMPLEMENTATION
    #include"zpl_json.h"

Credits:
    Dominik Madarasz (GitHub: zaklaus)

Version History:
    2.0.9 - zpl 4.0.0 support
    2.0.8 - Small cleanup in README and test file
    2.0.7 - Small fixes for tiny cpp warnings
    2.0.5 - Fix for bad access on deallocation
    2.0.4 - Small fix for cpp issues
    2.0.3 - Small bugfix in name with underscores
    2.0.1 - Catch error in name
    2.0.0 - Added basic error handling
    1.4.0 - Added Infinity and NaN constants
    1.3.0 - Added multi-line backtick strings
    1.2.0 - More JSON5 features and bugfixes
    1.1.1 - Small mistake fixed
    1.1.0 - Basic JSON5 support, comments and fixes
    1.0.4 - Header file fixes
    1.0.0 - Initial version
    
    
    Copyright 2017 Dominik Madarász
    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License. 
*/

#ifndef ZPL_INCLUDE_ZPL_JSON_H
#define ZPL_INCLUDE_ZPL_JSON_H

#ifdef ZPLJ_DEBUG
#define ZPLJ_ASSERT ZPL_ASSERT(0)
#else
#define ZPLJ_ASSERT
#endif

#ifdef __cplusplus
extern "C" {
#endif

    // TODO(ZaKlaus): INFINITY, NAN
    #include <math.h>

    typedef enum zplj_type_e {
        zplj_type_object_ev,
        zplj_type_string_ev,
        zplj_type_multistring_ev,
        zplj_type_array_ev,
        zplj_type_integer_ev,
        zplj_type_real_ev,
        zplj_type_constant_ev
    } zplj_type_e;

    typedef enum zplj_constant_e {
        zplj_constant_null_ev,
        zplj_constant_false_ev,
        zplj_constant_true_ev,
    } zplj_constant_e;

    // TODO(ZaKlaus): Error handling
    typedef enum zplj_error_e {
        zplj_error_none_ev,
        zplj_error_invalid_name_ev,
        zplj_error_invalid_value_ev,
    } zplj_error_e;

    typedef enum zplj_name_style_e {
        zplj_name_style_double_quote_ev,
        zplj_name_style_single_quote_ev,
        zplj_name_style_no_quotes_ev,
    } zplj_name_style_e;

    typedef struct zplj_object_t {
        zpl_allocator_t backing;
        u8    name_style;
        char *name;
        u8    type;
        zpl_array_t(struct zplj_object_t) nodes;

        union {
            char *string;
            zpl_array_t(struct zplj_object_t) elements;
            i64   integer;
            f64   real;
            u8    constant;
        };
    } zplj_object_t;

    ZPL_DEF void zplj_parse(zplj_object_t *root, usize len, char *const source, zpl_allocator_t a, b32 strip_comments, u8 *err_code);
    ZPL_DEF void zplj_free (zplj_object_t *obj);

    ZPL_DEF char *zplj__parse_object(zplj_object_t *obj, char *base, zpl_allocator_t a, u8 *err_code);
    ZPL_DEF char *zplj__parse_value (zplj_object_t *obj, char *base, zpl_allocator_t a, u8 *err_code);
    ZPL_DEF char *zplj__parse_array (zplj_object_t *obj, char *base, zpl_allocator_t a, u8 *err_code);

    ZPL_DEF char *zplj__trim        (char *str);
    ZPL_DEF char *zplj__skip        (char *str, char c);
    ZPL_DEF b32 zplj__validate_name (char *str, char *err);

#ifdef __cplusplus
}
#endif

#if defined(ZPLJ_IMPLEMENTATION) && !defined(ZPLJ_IMPLEMENTATION_DONE)
#define ZPLJ_IMPLEMENTATION_DONE

#ifdef __cplusplus
extern "C" {
#endif

    b32 zplj__is_control_char(char c);

    void zplj_parse(zplj_object_t *root, usize len, char *const source, zpl_allocator_t a, b32 strip_comments, u8 *err_code) {
        ZPL_ASSERT(root && source);
        zpl_unused(len);

        char *dest = source;

        if (strip_comments) {
            b32 is_lit = false;
            char lit_c = '\0';
            char *p = dest;
            char *b = dest;
            isize l = 0;

            while (*p) {
                if (!is_lit) {
                    if ((*p == '"' || *p == '\'')) {
                        lit_c = *p;
                        is_lit = true;
                        ++p;
                        continue;
                    }
                }
                else {
                    /**/ if (*p == '\\' && *(p+1) && *(p+1) == lit_c) {
                        p+=2;
                        continue;
                    }
                    else if (*p == lit_c) {
                        is_lit = false;
                        ++p;
                        continue;
                    }
                }

                if (!is_lit) {
                    // NOTE(ZaKlaus): block comment
                    if (p[0] == '/' && p[1] == '*') {
                        b = p;
                        l=2;
                        p+=2;

                        while (p[0] != '*' && p[1] != '/') {
                            ++p; ++l;
                        }
                        p+=2;
                        l+=2;
                        zpl_memset(b, ' ', l);
                    }

                    // NOTE(ZaKlaus): inline comment
                    if (p[0] == '/' && p[0] == '/') {
                        b = p;
                        l=2;
                        p+=2;

                        while (p[0] != '\n') {
                            ++p; ++l;
                        }
                        ++p;
                        ++l;
                        zpl_memset(b, ' ', l);
                    }
                }

                ++p;
            }
        }

        if (err_code) *err_code = zplj_error_none_ev;
        zplj_object_t root_ = {0};
        zplj__parse_object(&root_, dest, a, err_code);

        *root = root_;
    }

    void zplj_free(zplj_object_t *obj) {
        /**/ if (obj->type == zplj_type_array_ev && obj->elements) {
            for (isize i = 0; i < zpl_array_count(obj->elements); ++i) {
                zplj_free(obj->elements+i);
            }

            zpl_array_free(obj->elements);
        }
        else if (obj->type == zplj_type_object_ev && obj->nodes) {
            for (isize i = 0; i < zpl_array_count(obj->nodes); ++i) {
                zplj_free(obj->nodes+i);
            }

            zpl_array_free(obj->nodes);
        }
    }

    char *zplj__parse_array(zplj_object_t *obj, char *base, zpl_allocator_t a, u8 *err_code) {
        ZPL_ASSERT(obj && base);
        char *p = base;

        obj->type = zplj_type_array_ev;
        zpl_array_init(obj->elements, a);
        obj->backing = a;

        while(*p) {
            p = zplj__trim(p);

            zplj_object_t elem = {0};
            p = zplj__parse_value(&elem, p, a, err_code);

            if (err_code && *err_code != zplj_error_none_ev) {
                return NULL;
            }

            zpl_array_append(obj->elements, elem);

            p = zplj__trim(p);

            if (*p == ',') {
                ++p;
                continue;
            }
            else {
                return p;
            }
        }
        return p;
    }

    char *zplj__parse_value(zplj_object_t *obj, char *base, zpl_allocator_t a, u8 *err_code) {
        ZPL_ASSERT(obj && base);
        char *p = base;
        char *b = base;
        char *e = base;

        /**/ if (*p == '"' || *p == '\'') {
            char c = *p;
            obj->type = zplj_type_string_ev;
            b = p+1;
            e = b;
            obj->string = b;

            while(*e) {
                /**/ if (*e == '\\' && *(e+1) == c) {
                    e += 2;
                    continue;
                }
                else if (*e == '\\' && (*(e+1) == '\r' || *(e+1) == '\n')) {
                    *e = ' ';
                    e++;
                    continue;
                }
                else if (*e == c) {
                    break;
                }
                ++e;
            }

            *e = '\0';
            p = e+1;
        }
        else if (*p == '`') {
            obj->type = zplj_type_multistring_ev;
            b = p+1;
            e = b;
            obj->string = b;


            while(*e) {
                /**/ if (*e == '\\' && *(e+1) == '`') {
                    e += 2;
                    continue;
                }
                else if (*e == '`') {
                    break;
                }
                ++e;
            }

            *e = '\0';
            p = e+1;
        }
        else if (zpl_char_is_alpha(*p) || (*p == '-' && !zpl_char_is_digit(p[1]))) {
            obj->type = zplj_type_constant_ev;

            /**/ if (!zpl_strncmp(p, "true", 4)) {
                obj->constant = zplj_constant_true_ev;
                p += 4;
            }
            else if (!zpl_strncmp(p, "false", 5)) {
                obj->constant = zplj_constant_false_ev;
                p += 5;
            }
            else if (!zpl_strncmp(p, "null", 4)) {
                obj->constant = zplj_constant_null_ev;
                p += 4;
            }
            else if (!zpl_strncmp(p, "Infinity", 8)) {
                obj->type = zplj_type_real_ev;
                obj->real = INFINITY;
                p += 8;
            }
            else if (!zpl_strncmp(p, "-Infinity", 9)) {
                obj->type = zplj_type_real_ev;
                obj->real = -INFINITY;
                p += 9;
            }
            else if (!zpl_strncmp(p, "NaN", 3)) {
                obj->type = zplj_type_real_ev;
                obj->real = NAN;
                p += 3;
            }
            else if (!zpl_strncmp(p, "-NaN", 4)) {
                obj->type = zplj_type_real_ev;
                obj->real = -NAN;
                p += 4;
            }
            else {
                ZPLJ_ASSERT; if (err_code) *err_code = zplj_error_invalid_value_ev;
                return NULL;
            }
        }
        else if (zpl_char_is_digit(*p) ||
                 *p == '+' || *p == '-' ||
                 *p == '.') {
            obj->type = zplj_type_integer_ev;

            b = p;
            e = b;

            isize ib = 0;
            char buf[16] = {0};

            /**/ if (*e == '+') ++e;
            else if (*e == '-') {
                buf[ib++] = *e++;
            }

            if (*e == '.') {
                obj->type = zplj_type_real_ev;
                buf[ib++] = '0';

                do {
                    buf[ib++] = *e;
                }
                while(zpl_char_is_digit(*++e));
            }
            else {
                while(zpl_char_is_hex_digit(*e) || *e == 'x' || *e == 'X') {
                    buf[ib++] = *e++;

                }

                if (*e == '.') {
                    obj->type = zplj_type_real_ev;
                    u32 step = 0;

                    do {
                        buf[ib++] = *e;
                        ++step;
                    }
                    while(zpl_char_is_digit(*++e));

                    if (step < 2) {
                        buf[ib++] = '0';
                    }
                }
            }

            i64 exp = 0; f32 eb = 10;
            char expbuf[6] = {0};
            isize expi = 0;

            if (*e == 'e' || *e == 'E') {
                ++e;
                if (*e == '+' || *e == '-' || zpl_char_is_digit(*e)) {
                    if (*e == '-') {
                        eb = 0.1f;
                    }

                    if (!zpl_char_is_digit(*e)) {
                        ++e;
                    }

                    while(zpl_char_is_digit(*e)) {
                        expbuf[expi++] = *e++;
                    }

                }

                exp = zpl_str_to_i64(expbuf, NULL, 10);
            }

            if (*e == '\0') {
                ZPLJ_ASSERT; if (err_code) *err_code = zplj_error_invalid_value_ev;
            }

            // NOTE(ZaKlaus): @enhance
            if (obj->type == zplj_type_integer_ev) {
                obj->integer = zpl_str_to_i64(buf, 0, 0);

                while(--exp > 0) {
                    obj->integer *= (i64)eb;
                }
            }
            else {
                obj->real = zpl_str_to_f64(buf, 0);

                while(--exp > 0) {
                    obj->real *= eb;
                }
            }
            p = e;
        }
        else if (*p == '[') {
            p = zplj__trim(p+1);
            if (*p == ']') return p;
            p = zplj__parse_array(obj, p, a, err_code);

            if (err_code && *err_code != zplj_error_none_ev) {
                return NULL;
            }

            ++p;
        }
        else if (*p == '{') {
            p = zplj__trim(p+1);
            p = zplj__parse_object(obj, p, a, err_code);

            if (err_code && *err_code != zplj_error_none_ev) {
                return NULL;
            }

            ++p;
        }

        return p;
    }

    char *zplj__parse_object(zplj_object_t *obj, char *base, zpl_allocator_t a, u8 *err_code) {
        ZPL_ASSERT(obj && base);
        char *p = base;
        char *b = base;
        char *e = base;

        zpl_array_init(obj->nodes, a);
        obj->backing = a;

        p = zplj__trim(p);
        if (*p == '{') p++;

        while(*p) {
            zplj_object_t node = {0};
            p = zplj__trim(p);
            if (*p == '}') return p;

            if (*p == '"' || *p == '\'') {
                if (*p == '"') {
                    node.name_style = zplj_name_style_double_quote_ev;
                }
                else {
                    node.name_style = zplj_name_style_single_quote_ev;
                }

                char c = *p;
                b = ++p;
                e = zplj__skip(b, c);
                node.name = b;
                *e = '\0';

                p = ++e;
                p = zplj__trim(p);

                if (*p && *p != ':') {
                    ZPLJ_ASSERT; if (err_code) *err_code = zplj_error_invalid_name_ev;
                    return NULL;
                }
            }
            else {
                /**/ if (*p == '[') {
                    node.name = 0;
                    p = zplj__parse_value(&node, p, a, err_code);
                    goto l_parsed;
                }
                else if (zpl_char_is_alpha(*p) || *p == '_' || *p == '$') {
                    b = p;
                    e = b;

                    do {
                        ++e;
                    }
                    while(*e && (zpl_char_is_alphanumeric(*e) || *e == '_')
                          && !zpl_char_is_space(*e) && *e != ':');

                    if (*e == ':') {
                        p = e;
                    }
                    else {
                        while(*e) {
                            if (*e && (!zpl_char_is_space(*e) || *e == ':')) {
                                break;
                            }
                            ++e;
                        }
                        e = zplj__trim(e);
                        p = e;

                        if (*p && *p != ':') {
                            ZPLJ_ASSERT; if (err_code) *err_code = zplj_error_invalid_name_ev;
                            return NULL;
                        }
                    }

                    *e = '\0';
                    node.name = b;
                    node.name_style = zplj_name_style_no_quotes_ev;
                }
            }

            char errc;
            if (!zplj__validate_name(node.name, &errc)) {
                ZPLJ_ASSERT; if (err_code) *err_code = zplj_error_invalid_name_ev;
                return NULL;
            }

            p = zplj__trim(p+1);
            p = zplj__parse_value(&node, p, a, err_code);

            if (err_code && *err_code != zplj_error_none_ev) {
                return NULL;
            }

            l_parsed:

            zpl_array_append(obj->nodes, node);

            p = zplj__trim(p);

            /**/ if (*p == ',') {
                ++p;
                continue;
            }
            else if (*p == '\0' || *p == '}') {
                return p;
            }
            else {
                ZPLJ_ASSERT; if (err_code) *err_code = zplj_error_invalid_value_ev;
                return NULL;
            }
        }
        return p;
    }

    zpl_inline char *zplj__trim(char *str) {
        while (*str && zpl_char_is_space(*str)) {
            ++str;
        }

        return str;
    }

    zpl_inline b32 zplj__is_control_char(char c) {
        return (c == '"' || c == '\\' || c == '/' || c == 'b' ||
                c == 'f' || c == 'n'  || c == 'r' || c == 't');
    }

    zpl_inline b32 zplj__is_special_char(char c) {
        return (c == '<' || c == '>' || c == ':' || c == '/');
    }

#define jx(x) !zpl_char_is_hex_digit(str[x])
    zpl_inline b32 zplj__validate_name(char *str, char *err) {
        while(*str) {
            if ((str[0] == '\\' && !zplj__is_control_char(str[1])) &&
                (str[0] == '\\' && jx(1) && jx(2) && jx(3) && jx(4))) {
                *err = *str;
                return false;
            }

            ++str;
        }

        return true;
    }
#undef jx

    zpl_inline char *zplj__skip(char *str, char c) {
        while ((*str && *str != c) || (*(str-1) == '\\' && *str == c && zplj__is_control_char(c))) {
            ++str;
        }

        return str;
    }


#ifdef __cplusplus
}
#endif

#endif // ZPLJ_IMPLEMENTATION

#endif // ZPL_INCLUDE_ZPL_JSON_H
