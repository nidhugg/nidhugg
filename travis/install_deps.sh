#!/bin/bash
mkdir -p cache

# LLVM
if [ -n "$LLVM_VERSION" ]; then
    LLVM_TRIPLE=x86_64-linux-gnu
    LLVM_OS=ubuntu
    LLVM_REL_EXT=tar.xz

    case $LLVM_VERSION in
        3.[89]*|[4-6].*)
            LLVM_UBUNTU_VER=16.04
            ;;
        [7-9].*|10.*)
            LLVM_UBUNTU_VER=18.04
            ;;
        ?*)
            LLVM_UBUNTU_VER=20.04
            ;;
    esac
    case $LLVM_VERSION in
        *-rc*|1?.*)
            LLVM_URL_PREFIX="https://github.com/llvm/llvm-project/releases/download/llvmorg-$LLVM_VERSION"
            ;;
        *)
            LLVM_URL_PREFIX="http://releases.llvm.org/$LLVM_VERSION"
            ;;
    esac
    LLVM_URL="$LLVM_URL_PREFIX/clang+llvm-$LLVM_VERSION-$LLVM_TRIPLE-$LLVM_OS-$LLVM_UBUNTU_VER.$LLVM_REL_EXT"
    LLVM_DEP="cache/$LLVM_VERSION.$LLVM_REL_EXT"
    LLVM_DIR="cache/clang+llvm-$LLVM_VERSION"

    echo $LLVM_URL
    echo $LLVM_DEP
    echo $LLVM_DIR

    # download and install
    if [ ! -f $LLVM_DEP ] ; then
        echo "Downloading LLVM"
        wget $LLVM_URL -O $LLVM_DEP
        if [ ! $? -eq 0 ]; then
	    echo "Error: Download failed"
	    rm $LLVM_DEP || true
	    exit 1
        fi
    else
        echo "Found Cached Archive"
    fi

    if [ ! -d $LLVM_DIR ] ; then
        echo "Extracting LLVM"
        tar -xf $LLVM_DEP -C cache/
        if [ ! $? -eq 0 ]; then
	    echo "Error: Extraction Failed"
	    rm $LLVM_DEP || true
	    exit 1
        fi
        case $LLVM_VERSION in
            3.5.*)
	        mv cache/clang+llvm-$LLVM_VERSION-$LLVM_TRIPLE cache/clang+llvm-$LLVM_VERSION
	        ;;
            ?*)
                mv cache/clang+llvm-$LLVM_VERSION-$LLVM_TRIPLE-$LLVM_OS-$LLVM_UBUNTU_VER cache/clang+llvm-$LLVM_VERSION
	        ;;
        esac
    else
        echo "Found Cached Installation"
    fi
fi

# Boost
if [ -n "$DOWNLOAD_BOOST" ]; then
    if [ ! -f cache/boost/include/boost/version.hpp ]; then
        # Boost is needed, and is not in cache
        BOOST_VER_UNDERSCORED=`echo "$DOWNLOAD_BOOST" | tr . _`
        BOOST_DIR=boost_$BOOST_VER_UNDERSCORED
        BOOST_FILE=$BOOST_DIR.tar.gz
        BOOST_URL=https://boostorg.jfrog.io/artifactory/main/release/$DOWNLOAD_BOOST/source/$BOOST_FILE
        mkdir -p cache/boost

        echo "Downloading Boost"
        wget $BOOST_URL
        tar xf $BOOST_FILE
        pushd $BOOST_DIR

        echo "Building Boost"
        ./bootstrap.sh --prefix=../cache/boost --with-libraries=test,system,timer
        ./b2 -j6 -d0
        ./b2 install -d0

        popd # $BOOST_DIR
    else
        echo "Found Cached Boost"
    fi
fi
