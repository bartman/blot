#!/bin/bash

USER=$( id -u -n )
SUDO=
[ $USER = root ] || SUDO=sudo

run() { echo >&2 "# $@" ; $SUDO $@ ; }
say() { echo >&2 "$@" ; }
die() { echo >&2 "$@" ; exit 1 ; }

if [ -s /etc/redhat-release ]; then

        die "RHEL/CentOS not yet supported"

elif [ -f /etc/SuSE-release ] || [ -f /etc/SUSE-brand ]; then

        die "SuSE not yet supported"

elif [ -f /etc/debian_version ]; then

        say "Installing Debian/Ubuntu dependencies"

        run apt install make ninja-build cmake libglib2.0-dev

else
        die "Distribution is not handled"
fi
