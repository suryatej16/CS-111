#!/bin/bash

dev=
offset=0
in=

darg () {
        if [ -z "$dev" ]; then dev="$1"; else out="$1"; fi
}

usage()
{
        echo "Usage: $0 [DEVICE] [SECTOR] INFILE"
        echo "  DEVICE has the form \"osprd*\" and defaults to \"osprda\""
        echo "  SECTOR is the sector number and defaults to 0"
        echo "  INFILE is the file to copy to the device"
        exit 1
}

while [ $# -ne 0 ]
do
        arg=$1
        shift
        case "$arg" in
                -h | --help)
                        usage
                        ;;
                osprd[abcd])
                        darg "/dev/$arg"
                        ;;
                /dev/*)
                        darg "$arg"
                        ;;
                [0-9]*)
                        offset="$arg"
                        ;;
                *)
                        in="$arg"
                        ;;
        esac
done

if [ -z "$in" -a -t 0 ]; then
        echo "sector_write: Refusing to read from terminal!  Supply a filename or '-'." 1>&2
        exit 1
fi

[ -z "$dev" ] && dev=/dev/osprda
ifarg="if=$in"; [ "$in" = '-' -o "$in" = '/dev/stdin' -o -z "$in" ] && ifarg=
ofarg="of=$dev"; [ "$dev" = '-' -o "$dev" = '/dev/stdout' ] && ofarg=

if [ -z "$in" ]
then
        echo "Error: Must provide input file"
        usage
else
        dd $ifarg $ofarg bs=512 count=1 seek=$offset
fi