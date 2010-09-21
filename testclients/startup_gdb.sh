#!/bin/bash -x

TERMINAL="urxvtc -e"

WORKDIR="/home/profmaad/Workspace/cli-browser/"
SERVER="server/src/server"
CONTROLLER="controllers/cli-controller/src/cli-controller"
VIEWER="viewers/tabbed-gtk-viewer/src/tabbed-gtk-viewer"
CONFIG="/home/profmaad/.cli-browser/config"

$TERMINAL gdb --args $WORKDIR$SERVER -c $CONFIG &
#sleep 5
read
$TERMINAL $WORKDIR$CONTROLLER $CONFIG &
$TERMINAL $WORKDIR$VIEWER $CONFIG &

