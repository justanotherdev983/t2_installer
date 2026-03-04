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

#include "cdb.h"

#include "mine.h"

static int dump_field(struct cdb *c, char *field)
{
	char * buffer;
	int pos, len;

	cdb_findstart(c);
	if (cdb_find(c, field, strlen(field)) <= 0) return -1;

	pos = cdb_datapos(c);
	len = cdb_datalen(c);

	buffer = malloc(len+1);
	buffer[len] = 0;

	if (cdb_read(c, buffer, len, pos) == -1) return -1;

	printf("%s", buffer);

	if (len > 0 && buffer[len-1] != '\n')
		putchar('\n');

	putchar('\027');
	putchar('\n');

	free(buffer);

	return 0;
}

int gem_mkpdb(char * package)
{
	struct cdb c;
	int fd = -1;

	fd = open(package, O_RDONLY);
	if (fd == -1) goto error_errno;
	cdb_init(&c, fd);

	if ( dump_field(&c, "pkg_name") == -1 ) goto error;
	if ( dump_field(&c, "descs") == -1 ) goto error;
	if ( dump_field(&c, "dependencies") == -1 ) goto error;
	if ( dump_field(&c, "cksums") == -1 ) goto error;

	putchar('\004');
	putchar('\n');

	cdb_free(&c);
	close(fd);
	return 0;

error:
	errno = 0;

error_errno:
	fprintf(stderr, "While reading GEM file %s: %s\n", package,
	                errno ? strerror(errno) : "Unknown error");
	if (fd != -1) { cdb_free(&c); close(fd); }
	return 1;
}

