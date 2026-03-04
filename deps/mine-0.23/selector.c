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
#include <dialog.h>
#include <limits.h>
#include <sys/ioctl.h>
#include <stdio.h>

int screensize_x = 70;
int screensize_y = 19;

#define HELP_TEXT \
"Select and de-select packages.\n"					\
" (+)  closed directory        (-)  open directory\n"			\
" [*]  package installed       [+]  package marked for installation\n"	\
" [ ]  package not installed   [-]  package marked for removal\n"	\
" [>]  package marked for replacing currently installed version\n"	\
" < > / <->  dependency (not installed / marked for removal)\n"

char ** items;
struct directory_entry ** entries;
int item_counter;

char *instmenu1_items[] = {
	"1", "Full install (recommended)",
	"2", "Only a minimalistic system"
};

char *instmenu2_items[] = {
	"1", "Install selected packages",
	"2", "Alter package selection"
};

char *instmenu2a_items[] = {
	"1", "Alter package selection",
	"2", "Install selected packages"
};

char *extra_dir_items[] = {
	"a", "Open entire directory tree      ",
	"b", "Close entire directory tree     ",
	"c", "Un-Install all packages in tree ",
	"d", "Reset all packages in tree      ",
	"e", "Install all packages in tree    ",
	"f", "Upgrade all packages in tree    ",
	"g", "Re-Install all packages in tree ",
	"h", "Show current action-list        ",
	"i", "Install unresolved dependencies ",
	"j", "List unresolved dependencies    "
};

char *extra_pkg_items[] = {
	"a", "Show package information        ",
	"b", "Show current action-list        ",
	"c", "List unresolved dependencies    "
};

void check_dep_recurs(FILE *f, int level, struct package *p)
{
	struct dependency *dep;

	for (dep = p->deps; dep != NULL; dep = dep->next)
	  if ( (dep->pkg->is_installed && dep->pkg->action < 0) ||
	     (!dep->pkg->is_installed && !dep->pkg->action) ) {
		if ( dep->pkg->is_dependency ) {
			if ( f ) fprintf(f,"%*s%s needs %s (hashed)\n",
				level*3, "", p->name, dep->pkg->name);
		} else {
			if ( f ) fprintf(f,"%*s%s needs %s\n",
				level*3, "", p->name, dep->pkg->name);
			dep->pkg->is_dependency = 1;
			check_dep_recurs(f, level+1, dep->pkg);
		}
	  }
}

void create_dialog_itemlist(struct directory * dir, int blanks)
{
        struct directory_entry * e;
	char tmpstring1[512];
	char tmpstring2[50];
                        
        for (e = dir->list; e != NULL; e = e->next) {
		items[item_counter*2] = malloc(5);
		snprintf(items[item_counter*2], 5, "%4d", item_counter+1);
		entries[item_counter] = e;
		
                if ( e->content_is_subdir ) {
			char * fullname = e->content.dir->name;

			if ( !blanks && !strcmp(fullname, "all") )
				fullname = "All Packages";
			if ( !blanks && !strcmp(fullname, "base") )
				fullname = "Base Packages";
			if ( !blanks && !strcmp(fullname, "extra") )
				fullname = "Additional Packages";
			if ( !blanks && !strcmp(fullname, "dev") )
				fullname = "Development Packages";
			if ( !blanks && !strcmp(fullname, "doc") )
				fullname = "Documentation Packages";

                        snprintf(tmpstring1, 512,
			        "%*s(%c) %s/", blanks, "",
				e->content.dir->is_open_in_gui ? '-' : '+',
				fullname);
			strcpy(tmpstring2,"<dir>");
                } else {
			char ch = e->content.pkg->is_installed ? '*' : ' ';
			char *remark = "";

			if ( e->content.pkg->action < 0 )
				ch = e->content.pkg->is_installed ? '-' : 'X';
			if ( e->content.pkg->action > 0 )
				ch = e->content.pkg->is_installed ? '>' : '+';

			if ( e->content.pkg->installed_version &&
			     strcmp(e->content.pkg->installed_version,
			            e->content.pkg->version) ) remark="(new)";

                        snprintf(tmpstring1, 512, "%*s%c%c%c %s-%s %s", blanks,
				"", e->content.pkg->is_dependency ? '<' : '[',
				ch, e->content.pkg->is_dependency ? '>' : ']',
				e->content.pkg->name, e->content.pkg->version,
				remark);
			snprintf(tmpstring2, 50, "%.2f MB",
				(float)e->content.pkg->size[0] / 1048576);
                }
		items[item_counter*2+1] = malloc(60);
		snprintf(items[item_counter*2+1], 60, "%-*s%*s",
					40, tmpstring1, 10, tmpstring2);
		item_counter++;

		if ( e->content_is_subdir && e->content.dir->is_open_in_gui )
                        create_dialog_itemlist(e->content.dir, blanks+3);
        }
}

void install_deps(struct directory * dir)
{
        struct directory_entry * e;
                        
        for (e = dir->list; e != NULL; e = e->next) {
		if ( !e->content_is_subdir && e->content.pkg->is_dependency )
			e->content.pkg->action =
				e->content.pkg->is_installed ? 0 : 1;
		if ( e->content_is_subdir )
			install_deps(e->content.dir);
	}
}

void set_tree_flag(struct directory * dir, int set_dir, int value)
{
        struct directory_entry * e;
                        
	if ( set_dir ) dir->is_open_in_gui = value;

        for (e = dir->list; e != NULL; e = e->next) {
                if ( e->content_is_subdir && set_dir)
			e->content.dir->is_open_in_gui = value;

		if ( !e->content_is_subdir && !set_dir) {
			if ( value == 0 ) {
				e->content.pkg->action = value;
			} else if ( value < 0 ) {
				e->content.pkg->action =
					e->content.pkg->is_installed ? -1 : 0;
			} else if ( value == 1 ) {
				if ( ! e->content.pkg->is_installed )
						e->content.pkg->action = 1;
			} else if ( value == 2 ) {
				if ( e->content.pkg->installed_version &&
				     strcmp(e->content.pkg->installed_version,
				            e->content.pkg->version) )
						e->content.pkg->action = 1;
			} else if ( value == 3 ) {
				if ( e->content.pkg->is_installed )
						e->content.pkg->action = 1;
			}
		}

		if ( e->content_is_subdir )
                        set_tree_flag(e->content.dir, set_dir, value);
        }
}

void free_dialog_item_list()
{
	for (item_counter--; item_counter>0; item_counter--) {
		free(items[item_counter*2+1]);
		free(items[item_counter*2]);
	}
}

void show_deps()
{
	char filename[] = "/tmp/gas_deps_XXXXXX";
	int fd = mkstemp(filename);
	FILE *f = fdopen(fd, "w");
	struct package *p;

	for (p=packages; p != NULL; p = p->next) {
		p->is_dependency=0;
	}
	for (p=packages; p != NULL; p = p->next) {
		if ( (p->is_installed && p->action >= 0) ||
		     p->action > 0 ) check_dep_recurs(f, 0, p);
	}
	fclose(f);

	dialog_textbox(" Unresolved package dependencies ",
	               filename, screensize_y, screensize_x);
	unlink(filename);
}

void show_actions()
{
	struct package *p;
	char filename[200];
	FILE *f;
	int i;

	f = tmpfile();
	snprintf(filename, 200, "/proc/self/fd/%d", fileno(f));

	for (i=0, p=packages; p != NULL; p=p->next) if (p->action < 0) i++;

	if ( i ) {
		fprintf(f, "%d packages marked for removal:\n", i);

		for (i=0, p=packages; p != NULL; p=p->next)
			if ( p->action < 0 ) {
				fprintf(f, "%s-%s\n", p->name,
					p->installed_version ?
						p->installed_version : "?");
			}
	} else {
		fprintf(f, "No packages marked for removal.\n");
	}

	for (i=0, p=packages; p != NULL; p=p->next) if (p->action > 0) i++;

	if ( i ) {
		fprintf(f, "\n%d packages marked for installation:\n", i);

		for (i=0, p=packages; p != NULL; p=p->next)
			if ( p->action > 0 )
				fprintf(f, "%s-%s\n", p->name, p->version);
	} else {
		fprintf(f, "\nNo packages marked for installation.\n");
	}

	fflush(f); /* close afterwards because fclose() will remove file */
	dialog_textbox(" Current Action List ", filename,
	               screensize_y, screensize_x);
	fclose(f);
}

void show_pkginfo(struct package * p)
{
	char filename[PATH_MAX];
	char title[100];

	snprintf(filename, PATH_MAX, "%s/%s/info/packages/%s",
	         sourcedir, config, p->name);

	snprintf(title, 100, " Package Info for %s ", p->name);
	dialog_textbox(title, filename, screensize_y, screensize_x);
}

void extra_dir(struct directory * dir)
{
	int rc;

	dialog_vars.extra_button = 0;
	dialog_vars.help_button  = 0;
	dialog_vars.cancel_label = "Cancel";

	dialog_vars.input_result[0]=0;
	dialog_vars.default_item = NULL;
	rc = dialog_menu("Directory Extra Functions", "",
			16, 50, 10, 10, extra_dir_items);

	switch(dialog_vars.input_result[0]) {
		case 'a': set_tree_flag(dir, 1, 1); break;
		case 'b': set_tree_flag(dir, 1, 0); break;
		case 'c': set_tree_flag(dir, 0,-1); break;
		case 'd': set_tree_flag(dir, 0, 0); break;
		case 'e': set_tree_flag(dir, 0, 1); break;
		case 'f': set_tree_flag(dir, 0, 2); break;
		case 'g': set_tree_flag(dir, 0, 3); break;
		case 'h': show_actions();           break;
		case 'i': install_deps(dir);        break;
		case 'j': show_deps();              break;
	}

	dialog_vars.extra_button = 1;
	dialog_vars.help_button  = 1;
	dialog_vars.cancel_label = "Finish";
}

void extra_pkg(struct package * pkg)
{
	int rc;

	dialog_vars.extra_button = 0;
	dialog_vars.help_button  = 0;
	dialog_vars.cancel_label = "Cancel";

	dialog_vars.input_result[0]=0;
	dialog_vars.default_item = NULL;
	rc = dialog_menu("Package Extra Functions", "",
			9, 50, 3, 3, extra_pkg_items);

	switch(dialog_vars.input_result[0]) {
		case 'a': show_pkginfo(pkg); break;
		case 'b': show_actions();    break;
		case 'c': show_deps();       break;
	}

	dialog_vars.extra_button = 1;
	dialog_vars.help_button  = 1;
	dialog_vars.cancel_label = "Finish";
}

char selector_help_txt[] =
"\n``We do not choose the day of our birth nor may we choose the day\n"
"  of our death, yet choice is the sovereign faculty of the mind.''\n"
"    -- Thornton Wilder (1897-1975)\n"
"\n"
"\n"
"You can navigate through the menu by using either the arrow keys\n"
"or +/-/btab/tab (for up, down, left, right).\n"
"\n"
"Furthermore you can entry directories (de/select packages respect-\n"
"ively) by pressing the return-key while the button focus is on the\n"
"`Select' button.\n"
"\n"
"There are also a few extra commands for additional information and\n"
"convenience, which are accessible via the `Extras' button.\n"
"\n"
"When the list focus is set on a directory, the following extras\n"
"are available:\n"
"   * Open entire directory tree      \n" 
"   * Close entire directory tree     \n"
"   * Un-Install all packages in tree \n"
"   * Reset all packages in tree      \n"
"   * Install all packages in tree    \n"
"   * Upgrade all packages in tree    \n"
"   * Re-Install all packages in tree \n"
"   * Show current action-list        \n"
"   * Install unresolved dependencies \n"
"   * List unresolved dependencies    \n"
"\n"
"Whereas the following extras are available when the list focus is\n"
"on a file:\n"
"   * Show package information        \n"
"   * Show current action-list        \n"
"   * List unresolved dependencies    \n"
;

void selector_help()
{
	char filename[] = "/tmp/gas_help_XXXXXX";
	int fd = mkstemp(filename);

	write(fd, selector_help_txt, sizeof(selector_help_txt)-1);
	close(fd);

	dialog_textbox(" Help for G.A.S. (GEM Autoinstall Shell) GUI ",
	               filename, screensize_y, screensize_x);
	unlink(filename);
}

int instmenu1()
{
	dialog_vars.extra_button  = 0;
	dialog_vars.help_button   = 0;
	dialog_vars.nocancel      = 1;

	dialog_vars.input_result[0]=0;
	dialog_vars.default_item = NULL;
	dialog_menu(" G.A.S. (GEM Autoinstall Shell) GUI ",
			"", 8, 60, 2, 2, instmenu1_items);

	dialog_vars.extra_button  = 1;
	dialog_vars.help_button   = 1;
	dialog_vars.nocancel      = 0;

	if (dialog_vars.input_result[0] == '1') return 1;
	if (dialog_vars.input_result[0] == '2') return 2;
	return 0;
}

int instmenu2()
{
	struct filesystem *fs;
	struct package *p;

	char *df, line[1024];
	long long used, total;
	int i, lines = 0;

	for ( fs=filesystems; fs != NULL; fs=fs->next ) fs->install = 0;

	for (p=packages; p != NULL; p = p->next) {
		if ( (p->is_installed && p->action >= 0) || p->action > 0 )
			for ( i=1, fs=filesystems; i<=filesystem_count;
			      i++, fs=fs->next ) fs->install += p->size[i];
	}

	df = malloc(filesystem_count * 1024 + 1024);
	snprintf(df, 1024, "%-25s %10s %8s %10s\n", "Filesystem", "Used", "Free", "FS Size");
	for ( fs = filesystems; fs != NULL; fs = fs->next ) {
		if ( ! fs->useit ) continue;
		total = (long long)fs->fs_data.f_blocks * (long long)fs->fs_data.f_bsize;
		used = (long long)(fs->fs_data.f_blocks - fs->fs_data.f_bfree) *
					(long long)fs->fs_data.f_bsize + fs->install;
		snprintf(line, 1024, "%-25s %7Ld MB %7Ld%% %7Ld MB\n",
			fs->directory, used/1048576, 100 - (used*100) / total, total/1048576);
		strcat(df, line);
		lines++;
	}

	dialog_vars.extra_button  = 0;
	dialog_vars.help_button   = 0;
	dialog_vars.nocancel      = 1;

	dialog_vars.input_result[0]=0;
	dialog_vars.default_item = NULL;
	dialog_menu(" G.A.S. (GEM Autoinstall Shell) GUI ",
			df, 9+lines, 60, 2, 2, instmenu2_items);

	dialog_vars.extra_button  = 1;
	dialog_vars.help_button   = 1;
	dialog_vars.nocancel      = 0;

	if (dialog_vars.input_result[0] == '1') return 1;
	if (dialog_vars.input_result[0] == '2') return 2;
	return 0;
}

int checkinstsize()
{
	struct filesystem *fs;
	struct package *p;

	char *warning, line[1024];
	long long freespace;
	int i, lines = 0;

	for ( fs=filesystems; fs != NULL; fs=fs->next ) fs->install = 0;

	for (p=packages; p != NULL; p = p->next) {
		if ( (p->is_installed && p->action >= 0) || p->action > 0 )
			for ( i=1, fs=filesystems; i<=filesystem_count;
			      i++, fs=fs->next ) fs->install += p->size[i];
	}

	warning = malloc(filesystem_count * 1024 + 1024);
	snprintf(warning, 1024, "\n%46s\n\n", "! ! ! ! !  W A R N I N G  ! ! ! ! !");

	for ( fs = filesystems; fs != NULL; fs = fs->next ) {
		if ( ! fs->useit ) continue;
		
		freespace = 100 - ( ((long long)(fs->fs_data.f_blocks - fs->fs_data.f_bfree) *
			(long long)fs->fs_data.f_bsize + fs->install) * 100 ) /
			((long long)fs->fs_data.f_blocks * (long long)fs->fs_data.f_bsize);
		
		if ( freespace > 10 ) continue;
		
		snprintf(line, 1024, "%d%% space left on %s "
			"after install!\n", (int)freespace, fs->directory);
		strcat(warning, line);
		lines++;
	}

	if ( !lines ) return 2;

	dialog_vars.extra_button  = 0;
	dialog_vars.help_button   = 0;
	dialog_vars.nocancel      = 1;

	dialog_vars.input_result[0]=0;
	dialog_vars.default_item = NULL;
	dialog_menu(" G.A.S. (GEM Autoinstall Shell) GUI ",
			warning, 12+lines, 60, 2, 2, instmenu2a_items);

	dialog_vars.extra_button  = 1;
	dialog_vars.help_button   = 1;
	dialog_vars.nocancel      = 0;

	if (dialog_vars.input_result[0] == '1') return 1;
	if (dialog_vars.input_result[0] == '2') return 2;
	return 0;
}

int do_selection()
{
	char my_buffer[MAX_LEN + 1] = "";
	struct package *p;
	struct winsize ws;
	int bigmain = 0;
	int rc, pkgcount;
	char *main_item;

	/* Create enought space for the items[] and entries[] arrays */
	items = malloc( (directory_entry_count+20) * sizeof(char*) * 2 );
	entries = malloc( (directory_entry_count+20) * sizeof(void*) );
	main_item = malloc(10);

	dialog_vars.default_item = main_item;
	dialog_vars.cancel_label = "Finish";
	dialog_vars.ok_label     = "Select";
	dialog_vars.extra_label  = "Extras";
	dialog_vars.extra_button = 1;
	dialog_vars.help_label   = "Help";
	dialog_vars.help_button  = 1;
	dialog_vars.input_result = my_buffer;
	strcpy(dialog_vars.default_item, "   1");

	if ( !ioctl(1, TIOCGWINSZ, &ws) ) {
		screensize_x = ws.ws_col - 10;
		screensize_y = ws.ws_row - 3;
	}

	init_dialog(stdin, stdout);

	/* Yup - that's a label for goto.... */
main_loop:
	while (1) {
		dlg_put_backtitle();

		for (p=packages; p != NULL; p = p->next) {
			p->is_dependency=0;
		}

		pkgcount=0;
		for (p=packages; p != NULL; p = p->next) {
			if ( (p->is_installed && p->action >= 0) ||
			     p->action > 0 ) {
					pkgcount++;
					check_dep_recurs(NULL, 0, p);
			}
		}

		if ( !pkgcount && !bigmain ) {
			char *dirname = "all";
			struct directory_entry * e;
			if ( instmenu1() == 2 ) dirname = "base";
			for (e = rootdir->list; e != NULL; e = e->next) {
				if ( e->content_is_subdir &&
				     !strcmp(e->content.dir->name, dirname) )
					set_tree_flag(e->content.dir, 0, 1);
			}
			bigmain = 1;
			break;
		}

		item_counter=0;
		create_dialog_itemlist(rootdir, 0);

		dialog_vars.input_result[0]=0;
		dialog_vars.default_item = main_item;
		rc = dialog_menu(" G.A.S. (GEM Autoinstall Shell) GUI ",
				HELP_TEXT, screensize_y, screensize_x,
				screensize_y-12, item_counter, items);
		if ( rc == DLG_EXIT_HELP  ) {
			// DLG_EXIT_HELP modifies input_result
			// strcpy(main_item, dialog_vars.input_result);
			free_dialog_item_list();
			selector_help();
			continue;
		}
		if ( rc == DLG_EXIT_EXTRA ) {
			rc = atoi(dialog_vars.input_result) - 1;
			strcpy(main_item, dialog_vars.input_result);
			free_dialog_item_list();

			if (entries[rc]->content_is_subdir)
				extra_dir(entries[rc]->content.dir);
			else
				extra_pkg(entries[rc]->content.pkg);

			continue;
		}
		if ( rc ) {
			free_dialog_item_list();
			break;
		}

		rc = atoi(dialog_vars.input_result) - 1;

		if (entries[rc]->content_is_subdir) {
			entries[rc]->content.dir->is_open_in_gui =
				! entries[rc]->content.dir->is_open_in_gui;
		} else {
			if ( entries[rc]->content.pkg->action == 0 )
				entries[rc]->content.pkg->action = 1;
			else if ( entries[rc]->content.pkg->action < 0 )
				entries[rc]->content.pkg->action = 0;
			else if ( entries[rc]->content.pkg->action > 0 )
				entries[rc]->content.pkg->action =
					entries[rc]->content.pkg->
						is_installed ? -1 : 0;
		}

		strcpy(main_item, dialog_vars.input_result);
		free_dialog_item_list();
	}
	/* Yup - that's a goto. it is jumping upwards and building a loop!
	 * It's just here because I felt like using goto at this place.. */
	if ( bigmain && instmenu2() != 1 ) goto main_loop;
	if ( checkinstsize() != 2 ) goto main_loop;

	end_dialog();

	free(items);
	free(entries);

	free(main_item);
	dialog_vars.default_item = NULL;

	return 0;
}

