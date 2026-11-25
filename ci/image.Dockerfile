# CI images for blot's jobs (#1), built by the prep-container stage of
# .gitlab-ci.yml with kaniko.
#
# Generic: the base image (BASE_IMAGE) and the apt package list (PACKAGES)
# are build-args, set by the prep-container jobs from ci/images and the
# package files (ci/*.packages, through ci/image-tag.sh), so a new
# dependency is a change there, not here.  An image's tag is a hash of
# this file, the base image and the package list, so any change here
# builds every image again.  See the top of .gitlab-ci.yml for when the
# images are built and which tag the jobs use.
#
# Only this build runs apt; the jobs use the result as is.  The images are
# used only on the lab runners, so the apt proxy setting stays in them.

ARG BASE_IMAGE
FROM ${BASE_IMAGE}

# the space-separated apt package list (see above)
ARG PACKAGES
# an ARG, not an ENV: it applies to this build and doesn't persist into
# the image
ARG DEBIAN_FRONTEND=noninteractive

# apt downloads go through the lab's squid proxy (squid.localnet), as in
# thimulator's ci/image.Dockerfile and the runners' own apt setting; it
# replaces .gitlab/ci/prepare-apt-proxy.sh's old ci.jukie.net:8000
RUN if [ -d /etc/apt/apt.conf.d ] && [ -w /etc/apt/apt.conf.d ]; then echo 'Acquire::http::Proxy "http://10.10.11.236:3128/";' > /etc/apt/apt.conf.d/zz-squid-proxy.conf; fi

# CI containers are thrown away: dpkg needn't fsync, and needn't install
# docs, man pages (no man-db trigger work), translations or info pages
RUN printf '%s\n' \
        force-unsafe-io \
        'path-exclude=/usr/share/doc/*' \
        'path-include=/usr/share/doc/*/copyright' \
        'path-exclude=/usr/share/man/*' \
        'path-exclude=/usr/share/locale/*' \
        'path-exclude=/usr/share/info/*' \
        > /etc/dpkg/dpkg.cfg.d/ci-speedup

# one apt run, so the triggers run once
RUN test -n "$PACKAGES" \
        && apt-get update -qq \
        && apt-get install -y -qq --no-install-recommends $PACKAGES \
        && apt-get clean \
        && rm -rf /var/lib/apt/lists/*
