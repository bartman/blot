#!/bin/sh
# image-tag.sh [--base|--packages|--var] <name>
# image-tag.sh --names
#
# Print the tag of the CI image <name> (a line of ci/images): df-<12 hex
# digits>, the start of the sha256 of exactly what its build gets: the
# base image's name, the package list (its ci/*.packages files) and
# ci/image.Dockerfile.  So the tag changes when the image's content does,
# and only then: a comment in a package file, the order of the packages,
# or anything in .gitlab-ci.yml doesn't change it.
#   --base      print the base image instead
#   --packages  print the package list (one line, sorted, comments
#               dropped), as the build gets it
#   --var       print the name of the dotenv variable the jobs take the
#               image from: CI_IMAGE_<NAME> (upper case, - as _)
#   --names     print the names of the images
#
# The prep-container jobs run this in a busybox shell (see the top of
# .gitlab-ci.yml); it runs the same anywhere, to see an image's tag:
#   ci/image-tag.sh deb12

set -e
dir=$(dirname "$0")
table() { sed -e 's/#.*//' "$dir/images" | grep -v '^[[:space:]]*$'; }
what=tag
case "$1" in
--base|--packages|--var) what=${1#--}; shift ;;
--names) table | while read -r n rest; do echo "$n"; done; exit 0 ;;
esac
if [ $# -ne 1 ]; then
	echo "usage: $0 [--base|--packages|--var] <name> | --names" >&2
	exit 2
fi
line=$(table | while read -r n rest; do if [ "$n" = "$1" ]; then echo "$rest"; fi; done)
if [ -z "$line" ]; then
	echo "$0: no image $1 in $dir/images" >&2
	exit 1
fi
set -- $1 $line
name=$1 base=$2
shift 2
case "$what" in
base) echo "$base"; exit 0 ;;
var) echo "CI_IMAGE_$name" | tr 'a-z-' 'A-Z_'; exit 0 ;;
esac
files=
for f in "$@"; do files="$files $dir/$f"; done
pkgs=$(sed -e 's/#.*//' $files | tr -s ' \t' '\n\n' | grep -v '^$' | LC_ALL=C sort -u | tr '\n' ' ')
pkgs=${pkgs% }
if [ -z "$pkgs" ]; then
	echo "$0: no packages in$files" >&2
	exit 1
fi
if [ "$what" = packages ]; then
	echo "$pkgs"
	exit 0
fi
sum=$({ printf 'base %s\npackages %s\n' "$base" "$pkgs"; cat "$dir/image.Dockerfile"; } | sha256sum)
echo "df-$(echo "$sum" | cut -c1-12)"
