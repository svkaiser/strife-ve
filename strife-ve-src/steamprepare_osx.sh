#!/bin/sh

BASEDIR=$(dirname $0)
APPDIR=./xcode/Chocolate/Build/Products/Release/Prepared
STEAMDIR=./../steam-service
CONTENTDIR=$STEAMDIR/sdk/tools/ContentBuilder/content/strife-ve
BUILDDIR=$STEAMDIR/sdk/tools/ContentBuilder
SCRIPTDIR=$STEAMDIR/api/steam-service/buildscripts
APPNAME=Strife-VE.app

cd $BASEDIR

if [ -d $APPDIR/$APPNAME ]; then
    rm -rf $APPDIR/$APPNAME
fi

echo "Preparing application - App ID is 317040"
echo "Save it in $APPDIR/$APPNAME"
echo "Disable binary scrambling"

cd $STEAMDIR/sdk/tools/

open ContentPrep.app &
wait

read -p "Press [Enter] key after application has been prepared"

cd ./../../../Strife/

echo "Copying files to $CONTENTDIR"

cp -rf $APPDIR/$APPNAME $CONTENTDIR/macosx/
rsync -u $SCRIPTDIR/app_build_317040.vdf $BUILDDIR/scripts/
rsync -u $SCRIPTDIR/depot_build_317041.vdf $CONTENTDIR/
rsync -u $SCRIPTDIR/depot_build_317042.vdf $CONTENTDIR/
rsync -u $SCRIPTDIR/depot_build_317043.vdf $CONTENTDIR/
rsync -u $SCRIPTDIR/depot_build_317044.vdf $CONTENTDIR/

echo "Done"
