#! /bin/sh

inno=`../../../xt_base/info.sh innoloc`

appname=xictools_fasthenry
appdir=fasthenry
version=`../../release.sh`
osname=`../../../xt_base/info.sh osname`
arch=`uname -m`
top=../root/usr/local
toolroot=`../../../xt_base/info.sh toolroot`
base=../../../xt_base
baseutil=$base/packages/util
basefiles=$base/packages/files
pkgfiles=$base/packages/pkgfiles

utod=$baseutil/utod.exe
if [ ! -f $utod ]; then
    cwd=`pwd`
    cd $baseutil
    make utod.exe
    cd $cwd
fi

libdir=$top/$toolroot/fasthenry
$utod $libdir/README
$utod $libdir/README.mit
$utod $libdir/examples/*
$utod $libdir/examples/input/*
$utod $libdir/examples/results/linux_dss/*
$utod $libdir/examples/results/linux_klu/*
$utod $libdir/examples/results/linux_sparse/*
$utod $libdir/examples/torture/*

sed -e s/OSNAME/$osname/ -e s/VERSION/$version/ -e s/ARCH/$arch/ \
  -e s/TOOLROOT/$toolroot/g < files/$appname.iss.in > $appname.iss
$utod $appname.iss

$inno/iscc $appname.iss > build.log
if [ $? != 0 ]; then
    echo Compile failed!
    exit 1
fi

exfiles="$pkgfiles/$appname-$osname*.exe"
if [ -n "$exfiles" ]; then
    for a in $exfiles; do
        rm -f $a
    done
fi

pkg=Output/*.exe
if [ -f $pkg ]; then
    fn=$(basename $pkg)
    mv -f $pkg $pkgfiles
    echo ==================================
    echo Package file $fn
    echo moved to xt_base/packages/pkgfiles
    echo ==================================
fi
rmdir Output
rm $appname.iss
echo Done
