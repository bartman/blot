#!/bin/bash
set -e -x

# https://gitlab.com/gitlab-org/gitlab-runner/issues/991
# ended up using squid-deb-proxy...
if [ -d /etc/apt/apt.conf.d ] ; then
        echo "Acquire::http::Proxy \"http://ci.jukie.net:8000/\";" > /etc/apt/apt.conf.d/apt-proxy.conf
fi
apt update -qq
