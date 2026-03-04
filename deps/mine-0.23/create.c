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
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#include "cdb.h"
#include "cdb_make.h"

#include "mine.h"

int gem_create_addfile(struct cdb_make *c, char * key, char * file)
{
	unsigned int klen = strlen(key);
	unsigned int dlen;
	uint32 h = CDB_HASHSTART;
	struct stat stbuf;
	char buffer[512];
	int i, seg, fd;

	if ((fd = open(file, O_RDONLY)) == -1) goto error_errno;
	if (fstat(fd, &stbuf) == -1) goto error_errno;
	dlen = stbuf.st_size;

	if (cdb_make_addbegin(c, klen, dlen) == -1) goto error;
	for (i = 0; i < klen; i++) {
		if (buffer_PUTC(&c->b, key[i]) == -1) goto error;
		h = cdb_hashadd(h, key[i]);
	}

	for (i = 0; i < dlen; i+=seg) {
		seg = dlen-i < 512 ? dlen-i : 512;
		if (read(fd, buffer, seg) != seg) goto error_errno;
		if (buffer_put(&c->b, buffer, seg) == -1) goto error;
	}
	if (cdb_make_addend(c, klen, dlen, h) == -1) goto error;
	close(fd);

	return 0;

error:
	errno = 0;

error_errno:
	fprintf(stderr, "While writing key '%s': %s\n", key,
	                errno ? strerror(errno) : "Unknown error");
	exit(1);
}

int gem_create_addtext(struct cdb_make *c, char * key, char * text)
{
	unsigned int klen = strlen(key);
	unsigned int dlen = strlen(text);
	uint32 h = CDB_HASHSTART;
	int i;

	if (cdb_make_addbegin(c, klen, dlen) == -1) goto error;
	for (i = 0; i < klen; i++) {
		if (buffer_PUTC(&c->b, key[i]) == -1) goto error;
		h = cdb_hashadd(h, key[i]);
	}

	for (i = 0; i < dlen; i++) {
		if (buffer_PUTC(&c->b, text[i]) == -1) goto error;
	}
	if (cdb_make_addend(c, klen, dlen, h) == -1) goto error;

	return 0;

error:
	fprintf(stderr, "While writing key '%s': Unknown error\n", key);
	exit(1);
}

char * varadm_subtrees[] =
{
	"packages",
	"flists",
	"md5sums",
	"cksums",
	"dependencies",
	"descs",
	NULL
};

int gem_create(char * varadm, char * tarbz2, char * package, char * outfile)
{
	struct cdb_make c;
	char filename[1024];
	char *key, *value;
	int verset=0;
	int cfgset=0;
	int fd, i;
	FILE *f;

	fd = open(outfile, O_WRONLY | O_TRUNC | O_CREAT, 0666);
	if (fd == -1) goto error_errno;

	if (cdb_make_start(&c,fd) == -1) goto error;

	gem_create_addtext(&c, "creator", "GEM MINE " MINE_VERSION);

	gem_create_addtext(&c, "pkg_name", package);
	gem_create_addfile(&c, "pkg_tarbz2", tarbz2);

	for (i=0; varadm_subtrees[i]; i++) {
		snprintf(filename, 1024, "%s/%s/%s", varadm,
		                   varadm_subtrees[i], package);
		gem_create_addfile(&c, varadm_subtrees[i], filename);
	}

	snprintf(filename, 1024, "%s/descs/%s", varadm, package);
	if ((f=fopen(filename, "r")) == NULL) goto error;
	while ( fgets(filename, 1024, f) != NULL )
	{
		if (filename[0] == '[') key=filename+1; else continue;
		if (strrchr(key, '\n') != NULL) *(strrchr(key, '\n')) = 0;
		value = strchr(key, ' ');
		if (value == NULL) value = filename + strlen(filename);
		else { *value=0; value++; }
		if (strrchr(key, ']') != NULL) *(strrchr(key, ']')) = 0;

		gem_create_addtext(&c, key, value);

		if (!strcmp(key, "V") && !verset) {
			if (strchr(value, ' ') != NULL)
				*(strchr(value, ' ')) = 0;
			gem_create_addtext(&c, "pkg_version", value);
			verset=1;
		}
		if (!strcmp(key, "CONFIG") && !cfgset) {
			gem_create_addtext(&c, "pkg_config", value);
			cfgset=1;
		}
	}
	fclose(f);

	if (cdb_make_finish(&c) == -1) goto error;
	if (close(fd) == -1) goto error_errno;

	return 0;

error:
	errno = 0;

error_errno:
	fprintf(stderr, "While creating GEM file %s: %s\n", outfile,
	                errno ? strerror(errno) : "Unknown error");
	exit(1);
}

