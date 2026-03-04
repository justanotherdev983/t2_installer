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

/* we need to reverse the package file list so we first remove
 * the content of a directory and after that the directory itself.
 * so we temporarily store the data in a linked list.
 */
struct flist;
struct flist {
	char *filename;
	struct flist *next;
};

int gem_remove(char * root, int mode_test, int mode_verbose, 
               int mode_force, int mode_sub, char * package)
{
	struct flist *flist_tmp, *flist = 0;
	char buffer[1024], realfn1[1024], realfn2[1024];
	char buf1[1024], buf2[1024];
	char *filename;
	int sub_done = 0;
	int errors = 0;
	FILE *f;

	if ( ! mode_force )
		md5sum_initdb(root, mode_verbose);

	if ( mode_sub && !strchr(package, ':') ) {
		DIR *d;

		snprintf(buffer, 1024, "%s/var/adm/flists", root);
		d = opendir(buffer);
		if ( d ) {
			int len = snprintf(buffer, 1024, "%s:", package);
			struct dirent *de;
			while ( (de = readdir(d)) != 0 ) {
				if ( !strncmp(de->d_name, buffer, len) ) {
					if ( mode_verbose )
						printf("Removing sub-package of %s: %s\n", package, de->d_name);
					errors += gem_remove(root, mode_test, mode_verbose, mode_force, 0, de->d_name);
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
			return errors != 0;
		fprintf(stderr, "No such package: %s\n", package);
		return 1;
	}

	if ( mode_test && mode_verbose )
		printf("-- %s --\n", package);

	while ( fgets(buffer, 1024, f) != NULL ) {
		strtok(buffer, " \t\n");
		filename = strtok(NULL, "\n");
		if (filename) {
			flist_tmp = malloc(sizeof(struct flist));
			flist_tmp->filename = strdup(filename);
			flist_tmp->next = flist;
			flist = flist_tmp;
		}
	}
	fclose(f);

	FILE *logfile = NULL;
	if ( ! mode_test ) 
	{
		char postinst[1024];
		snprintf(postinst, 1024, "%s/var/adm/postinstall/%s-remove.XXXXXX",
					 root, package);
		if ( mkstemp(postinst) != -1 ) 
			logfile = fopen(postinst, "w");
		if ( mode_verbose ) {
			if ( logfile == NULL )
				printf("Not writing postremove log\n");
			else
				printf("Writing postremove log to %s\n", postinst);
		}
	}

	while ( flist )
	{
		filename = flist->filename;
		flist = (flist_tmp=flist)->next;
		free(flist_tmp);

		if ( ! mode_force && md5sum_check(root, filename) ) {
			if ( ! mode_test || ! mode_verbose )
				printf("%s: ", package);
			printf("WARNING: Skip modified/duplicate "
			       "file: %s\n", filename);
		}
		else
		if ( mode_test ) {
			if ( ! mode_verbose )
				printf("%s: ", package);
			printf("removing %s\n", filename);
		}
		else {
			struct stat statbuf;

			snprintf(realfn1, 1024, "%s/%s", root, filename);
			if ( lstat(realfn1, &statbuf) )
				printf("WARNING: Could not stat file %s.\n",
				       realfn1);

			if ( ! strncmp(filename, "var/adm/", 8) &&
			     ! (S_ISDIR(statbuf.st_mode)) ) {
				if ( mode_verbose )
					printf("%s: moving %s to var/adm/backup\n",
					package, filename);

				snprintf(realfn1, 1024, "%s/var/adm/backup",
				         root);
				mkdir(realfn1, 0700);

				snprintf(realfn1, 1024, "%s", filename+8);
				sscanf(realfn1, "%[^/]/%[^/]", buf1, buf2);

				snprintf(realfn1, 1024, "%s/%s", root, filename);
				snprintf(realfn2, 1024, "%s/var/adm/backup/%s_%s",
				         root, buf2, buf1);

				if ( rename(realfn1, realfn2) ) {
					printf("While removing package %s: %s: %s\n",
					       package, realfn1, strerror(errno));
					errors++;
				}
			}
			else
			{
				if ( mode_verbose )
					printf("%s: removing %s\n",
					       package, filename);
				if ( remove(realfn1) && errno != ENOTEMPTY ) {
					printf("While removing package %s: %s: %s\n",
					       package, realfn1, strerror(errno));
					errors++;
				}
			}
			if ( logfile != NULL )
				fprintf(logfile, "%s: %s\n", package, filename);
		}

		free(filename);
	}

	if ( logfile != NULL ) fclose(logfile);

	if ( errors )
		fprintf(stderr, "%d error%s while removing package %s.\n",
		                errors, errors != 1 ? "s" : "", package);
	return errors != 0;
}

