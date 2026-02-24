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

#ifndef MINE_H
#define MINE_H

char ** add_string_array(char ** strings, char * new_string);

void free_string_array(char ** strings);

int glob_check(char ** patterns, char * filename);

extern int gem_check(char * root, int mode_sub, char * package);

extern int gem_create(char * varadm,  char * tarbz2,
                      char * package, char * outfile);

extern int gem_install(char * root, int mode_test, int mode_verbose,
                       int mode_force, char * package, char ** patterns);

extern int gem_remove(char * root, int mode_test, int mode_verbose,
                      int mode_force, int mode_sub, char * package);

extern int gem_pkglist(char * root, int mode_sub, char * package);

extern int gem_showfile(char * root, char * name, int mode_header_block, int mode_sub, char * package);

extern int gem_showkey(char * name, int mode_header_line,
                       int mode_header_block, char * package);

extern int gem_mkpdb(char * package);

#endif

