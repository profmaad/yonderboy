AM_CXXFLAGS = -I../../libev/ $(server_DEPS_CFLAGS)
server_LDADD = -lpthread $(server_DEPS_LIBS)

bin_PROGRAMS = server
server_SOURCES = main.cpp macros.h log.h defaults.h \
		ev_cpp.h ev_cpp.cpp \
		ipc_listener.h ipc_listener.cpp \
		abstract_host.h abstract_host.cpp \
		hosts_manager.h hosts_manager.cpp \
		package.h package.cpp \
		package_factories.h package_factories.cpp \
		controller_listener.h controller_listener.cpp \
		controller_host.h controller_host.cpp \
		viewer_listener.h viewer_listener.cpp \
		viewer_host.h viewer_host.cpp \
		renderer_host.h renderer_host.cpp \
		server_controller.h server_controller.cpp \
		abstract_decision_maker.h abstract_decision_maker.cpp \
		meta_decision_maker.h meta_decision_maker.cpp \
		abstract_persistence_manager.h abstract_persistence_manager.cpp abstract_persistence_manager_sync.cpp \
		persistent_storage.h \
		file_persistence_manager.h file_persistence_manager.cpp \
		configuration_manager.h configuration_manager.cpp \
		view.h view.cpp \
		display_manager.h display_manager.cpp \
		job.h job.cpp \
		job_manager.h job_manager.cpp \
		package_router.h package_router.cpp
