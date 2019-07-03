#!/bin/sh
mkdir -p cache

# LLVM
LLVM_TRIPLE=x86_64-linux-gnu
LLVM_OS=ubuntu
case $LLVM_VERSION in
    3.3)
        LLVM_REL_EXT=tar.gz
        ;;
    3.4.2)
        LLVM_REL_EXT=xz
        ;;
    *)
        LLVM_REL_EXT=tar.xz
        ;;
esac

case $LLVM_VERSION in
    3.3)
        LLVM_OS=Ubuntu
        LLVM_TRIPLE=amd64
        LLVM_UBUNTU_VER=12.04.2
        ;;
    3.[0-7]*|7.1.0)
        LLVM_UBUNTU_VER=14.04
        ;;
    ?*)
        LLVM_UBUNTU_VER=16.04
        ;;
esac
LLVM_URL="http://releases.llvm.org/$LLVM_VERSION/clang+llvm-$LLVM_VERSION-$LLVM_TRIPLE-$LLVM_OS-$LLVM_UBUNTU_VER.$LLVM_REL_EXT"
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
