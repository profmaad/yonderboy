# yonderboy: a modular browser

## Description

yonderboy is supposed to be the core of a modular browser architecture, i.e. a daemon that handles all common browser tasks like session management, bookmarks, history keeping, cookie management, ... while
user interface, renderering and control are moved to outside processes communicating with the server via a socket connection.

This (hopefully) enables a wide variety of usage scenarios without having to reimplement the core functionality each time.

Apart from the core daemon, three classes of programs exist:

 * Controllers: these are used to control what the daemon and the connected components do, i.e. buttons in a user interface work as a controller
 * Viewers: they actually show the renderering results, most likely to a user
 * Renderers: they contain the renderering engine (i.e. WebKit, Gecko, ...) and expose it to the viewer via some means

## Current state

The core daemon exists and implements all the basic functionality to manage the other components. A lot of core functionality like session management and bookmarks is still missing, though.
A basic, though very usable, command line based controller is available (cli-controller), as well as a basic, gtk-based, tabbed viewer (tabbed-gtk-viewer). Additionally, a reference implementation for a renderer based on webkitgtk exists (webkit-renderer).

As the viewer and the renderer integrate via XEMBED, the system currently only works on Linux with X11. The daemon doesn't care how the viewer and renderer integrate, though, so it is possible to implement
both components in a system-agnostic way.

## TODO

 * create new tab functionality
 * session management
 * bookmark management
 * history keeping
 * cookie management
 * SSL management
 * ...

## Requirements

 * autotools
 * libreadline
 * libpthread
 * libpopt (only for tabbed-gtk-viewer and cli-controller)
 * yaml-cpp >= 0.2.5 (http://code.google.com/p/yaml-cpp/)
 * GTK+ >= 2.20 (http://www.gtk.org/) (only for webkit-renderer and tabbed-gtk-viewer)
 * WebKitGTK >= 1.2.0 (http://webkitgtk.org/) (only for webkit-renderer)
 * libvte >= 0.23.5  (only for tabbed-gtk-viewer)

## Build

To build all components, just use the autotools standard procedure:

 * ./autogen.sh
 * ./configure
 * make
 * make install (to install, not really a good idea yet)

To build a single component, execute ./autogen.sh and ./configure, then change to the corresponding directory and `make`.

## Usage

First, the config file (*config.default*) and the network protocol specification (*net-spec.yml*) need to be put somewhere the components can find them. The default location for the config file is at `$XDG_CONFIG_HOME/yonderboy/config`, the *net-spec.yml* location is set in the config file and defaults to `$XDG_CONFIG_HOME/yonderboy/net-spec.yml`.

To try the whole thing out, just start the daemon at `server/src/server` and then at least the viewer at `viewers/tabbed-gtk-viewer/src/tabbed-gtk-viewer`.
The viewer will open with one tab and associated renderer, as well as a terminal emulator containing the controller in the bottom area of the window.

## License

Copyright (C) 2009-2010 *Prof. MAAD* aka Max Wolter

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
