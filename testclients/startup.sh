#!/bin/bash -x

TERMINAL="urxvtc -e"

SERVER="../server/src/server"
CONTROLLER="ruby controller.rb"
VIEWER="ruby viewer.rb"

#$TERMINAL $SERVER -c ~/.cli-browser/config &
$TERMINAL gdb --args $SERVER -c ~/.cli-browser/config &
#sleep 5
read
$TERMINAL $CONTROLLER &
$TERMINAL $VIEWER ~/.cli-browser/viewer.sock &

