/*
** Gabriel Schneider - 2020
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <assert.h>

//#define DEBUG_F 1

#ifdef DEBUG_F

#define DEBUG(fmt, args...)						\
	printf("(%s:%d) " fmt, __FILE__,__LINE__, ##args)

#else

#define DEBUG(f, fmt, args...) /* Do nothing */

#endif

#define MAX_INPUT_STR 100000
#define MAX_SYM_SZ 100

//TODO: create a function register

enum Token_Type {
	TOKEN_OPEN=0,
	TOKEN_CLOSE=1,
	TOKEN_NUMBER=2,
	TOKEN_SYMBOL=3,
};

/*
** Used to debug the lexer
 */
char* Token_Type_Str[4] = {"(", ")", "NUM", "SYM"};

typedef struct {
	enum Token_Type type;

	union {
		int number;
		char* name;
	}data;

}Token;

Token* tokenize(char* in, int* ret_sz) {
	int cursor = 0;
	int sz = 0, alloc=1;
	Token* vec = (Token*) malloc(sizeof(Token));
	assert(vec != NULL);

	while(in[cursor] != '\0') {

		Token cur;

		if(in[cursor]=='(') {
			cur.type = TOKEN_OPEN;
			cursor++;

		} else if(in[cursor]==')') {
			cur.type = TOKEN_CLOSE;
			cursor++;

		} else if(isdigit(in[cursor])) {
			int number;
			sscanf(in+cursor,"%d",&number);


			while(in[cursor] != '\0'
				  && isdigit(in[cursor])) {
				cursor++;
			}

			cur.type = TOKEN_NUMBER;
			cur.data.number = number;
			DEBUG("TOKEN_NUMBER: %d\n", number);

		} else if(isalpha(in[cursor])) {

			char* name = (char*) malloc(MAX_SYM_SZ);
			assert(name!=NULL);
		
			int ptr = 0;

			while(in[cursor] != '('
				  && in[cursor] != ')'
		 		  && in[cursor] != ' '
				  && in[cursor] != '\0')
			  {

				if(isalpha(in[cursor])) in[cursor] = toupper(in[cursor]);
				name[ptr] = in[cursor];
				ptr++, cursor++;
			}

			name[ptr] = '\0';

			cur.type = TOKEN_SYMBOL;
			//cur.data.name = (char*) malloc(MAX_SYM_SZ);
			//strcpy(cur.data.name, name);
			cur.data.name = name;

			DEBUG("TOKEN_NAME: %s, %s\n", name, cur.data.name);

		} else {
			cursor++;
			continue;
		}

		if(alloc <= sz+1) {

			alloc *= 2;
			vec = (Token*) realloc(vec, sizeof(Token)*alloc);
		}

		DEBUG("TOKEN: %s\n", Token_Type_Str[cur.type]);

		vec[sz++] = cur;
	}

	*ret_sz = sz;

	return vec;
}

typedef uintptr_t Lisp_Object;
#define NIL (Lisp_Object)0

Lisp_Object ENV = NIL;


/*
** How functions work in myLisp: There are two kinds of functions, builtin and
** user-defined. Every symbol has a slot pointing to a struct which holds a
** pointer to the C function and the number of arguments it has.
 */

typedef struct {
	int nargs; // nargs are set to -1 if this is not a valid function
	//TODO: &optional, &key, &rest, etc...
	union {
		Lisp_Object (*f0)(void);
		Lisp_Object (*f1)(Lisp_Object);
		Lisp_Object (*f2)(Lisp_Object, Lisp_Object);
		Lisp_Object (*f3)(Lisp_Object, Lisp_Object, Lisp_Object);
		Lisp_Object (*f4)(Lisp_Object, Lisp_Object, Lisp_Object, Lisp_Object);
	}ptr;
}Lisp_Function;

typedef struct {
	char* name; // Symbol's name
	Lisp_Object (*function)(Lisp_Object); // Builtin function pointer
	//TODO: use this, lol
	Lisp_Function fn;
	Lisp_Object obj; // The object which is stored in the symbol
	//Lisp_Object fn_obj; // User defined function, this slot holds the function stored by the symbol. AND WE WONT USE IT FOR NOW
}Lisp_Symbol;


enum Tag {
	TAG_SYMBOL = 0,
	TAG_NUMBER = 1,
	TAG_CONS = 2
	/* ... */
};

Lisp_Object ptr_tag(Lisp_Object obj, enum Tag tag) {
	return obj | (int)tag;
}

Lisp_Object ptr_untag(Lisp_Object obj) {
	return obj & ~((Lisp_Object) 7);
}

int ptr_getTag(Lisp_Object obj) {
	return (int)(obj&7);
}

#define OBJ(val, type)							\
	Obj_New_ ## type (val)

Lisp_Object Obj_New_symbol(char* str) {

	Lisp_Symbol* symb = (Lisp_Symbol*) malloc(sizeof(Lisp_Symbol));
	symb->obj = NIL;
	symb->fn = (Lisp_Function){.nargs=-1};

	char* nstr = (char*) malloc(strlen(str)+1);
	strcpy(nstr, str);
	nstr[strlen(str)]='\0';
	int sz = strlen(str);

	for(int i=0;i<sz;i++) nstr[i] = toupper(nstr[i]);

	symb->name = nstr;
	Lisp_Object ret = ptr_tag((Lisp_Object)symb, TAG_SYMBOL);

	return ret;
}

Lisp_Object Obj_New_number(int val) {
	Lisp_Object nval = val;
	nval = val<<3;

	return ptr_tag(nval, TAG_NUMBER);
}

typedef struct {

	Lisp_Object car;
	Lisp_Object cdr;

}Lisp_Cons_Cell;

Lisp_Object fcons(Lisp_Object a, Lisp_Object b) {
	Lisp_Cons_Cell* cell = (Lisp_Cons_Cell*) malloc(sizeof(Lisp_Cons_Cell));
	cell->car = a, cell->cdr = b;

	return ptr_tag((Lisp_Object)cell, TAG_CONS);
}

#define GET_VAL(a, type)						\
	GET_VAL_ ## type (a)

#define GET_VAL_number(a)						\
	(int)(a>>3)

#define GET_VAL_cons(a)							\
	(Lisp_Cons_Cell*) ptr_untag(a)

// For now we just return the symbol's name
// TODO: maybe rename get_val to get representation or something
#define GET_VAL_symbol(a)						\
	(char*) (((Lisp_Symbol*)ptr_untag(a))->name)

Lisp_Object fcar(Lisp_Object a) {
	assert(ptr_getTag(a) == TAG_CONS);

	return (GET_VAL(a, cons))->car;
}

Lisp_Object fcdr(Lisp_Object a) {
	assert(ptr_getTag(a) == TAG_CONS);

	return (GET_VAL(a, cons))->cdr;
}

Lisp_Object fcadr(Lisp_Object a) {
	return fcar(fcdr(a));
}


void _Lisp_Print(Lisp_Object obj, int head);

void Lisp_Print_cons(Lisp_Object obj, int head) {
	if(head) putchar('(');

	_Lisp_Print(fcar(obj), 1);

	if(ptr_untag(fcdr(obj))==NIL) {
		putchar(')');
	} else if(ptr_getTag(fcdr(obj))!=TAG_CONS) {
		printf(" . ");
		_Lisp_Print(fcdr(obj), 0);
		putchar(')');
	} else {
		putchar(' ');
		_Lisp_Print(fcdr(obj), 0);
	}
}


void _Lisp_Print(Lisp_Object obj, int head) {
	enum Tag tag = ptr_getTag(obj);

	switch(tag) {
		case TAG_NUMBER:
			printf("%d", GET_VAL(obj, number));
			break;
		case TAG_SYMBOL:
			if(obj == NIL) {
				printf("nil");
			} else {
				printf("%s", GET_VAL(obj, symbol));
			}
			break;
		case TAG_CONS:
			Lisp_Print_cons(obj, head);
			break;
		default:
			break;
	}
}


void Lisp_Print(Lisp_Object obj) {
	_Lisp_Print(obj, 1);
	putchar('\n');
}

Lisp_Object parse(Token* tokens, int pos, int sz) {
	if(pos==sz) return NIL; /* the stop condition */

	if(tokens[pos].type == TOKEN_OPEN) {

		Lisp_Object car = parse(tokens,pos+1,sz);

		int aux = 1, balance = -1;

		while(balance != 0) { /*  Looking for the matching ) */

			assert(aux+pos < sz);

			if(tokens[pos+aux].type == TOKEN_OPEN)
				balance--;
			if(tokens[pos+aux].type == TOKEN_CLOSE)
				balance ++;

			aux++;
		}

		Lisp_Object cdr = parse(tokens, pos+aux, sz);

		return fcons(car, cdr);

	} else if(tokens[pos].type == TOKEN_CLOSE) {
		return NIL;
	} else if(tokens[pos].type == TOKEN_NUMBER) {
		int n = tokens[pos].data.number;
		Lisp_Object car = OBJ(n, number);

		return fcons(car, parse(tokens, pos+1, sz));
	} else if(tokens[pos].type == TOKEN_SYMBOL) {
		char* str = tokens[pos].data.name;

		Lisp_Object car;
		if(strcmp(str, "NIL") == 0) { // added some NIL sematics
			car = NIL;
		} else {
			car = OBJ(str, symbol);
		}
		return fcons(car, parse(tokens, pos+1, sz));
	}

	return NIL;
}

Lisp_Object fintern(char* name) {
	//TODO: check if name is already defined
	return Obj_New_symbol(name);
}

// Add symbol with builtin function to the enviroment.
void register_function(Lisp_Object sym, Lisp_Object (*function)(Lisp_Object)) {
	((Lisp_Symbol*) ptr_untag(sym))->function = function;

	ENV = fcons(sym, ENV);
}


void prot_register_function(Lisp_Object sym, Lisp_Object (*function)(), int nargs) {

	Lisp_Function fn = (Lisp_Function){.nargs=nargs};
	if(nargs==0) {
		fn.ptr.f0 = function;
	} else if(nargs==1) {
		fn.ptr.f1 = function;
	} else if(nargs==2) {
		fn.ptr.f2 = function;
	} else if(nargs==3) {
		fn.ptr.f3 = function;
	} else if(nargs==4) {
		fn.ptr.f4 = function;
	}

	((Lisp_Symbol*) ptr_untag(sym))->fn = fn;

	ENV = fcons(fcons(sym, NIL), ENV);
}

Lisp_Object test_fn(Lisp_Object a, Lisp_Object b) {
	puts("test_fn called");
	return fcons(a, b);
}

Lisp_Object test_hello() {
	return fintern("hello-there");
}

// something like funcall but it uses the funcion pointer,
// where funcall uses the symbol's value
Lisp_Object call_builtin(Lisp_Object car, Lisp_Object cdr) {

	Lisp_Object ans=NIL;

	if(((Lisp_Symbol*) ptr_untag(car))->function != NULL) {

		DEBUG("call_builtin: %s\n", GET_VAL(car, symbol));

		Lisp_Object (*fn_ptr)(Lisp_Object) = ((Lisp_Symbol*) ptr_untag(car))->function;
		fn_ptr(cdr);

		ans=fn_ptr(cdr);
	}

	printf("call_builtin: ");
	_Lisp_Print(cdr, 1);
	printf(" => ");
	Lisp_Print(ans);

	return ans;
}

Lisp_Object fnth(Lisp_Object list, int n) {
	Lisp_Object cur = list;
	int iter = 0;
	while(iter < n) {
		iter++;
		cur = fcdr(cur);
	}
	return fcar(cur);
}

Lisp_Object prot_call_builtin(Lisp_Object fn, Lisp_Object args) {

	Lisp_Object ans=NIL;

	int nargs = ((Lisp_Symbol*) ptr_untag(fn))->fn.nargs;

	if(nargs == 0) {
		ans = ((Lisp_Symbol*) ptr_untag(fn))->fn.ptr.f0();
	} else if(nargs == 1) {
		ans = ((Lisp_Symbol*) ptr_untag(fn))->fn.ptr.f1(fcar(args));
	} else if(nargs == 2) {
		ans = ((Lisp_Symbol*) ptr_untag(fn))->fn.ptr.f2(fcar(args), fcar(fcdr(args)));
	} else {
		printf("NARGS NOT SUPPORTED! (%d)\n", nargs);
		return NIL;
	}

	return ans;
}

Lisp_Object fatom(Lisp_Object expr) {
	if(ptr_getTag(expr)!= TAG_CONS) {
		return fintern("t");
	}

	return NIL;
}

void fpush_env(Lisp_Object a) {

	ENV = fcons(a,ENV);
}

//TODO: implement eq for any object
Lisp_Object feq(Lisp_Object a, Lisp_Object b) {

	if(a==NIL || b == NIL) return NIL;

	if(ptr_getTag(a)==ptr_getTag(b)) {
		if(ptr_getTag(a)==TAG_SYMBOL) {
			if(strcmp(((Lisp_Symbol*)a)->name,((Lisp_Symbol*)b)->name) == 0) {
				return fintern("t");
			}
		}
	}

	return NIL;
}

Lisp_Object fassoc(Lisp_Object a, Lisp_Object e) {
	Lisp_Object cur = e;

	while(cur!=NIL && feq(a,fcar(fcar(cur))) == NIL) {
		cur = fcdr(cur);
	}

	if(cur != NIL) return fcdr(fcar(cur));

	return cur;
}

Lisp_Object flength(Lisp_Object a) {
	int sz=0;
	Lisp_Object cur = a;
	while(a!=NIL) {
		a = fcdr(a);
		sz++;
	}

	return OBJ(sz, number);
}

Lisp_Object eval(Lisp_Object expr, Lisp_Object env) {

	Lisp_Object ans = NIL;

	if(fatom(expr)!=NIL) {
		if(ptr_getTag(expr) == TAG_SYMBOL) ans = fassoc(expr, env);
		if(ptr_getTag(expr) == TAG_NUMBER) ans = expr;
	} else if(fatom(fcar(expr))!=NIL) {
		if(feq(fintern("quote"), fcar(expr))!=NIL) {
			ans = fcadr(expr);
		} else if(feq(fintern("setf"), fcar(expr))!=NIL) {
			Lisp_Object b = eval(fnth(expr, 2), env);
			fpush_env(fcons(fnth(expr, 1), b));
		} else if(feq(fintern("atom"), fcar(expr))!=NIL) {
			Lisp_Object a = eval(fcadr(expr), env);
			ans = fatom(a);
		} else if(feq(fintern("car"), fcar(expr))!=NIL) {
			Lisp_Object a = eval(fcadr(expr), env);
			ans = fcar(a);
		} else if(feq(fintern("cdr"), fcar(expr))!=NIL) {
			Lisp_Object a = eval(fcadr(expr), env);
			ans = fcdr(a);
		} else if(feq(fintern("eq"), fcar(expr))!=NIL) {
			Lisp_Object a = eval(fcadr(expr), env);
			Lisp_Object b = eval(fcadr(fcdr(expr)), env);
			ans = feq(a, b);
		} else if(feq(fintern("exit"), fcar(expr))!=NIL) {
			exit(0);
		} else if(feq(fintern("cons"), fcar(expr))!=NIL) {
			Lisp_Object a = eval(fcadr(expr), env);
			Lisp_Object b = eval(fcadr(fcdr(expr)), env);
			ans = fcons(a, b);
		} else if(feq(fintern("length"), fcar(expr))!=NIL) {
			Lisp_Object a = eval(fcadr(expr), env);
			ans = flength(a);
		} else {
			Lisp_Object fn = fassoc(fcar(expr), env);

			printf("FN: ");
			Lisp_Print(fn);
			assert(fn != NIL);
			assert(fatom(fn)==NIL);
			assert(feq(fintern("lambda"), fcar(fn))!=NIL);

			ans = eval(fcons(fn, fcdr(expr)), env);
		}
	} else if(feq(fintern("lambda"), fcar(fcar(expr)))!=NIL) {
		Lisp_Object local_env = env;
		Lisp_Object arglist = fcadr(fcar(expr));
		Lisp_Object args = fcdr(expr);
		Lisp_Object body = fcadr(fcdr(fcar(expr)));

		assert(flength(arglist)==flength(args));

		while(args != NIL) {
			Lisp_Object p = fcons(fcar(arglist), eval(fcar(args), env));
			args = fcdr(args);
			arglist = fcdr(arglist);

			local_env = fcons(p, local_env);
		}

		ans = eval(body, local_env);
	}


	return ans;
}

int main(int argc, char** argv) {

	Lisp_Object input = NIL, res = NIL;


	fpush_env(fcons(fintern("a"), OBJ(12, number)));

	puts("Created by Gabriel Schneider - 2020\nWelcome to myLisp 0.0.0\n");

	while(feq(fintern("quit"), res)==NIL) {

		printf("REPL> ");

		char buf[MAX_INPUT_STR];
		scanf("%[^\n]", buf);
		getchar();

		int sz;
		Token* tokens = tokenize(buf, &sz);

		//DEBUG("Read %d tokens\n", sz);

		input = parse(tokens, 0, sz);
		input = fcar(input);

		for(int i=0;i<sz;i++) {
			if(tokens[i].type==TOKEN_SYMBOL) {
				free(tokens[i].data.name);
			}
		}

		free(tokens);

		res = eval(input, ENV);

		Lisp_Print(res);
	}


	//Lisp_Object b = OBJ(10, number);
	//Lisp_Object c = OBJ("test-fn", symbol);

	//Lisp_Object fn = fintern("test-fn");
	//Lisp_Object hellofn = fintern("hellofn");

	//prot_register_function(fn, test_fn, 2);
	//prot_register_function(hellofn, test_hello, 0);

	//Lisp_Object args = fcons(b, fcons(c, NIL));
	//Lisp_Object ans = prot_call_builtin(fn, args);


	//ans = prot_call_builtin(hellofn, NIL);




	//register_function(fn, test_fn);
	//call_builtin(fn, fcons(b,NIL));

	//Lisp_Print(fcons(a,fcons(b, NIL)));

	return 0;
}
