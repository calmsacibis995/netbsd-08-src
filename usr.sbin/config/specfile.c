#include <stdio.h>
#include <ctype.h>
#include <setjmp.h>
#include "y.tab.h"
#include "config.h"

#define T_OPTIONAL            1
#define T_STANDARD            2
#define T_COMPILE_WITH        3
#define T_CONFIG_DEPENDENT    4
#define T_DEVICE_DRIVER       5
#define T_PROFILING_ROUTINE   6
#define T_IDENTIFIER          7
#define T_OR                  8
#define T_REQUIRES            9
#define T_NOT                10
#define EXPR_GROUP           11
#define T_LEFTPAREN          12
#define T_RIGHTPAREN         13

#define is_paren(x) ((x == '(') || (x == ')'))
struct file_keyword {
    char *key;
    int token;
} file_kw_table[] = {
    "compile-with",            T_COMPILE_WITH,
    "config-dependent",        T_CONFIG_DEPENDENT,
    "device-driver",           T_DEVICE_DRIVER,
    "optional",                T_OPTIONAL,
    "or",                      T_OR,
    "not",                     T_NOT,
    "requires",                T_REQUIRES,
    "standard",                T_STANDARD
    };

extern struct file_list *fltail_lookup(),*new_fent(),*fl_lookup();
static char *current_file;
static int current_line;
static jmp_buf parse_problem;

int file_tok(token)
     char *token;
{
    int i, length;

    if (is_paren(*token))
	return (*token == '(' ?  T_LEFTPAREN : T_RIGHTPAREN);
    for (i =0; i <(sizeof(file_kw_table)/sizeof(file_kw_table[0])); i++) {
	if (!strcmp(file_kw_table[i].key, token))
	    return file_kw_table[i].token;
    }
    length = strlen(token);
    for (i = 0; i < length; i++) {
	if (!isalnum(token[i]) && (token[i] != '_') &&
	    (token[i] != '-')) return -1;
    }
    return T_IDENTIFIER;
}

struct name_expr {
    int type;
    char *name;
    struct name_expr *next,*left,*right;
};

struct name_expr *
alloc_name_expr(name)
     char *name;
{
    struct name_expr *new;
    int token,type;

    type = 0;
    token = file_tok(name);
    switch (token) {
    case T_OR: 
    case T_NOT: 
    case T_IDENTIFIER: 
    case T_LEFTPAREN:
    case T_RIGHTPAREN:
	type = token;
	break;
    default: return NULL;
    }
    new = malloc(sizeof(struct name_expr));
    new->type = type;
    if (type == T_IDENTIFIER) new->name = ns(name);
    else new->name = NULL;
    new->left = new->right = new->next = NULL;
    return new;
}

void parse_err(char *emessage)
{
    fprintf(stderr, "%s:%d: %s\n", current_file, current_line, emessage);
    if (!fatal_errors) longjmp(parse_problem,1);
    exit(1);
}

delete_expr(expr)
     struct name_expr *expr;
{
    if (expr->type == T_IDENTIFIER)
	free(expr->name);
    free(expr);
}
struct name_expr *yank_node(expr)
     struct name_expr **expr;
{
    struct name_expr *node;

    if (*expr == NULL) return NULL;
    node = *expr;
    *expr = (*expr)->next;
    return node;
}
struct name_expr *yank_expr(expr)
     struct name_expr **expr;
{
    struct name_expr *node,*tail, *subexpr,*next;

    node = yank_node(expr);
    if (node == NULL) return NULL;
    if (node->type == T_LEFTPAREN) 
	for (tail = subexpr = NULL;;) {
	    next = yank_expr(expr);
	    if (!next) parse_err("missing ')'");
	    if (next->type == T_RIGHTPAREN) {
		if (subexpr == NULL)
		    parse_err("empty inner expression i.e '()' ");
		node->left = subexpr;
		node->type = EXPR_GROUP;
		break;
	    } else {
		if (subexpr == NULL)
		    tail = subexpr = next;
		else {
		    tail->next = next;
		    tail = next;
		}
		tail->next = NULL;
	    }
	}
    return(node);
}

struct name_expr *
paren_squish(tree)
     struct name_expr *tree;
{
    struct name_expr *result,*expr,*tail;
    
    result = tail = NULL;

    while ((expr = yank_expr(&tree)) != NULL) {
	if (expr->type == T_RIGHTPAREN)
	    parse_err("unexpected ')'");
	if (result == NULL) {
	    tail = result = expr;
	}
	else {
	    tail->next = expr;
	    tail = expr;
	}
	tail->next = NULL;
    }
    return(result);
}

struct name_expr *
not_squish(tree)
     struct name_expr *tree;
{
    struct name_expr *result,*next,*tail,*node;

    tail = result = next = NULL;
    
    while ((next = yank_node(&tree)) != NULL) {
	if (next->type == EXPR_GROUP)
	    next->left = not_squish(next->left);
	if (next->type == T_NOT) {
	    int notlevel = 1;
	    
	    node = yank_node(&tree);
	    while (node->type == T_NOT) {
		++notlevel;
		node = yank_node(&tree);
	    }
	    if (node == NULL)
		parse_err("no expression following 'not'");
	    if (node->type == T_OR)
		parse_err("nothing between 'not' and 'or'");
	    if (notlevel % 2 != 1)
		next = node;
	    else
		next->left = node;
	}
	
	/* add the node to our result plan */
	if (result == NULL)
	    tail = result = next;
	else {
	    tail->next = next;
	    tail = next;
	}
	tail->next = NULL;
    }
    return(result);
}
struct name_expr *
or_squish(tree)
     struct name_expr *tree;
{
    struct name_expr *next, *tail,*result;
    
    tail = result = next = NULL;
    
    while ((next = yank_node(&tree)) != NULL) {
	
	if (next->type == EXPR_GROUP)
	    next->left = or_squish(next->left);
	
	if (next->type == T_NOT)
	    next->left = or_squish(next->left);
	
	if (next->type == T_OR) {
	    if (result == NULL)
		parse_err("no expression before 'or'");
	    next->left = result;
	    next->right = or_squish(tree);
	    if (next->right == NULL)
		parse_err("no expression after 'or'");
	    next->next = NULL;
	    return(next);
	}
	
	/* add the node to our result plan */
	if (result == NULL)
	    tail = result = next;
	else {
	    tail->next = next;
	    tail = next;
	}
	tail->next = NULL;
    }
    return(result);
}

struct name_expr *
parse_name_expr(fp,seed, read_ahead)
     FILE *fp;
     char *seed, **read_ahead;
{
    struct name_expr *list, *tail,*new,*current;
    char *str;


    list = NULL;
    *read_ahead = NULL;
    if (seed) {
	list = alloc_name_expr(seed);
	if (list == NULL) {
	    *read_ahead = seed;
	    return NULL;
	}
    }
    tail = list;
    for (;;) {
	str = get_word(fp);
	if ((str == (char *)EOF) || str == NULL) {
	    *read_ahead = str;
	    break;
	}
	new = alloc_name_expr(str);
	if (!new) {
	    *read_ahead = str;
	    break;
	}
	if (tail) tail->next = new;
	else {
	    list = new;
	}
	tail = new;
    }
    list = paren_squish(list);
    list = not_squish(list);
    list = or_squish(list);
    return list;
}
int is_simple(expr)
     struct name_expr *expr;
{
    return expr->type == T_IDENTIFIER;
}

int f_not(expr,explain)
     struct name_expr *expr;
     int explain;
{
    int result;

    result = !depend_check(expr->left,explain);
    return result;
}
int f_or(expr,explain)
     struct name_expr *expr;
     int explain;
{
    int result;

    if (depend_check(expr->left,explain))
	return 1;
    if (depend_check(expr->right,explain))
	return 1;
    return 0;
}
int f_identifier(expr,explain)
     struct name_expr *expr;
     int explain;
{
    struct opt *op;
    struct device *dp;

    for (op = opt; op != 0; op = op->op_next)
	if (opteq(op->op_name, expr->name)) return 1;
    for (dp = dtab; dp != 0; dp = dp->d_next)
	if (eq(dp->d_name, expr->name)) return 1;
    return 0;
}
print_expr(expr)
     struct name_expr *expr;
{
    struct name_expr *current;
    for (current = expr; current != NULL; current= current->next) {
	switch (current->type) {
	case T_NOT:
	    fprintf (stderr, "not ");
	    print_expr(current->left);
	    break;
	case T_OR:
	    print_expr(current->left);
	    fprintf (stderr, "or ");
	    print_expr(current->right);
	    break;
	case EXPR_GROUP:
	    fprintf (stderr, "(");
	    print_expr(current->left);
	    fprintf (stderr, ")");
	    break;
	case T_IDENTIFIER:
	    fprintf(stderr,"%s ", current->name);
	    break;
	default:
	    parse_err("unknown expression type");
	}
    }
}
int depend_check(expr, explain)
     struct name_expr *expr;
     int explain;
{
    struct name_expr *current;
    int result;

    for (current= expr; current; current= current->next) {
	switch(current->type) {
	case T_NOT:
	    result = f_not(current,0);
	    break;
	case T_OR:
	    result = f_or(current,0);
	    break;
	case EXPR_GROUP:
	    result = depend_check(current->left, 0);
	    break;
	case T_IDENTIFIER:
	    result = f_identifier(current,0);
	    break;
	}
	if (result) continue;
	return 0;
    }
    return 1;
}



read_file(filename, fatal_on_open, override)
     char *filename;
     int fatal_on_open, override;
{
    FILE *fp;
    size_t length;
    char ebuf[1024];
    
    fp = fopen(filename, "r");
    if (!fp)
	if (fatal_on_open) {
	    perror(filename);
	    exit(1);
	}
	else return;
    current_line = 0;
    current_file = filename;
    for (;;) {
	char *str, *kf_name, *read_ahead,*compile_with;
	extern char *get_word(),*get_quoted_word();
	int token, optional,driver,config_depend, is_dup,filetype;
	struct name_expr *depends_on,*requires;
	struct file_list *tp,*tmp, *fl,*pf;	
	enum {BEFORE_FILENAME,BEFORE_SPEC,BEFORE_DEPENDS,PAST_DEPENDS,
		  SPECIALS} parse_state;


	if (setjmp(parse_problem)) {
	    while (1) {
		str = get_word(fp);
		if (!str || (str == (char *) EOF)) break;
	    }
	    if (!str) current_line++;
	    continue;
	}
	str = get_word(fp);
	current_line++;
	if (str == NULL) continue;
	if (str == (char *) EOF) break;
	if (*str == '#') {
	    fprintf(stderr, "shouldn't get here");
	    exit(1);
	}
	parse_state = BEFORE_FILENAME;
	kf_name = read_ahead = compile_with = NULL;
	optional= driver = config_depend = filetype =0;
	depends_on = requires = NULL;
	is_dup = 0;
	while ((str != NULL) && (str != (char *)EOF)) {
	    switch (parse_state) {
	    case BEFORE_FILENAME: {
		kf_name = ns(str);
		parse_state = BEFORE_SPEC;
		break;
	    }
	    case BEFORE_SPEC: {
		token = file_tok(str);
		if ((token != T_OPTIONAL) && (token != T_STANDARD))
		    parse_err("unexpected token starts inclusion specification");
		optional = (token == T_OPTIONAL);
		parse_state = BEFORE_DEPENDS;
		break;
	    }
	    case BEFORE_DEPENDS: {
		depends_on = parse_name_expr(fp,str, &read_ahead);
		str = read_ahead;
		parse_state = PAST_DEPENDS;
		continue;
		break;
	    }
	    case PAST_DEPENDS:
	    case SPECIALS: 
		token = file_tok(str);
		switch (token) {
		case T_COMPILE_WITH:  {
		    str = get_quoted_word(fp);
		    if ((str == 0) || (str == (char *) EOF))
			parse_err("missing compile command string");
		    compile_with = ns(str);
		}
		case T_CONFIG_DEPENDENT: {
		    config_depend = 1;
		    break;
		}
		case T_DEVICE_DRIVER: {
		    driver = 1;
		    break;
		}
		case T_REQUIRES: {
		    requires = parse_name_expr(fp,NULL, &read_ahead);
		    if (!requires) 
			parse_err("'requires' but no expression");
		    str = read_ahead;			
		    continue;
		    break;
		}
		default:
		    parse_err("unexpected token");
		}
		break;
	    default:
		parse_err("unknown state");
	    }
	    str = get_word(fp);
	}
	if (parse_state == BEFORE_SPEC)
	    parse_err("filename, but no specification");
	if (!kf_name)
	    parse_err("no filename specified");
	fl = fl_lookup(kf_name);
	if (fl && !override) {
	    (void) sprintf(ebuf, "duplicate file name '%s'", kf_name);
	    parse_err(ebuf);
	}
	if ((pf = fl_lookup(kf_name)) &&
	    (pf->f_type != INVISIBLE || (pf->f_flags | DUPLICATE)))
	    is_dup = 1;
	else
	    is_dup = 0;
	if (override && ((tmp = fltail_lookup(kf_name)) != 0))
	    fprintf(stderr, "%s:%dLocal file %s overrides %s.\n",
		   current_file, current_line, kf_name, tmp->f_fn);
	if (!optional) {
	    if (driver)
		parse_err("'standard' incompatible with 'device-driver'");
	    if (depends_on)
		parse_err("'standard' can't have dependencies");
	}
	else if (!depends_on) 
	    parse_err("'optional' requires dependency specification");
	if (driver && !is_simple(depends_on))
	    parse_err("device-driver's must have a singular name");
	if (driver && eq("profiling-routine", depends_on->name))
	    parse_err("not a valid device-driver name");
	if (is_simple(depends_on) &&
	    eq("profiling-routine", depends_on->name)) filetype = PROFILING;
	else if (depend_check(depends_on,0)) filetype = NORMAL;
	else filetype = INVISIBLE;

	if (filetype == NORMAL && requires && !depend_check(requires,0)) {
	    fprintf(stderr, "%s:%d: requirement expression failed: ",
		    current_file, current_line);
	    print_expr(requires);
	    fprintf(stderr, "\n");
	    parse_err("requirements not met");
	}
	tp = new_fent();
	tp->f_fn = kf_name;
	tp->f_type = filetype;
	if (driver)
	    tp->f_needs = depends_on->name;
	else tp->f_needs = NULL; /* memory leak */
	tp->f_was_driver = driver;
	tp->f_special = compile_with;
	tp->f_flags = 0;
	tp->f_flags |= (config_depend ? CONFIGDEP : 0);
	tp->f_flags |= (is_dup ? DUPLICATE : 0);
    }
    return;

}
