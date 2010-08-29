AM_CXXFLAGS = -I../../../shared/ -I../../../libev/ $(GTK2_CFLAGS) $(WEBKIT_CFLAGS)
webkit_renderer_LDADD = -lpthread $(GTK2_LIBS) $(WEBKIT_LIBS)

bin_PROGRAMS = webkit-renderer
webkit_renderer_SOURCES = main.cpp defaults.h shared.h \
			renderer_controller.h renderer_controller.cpp
