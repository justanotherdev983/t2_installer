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
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>

#include "cdb.h"

#include "mine.h"

int gem_pkglist_gemfile(char * package)
{
        struct cdb c;
	uint32 pos, len;
        char * buffer;
	char * line;
        int fd=-1, rc;
        int ret = 1;

        fd = open(package, O_RDONLY);
        if (fd == -1) goto error_errno;
        cdb_init(&c, fd);

	rc = cdb_find(&c, "packages", 8);
	if ( rc <= 0 ) goto error;
	pos = cdb_datapos(&c);
	len = cdb_datalen(&c);
	buffer = malloc(len+1);
	buffer[len] = 0;
	if (cdb_read(&c, buffer, len, pos) == -1) goto error;

	line=strtok(buffer, "\n");
	while (line != NULL) {
		if (!strncmp("Package Name and Version: ", line, 26))
			{ puts(line + 26); ret=0; break; }
		line=strtok(NULL, "\n");
	}

	if (ret)
		fprintf(stderr, "While parsing package '%s': Package "
		                "name and version not found!\n", package);

	free(buffer);
	cdb_free(&c);
	close(fd);
	return ret;

error:
        errno = 0;

error_errno:
        fprintf(stderr, "While reading GEM file %s: %s\n", package,
                        errno ? strerror(errno) : "Unknown error");
        if (fd != -1) { cdb_free(&c); close(fd); }
        return 1;
}

int gem_pkglist(char * root, int mode_sub, char * package)
{
        char buffer[1024];
        FILE *f;

	if ( mode_sub && !strchr(package, ':') ) {
		DIR *d;

		snprintf(buffer, 1024, "%s/var/adm/flists", root);
		d = opendir(buffer);
		if ( d ) {
			int len = snprintf(buffer, 1024, "%s:", package);
			struct dirent *de;
			while ( (de = readdir(d)) != 0 ) {
				if ( !strncmp(de->d_name, buffer, len) )
					gem_pkglist(root, 0, de->d_name);
			}
			closedir(d);
		}
	}

        snprintf(buffer, 1024, "%s/var/adm/packages/%s", root, package);
        if ( (f = fopen(buffer, "r")) == NULL )
                return gem_pkglist_gemfile(package);

        while ( fgets(buffer, 1024, f) != NULL )
		if (!strncmp("Package Name and Version: ", buffer, 26))
			fputs(buffer + 26, stdout);
	fclose(f);

	return 0;
}

