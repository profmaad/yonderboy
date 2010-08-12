AM_CXXFLAGS = -I../../../shared/ -I../../../libev/ $(webkit_renderer_DEPS_CFLAGS)
webkit_renderer_LDADD = -lpthread $(webkit_renderer_DEPS_LIBS)

bin_PROGRAMS = webkit-renderer
webkit_renderer_SOURCES = main.cpp defaults.h shared.h \
			renderer_controller.h renderer_controller.cpp
