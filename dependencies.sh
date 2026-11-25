#!/bin/bash
set -e

self=${0##*/}

USER=$( id -u -n )
SUDO=
[ $USER = root ] || SUDO=sudo

die() { echo >&2 "ERROR: $*" ; exit 1 ; }

YES=
CI_ONLY=

show_help() {
    cat <<END
$self ...

    -h          this help
    -y          answer yes to package manager questions
    --ci-only   install only what CI needs to build/test (implies -y):
                ci/common.packages, the CI images' list
    gcc         use gcc as the compiler
    clang       use clang as the compiler

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
        --ci-only)
            CI_ONLY=1
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

    if [ -n "$CI_ONLY" ] ; then
        # the package list of the GitLab CI images (see the top of
        # .gitlab-ci.yml): CI builds with gcc; clang is added when asked
        list="$(cd "$(dirname "$0")" && pwd)/ci/common.packages"
        [ -r "$list" ] || die "can't read $list"
        packages=( $(sed -e 's/#.*//' "$list") )
        [ ${#packages[@]} -gt 0 ] || die "no packages in $list"
        case "$CC" in
            *clang*) packages+=( clang ) ;;
        esac
        run apt install $YES "${packages[@]}"
        exit 0
    fi

    compiler=()
    case "$CC" in
        *gcc*) compiler=( gcc g++ ) ;;
        *clang*) compiler=( clang ) ;;
        *) compiler=( gcc g++ clang ) ;;
    esac

    run apt install $YES make ninja-build cmake libglib2.0-dev git libgtest-dev libspdlog-dev libfmt-dev "${compiler[@]}"

else
    die "Distribution is not handled"
fi
