#! /bin/sh

# These commands are run inside Qemu after Qemu loads a version of the current
# directory into memory.

make clean
make || exit 1
echo
echo "*** osprd.ko built successfully.  Now loading module."
insmod osprd.ko
./create-devs
echo "*** Module loaded and ready to test!"
echo