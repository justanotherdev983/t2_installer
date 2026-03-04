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
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#include "mine.h"
#include "md5sum.h"

int gem_check(char * root, int mode_sub, char * package)
{
	char buffer[1024];
	char *filename;
	int modified = 0;
	int sub_done = 0;
	FILE *f;

	md5sum_initdb(root, 0 /* verbose */);

	if ( mode_sub > 0 && !strchr(package, ':') ) {
		DIR *d;

		snprintf(buffer, 1024, "%s/var/adm/flists", root);
		d = opendir(buffer);
		if ( d ) {
			int len = snprintf(buffer, 1024, "%s:", package);
			struct dirent *de;
			while ( (de = readdir(d)) != 0 ) {
				if ( !strncmp(de->d_name, buffer, len) ) {
					modified += gem_check(root, -1, de->d_name);
					sub_done = 1;
				}
			}
			closedir(d);
		}
	}

	snprintf(buffer, 1024, "%s/var/adm/flists/%s", root, package);
	f = fopen(buffer, "r");
	if ( f == NULL ) {
		if ( sub_done )
                        return modified == 0;
		fprintf(stderr, "No such package: %s\n", package);
		return 1;
	}

	while ( fgets(buffer, 1024, f) != NULL )
	{
		strtok(buffer, " \t\n");
		filename = strtok(NULL, "\n");

		if ( md5sum_check(root, filename) ) {
			printf("Modified/duplicate "
			       "file: %s\n", filename);
			modified++;
		}
	}

	fclose(f);

	if ( mode_sub == -1 )
		return modified;

	if ( modified )
		fprintf(stderr, "%d modified file%s of package %s.\n",
		                modified, modified != 1 ? "s" : "", package);
	return modified == 0;
}

