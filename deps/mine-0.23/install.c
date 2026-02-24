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
#include <fnmatch.h>

#include "cdb.h"
#include "bzlib.h"
#include "libtar.h"

#include "mine.h"
#include "md5sum.h"

int glob_check(char ** patterns, char * filename)
{
	/*Parse through the patterns passed*/
	int i=0;
	while(patterns[i]!=NULL) {
		/*For now, no options just default*/
		if(! fnmatch(patterns[i], filename, 0)) return 1;
		i++;
	}
	return 0;
}


int gem_install(char * root, int mode_test, int mode_verbose,
		int mode_force, char * package, char ** patterns)
{
	int gem2bunzip[2] = { -1, -1 };
	int bunzip2tar[2] = { -1, -1 };

	char * pname = NULL;
	int gem_fd = -1;
	struct cdb c;
	int pos, len;
	int rc;

	char *filename;
	char buffer[1024];
	char buffer2[1024];
	char buffer3[1024];
	TAR *t = NULL;
	BZFILE *b = NULL;

	if ( ! mode_force )
		md5sum_initdb(root, mode_verbose);

	if ( (gem_fd = open(package, O_RDONLY)) < 0 ) goto error_errno;
	cdb_init(&c, gem_fd);

	rc = cdb_find(&c, "pkg_name", 8);
	if ( rc <= 0 ) goto error;
	pos = cdb_datapos(&c); len = cdb_datalen(&c);
	pname = malloc(len+1); pname[len] = 0;
	if (cdb_read(&c, pname, len, pos) == -1) goto error;

	pipe(gem2bunzip);
	pipe(bunzip2tar);

	/*
	 * Extract tar.bz2 from GEM file
	 */
	if (!fork()) {
		close(gem2bunzip[0]);
		close(bunzip2tar[0]);
		close(bunzip2tar[1]);

		rc = cdb_find(&c, "pkg_tarbz2", 10);
		if ( rc <= 0 ) exit(1);

		pos = cdb_datapos(&c);
		len = cdb_datalen(&c);
		if (len <= 0) exit(1);

		while (len > 0) {
			if ( cdb_read(&c, buffer, len>512 ? 512 : len,
			                  pos) == -1 ) goto error;
			write(gem2bunzip[1], buffer, len > 512 ? 512 : len);
			pos += len > 512 ? 512 : len;
			len -= 512;
		}

		cdb_free(&c);
		close(gem_fd);

		exit(0);
	}

	/*
	 * Bunzip tar.bz2 file
	 */
	if (!fork()) {
		close(gem2bunzip[1]);
		close(bunzip2tar[0]);

		b = BZ2_bzdopen(gem2bunzip[0], "r");
		while ( (rc=BZ2_bzread(b, buffer, 512)) > 0 )
			write(bunzip2tar[1], buffer, rc);
		BZ2_bzclose(b);
		
		exit(0);
	}

	/*
	 * Install the tar file
	 */
	close(gem2bunzip[0]);
	close(gem2bunzip[1]);
	close(bunzip2tar[1]);

	if (tar_fdopen(&t, bunzip2tar[0], "pipe", NULL,
		       O_RDONLY, 0, 0) == -1) goto error_errno;
	if ( mode_test && mode_verbose ) printf("-- %s --\n", pname);

	FILE *logfile = NULL;
	if ( ! mode_test ) 
	{
		char postinst[1024];
		snprintf(postinst, 1024, "%s/var/adm/postinstall/%s-install.XXXXXX", root, pname);
		if ( mkstemp(postinst) != -1 ) 
			logfile = fopen(postinst, "w");
		if ( mode_verbose ) {
			if ( logfile == NULL )
				printf("Not writing postinstall log\n");
			else
				printf("Writing postinstall log to %s\n", postinst);
		}
	}

	filename = 0;
	while (th_read(t) == 0)
	{
		filename = th_get_pathname(t);
		snprintf(buffer, sizeof(buffer), "%s/%s", root, filename);
		snprintf(buffer2, sizeof(buffer), "%s.GEMnew", buffer);
		snprintf(buffer3, sizeof(buffer), "%s.GEMold", buffer);

		if ( ! mode_test ) {
			unlink(buffer2);
			unlink(buffer3);
		}

		if (patterns && glob_check(patterns, filename)) {
			if ( mode_verbose ) {
				printf("Exclude glob "
                                        "file %s\n", filename);
				if (mode_test) th_print_long_ls(t);
			}
			if (TH_ISREG(t) && tar_skip_regfile(t) != 0)
				goto error_errno;
		}
		else if ( ! mode_force && md5sum_check(root, filename) ) {
			if ( mode_test && mode_verbose ) {
				printf("WARNING: Skip modified/duplicate "
				       "file %s:\n", filename);
				th_print_long_ls(t);
			} else {
				printf("%s: WARNING: Skip modified/duplicate "
				       "file: %s\n", pname, filename);
			}
			if ( mode_test ) {
				if (TH_ISREG(t) && tar_skip_regfile(t) != 0)
							goto error_errno;
			} else {
				if (tar_extract_file(t, buffer2) != 0)
							goto error_errno;
			}
		}
		else
		if ( mode_test ) {
			if ( mode_verbose )
				th_print_long_ls(t);
			else
				printf("%s: %s\n", pname, filename);
			if (TH_ISREG(t) && tar_skip_regfile(t) != 0)
							goto error_errno;
		}
		else
		{
			if ( mode_verbose )
				printf("%s: %s\n", pname, filename);
			rename(buffer, buffer3);
			if (tar_extract_file(t, buffer) != 0)
							goto error_errno;
			unlink(buffer3);
			if ( logfile != NULL )
				fprintf(logfile, "%s: %s\n", pname, filename);
		}
	}

	tar_close(t); close(bunzip2tar[0]);
	cdb_free(&c); close(gem_fd);

	if ( logfile != NULL ) fclose(logfile);

	return 0;

error:
	errno = 0;

error_errno:
	fprintf(stderr, "While installing GEM file %s%s%s%s: %s\n", package,
			filename?" (":"", filename?filename:"", filename?"(":"",
	                errno ? strerror(errno) : "Unknown error");
	if ( t != NULL) tar_close(t);
	if ( gem_fd != -1 ) { cdb_free(&c); close(gem_fd); }
	if ( gem2bunzip[0] != -1 ) close(gem2bunzip[0]);
	if ( gem2bunzip[1] != -1 ) close(gem2bunzip[1]);
	if ( bunzip2tar[0] != -1 ) close(bunzip2tar[0]);
	if ( bunzip2tar[1] != -1 ) close(bunzip2tar[1]);
	return 1;
}

