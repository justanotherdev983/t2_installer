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
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

/* disk eject ... */
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/cdrom.h>

#ifndef ROCKCFG_ID
#  define ROCKCFG_ID "1.7-x86-reference"
#endif

char * config    = ROCKCFG_ID;
char * targetdir = "/mnt/target";
char * sourcedev = "/dev/cdroms/cdrom0";
char * sourcedir = "/mnt/cdrom0";
char * sourceopt = "ro";

char * mine_command = "mine";
char * mine_fopt = "";

void help()
{
	fprintf(stderr,
"\n"
"G.A.S. - GEM Autoinstall Shell\n"
"\n"
"Usage: gas [options]\n"
"\n"
"  -c " ROCKCFG_ID "\n"
"		ROCK Linux Build Config-ID.\n"
"\n"
"  -t /mnt/target\n"
"		The root directory of the target system.\n"
"\n"
"  -d /dev/cdroms/cdrom0\n"
"		The source device file.\n"
"\n"
"  -s /mnt/cdrom0\n"
"		The mount point for the source device file (or URL).\n"
"\n"
"  -o ro\n"
"		The mount options for mounting the source device file.\n"
"\n"
"  -D		Read package db and dump the memory DB as tree (debug).\n"
"  -T		Don't really execute 'mine', print commands instead (debug).\n"
"  -F		Start mine with the '-f' option (used by install scripts).\n"
"\n"
	);
}

void eject_disk ()
{
	int fd = open(sourcedev, O_RDONLY|O_NONBLOCK);

	if (fd == -1) {
		printf ("Unable to open %s: %s\n", sourcedev, strerror(errno));
		return;
	}

	if ( ioctl(fd, CDROMEJECT) )
		printf ("Ejecting %s failed: %s\n", sourcedev, strerror(errno));

	close (fd);
}

void remove_package(struct package *p, int just_print_commands)
{
	char command[1024];

	snprintf(command, 1024, "%s -r %s -R %s %s",
		mine_command, mine_fopt, targetdir, p->name);

	if ( just_print_commands ) {
		printf("%s\n", command);
		fflush(stdout);
	} else {
		printf("Removing %s ...\n", p->name);
		fflush(stdout);
		system(command);
	}
}

char *getgemfile(struct package *p)
{
	static char gemfile[1024];
	char command[1024];

	while ( !strncmp(sourcedir, "http://", 7) || !strncmp(sourcedir, "ftp://", 6) ) {
		snprintf(gemfile, 1024, "%s/%s-%s.gem",
		         targetdir, p->name, p->version);

		snprintf(command, 1024, "wget -O '%s' -q '%s/%s/pkgs/%s-%s.gem'",
		         gemfile, sourcedir, config, p->name, p->version);
		system(command);
		snprintf(command, 1024, "test -s '%s' || rm -f %s", gemfile, gemfile);
		system(command);
		if ( !access(gemfile, F_OK) ) return gemfile;

		snprintf(command, 1024, "wget -O '%s' -q '%s/%s/pkgs/%s.gem'",
		         gemfile, sourcedir, config, p->name);
		system(command);
		snprintf(command, 1024, "test -s '%s' || rm -f %s", gemfile, gemfile);
		system(command);
		if ( !access(gemfile, F_OK) ) return gemfile;

		printf("Can't find: %s/%s/pkgs/%s-%s.gem\n",
			sourcedir, config, p->name, p->version);
		printf("Can't find: %s/%s/pkgs/%s.gem\n",
			sourcedir, config, p->name);
		printf("Not found on webserver: %s\n\n", p->name);
		while (1) {
			printf("Skip or Retry (default=Retry) > ");
			fflush(stdout);
			fgets(command, 1024, stdin);
			if ( command[0] == 0 || command[0] == '\n' ||
			     command[0] == 'r' || command[0] == 'R' ) break;
			if ( command[0] == 's' || command[0] == 'S' ) return NULL;
		}
	}

	while (1) {
		snprintf(gemfile, 1024, "%s/%s/pkgs/%s-%s.gem",
			sourcedir, config, p->name, p->version);
		if ( !access(gemfile, F_OK) ) return gemfile;

		snprintf(gemfile, 1024, "%s/%s/pkgs/%s.gem",
			sourcedir, config, p->name);
		if ( !access(gemfile, F_OK) ) return gemfile;

		/* looks like there are sometimes races with asyncrounously running mine subprocesses or something
		 * alike. So Let's try umounting again after a second if it fails the first time. */
		snprintf(command, 1024, "umount -v %s &> /dev/null || { sleep 1; umount -v %s; }", sourcedir, sourcedir);
		system(command);

		printf("Can't find: %s/%s/pkgs/%s-%s.gem\n",
			sourcedir, config, p->name, p->version);
		printf("Can't find: %s/%s/pkgs/%s.gem\n",
			sourcedir, config, p->name);
		printf("Not found on current disk: %s\n\n", p->name);
		printf("Please insert Disk #%d and press ENTER.\n\n",
		       p->disk_number);
		while (1) {
			printf("Skip, Retry or Eject disk (default=Retry) > ");
			fflush(stdout);
			fgets(command, 1024, stdin);
			if ( command[0] == 0 || command[0] == '\n' ||
			     command[0] == 'r' || command[0] == 'R' ) break;
			if ( command[0] == 's' || command[0] == 'S' ) return NULL;
			if ( command[0] == 'e' || command[0] == 'E' )
				eject_disk ();
		}

		snprintf(gemfile, 1024, "%s/%s/pkgs/%s-%s.gem",
			sourcedir, config, p->name, p->version);
		if ( !access(gemfile, F_OK) ) return gemfile;

		snprintf(gemfile, 1024, "%s/%s/pkgs/%s.gem",
			sourcedir, config, p->name);
		if ( !access(gemfile, F_OK) ) return gemfile;

		snprintf(command, 1024, "mount -v -o %s %s %s",
			sourceopt, sourcedev, sourcedir);
		system(command);
	}
}

void install_package(struct package *p, int just_print_commands)
{
	char command[1024], *gemfile;
	int rc;

retry_mine:
	gemfile = getgemfile(p);
	if ( gemfile == NULL ) return;

	snprintf(command, 1024, "%s -i %s -R %s %s",
		mine_command, mine_fopt, targetdir, gemfile);

	if ( just_print_commands ) {
		printf("%s\n", command);
		fflush(stdout);
	} else {
		printf("Disk #%d: Installing %s-%s ...\n",
		       p->disk_number, p->name, p->version);
		fflush(stdout);
		rc = system(command);

		if ( rc ) {
			printf("Error while installing package %s!\n\n", p->name);
			while (1) {
				printf("Skip or Retry (default=Retry) > ");
				fflush(stdout);
				fgets(command, 1024, stdin);
				if ( command[0] == 0 || command[0] == '\n' ||
				     command[0] == 'r' || command[0] == 'R' ) {
					if ( !strncmp(sourcedir, "http://", 7) ||
					     !strncmp(sourcedir, "ftp://", 6) ) unlink(gemfile);
					goto retry_mine;
				}
				if ( command[0] == 's' || command[0] == 'S' ) break;
			}
		}
	}

	if ( !strncmp(sourcedir, "http://", 7) || !strncmp(sourcedir, "ftp://", 6) )
		unlink(gemfile);
}

int main(int argc, char ** argv)
{
	int just_dump_tree = 0;
	int just_print_commands = 0;
	struct package *p;
	int opt, disk;

	while ( (opt = getopt(argc, argv, "c:t:d:s:o:DTF")) != -1 ) {
		switch (opt) {
			case 'c':
				config = optarg;
				break;
			case 't':
				targetdir = optarg;
				break;
			case 'd':
				sourcedev = optarg;
				break;
			case 's':
				sourcedir = optarg;
				break;
			case 'o':
				sourceopt = optarg;
				break;
			case 'D':
				just_dump_tree = 1;
				break;
			case 'T':
				just_print_commands = 1;
				break;
			case 'F':
				mine_fopt = "-f";
				break;
			default:
				help();
				return 0;
		}
	}

	if ( read_packages() ) return 1;

	if ( just_dump_tree ) {
		print_dirtree(rootdir, 0);
		return 0;
	}
	do_selection();

	for (p = packages; p != NULL; p = p->next)
		if ( p->action < 0 )
			remove_package(p, just_print_commands);

	for (disk = 0; disk <= max_disk_number; disk++)
	  for (p = packages; p != NULL; p = p->next)
		if ( p->disk_number == disk && p->action > 0 )
			install_package(p, just_print_commands);

	return 0;
}

