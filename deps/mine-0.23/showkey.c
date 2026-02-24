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

int gem_showkey(char * name, int mode_header_line,
                int mode_header_block, char * package)
{
	struct cdb c;
	char * buffer;
	int found = 0;
	int fd=-1, rc;
	int pos, len;
	int ret = 0;

	fd = open(package, O_RDONLY);
	if (fd == -1) goto error_errno;
	cdb_init(&c, fd);

	if (mode_header_block) {
		rc = cdb_find(&c, "pkg_name", 8);
		if ( rc <= 0 ) goto error;
		pos = cdb_datapos(&c);
		len = cdb_datalen(&c);
		buffer = malloc(len+1);
		buffer[len] = 0;
		if (cdb_read(&c, buffer, len, pos) == -1) goto error;
		printf("\n-- %s --\n", buffer);
		free(buffer);
	}

	cdb_findstart(&c);
	while (1) {
		rc = cdb_findnext(&c, name, strlen(name));
		if ( rc == -1 ) goto error;
		if ( rc == 0  ) break;

		pos = cdb_datapos(&c);
		len = cdb_datalen(&c);
		if (len <= 0) continue;

		if (!strcmp(name, "pkg_tarbz2")) {
			buffer = malloc(512);
			while (len > 0) {
				if ( cdb_read(&c, buffer, len>512 ? 512 : len,
				              pos) == -1 ) goto error;
				write(1, buffer, len > 512 ? 512 : len);
				pos += len > 512 ? 512 : len;
				len -= 512;
			}
			free(buffer); found++;
			continue;
		}

		buffer = malloc(len+2);
		if (cdb_read(&c, buffer, len, pos) == -1) goto error;
		if ( buffer[len-1] == '\n' ) buffer[len] = 0;
		else { buffer[len] = '\n'; buffer[len+1] = 0; }

		if (mode_header_line)
			printf("%s: %s", package, buffer);
		else
			printf("%s", buffer);

		free(buffer);
		found++;
	}

	if ( found == 0 && ! (name[0] >= 'A' && name[0] <= 'Z') ) {
		fprintf(stderr, "While reading GEM file %s: Unknown key: %s\n",
				package, name);
		ret++;
	}

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

