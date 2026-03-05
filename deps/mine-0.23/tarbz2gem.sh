#!/bin/sh -e

if [ $# != 2 -o ! -f "$1" ] ; then
	echo
	echo "Convert *.tar.bz2 file to *.gem file."
	echo
	echo "Usage: $0 foobar.tar.bz2 foobar.gem"
	echo
	exit 1
fi

tmpdir=`mktemp -d`
tar -xvIf "$1" -C $tmpdir var/adm
package="$( ls $tmpdir/var/adm/packages )"
mine -C $tmpdir/var/adm "$1" "$package" "$2"
rm -rf $tmpdir

