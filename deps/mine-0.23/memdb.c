/*
 *  GEM MINE - The ROCK Linux Package Manager
 *  Copyright (C) 2002-2005 Clifford Wolf
 *  
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdlib.h>
#include <string.h>

#include "memdb.h"

char * memdb_search_result = NULL;

#ifdef USE_HASHOPT
int get_key_hash(char * key) {
	int c, hash=0;

	/* This is from Knuth's "The Art of Computer Programming",
	 * volume 3 "Sorting and Searching", chapter 6.4.
	 *
	 * I think it's not optimal - but it is good enough for our
	 * purposes.
	 */
	for (hash=c=strlen(key); c--;) 
	     hash = ((hash<<5)^(hash>>27))^key[c];

	return hash;
}
#endif /* USE_HASHOPT */

#ifdef USE_AVL

int compare_entries(struct memdb_entry_t * a, struct memdb_entry_t * b) {
#ifdef USE_HASHOPT
	if ( a->key_hash != b->key_hash )
		return a->key_hash < b->key_hash ? -1 : +1;
#endif /* USE_HASHOPT */
	return strcmp(a->key, b->key);
}

int found_something(struct memdb_entry_t * a) {
	if (memdb_search_result)
		memdb_search_result = "Duplicate entry";
	else 
		memdb_search_result = a->value;
	return 1;
}

#endif /* USE_AVL */

void memdb_init(struct memdb_t *db) {
	db->first = NULL;
#ifdef USE_AVL
	db->avl_tree.compar = (int(*)(void*, void*)) compare_entries;
	db->avl_tree.root = NULL;
#endif /* USE_AVL */
}

void memdb_put_noalloc(struct memdb_t *db, char *key, char *value) {
	struct memdb_entry_t *p;

	p = malloc(sizeof(struct memdb_entry_t));
	p->key  = key;		p->value  = value;
	p->next = db->first;	db->first = p;

#ifdef USE_HASHOPT
	p->key_hash = get_key_hash(key);
#endif /* USE_HASHOPT */

#ifdef USE_AVL
	avl_insert((struct avl_tree*)db,(struct avl*)p);
#endif /* USE_AVL */
}

void memdb_put(struct memdb_t *db, char *key, char *value) {
	struct memdb_entry_t *p;

	p = malloc(sizeof(struct memdb_entry_t));

	p->key = malloc( strlen(key) + 1 );
	strcpy(p->key, key);

	p->value = malloc( strlen(value) + 1 );
	strcpy(p->value, value);

	p->next = db->first;
	db->first = p;

#ifdef USE_HASHOPT
	p->key_hash = get_key_hash(key);
#endif /* USE_HASHOPT */

#ifdef USE_AVL
	avl_insert((struct avl_tree*)db,(struct avl*)p);
#endif /* USE_AVL */
}

#ifdef USE_AVL

char * memdb_get(struct memdb_t *db, char *key) {
	struct memdb_entry_t p;

	p.key = key;
	memdb_search_result = NULL;

#ifdef USE_HASHOPT
	p.key_hash = get_key_hash(key);
#endif /* USE_HASHOPT */

	avl_search((struct avl_tree*)db, (struct avl*)&p,
	           (int(*)(avl*)) found_something);

	return memdb_search_result;
}

#else /* USE_AVL */

char * memdb_get(struct memdb_t *db, char *key) {
	struct memdb_entry_t *p;
#ifdef USE_HASHOPT
	int key_hash = get_key_hash(key);
#endif /* USE_HASHOPT */

	p = db->first;
	memdb_search_result = NULL;

	while ( p != NULL ) {
#ifdef USE_HASHOPT
	    if ( key_hash == p->key_hash )
#endif /* USE_HASHOPT */
		if ( ! strcmp(key, p->key) ) {
			if ( memdb_search_result == NULL)
				memdb_search_result = p->value;
			else
				memdb_search_result = "Duplicate entry";
		}
		p = p->next;
	}

	return memdb_search_result;
}

#endif /* USE_AVL */

void memdb_free(struct memdb_t *db) {
	struct memdb_entry_t *p, *n;

	p = db->first;
	while ( p != NULL ) {
		n = p->next;
#ifdef USE_AVL
		avl_remove((struct avl_tree*)db,(struct avl*)p);
#endif /* USE_AVL */
		free(p->key);
		free(p->value);
		free(p);
		p = n;
	}

	db->first = NULL;
}

