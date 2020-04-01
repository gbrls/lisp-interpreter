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
	char* nstr = (char*) malloc(strlen(str)+1);
	strcpy(nstr, str);
	nstr[strlen(str)]='\0';
	Lisp_Object ret = ptr_tag((Lisp_Object)nstr, TAG_SYMBOL);

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

#define GET_VAL_symbol(a)						\
	(char*) ptr_untag(a)

Lisp_Object fcar(Lisp_Object a) {
	assert(ptr_getTag(a) == TAG_CONS);

	return (GET_VAL(a, cons))->car;
}

Lisp_Object fcdr(Lisp_Object a) {
	assert(ptr_getTag(a) == TAG_CONS);

	return (GET_VAL(a, cons))->cdr;
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
			printf("%s", GET_VAL(obj, symbol));
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

		Lisp_Object car = OBJ(str, symbol);
		return fcons(car, parse(tokens, pos+1, sz));
	}

	return NIL;
}

int main(int argc, char** argv) {

	char buf[MAX_INPUT_STR];
	scanf("%[^\n]", buf);

	int sz;
	Token* tokens = tokenize(buf, &sz);

	//DEBUG("Read %d tokens\n", sz);

	Lisp_Object a = parse(tokens, 0, sz);
	Lisp_Print(a);

	for(int i=0;i<sz;i++) {
		if(tokens[i].type==TOKEN_SYMBOL) {
			free(tokens[i].data.name);
		}
	}

	free(tokens);

	//Lisp_Object a = OBJ(10, number);
	//Lisp_Object b = OBJ("hello-there", symbol);

	//Lisp_Print(fcons(a,fcons(b, NIL)));

	return 0;
}
