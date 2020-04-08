#ifndef mpc_h
#define mpc_h

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <ctype.h>

/*
** State Type
*/

typedef struct {
  long pos;
  long row;
  long col;
  int term;
} mpc_state_T;

/*
** Error Type
*/

typedef struct {
  mpc_state_T state;
  int expected_num;
  char *filename;
  char *failure;
  char **expected;
  char received;
} mpc_err_T;

void mpc_err_delete(mpc_err_T *e);
char *mpc_err_string(mpc_err_T *e);
void mpc_err_print(mpc_err_T *e);
void mpc_err_print_to(mpc_err_T *e, FILE *f);

/*
** Parsing
*/

typedef void mpc_val_T;

typedef union {
  mpc_err_T *error;
  mpc_val_T *output;
} mpc_result_T;

struct mpc_parser_T;
typedef struct mpc_parser_T mpc_parser_T;

int mpc_parse(const char *filename, const char *string, mpc_parser_T *p, mpc_result_T *r);
int mpc_nparse(const char *filename, const char *string, size_t length, mpc_parser_T *p, mpc_result_T *r);
int mpc_parse_file(const char *filename, FILE *file, mpc_parser_T *p, mpc_result_T *r);
int mpc_parse_pipe(const char *filename, FILE *pipe, mpc_parser_T *p, mpc_result_T *r);
int mpc_parse_contents(const char *filename, mpc_parser_T *p, mpc_result_T *r);

/*
** Function Types
*/

typedef void(*mpc_dtor_T)(mpc_val_T*);
typedef mpc_val_T*(*mpc_ctor_T)(void);

typedef mpc_val_T*(*mpc_apply_T)(mpc_val_T*);
typedef mpc_val_T*(*mpc_apply_to_t)(mpc_val_T*,void*);
typedef mpc_val_T*(*mpc_fold_T)(int,mpc_val_T**);

typedef int(*mpc_check_T)(mpc_val_T**);
typedef int(*mpc_check_with_t)(mpc_val_T**,void*);

/*
** Building a Parser
*/

mpc_parser_T *mpc_new(const char *name);
mpc_parser_T *mpc_copy(mpc_parser_T *a);
mpc_parser_T *mpc_define(mpc_parser_T *p, mpc_parser_T *a);
mpc_parser_T *mpc_undefine(mpc_parser_T *p);

void mpc_delete(mpc_parser_T *p);
void mpc_cleanup(int n, ...);

/*
** Basic Parsers
*/

mpc_parser_T *mpc_any(void);
mpc_parser_T *mpc_char(char c);
mpc_parser_T *mpc_range(char s, char e);
mpc_parser_T *mpc_oneof(const char *s);
mpc_parser_T *mpc_noneof(const char *s);
mpc_parser_T *mpc_satisfy(int(*f)(char));
mpc_parser_T *mpc_string(const char *s);

/*
** Other Parsers
*/

mpc_parser_T *mpc_pass(void);
mpc_parser_T *mpc_fail(const char *m);
mpc_parser_T *mpc_failf(const char *fmt, ...);
mpc_parser_T *mpc_lift(mpc_ctor_T f);
mpc_parser_T *mpc_lift_val(mpc_val_T *x);
mpc_parser_T *mpc_anchor(int(*f)(char,char));
mpc_parser_T *mpc_state(void);

/*
** Combinator Parsers
*/

mpc_parser_T *mpc_expect(mpc_parser_T *a, const char *e);
mpc_parser_T *mpc_expectf(mpc_parser_T *a, const char *fmt, ...);
mpc_parser_T *mpc_apply(mpc_parser_T *a, mpc_apply_T f);
mpc_parser_T *mpc_apply_to(mpc_parser_T *a, mpc_apply_to_t f, void *x);
mpc_parser_T *mpc_check(mpc_parser_T *a, mpc_dtor_T da, mpc_check_T f, const char *e);
mpc_parser_T *mpc_check_with(mpc_parser_T *a, mpc_dtor_T da, mpc_check_with_t f, void *x, const char *e);
mpc_parser_T *mpc_checkf(mpc_parser_T *a, mpc_dtor_T da, mpc_check_T f, const char *fmt, ...);
mpc_parser_T *mpc_check_withf(mpc_parser_T *a, mpc_dtor_T da, mpc_check_with_t f, void *x, const char *fmt, ...);

mpc_parser_T *mpc_not(mpc_parser_T *a, mpc_dtor_T da);
mpc_parser_T *mpc_not_lift(mpc_parser_T *a, mpc_dtor_T da, mpc_ctor_T lf);
mpc_parser_T *mpc_maybe(mpc_parser_T *a);
mpc_parser_T *mpc_maybe_lift(mpc_parser_T *a, mpc_ctor_T lf);

mpc_parser_T *mpc_many(mpc_fold_T f, mpc_parser_T *a);
mpc_parser_T *mpc_many1(mpc_fold_T f, mpc_parser_T *a);
mpc_parser_T *mpc_count(int n, mpc_fold_T f, mpc_parser_T *a, mpc_dtor_T da);

mpc_parser_T *mpc_or(int n, ...);
mpc_parser_T *mpc_and(int n, mpc_fold_T f, ...);

mpc_parser_T *mpc_predictive(mpc_parser_T *a);

/*
** Common Parsers
*/

mpc_parser_T *mpc_eoi(void);
mpc_parser_T *mpc_soi(void);

mpc_parser_T *mpc_boundary(void);
mpc_parser_T *mpc_boundary_newline(void);

mpc_parser_T *mpc_whitespace(void);
mpc_parser_T *mpc_whitespaces(void);
mpc_parser_T *mpc_blank(void);

mpc_parser_T *mpc_newline(void);
mpc_parser_T *mpc_tab(void);
mpc_parser_T *mpc_escape(void);

mpc_parser_T *mpc_digit(void);
mpc_parser_T *mpc_hexdigit(void);
mpc_parser_T *mpc_octdigit(void);
mpc_parser_T *mpc_digits(void);
mpc_parser_T *mpc_hexdigits(void);
mpc_parser_T *mpc_octdigits(void);

mpc_parser_T *mpc_lower(void);
mpc_parser_T *mpc_upper(void);
mpc_parser_T *mpc_alpha(void);
mpc_parser_T *mpc_underscore(void);
mpc_parser_T *mpc_alphanum(void);

mpc_parser_T *mpc_int(void);
mpc_parser_T *mpc_hex(void);
mpc_parser_T *mpc_oct(void);
mpc_parser_T *mpc_number(void);

mpc_parser_T *mpc_real(void);
mpc_parser_T *mpc_float(void);

mpc_parser_T *mpc_char_lit(void);
mpc_parser_T *mpc_string_lit(void);
mpc_parser_T *mpc_regex_lit(void);

mpc_parser_T *mpc_ident(void);

/*
** Useful Parsers
*/

mpc_parser_T *mpc_startwith(mpc_parser_T *a);
mpc_parser_T *mpc_endwith(mpc_parser_T *a, mpc_dtor_T da);
mpc_parser_T *mpc_whole(mpc_parser_T *a, mpc_dtor_T da);

mpc_parser_T *mpc_stripl(mpc_parser_T *a);
mpc_parser_T *mpc_stripr(mpc_parser_T *a);
mpc_parser_T *mpc_strip(mpc_parser_T *a);
mpc_parser_T *mpc_tok(mpc_parser_T *a);
mpc_parser_T *mpc_sym(const char *s);
mpc_parser_T *mpc_total(mpc_parser_T *a, mpc_dtor_T da);

mpc_parser_T *mpc_between(mpc_parser_T *a, mpc_dtor_T ad, const char *o, const char *c);
mpc_parser_T *mpc_parens(mpc_parser_T *a, mpc_dtor_T ad);
mpc_parser_T *mpc_braces(mpc_parser_T *a, mpc_dtor_T ad);
mpc_parser_T *mpc_brackets(mpc_parser_T *a, mpc_dtor_T ad);
mpc_parser_T *mpc_squares(mpc_parser_T *a, mpc_dtor_T ad);

mpc_parser_T *mpc_tok_between(mpc_parser_T *a, mpc_dtor_T ad, const char *o, const char *c);
mpc_parser_T *mpc_tok_parens(mpc_parser_T *a, mpc_dtor_T ad);
mpc_parser_T *mpc_tok_braces(mpc_parser_T *a, mpc_dtor_T ad);
mpc_parser_T *mpc_tok_brackets(mpc_parser_T *a, mpc_dtor_T ad);
mpc_parser_T *mpc_tok_squares(mpc_parser_T *a, mpc_dtor_T ad);

/*
** Common Function Parameters
*/

void mpcf_dtor_null(mpc_val_T *x);

mpc_val_T *mpcf_ctor_null(void);
mpc_val_T *mpcf_ctor_str(void);

mpc_val_T *mpcf_free(mpc_val_T *x);
mpc_val_T *mpcf_int(mpc_val_T *x);
mpc_val_T *mpcf_hex(mpc_val_T *x);
mpc_val_T *mpcf_oct(mpc_val_T *x);
mpc_val_T *mpcf_float(mpc_val_T *x);
mpc_val_T *mpcf_strtriml(mpc_val_T *x);
mpc_val_T *mpcf_strtrimr(mpc_val_T *x);
mpc_val_T *mpcf_strtrim(mpc_val_T *x);

mpc_val_T *mpcf_escape(mpc_val_T *x);
mpc_val_T *mpcf_escape_regex(mpc_val_T *x);
mpc_val_T *mpcf_escape_string_raw(mpc_val_T *x);
mpc_val_T *mpcf_escape_char_raw(mpc_val_T *x);

mpc_val_T *mpcf_unescape(mpc_val_T *x);
mpc_val_T *mpcf_unescape_regex(mpc_val_T *x);
mpc_val_T *mpcf_unescape_string_raw(mpc_val_T *x);
mpc_val_T *mpcf_unescape_char_raw(mpc_val_T *x);

mpc_val_T *mpcf_null(int n, mpc_val_T** xs);
mpc_val_T *mpcf_fst(int n, mpc_val_T** xs);
mpc_val_T *mpcf_snd(int n, mpc_val_T** xs);
mpc_val_T *mpcf_trd(int n, mpc_val_T** xs);

mpc_val_T *mpcf_fst_free(int n, mpc_val_T** xs);
mpc_val_T *mpcf_snd_free(int n, mpc_val_T** xs);
mpc_val_T *mpcf_trd_free(int n, mpc_val_T** xs);
mpc_val_T *mpcf_all_free(int n, mpc_val_T** xs);

mpc_val_T *mpcf_strfold(int n, mpc_val_T** xs);
mpc_val_T *mpcf_maths(int n, mpc_val_T** xs);

/*
** Regular Expression Parsers
*/

enum {
  MPC_RE_DEFAULT   = 0,
  MPC_RE_M         = 1,
  MPC_RE_S         = 2,
  MPC_RE_MULTILINE = 1,
  MPC_RE_DOTALL    = 2
};

mpc_parser_T *mpc_re(const char *re);
mpc_parser_T *mpc_re_mode(const char *re, int mode);

/*
** AST
*/

typedef struct mpc_ast_T {
  char *tag;
  char *contents;
  mpc_state_T state;
  int children_num;
  struct mpc_ast_T** children;
} mpc_ast_T;

mpc_ast_T *mpc_ast_new(const char *tag, const char *contents);
mpc_ast_T *mpc_ast_build(int n, const char *tag, ...);
mpc_ast_T *mpc_ast_add_root(mpc_ast_T *a);
mpc_ast_T *mpc_ast_add_child(mpc_ast_T *r, mpc_ast_T *a);
mpc_ast_T *mpc_ast_add_tag(mpc_ast_T *a, const char *t);
mpc_ast_T *mpc_ast_add_root_tag(mpc_ast_T *a, const char *t);
mpc_ast_T *mpc_ast_tag(mpc_ast_T *a, const char *t);
mpc_ast_T *mpc_ast_state(mpc_ast_T *a, mpc_state_T s);

void mpc_ast_delete(mpc_ast_T *a);
void mpc_ast_print(mpc_ast_T *a);
void mpc_ast_print_to(mpc_ast_T *a, FILE *fp);

int mpc_ast_get_index(mpc_ast_T *ast, const char *tag);
int mpc_ast_get_index_lb(mpc_ast_T *ast, const char *tag, int lb);
mpc_ast_T *mpc_ast_get_child(mpc_ast_T *ast, const char *tag);
mpc_ast_T *mpc_ast_get_child_lb(mpc_ast_T *ast, const char *tag, int lb);

typedef enum {
  mpc_ast_trav_order_pre,
  mpc_ast_trav_order_post
} mpc_ast_trav_order_t;

typedef struct mpc_ast_trav_t {
  mpc_ast_T             *curr_node;
  struct mpc_ast_trav_t *parent;
  int                    curr_child;
  mpc_ast_trav_order_t   order;
} mpc_ast_trav_t;

mpc_ast_trav_t *mpc_ast_traverse_start(mpc_ast_T *ast,
                                       mpc_ast_trav_order_t order);

mpc_ast_T *mpc_ast_traverse_next(mpc_ast_trav_t **trav);

void mpc_ast_traverse_free(mpc_ast_trav_t **trav);

/*
** Warning: This function currently doesn't test for equality of the `state` member!
*/
int mpc_ast_eq(mpc_ast_T *a, mpc_ast_T *b);

mpc_val_T *mpcf_fold_ast(int n, mpc_val_T **as);
mpc_val_T *mpcf_str_ast(mpc_val_T *c);
mpc_val_T *mpcf_state_ast(int n, mpc_val_T **xs);

mpc_parser_T *mpca_tag(mpc_parser_T *a, const char *t);
mpc_parser_T *mpca_add_tag(mpc_parser_T *a, const char *t);
mpc_parser_T *mpca_root(mpc_parser_T *a);
mpc_parser_T *mpca_state(mpc_parser_T *a);
mpc_parser_T *mpca_total(mpc_parser_T *a);

mpc_parser_T *mpca_not(mpc_parser_T *a);
mpc_parser_T *mpca_maybe(mpc_parser_T *a);

mpc_parser_T *mpca_many(mpc_parser_T *a);
mpc_parser_T *mpca_many1(mpc_parser_T *a);
mpc_parser_T *mpca_count(int n, mpc_parser_T *a);

mpc_parser_T *mpca_or(int n, ...);
mpc_parser_T *mpca_and(int n, ...);

enum {
  MPCA_LANG_DEFAULT              = 0,
  MPCA_LANG_PREDICTIVE           = 1,
  MPCA_LANG_WHITESPACE_SENSITIVE = 2
};

mpc_parser_T *mpca_grammar(int flags, const char *grammar, ...);

mpc_err_T *mpca_lang(int flags, const char *language, ...);
mpc_err_T *mpca_lang_file(int flags, FILE *f, ...);
mpc_err_T *mpca_lang_pipe(int flags, FILE *f, ...);
mpc_err_T *mpca_lang_contents(int flags, const char *filename, ...);

/*
** Misc
*/


void mpc_print(mpc_parser_T *p);
void mpc_optimise(mpc_parser_T *p);
void mpc_stats(mpc_parser_T *p);

int mpc_test_pass(mpc_parser_T *p, const char *s, const void *d,
  int(*tester)(const void*, const void*),
  mpc_dtor_T destructor,
  void(*printer)(const void*));

int mpc_test_fail(mpc_parser_T *p, const char *s, const void *d,
  int(*tester)(const void*, const void*),
  mpc_dtor_T destructor,
  void(*printer)(const void*));

#ifdef __cplusplus
}
#endif

#endif
