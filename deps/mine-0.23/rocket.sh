#!/bin/bash

help()
{
	cat << EOT

ROCKET - ROCK nEtwork packeTmanager ... or something alike ... ;-)

The GEM Pools (install sources) are configured in /etc/rocket.conf

rocket updsrc .............. update local rock source tree
rocket smap <patch-id> ..... apply patch from ROCK Linux SubMaster
rocket which <regex> ....... search in the "provides" package information

rocket build <pkg> ......... build tar.bz2 and .gem from sources
rocket emerge <pkg> ........ build and install a package

rocket create <pkg> ........ create .tar.bz2 and .gem file from local package
rocket index ............... create the index file for a directory with GEMs

rocket update .............. update the package cache
rocket install <pkg> ....... install/update package
rocket remove <pkg> ........ remove package

rocket search <regex> ...... search packge descriptions for this regex
rocket fsearch <regex> ..... search packge file list for this regex
rocket list <regex> ........ list all packages with a name matching the regex

rocket info <pkg> .......... print package details
rocket flist <pkg> ......... print package file list

EOT
	exit 1
}

upd_archive() {
	echo "Reading $1/packages.db ..."
	gawk '

BEGIN { chunk=0; pkg="NONE"; ver="UNKOWN"; }

$0 == "\027" { chunk++; next; }
$0 == "\004" {
	if (!ignore) {
		close("/var/adm/rocket/descs/" pkg);
		close("/var/adm/rocket/dependencies/" pkg);
		close("/var/adm/rocket/cksums/" pkg);
		print pkg " '"$1"'/" pkg "-" ver ".gem" \
			>> "/var/adm/rocket/locations.tmp";
	}
	chunk=0; pkg="NONE"; next;
}

chunk == 0 {
	pkg=$0;
	ignore = (match(pkg, "'$2'") != 0);
}
chunk == 1 && !ignore { print > "/var/adm/rocket/descs/" pkg; }
chunk == 2 && !ignore { print > "/var/adm/rocket/dependencies/" pkg; }
chunk == 3 && !ignore { print > "/var/adm/rocket/cksums/" pkg; }

chunk == 1 && $1 == "[V]" { ver = $2 "-" $3; }

' < <( curl -s "$1/packages.db" | gunzip; )
}

call_postinstall() {
	if [ -n "$( /bin/ls /var/adm/postinstall/ | head -n1 )" ]; then
		read -p "Do you want to run the postinstall scripts now? [Y/n] " yn
		if [ "$yn" = "y" -o "$yn" = "Y" -o "$yn" = "" ]; then
			postinstall
		fi
	fi
}

case "$1" in

    updsrc)
	cd /usr/src/rock-src || exit 1
	./scripts/Update-Src
	./scripts/Cleanup
	;;

    smap)
	cd /usr/src/rock-src || exit 1
	url="https://www.rocklinux.net/submaster/data/$(
		echo $1 | sed 's,[^0-9],,g; s,^\(....\)\(..\)\(.*\),\1/\2/\3.patch,'; )"
	rm -f rocket-smap-temp.patch
	wget -O rocket-smap-temp.patch "$url" || exit 1
	patch -p0 < rocket-smap-temp.patch || exit 1
	rm -f rocket-smap-temp.patch
	;;

    which)
	cd /usr/src/rock-src || exit 1
	egrep -l "^\[PROVIDES\].* ($2)($| )" package/*/*/*.cache | \
			cut -f4 -d/ | sed 's,\.cache$,,'
	;;

    build)
	shift
	"$0" emerge "$@"; rc=$?
	if [ $rc -eq 0 ]; then
		"$0" create "$@"
	fi
	exit $rc
	;;

    emerge)
	shift; cd /usr/src/rock-src || exit 1
	if [ ! -d config/rocket ]; then
		mkdir -p config/rocket
		cp -r /etc/ROCK-CONFIG/* config/rocket/
		./scripts/Config -cfg rocket -oldconfig
	fi
	./scripts/Build-Pkg -download -cfg rocket "$@"; rc=$?
	call_postinstall
	exit $rc
	;;

    create)
	shift
	for pkg; do
		for f in /var/adm/packages/$pkg /var/adm/packages/$pkg:*; do
			if [ -f "$f" ]; then
				p="$(basename $f)"
				v="$(grep '^Package Name and Version' $f | \
						cut -f6,7 -d' ' | tr ' ' - )"
				mine -T /var/adm / "$p" "$p-$v.tar.bz2"
				mine -C /var/adm "$p-$v.tar.bz2" "$p" "$p-$v.gem"
			fi
		done
	done
	;;

    update)
	rm -rf /var/adm/rocket
	mkdir -p /var/adm/rocket/{descs,dependencies,cksums}
	touch /var/adm/rocket/locations.tmp
	ignore_list="($(
			grep '^[ 	]*ignore' /etc/rocket.conf | \
				sed 's,^[ 	]*ignore[ 	]*,,' | \
				tr ' \t\n' '|' | sed 's,||*,|,g; s,|$,,; s,^|,,;'
		))"
	while read keyword p1 p2; do
		case "$keyword" in
		    archive)
			upd_archive "$p1" "$ignore_list"
			;;
		    meta-archive)
			url="$( curl -s "$p1" |
				gawk 'BEGIN { srand(); } { print rand(), $0; }' |
				sort | head -n1 | cut -f2 -d' '; )"
			upd_archive "$url" "$ignore_list"
			;;
		    ignore)
			;;
		    *)
			echo "Unknown keyword '$keyword' in config file!" >&2
			;;
		esac
	done < <( grep '^ *[^#]' /etc/rocket.conf | tac; )
	awk '{ l[$1] = $2; } END { for (i in l) print i, l[i]; }' \
		< /var/adm/rocket/locations.tmp > /var/adm/rocket/locations.txt
	rm -f /var/adm/rocket/locations.tmp
	;;

    search)
	matches=0;
	while read f; do
		echo
		if [ -f "/var/adm/packages/${f##*/}" ]; then
			echo "Match #$((matches++)) ${f##*/} (installed):"
		else
			echo "Match #$((matches++)) ${f##*/} (not installed):"
		fi
		egrep -i "^(\[I\]|\[T\].*$2)" $f
	done < <( egrep -lir "^\[(I|T)\].*$2" /var/adm/rocket/descs | sort; )
	if [ $matches -gt 0 ]; then
		echo
	fi
	;;

    fsearch)
	matches=0;
	if [ "${2#^}" != "$2" ]; then
		regex="^[^ ]+ [^ ]+ ${2#^}"
	else
		regex="^[^ ]+ [^ ]+ .*$2"
	fi
	while read f; do
		echo
		if [ -f "/var/adm/packages/${f##*/}" ]; then
			echo "Match #$((matches++)) ${f##*/} (installed):"
		else
			echo "Match #$((matches++)) ${f##*/} (not installed):"
		fi
		egrep -i "$regex" $f | cut -f3- -d' '
	done < <( egrep -lir "$regex" /var/adm/rocket/cksums | sort; )
	if [ $matches -gt 0 ]; then
		echo
	fi
	;;

    list)
	while read p; do
		if [ -f "/var/adm/packages/$p" ]
		then s='i'; else s='.'; fi
		grep '^\[I\].*' /var/adm/rocket/descs/$p |
			sed "s,^....,$s $p\t," | expand -t25,35,45,55
	done < <( egrep "^.*$2.* " /var/adm/rocket/locations.txt | cut -f1 -d' ' | sort; )
	;;

    info)
	echo
	if [ -f "/var/adm/packages/$2" ]; then
		echo "$2 (installed)"
	else
		echo "$2 (not installed)"
	fi
	echo
	egrep '^\[[I ]\]' /var/adm/rocket/descs/$2 && echo
	egrep '^\[[T ]\]' /var/adm/rocket/descs/$2 && echo
	egrep '^\[[U ]\]' /var/adm/rocket/descs/$2 && echo
	egrep '^\[[MA]\]' /var/adm/rocket/descs/$2 && echo
	egrep '^\[[V ]\]' /var/adm/rocket/descs/$2 && echo
	egrep '^\[[LS]\]' /var/adm/rocket/descs/$2 && echo
	;;

    flist)
	cut -f3- -d' ' /var/adm/rocket/cksums/$2
	;;

    install)
	shift
	deps=$( echo $( echo "$*" | tr ' ' '\n' |
gawk '

function get_deps(p,
		depsfn) {
	if (A[p]) return;
	A[p] = 1;

	depsfn = "/var/adm/rocket/dependencies/" p;

	while ((getline < depsfn) > 0) {
		if (!D[$2]) {
			D[$2] = 1;
			get_deps($2);
		}
	}

	close(depsfn);
}

{
	P[$1] = 1;
	get_deps($1);
}

END {
	for (d in D) {
		if (!P[d]) {
			pkgfn = "/var/adm/packages/" d;
			if ((getline < pkgfn) > 0)
				close(pkgfn);
			else {
				descfn = "/var/adm/rocket/descs/" d;
				if ((getline < descfn) > 0) {
					close(descfn);
					print d;
				}
			}
		}
	}
}

'; ); )
	if [ -n "$deps" ]; then
		echo "You requested for install:"
		echo "	$*"
		echo "The following dependencies are automatically added:"
		echo "	$deps"
		read -p "Do you want to continue? [Y/n] " yn
		if [ "$yn" != "y" -a "$yn" != "Y" -a "$yn" != "" ]; then
			echo ""
			echo "Hint: Add the packages you don't want to be installed"
			echo "to /etc/rocket.conf (with the 'ignore' keyword), run"
			echo "'rocket update' and re-run the command"
			echo ""
			echo "	$0 $*"
			echo ""
			echo "Abort."
			exit 1
		fi
		set -- $* $deps
	fi
	for pkg; do
		url="$( grep "^$pkg " /var/adm/rocket/locations.txt | \
				cut -f2 -d' ' | tail -n1; )"
		if [ -z "$url" ]; then
			echo "Package $pkg not found - maybe you need to add the archive"
			echo "to /etc/rocket.conf and run 'rocket update' first."
		else
			echo "+ mine -i '$url'"; mine -i "$url"
		fi
	done
	call_postinstall
	exit 0
	;;

    remove)
	shift
	echo "+ mine -rs $*"
	mine -rs "$@"
	call_postinstall
	;;

    index)
	mine -P *.gem | gzip > packages.db
	;;

    *)
	help
	;;
esac

