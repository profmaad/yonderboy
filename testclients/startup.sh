#!/bin/bash -x

TERMINAL="urxvtc -e"

WORKDIR="/home/profmaad/Workspace/cli-browser/"
SERVER="server/src/server"
CONTROLLER="controllers/cli-controller/src/cli-controller"
VIEWER="viewers/tabbed-gtk-viewer/src/tabbed-gtk-viewer"
CONFIG="/home/profmaad/.cli-browser/config"

$TERMINAL $WORKDIR$SERVER -c $CONFIG &
sleep 1
$TERMINAL $WORKDIR$CONTROLLER $CONFIG &
$TERMINAL $WORKDIR$VIEWER $CONFIG &

