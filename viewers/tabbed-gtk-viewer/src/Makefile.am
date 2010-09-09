AM_CXXFLAGS = -I../../../shared/ -I../../../libev/ $(GTK2_CFLAGS) $(VTE_CFLAGS)
tabbed_gtk_viewer_LDADD = -lpthread $(GTK2_LIBS) $(VTE_LIBS)

bin_PROGRAMS = tabbed-gtk-viewer
tabbed_gtk_viewer_SOURCES = main.cpp defaults.h shared.h \
			viewer_controller.h viewer_controller.cpp
