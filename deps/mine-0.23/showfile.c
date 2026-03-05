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

#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>

#include "mine.h"

int gem_showfile(char * root, char * name, int mode_header_block, int mode_sub, char * package)
{
	char buffer[1024];
	FILE *f;

	if ( mode_sub && !strchr(package, ':') ) {
		DIR *d;

		snprintf(buffer, 1024, "%s/var/adm/%s", root, name);
		d = opendir(buffer);
		if ( d ) {
			int len = snprintf(buffer, 1024, "%s:", package);
			struct dirent *de;
			while ( (de = readdir(d)) != 0 ) {
				if ( !strncmp(de->d_name, buffer, len) )
					gem_showfile(root, name, mode_header_block, 0, de->d_name);
			}
			closedir(d);
		}
	}

	snprintf(buffer, 1024, "%s/var/adm/%s/%s", root, name, package);
	if ( (f = fopen(buffer, "r")) == NULL ) {
		return gem_showkey(name, 0, mode_header_block, package);
	}

	if (mode_header_block)
		printf("\n-- %s --\n", package);

	while ( fgets(buffer, 1024, f) != NULL ) fputs(buffer, stdout);
	fclose(f);

	return 0;
}

