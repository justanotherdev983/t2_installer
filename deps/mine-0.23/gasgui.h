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

#ifndef PKGMAN_H
#define PKGMAN_H 1

/* struct statfs */
#include <sys/vfs.h>

struct dependency {
	char *name;
	struct package *pkg;
	struct dependency *next;
};

struct package {
	char *name, *version;
	char *installed_version;
	int is_installed, disk_number, is_dependency;
	int action; /* -1=remove, 0=nothing, 2=install */
	int *size; /* filesystem_count + 1 elements */
	struct package *next;
	struct dependency *deps;
};

struct directory_entry {
	union {
		struct package * pkg;
		struct directory *dir;
	} content;
	int content_is_subdir;
	struct directory_entry *next;
};

struct directory {
	char *name;
	int is_open_in_gui;
	struct directory_entry *list;
	struct directory *next;
};

struct filesystem {
	char *device;
	char *directory;
	struct statfs fs_data;
	struct filesystem *next;
	long long install;
	int useit;
};

extern struct package * packages;
extern struct directory * directories;
extern struct directory * rootdir;
extern int directory_entry_count;
extern int max_disk_number;

extern struct filesystem *filesystems;
extern int filesystem_count;

extern char * config;
extern char * targetdir;
extern char * sourcedev;
extern char * sourcedir;
extern char * sourceopt;

int read_packages();
void print_dirtree(struct directory * dir, int blanks);
int do_selection();

#endif /* PKGMAN_H */

