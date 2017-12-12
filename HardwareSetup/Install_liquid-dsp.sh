#!/bin/bash

# This script attempts to build and install liquid-dsp from the
# github.com repo source for use with CRTS.

# This installs a particular tagged version of liquid-dsp so that you do not
# suffer from upstream code changes, and things stay more consistent and
# manageable.

# exit with error status if commands fail
set -e

# fail if any pipe line command fails
set -o pipefail

scriptdir=$(dirname ${BASH_SOURCE[0]})
cd $scriptdir
scriptdir=${PWD}


# Where is the liquid-dsp source code?
url=https://github.com/jgaeddert/liquid-dsp.git


# Try running from a git repo clone directory:
#
#  git log --date-order --tags --simplify-by-decoration --pretty="format:%ai %d"'
#
# You can just guess a name to start with and then run the above 'git log'
# command in the git clone directory.

# TODO: port code to use newer liquid-dsp version
#tag=v1.3.0

# git tag/version
tag=a4d7c80d3


# package name.  Just for file name space separation. Any name will do.
package=liquid-dsp


prefix=
topsrcdir=


function usage()
{
    cat << EOF

   Usage: $0 --prefix PREFIX [--srcdir SRCDIR]

     Download, build, and install the liquid-dsp libraries from the github source.
   This script currently installs release github tag $tag which we believe
   to be a release version.


    --------------------------- OPTIONS ---------------------------------


      --prefix PREFIX   Set the top installation prefix directory to
                        install liquid-dsp in to PREFIX.


      --srcdir SRCDIR    Set the top build source directory to SRCDIR.
                         This directory will be where the software is built
                         in.  The default comes from mktemp.

EOF
    exit 1
}

# Parse command line options
while [ -n "$1" ] ; do
    case "$1" in
        --prefix)
            shift 1
            prefix="$1"
            ;;
        --srcdir)
            shift 1
            topsrcdir="$1"
            ;;
        *)
            usage
            ;;
    esac
    shift 1
done

[ -z "$prefix" ] && usage
[ -z "$topsrcdir" ] && topsrcdir=$(mktemp -dt $(whoami)-$package-XXXXXX)


# Build attempt.  Appended to package and tag for making installation
# build attempt have a unique name.  Not like you have to use it.
attempt=1


# Unique relative installation name for these tags and attempt.
name=${package}-${tag}-$attempt


# Where all the build/source work happens.  Every file for every build
# attempt except installed files.  We put all the sources and build
# attempts in here.
srcdir=$topsrcdir/$name


# These were just dummy variables to help us configure
unset attempt name

# So the only variables left set are:
#   tag prefix topsrcdir srcdir scriptdir

########################################################################
################## END stuff you may want to change ####################
########################################################################


# Just so you understand: when we build this "package" we use two full
# copies of the source code, one that is the local git clone, and one copy
# of the source is the tagged version that is extracted from this local
# clone for each build attempt.  The build directory made from this
# local extracted source.  Patches could be added to the extracted source.
# We don't change the local clone source.


if [ -e $prefix ] ; then
    set +x
    echo "prefix directory (or file) $prefix exists"
    echo "Remove it if you would like to run this script."
    exit 1
fi
if [ -e $srcdir ] ; then
    echo "work/build directory $srcdir already exists"
    echo "remove it if you would like to run this script."
    exit 1
fi

set -x

mkdir -p $srcdir


# Usage: PullCheckGitUrl URL TAG REPO_DIR SRC_DIR
#                        $1  $2  $3       $4
# global topsrcdir must be defined
# This function sets up all the source files.
# TODO: consider what branch we use.  It's usually master.
function PullCheckGitUrl()
{
    [ -n "$4" ] || exit 1
    local url=$1
    local repo_dir=$3
    local src_dir=$4
    local cwd="$PWD"

    if [ ! -f "$repo_dir/.git/config" ] ; then
        git clone --recursive "$url" "$repo_dir"
    fi

    cd $repo_dir

    url="$(git config --get remote.origin.url)"
    set +x
    if [ "$url" != "$1" ] ; then
        echo "git cloned repo in \"$PWD\" is not from $1 it's from $url"
        exit 1
    fi
    echo -e "\ngit clone of \"$1\" in \"$PWD\" was found.\n"
    echo -e "pulling latest changes from $url\n"
    set -x
    #git pull --recurse-submodules
    git pull

    # pull the version of the source we want into the src_dir note: We are
    # not counting on the authors of this liquid-dsp code to not modify
    # the source code as they build the code.  Most package build systems
    # are notorious for polluting source code as they build, so we start
    # with clean copy of source files at the beginning of the build for
    # each build attempt; hence we do the following:
    git archive\
 --format=tar "$2" | $(cd "$src_dir" && tar -xf -)
    cd "$cwd"
}

# PullCheckGitUrl URL TAG REPO_DIR SRC_DIR
PullCheckGitUrl $url $tag $topsrcdir/git $srcdir

cd $srcdir

# We need to apply a small patch
patch -p1 < $scriptdir/liquid-dsp_isnan.patch

unset topsrcdir srcdir scriptdir
# we now just have: prefix package tag

./bootstrap.sh
# flag -march=native fixes error: inlining failed in call to always_inline
CFLAGS="-march=native -g -O2 -Wall" ./configure --prefix=$prefix
make
make install

set +x
echo -e "SUCCESSFULLY installed $package (tag=$tag) in:\n  $prefix"
