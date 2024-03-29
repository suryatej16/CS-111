#!/bin/bash

dev=
offset=0
out=

darg () {
        if [ -z "$dev" ]; then dev="$1"; else out="$1"; fi
}

while [ $# -ne 0 ]
do
        arg=$1
        shift
        case "$arg" in
                -h | --help)
                        echo "Usage: $0 [DEVICE] [SECTOR] [OUTFILE]"
                        echo "  DEVICE has the form \"osprd*\" and defaults to \"osprda\""
                        echo "  SECTOR is the sector number and defaults to 0"
                        echo "  OUTFILE is the output file or \"-\" for stdout and defaults to \"-\""
                        exit 1
                        ;;
                osprd[abcd])
                        darg "/dev/$arg"
                        ;;
                /dev/*|-)
                        darg "$arg"
                        ;;
                [0-9]*)
                        offset="$arg"
                        ;;
                *)
                        out="$arg"
                        ;;
        esac
done

[ -z "$dev" ] && dev=/dev/osprda
ifarg="if=$dev"; [ "$dev" = '-' -o "$dev" = '/dev/stdin' ] && ifarg=
ofarg="of=$out"; [ "$out" = '-' -o "$out" = '/dev/stdout' ] && ofarg=

if [ -z "$out" ]
then
        dd $ifarg bs=512 count=1 skip=$offset | hexdump -C
else
        dd $ifarg $ofarg bs=512 count=1 skip=$offset
fi