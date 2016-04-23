// hash.cpp : Defines the entry point for the console application.
//


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hashtab.h"



static unsigned int symhash(struct hashtab *h, void *key){

	char *p, *keyp;
	unsigned int size;
	unsigned int val;

	val = 0;
	keyp = key;
	size = strlen(keyp);
	for (p = keyp; (p - keyp) < size; p++)
		val = (val << 4 | (val >> (8*sizeof(unsigned int)-4))) ^ (*p);
	return val & (h->size - 1);
}

static int symcmp(struct hashtab *h, void *key1, void *key2){

	char *keyp1, *keyp2;

	h = h;

	keyp1 = key1;
	keyp2 = key2;
	return strcmp(keyp1, keyp2);
}

struct hashtab* g_table; 

int main(int argc, char* argv[]){

	int a, b;

	argc = argc;
	argv = argv;

	

	g_table = hashtab_create(symhash, symcmp, 512);

	hashtab_insert( g_table, &a, &b );

	hashtab_destroy( g_table);

	return 0;
}
