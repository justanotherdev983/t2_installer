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

#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "md5sum.h"
#include "memdb.h"

struct memdb_t md5_memdb;
int md5_memdb_filled;

/* This function is defined in md5.c (which is from GNU textutils 2.0) */
extern char * md5_file(const char *filename);

char * md5sum_create(char * root, char * filename) {
	char realfilename[1024];
	struct stat statbuf;

	snprintf(realfilename, 1024, "%s/%s", root, filename);
	return (stat(realfilename, &statbuf) != 0 || S_ISFIFO(statbuf.st_mode))
		? "" : md5_file(realfilename);
}

/* Returns 1 if file is duplicate, 2 if file is modified. */
int md5sum_check(char * root, char * filename)
{
	char *md5_f, *md5_d;

	md5_f = md5sum_create(root, filename);
	md5_d = memdb_get(&md5_memdb, filename);
	
	if (md5_f == NULL || strcmp(md5_f, "") == 0)
		return 0;
	else if (md5_d == NULL)
		return 1;
	else if (strcmp(md5_f, md5_d) == 0)
		return 0;
	else
		return 2;
}

void md5sum_initdb(char * root, int verbose)
{
	struct dirent *md5_dent;
	char buffer[1024], buffer2[1024];
	char *md5sum, *filename;
	const char const *admdirs[6] = 
	{"cksums", "dependencies", "descs", "flists", "md5sums", "packages"};
	FILE *f;
	DIR *d;

	if ( md5_memdb_filled ) return;

	snprintf(buffer, 1024, "%s/var/adm/md5sums", root);
	if ( (d = opendir(buffer)) == NULL ) return;
	if (verbose) printf("Reading MD5 checksum database into memory ...\n");
	memdb_init(&md5_memdb);

	while ( (md5_dent = readdir(d)) != NULL ) {
		snprintf(buffer, 1024, "%s/var/adm/md5sums/%s",
		                       root, md5_dent->d_name);
		f = fopen(buffer, "r");
		if ( f != NULL ) {
			/* Add the /var/adm files of each package to the 
			   md5sum memdb so md5sum_check() recognizes them. */
			int n;
			for (n = 0; n < 6; n++) {
				snprintf(buffer2, 1024, "var/adm/%s/%s",
					admdirs[n], md5_dent->d_name);
				memdb_put(&md5_memdb, buffer2, 
					md5sum_create(root, buffer2));
			}

			while (fgets(buffer, 1024, f) != NULL) {
				md5sum = strtok(buffer, " \t\n");
				filename = strtok(NULL, "\n");
				if ( md5sum && filename) {
					while ( *filename == ' ' ) filename++;
					memdb_put(&md5_memdb, filename, md5sum);
				}
			}
			fclose(f);
		}
	}
	closedir(d);

	md5_memdb_filled = 1;
}
