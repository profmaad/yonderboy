AM_CXXFLAGS = -I../../../shared/ -I../../../libev/ $(YAML_CFLAGS)
cli_controller_LDADD = -lreadline -lpopt $(YAML_LIBS)

bin_PROGRAMS = cli-controller
cli_controller_SOURCES = main.cpp defaults.h shared.h \
			controller.h controller.cpp
