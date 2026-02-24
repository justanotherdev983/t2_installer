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

#ifndef MEMDB_H
#define MEMDB_H

#ifdef USE_AVL
#include "avl.h"
#endif

struct memdb_entry_t {
#ifdef USE_AVL
	struct avl avl;
#endif
#ifdef USE_HASHOPT
	int key_hash;
#endif
	char *key, *value;
	struct memdb_entry_t *next;
};

struct memdb_t {
#ifdef USE_AVL
	struct avl_tree avl_tree;
#endif
	struct memdb_entry_t *first;
};

extern void memdb_init(struct memdb_t *db);
extern void memdb_put_noalloc(struct memdb_t *db, char *key, char *value);
extern void memdb_put(struct memdb_t *db, char *key, char *value);
extern char * memdb_get(struct memdb_t *db, char *key);
extern void memdb_free(struct memdb_t *db);

extern char * memdb_search_result;

#endif

