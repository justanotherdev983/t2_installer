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

#include "gasgui.h"
#include "memdb.h"
#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

struct directory rootdir_data = { "", 1, NULL, NULL };

struct package * packages = NULL;
struct directory * directories = &rootdir_data;
struct directory * rootdir = &rootdir_data;

int directory_entry_count = 0;
int max_disk_number = 0;

struct filesystem *filesystems = NULL;
int filesystem_count;

struct memdb_t files_on_disks;
struct memdb_t pkg_addr_hash;

struct directory *
get_create_directory(struct directory * base, char *name)
{
	struct directory_entry *rc;
	int len;

	if (strchr(name, '/') == NULL) len = strlen(name);
	else len = strchr(name, '/') - name;

	for (rc = base->list; rc != NULL; rc = rc->next)
		if ( rc->content_is_subdir &&
		     !strncmp(rc->content.dir->name, name, len) &&
		     !rc->content.dir->name[len]) {
			if ( name[len] ) return get_create_directory(
						rc->content.dir, name+len+1);
			return rc->content.dir;
		}

	rc = calloc(1, sizeof(struct directory_entry));
	rc->next = base->list; base->list = rc;
	directory_entry_count++;

	rc->content_is_subdir = 1;
	rc->content.dir = calloc(1, sizeof(struct directory));
	rc->content.dir->name = malloc(len+1);
	strncpy(rc->content.dir->name, name, len);
	rc->content.dir->name[len]=0;

	rc->content.dir->next = directories;
	directories = rc->content.dir;

	if ( name[len] )
		return get_create_directory(rc->content.dir, name+len+1);
	return rc->content.dir;
}

void print_dirtree(struct directory * dir, int blanks)
{
	struct directory_entry * e;

	for (e = dir->list; e != NULL; e = e->next) {
		if ( e->content_is_subdir ) {
			printf("%*s=== %s/\n",
				blanks, "", e->content.dir->name);
			print_dirtree(e->content.dir, blanks+3);
		} else {
			printf("%*s[%c] %s - %s (%d)\n", blanks, "",
				e->content.pkg->is_installed ? '*' : ' ',
				e->content.pkg->name, e->content.pkg->version,
				e->content.pkg->disk_number);
		}
	}
}

int dirmatch(char *filename, char *dirname)
{
	int i;

	for (i=0; dirname[i]; i++) {
		if ( filename[i] != dirname[i] ) return 0;
	}
	return filename[i] == '/';
}

int read_package(char *packagename, FILE *dbf)
{
	static struct package *last_p = NULL;
	struct package *p;
	struct directory *d;
	struct directory_entry *e;
	struct dependency *dep;
	char line[1024], *t;
	char filename[PATH_MAX];
	int line_length, i;
	FILE *f = dbf;

	if ( !dbf ) {
		snprintf(filename, PATH_MAX, "%s/%s/info/descs/%s",
			sourcedir, config, packagename);

		f = fopen(filename, "r");
		if ( f == NULL ) {
			fprintf(stderr, "Can't open %s: %s\n",
				filename, strerror(errno));
			return -1;
		}
	}

	p = calloc(1, sizeof(struct package));
	p->name = packagename; p->version="0000";

	if ( ! packages ) packages = p;
	if ( last_p ) last_p->next = p;
	(last_p=p)->next = NULL;

	snprintf(line, 1024, "%p", p);
	memdb_put(&pkg_addr_hash, packagename, line);

	d = get_create_directory(rootdir, "all");
	e = calloc(1, sizeof(struct directory_entry));
	e->next = d->list; d->list = e; e->content.pkg = p;
	directory_entry_count++;

	if ( strlen(packagename) > 4 && !strcmp(packagename+strlen(packagename)-4, ":dev") ) {
		d = get_create_directory(rootdir, "dev");
		e = calloc(1, sizeof(struct directory_entry));
		e->next = d->list; d->list = e; e->content.pkg = p;
		directory_entry_count++;
	}

	if ( strlen(packagename) > 4 && !strcmp(packagename+strlen(packagename)-4, ":doc") ) {
		d = get_create_directory(rootdir, "doc");
		e = calloc(1, sizeof(struct directory_entry));
		e->next = d->list; d->list = e; e->content.pkg = p;
		directory_entry_count++;
	}

	/* read basic info like V and C tags */ 
	while ( fgets(line, 1024, f) != NULL && strcmp(line,"\027\n") ) {
		if ( (line_length = strlen(line)) < 3 ) continue;
		if ( line[line_length-1] == '\n' ) line[--line_length] = 0;

		if ( !strncmp(line, "[V] ", 4) ) {
#ifdef ROCK20
			p->version = malloc(line_length-3);
			sscanf(line+4, "%s", p->version);
#else
			p->version = strdup(line+4);
			for (i=0; p->version[i]; i++)
				if (p->version[i] == ' ')
					p->version[i] = '-';
#endif
			continue;
		}

		if ( !strncmp(line, "[C] ", 4) ) {
			t=strtok(line, " ");
			while ( (t=strtok(NULL, " ")) != NULL ) {
				d = get_create_directory(rootdir, t);
				e = calloc(1, sizeof(struct directory_entry));
				e->next = d->list; d->list = e;
				e->content.pkg = p;
				directory_entry_count++;
			}
			continue;
		}
	}
	if ( !dbf ) fclose(f);

	/* read package dependencies */
	if ( !dbf ) {
		snprintf(filename, PATH_MAX, "%s/%s/info/dependencies/%s",
			sourcedir, config, packagename);
		f = fopen(filename, "r");
		if ( f == NULL ) {
			fprintf(stderr, "Can't open %s: %s\n",
				filename, strerror(errno));
			return -1;
		}
	}

	while ( fgets(line, 1024, f) != NULL && strcmp(line,"\027\n") ) {
		if ( (t = strchr(line, '\n')) != NULL ) *t=0;
		if ( (t = strchr(line, ' ')) == NULL ) continue;
		t++;

		if ( !strcmp(t, packagename) ) continue;
		dep = calloc(1, sizeof(struct dependency));
		dep->name = malloc(strlen(t)+1); strcpy(dep->name, t);
		dep->next = p->deps; p->deps = dep;
	}
	if ( !dbf ) fclose(f);

	snprintf(line, 1024, "%s/pkgs/%s-%s.gem", config, p->name, p->version);
	if ( memdb_get(&files_on_disks, line) ) {
		p->disk_number = atoi(memdb_search_result);
	} else {
		snprintf(line, 1024, "%s/pkgs/%s.gem", config, p->name);
		if ( memdb_get(&files_on_disks, line) )
			p->disk_number = atoi(memdb_search_result);
	}

	snprintf(filename, PATH_MAX, "%s/var/adm/descs/%s",
		targetdir, packagename);
	f = fopen(filename, "r");
	if ( f == NULL ) {
		p->is_installed = 0;
	} else {
		p->is_installed = 1;

		while ( fgets(line, 1024, f) != NULL ) {
			if ( (line_length = strlen(line)) < 3 ) continue;
			if ( line[line_length-1] == '\n' )
						line[--line_length] = 0;

			if ( !strncmp(line, "[V] ", 4) ) {
#ifdef ROCK20
				p->installed_version = malloc(line_length-3);
				sscanf(line+4, "%s", p->installed_version);
#else
				p->installed_version = strdup(line+4);
				for (i=0; p->installed_version[i]; i++)
					if (p->installed_version[i] == ' ')
						p->installed_version[i] = '-';
#endif
				continue;
			}
		}
		fclose(f);
	}

	if ( !dbf ) {
		snprintf(filename, PATH_MAX, "%s/%s/info/cksums/%s",
			sourcedir, config, packagename);
		f = fopen(filename, "r");
	} else {
		f = dbf;
	}
	p->size = calloc(filesystem_count + 1, sizeof(int));
	if ( f != NULL ) {
		struct filesystem *ifs, *fs;
		int file_size, fs_nr, i;
		char * file_name;

		while ( fgets(line, 1024, f) != NULL && strcmp(line,"\027\n") ) {
			if ( strtok(line, " ") == NULL ) continue;
			
			file_name = strtok(NULL, " ");
			if ( file_name == NULL ) continue;
			file_size = atoi(file_name);

			file_name = strtok(NULL, " \n");
			if ( file_name == NULL ) continue;


			p->size[0] += file_size;
			snprintf(filename, PATH_MAX, "%s/%s", targetdir, file_name);
			for ( fs_nr=i=1, fs=ifs=filesystems; ifs != NULL; ifs=ifs->next, i++ )
				if ( dirmatch(filename, ifs->directory) )
					{ fs_nr = i; fs = ifs; }
			p->size[fs_nr] += file_size;
			fs->useit = 1;
		}
		if ( !dbf ) fclose(f);
	}

	/* seek until EOF mark for this package */
	if ( dbf ) {
		while ( fgets(line, 1024, f) != NULL && strcmp(line,"\004\n") ) continue;
	}

	return 0;
}

char *root_order[] = {
	"all",
	"base",
	"extra",
	"dev",
	"doc",
	NULL
};

int rootdir_strcmp(char *n1, char *n2) {
	int v1=1000, v2=1000, i;

	for (i=0; root_order[i]; i++) {
		if ( !strcmp(root_order[i], n1) ) v1 = i;
		if ( !strcmp(root_order[i], n2) ) v2 = i;
	}

	if ( v1 == v2 ) return strcmp(n1, n2);
	return v1 == v2 ? 0 : v1 > v2 ? +1 : -1;
}

void sort_directory(struct directory *d)
{
	struct directory_entry *e, **l;
	int size=0, i=0, keep_running=1;
	char *n1, *n2;

	/* Create array for easier sorting */
	for (e = d->list; e != NULL; e = e->next) size++;
	l = malloc( sizeof(struct directory_entry*) * size );
	for (e = d->list; e != NULL; e = e->next) l[i++] = e;

	/* Do normal bubble-sort */
	while ( keep_running )
	  for (keep_running=i=0; i < size-1; i++) {
		if ( l[i]->content_is_subdir )
			n1 = l[i]->content.dir->name;
		else
			n1 = l[i]->content.pkg->name;

		if ( l[i+1]->content_is_subdir )
			n2 = l[i+1]->content.dir->name;
		else
			n2 = l[i+1]->content.pkg->name;

		if ( d == rootdir ) {
			if ( rootdir_strcmp(n1, n2) > 0 ) {
				e = l[i]; l[i] = l[i+1]; l[i+1] = e;
				keep_running=1;
			}
		} else {
			if ( strcmp(n1, n2) > 0 ) {
				e = l[i]; l[i] = l[i+1]; l[i+1] = e;
				keep_running=1;
			}
		}
	  }

	/* Re-create linked-list */
	d->list = l[0];
	for (i=0; i < size-1; i++) l[i]->next = l[i+1];
	l[i]->next = NULL;

	/* Free helper-array */
	free(l);
}

void get_filesystem_info()
{
	char device[1024], directory[1024];
	struct filesystem *fs = filesystems;
	FILE *f;

	f = fopen("/proc/mounts", "r");
	while ( fscanf(f, "%1000s %1000s %*s %*s %*s %*s\n", device, directory) == 2 ) {
		if ( ! strcmp(device, "rootfs") ) continue;
		// if ( !strcmp(device, "none") ) continue;

		if ( fs == NULL ) {
			fs = filesystems = malloc(sizeof(struct filesystem));
		} else {
			fs->next = malloc(sizeof(struct filesystem));
			fs = fs->next;
		}

		fs->device = malloc(strlen(device)+1);
		strcpy(fs->device, device);

		fs->directory = malloc(strlen(directory)+1);
		strcpy(fs->directory, directory);

		statfs(directory, &fs->fs_data);
		fs->install = fs->useit = 0;
		fs->next = NULL;

		filesystem_count++;
	}
	fclose(f);
}

int read_packages()
{
	struct dirent **namelist;
	struct directory *d;
	struct package *p;
	struct dependency **next_dep;
	char filename[PATH_MAX];
	FILE *f;
	int i;

	memdb_init(&files_on_disks);
	memdb_init(&pkg_addr_hash);
	get_filesystem_info();

	printf("Reading package database. Please wait...\n");
	if ( strncmp(sourcedir, "http://", 7) && strncmp(sourcedir, "ftp://", 6) ) {
		/* determine package files and max disk number */
		snprintf(filename, PATH_MAX, "%s/index.txt", sourcedir);
		if ( (f = fopen(filename, "r")) != NULL ) {
			char pkgdir[200], disk[200], line[160];

			snprintf(pkgdir, 200, "%s/pkgs/", config);

			while ( fgets(line, 160, f) != NULL ) {
				sscanf(line, "disk%s %s", disk, filename);
				if ( !strncmp(filename, pkgdir, strlen(pkgdir)) )
					memdb_put(&files_on_disks, filename, disk);
				if ( atoi(disk) > max_disk_number )
					max_disk_number = atoi(disk);
			}
			fclose(f);
		}
	}

	if ( strncmp(sourcedir, "http://", 7) && strncmp(sourcedir, "ftp://", 6) )
		snprintf(filename, PATH_MAX, "gzip -d < %s/%s/pkgs/packages.db",
		         sourcedir, config);
	else
		snprintf(filename, PATH_MAX, "wget -O - -nv %s/%s/pkgs/packages.db | gzip -d",
		         sourcedir, config);
	if ( (f = popen(filename, "r")) != NULL ) {
		char *package, line[160];
		while ( fgets(line, 160, f) != NULL ) {
			package = malloc(strlen(line)+1);
			sscanf(line, "%s", package);
			if ( fgets(line, 160, f) == NULL ||
			     strcmp(line, "\027\n") ) {
				fprintf(stderr, "Format error in packages.db!\n");
				return -1;
			}
			read_package(package, f);
		}
		pclose(f);
	}
	if ( !packages &&
	     strncmp(sourcedir, "http://", 7) && strncmp(sourcedir, "ftp://", 6) ) {
		snprintf(filename, PATH_MAX, "%s/%s/info/descs", sourcedir, config);
		i = scandir(filename, &namelist, 0, alphasort);
		if (i < 0) {
			fprintf(stderr, "Can't read %s with scandir(): %s\n",
				filename, strerror(errno));
			return -1;
		} else {
			/* read packages in reversed order so the linked list is
			 * ordered well (new entries are insterted at the begin) */
			while (i--) {
				if ( namelist[i]->d_name[0] != '.' &&
				     strcmp(namelist[i]->d_name, "TRANS.TBL") )
						read_package(namelist[i]->d_name, NULL);
				else
						free(namelist[i]);
			}
			free(namelist);
		}
	}
	if ( !packages ) {
		fprintf(stderr, "No packages found! Please check the install source.\n");
		return -1;
	}

	for (d = directories; d != NULL; d = d->next) sort_directory(d);

	for (p = packages; p != NULL; p = p->next) {
	  /* fill the dependenies with references to the individual packges */
	  for (next_dep = &(p->deps); *next_dep != NULL;) {
		char* pkg_ptr_str = memdb_get(&pkg_addr_hash, (*next_dep)->name);
		if (pkg_ptr_str)
			sscanf(pkg_ptr_str, "%p", &(*next_dep)->pkg);
		if ((*next_dep)->pkg) {
			next_dep = &((*next_dep)->next);
		} else {
			/* drop dependency since it is not in db - this can happend
			   if multiple builds are run with modified packages */
			struct dependency *tbd_dep = *next_dep;
			*next_dep = tbd_dep->next;
			free (tbd_dep->name);
			free (tbd_dep);
		}
	  }
	}
	memdb_free(&files_on_disks);
	memdb_init(&pkg_addr_hash);
	return 0;
}

