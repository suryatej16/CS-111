#! /bin/bash

function die () {
    echo -e "$@" 1>&2
    exit 1
}

function singlequote () {
    echo "$@" | sed -e "s/'/"'"'"'"'"/g
s/"/'"'"'"'"'/g"
}

function find_readable_dir() {
    suffix="$1"
    shift
    for i in "$@"; do
        [ -n "$i" -a -r "$i/$suffix" ] && echo "$i" && return 0
    done
    return 1
}

make=make
if which gmake 2>&1 | grep -v '^\(which: \)*no' >/dev/null && which gmake 2>/dev/null | grep . >/dev/null; then
    make=gmake
fi

tmpdir=/tmp
if uname | grep -i cygwin >/dev/null 2>&1; then
    labdir=`pwd`
    tmpdir=${labdir:0:12}/tmp
    if ! [ -d "$tmpdir" ] ; then
        [ -f "$tmpdir" ] && die "${tmpdir} exists and is a regular file!"
        mkdir "$tmpdir"
    fi
else
    tmpdir="${tmpdir}/cs111_$LOGNAME$$"
    mkdir "$tmpdir" || exit 1
fi

verbose=n
loadvm="-loadvm osp"
clean=y
method=n
hda=
hdb=
hdc=
hdd=
fda="-fda /dev/null"
fdax=
fdb=
qopt=""
grabqopt="X"
kqopt=" -k en-us"
m=128
while expr "$1" : "-.*" >/dev/null 2>&1; do
    if [ "$1" = "--help" ]; then
        echo "Usage: ./run-qemu [--verbose] [cs111.iso location]" 1>&2
        exit 0
    elif [ "$1" = "-V" -o "$1" = "--verbose" ]; then
        verbose=y
        shift
    elif [ "$1" = "--no-loadvm" ]; then
        loadvm=""
        shift
    elif [ "$1" = "--no-clean" ]; then
        clean=n
        shift
    elif [ "$1" = "--no-grab" -o "$1" = "-no-grab" ]; then
        grabqopt="-no-grab "
        shift
    elif [ "$1" = "--grab" -o "$1" = "-grab" ]; then
        grabqopt=""
        shift
    elif [ "$1" = "--use-tftp" -o "$1" = "--use-net" ]; then
        method=n
        shift
    elif [ "$1" = "--use-iso" -o "$1" = "--use-floppy" -o "$1" = "--use-fd" ]; then
        method=i
        shift
    elif [ "$1" = "-hda" ]; then
        x=`singlequote "$2"`
        hda="-hda $x"
        shift 2
    elif [ "$1" = "-hdb" ]; then
        x=`singlequote "$2"`
        hdb="-hdb $x"
        shift 2
    elif [ "$1" = "-hdc" ]; then
        x=`singlequote "$2"`
        hdc="-hdc $x"
        shift 2
    elif [ "$1" = "-cdrom" ]; then
        x=`singlequote "$2"`
        hdc="-cdrom $x"
        shift 2
    elif [ "$1" = "-hdd" ]; then
        x=`singlequote "$2"`
        hdd="-hdd $x"
        shift 2
    elif [ "$1" = "-fda" ]; then
        x=`singlequote "$2"`
        fda="-fda $x"
        fdax=1
        shift 2
    elif [ "$1" = "-fdb" ]; then
        x=`singlequote "$2"`
        fdb="-fdb $x"
        shift 2
    elif [ "$1" = "-m" ]; then
        x=`singlequote "$2"`
        m="$x"
        shift 2
    elif [ "$1" = "-vnc" ]; then
        x=`singlequote "$2"`
        qopt="$qopt -vnc $x"
        shift 2
    elif [ "$1" = "-rvnc" ]; then
        x=`singlequote "$2"`
        qopt="$qopt -vnc localhost:$x,reverse"
        shift 2
    elif [ "$1" = "-k" ]; then
        x=`singlequote "$2"`
        if [ "$x" = none -o -z "$x" ]; then
            kqopt=""
        else
            kquot=" -k $x"
        fi
    else
        break
    fi
done
qopt="$qopt$kqopt -m $m"

function myclean () {
    rm -rf $tmpdir
    echo Cleaning up image in $tmpdir
}

if [ $clean = y ]; then
    trap myclean EXIT
fi

function myrun() {
    test $verbose = y && echo '+' "$@" 1>&2
    "$@"
}

function myrunquiet() {
    if [ $verbose = y ]; then
        echo '+' "$@" 1>&2
        "$@"
    else
        "$@" >/dev/null 2>&1
    fi
}

function fix_qopt() {
    qemu_bin="$1"
    if [ "$grabqopt" = X ]; then
        if "$qemu_bin" --help | grep no-grab >/dev/null 2>&1; then
            grabqopt="-no-grab "
        else
            grabqopt=""
        fi
    fi
    qopt="$grabqopt$qopt"
}

function run_iso() {
    qemu_bin="$1"
    biosdir="$2"
    bsw=
    [ -n "$biosdir" ] && bsw=-L
    fix_qopt "$qemu_bin"

    echo "Building image in $tmpdir"

    TMPFILE="$tmpdir/lab.iso"
    ( uname | grep -i cygwin >/dev/null 2>&1 ) && TMPFILE=`cygpath -w $TMPFILE`

    mkisofsarg=
    if [ -r .mkisofs ]; then
        mkisofsarg=`cat .mkisofs`
    fi
    myrunquiet mkisofs -R -iso-level 3 -o $TMPFILE -m CVS -m .svn -m '*.tar.gz' -m '*.qcow2' -m '*.qvm' -m '*.ko' -m '*.o' -m '*.img' -m '*.iso' $mkisofsarg .

    [ -z "$fdax" ] && fda="-fda $TMPFILE"
    "$qemu_bin" -help | grep -e -no-kqemu >/dev/null 2>&1 && qopt="$qopt -no-kqemu"
    myrun "$qemu_bin" $qopt $bsw $biosdir $hda $hdb $hdc $hdd $fda $fdb $loadvm -tftp "$tmpdir" -redir tcp:2222::22
    exit $?
}

function mk_tmp_tarball() {
    $make tarball || exit 1
    mv lab2-${USER}.tar.gz "$tmpdir/lab.tar.gz"
}

function run_net() {
    qemu_bin="$1"
    biosdir="$2"
    bsw=
    [ -n "$biosdir" ] && bsw=-L
    fix_qopt "$qemu_bin"

    mk_tmp_tarball

    "$qemu_bin" -help | grep -e -no-kqemu >/dev/null 2>&1 && qopt="$qopt -no-kqemu"
    myrun "$qemu_bin" $qopt $bsw $biosdir $hda $hdb $hdc $hdd $fda $fdb $loadvm -tftp "$tmpdir" -redir tcp:2222::22
    exit $?
}


if uname | grep -i Darwin >/dev/null 2>&1; then
    # Mac OS X
    if [ -z "$hda$hdc" ]; then
        cs111d=`find_readable_dir cs111.iso "$1" "$HOME" "$HOME/Documents/QEMU" "."`
        [ -n "$cs111d" -a -r "$cs111d/cs111.iso" -a -z "$hdc" ] && hdc="-cdrom $cs111d/cs111.iso"
        [ -n "$cs111d" -a -r "$cs111d/cs111snap.qcow2" -a -z "$hda" ] && hda="-hda $cs111d/cs111snap.qcow2"
        [ -z "$hdc" -a -n "$1" -a -r "$1" ] && hdc="-cdrom $1"
        [ -z "$hda" -a -n "$2" -a -r "$2" ] && hda="-hda $2"
        [ -z "$hda$hdc" ] && die "Can't find cs111.iso!  Please run me as follows: './run-qemu DIR',
where DIR is the location of the directory containing cs111.iso."
    fi
    [ -f osprd.c ] || die "Not a Lab 2 directory!"

    expected_qemu="/Applications/Q.app/Contents/MacOS/i386-softmmu.app/Contents/MacOS/i386-softmmu"
    if [ ! -x $expected_qemu ]; then
        die "Can't figure out how to run Q!"
    fi

    if which mkisofs 2>&1 | grep -v '^\(which: \)*no' >/dev/null && [ -x $expected_qemu ]; then
        if echo "$method" | grep i >/dev/null 2>&1; then
            run_iso "$expected_qemu"
        fi
    fi
    run_net "$expected_qemu"
else
    # Linux / Cygwin
    which qemu > /dev/null || die 'Cannot find a qemu executable!  Fix your $PATH environment variable,
or (if using the lab machines), verify you'"'"'re booted into the CS111 partition.'

    qemudir=`which qemu | sed 's;/qemu$;;' | sed 's;bin$;;'`

    if [ -z "$hda$hdc" ]; then
        cs111d=`find_readable_dir cs111.iso "$1" "$HOME" "${qemudir}share/qemu" "${qemudir}share" "${qemudir}" "." ".."`
        [ -n "$cs111d" -a -r "$cs111d/cs111.iso" -a -z "$hdc" ] && hdc="-cdrom $cs111d/cs111.iso"
        [ -n "$cs111d" -a -r "$cs111d/cs111snap.qcow2" -a -z "$hda" ] && hda="-hda $cs111d/cs111snap.qcow2"
        [ -z "$hdc" -a -n "$1" -a -r "$1" ] && hdc="-cdrom $1"
        [ -z "$hda" -a -n "$2" -a -r "$2" ] && hda="-hda $2"
        [ -z "$hda$hdc" ] && die "Can't find cs111.iso!  Please run me as follows: './run-qemu DIR',
where DIR is the location of the directory containing cs111.iso."
    fi
    [ -f osprd.c ] || die "Not a Lab 2 directory!"

    biosdir=
    if uname | grep -i cygwin >/dev/null 2>&1; then
        qemudir=`cygpath -w $qemudir`
        debospimg=`cygpath -w $debospimg`
        biosdir="$qemudir"
    fi

    if which mkisofs 2>&1 | grep -v '^\(which: \)*no' >/dev/null && which mkisofs 2>/dev/null | grep . >/dev/null; then
        if echo "$method" | grep i >/dev/null 2>&1; then
            run_iso qemu "$biosdir"
        fi
    fi
    run_net qemu "$biosdir"
fi