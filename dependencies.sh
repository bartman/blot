#!/bin/bash
set -e

self=${0##*/}

USER=$( id -u -n )
SUDO=
[ $USER = root ] || SUDO=sudo

die() { echo >&2 "ERROR: $*" ; exit 1 ; }

YES=

show_help() {
    cat <<END
$self ...

    -h      this help
    -y      answer yes to package manager questions
    gcc     use gcc as the compiler
    clang   use clang as the compiler

END
}

while [ -n "$1" ] ; do
    case "$1" in
        -h|--help)
            show_help
            exit 0
            ;;
        -y|--yes)
            YES=-y
            ;;
        gcc|clang)
            CC=$1
            ;;
        *)
            die "invalid option $1"
            ;;
    esac
    shift
done

run() { echo >&2 "# $@" ; $SUDO "$@" ; }
say() { echo >&2 "$@" ; }
die() { echo >&2 "$@" ; exit 1 ; }

if [ -s /etc/redhat-release ]; then

    die "RHEL/CentOS not yet supported"

elif [ -f /etc/SuSE-release ] || [ -f /etc/SUSE-brand ]; then

    die "SuSE not yet supported"

elif [ -f /etc/debian_version ]; then

    say "Installing Debian/Ubuntu dependencies"

    compiler=()
    case "$CC" in
        *gcc*) compiler=( gcc g++ ) ;;
        *clang*) compiler=( clang ) ;;
        *) compiler=( gcc g++ clang ) ;;
    esac

    run apt install $YES make ninja-build cmake libglib2.0-dev git libgtest-dev "${compiler[@]}"

else
    die "Distribution is not handled"
fi
