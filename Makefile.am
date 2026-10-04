AUTOMAKE_OPTIONS = subdir-objects
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS} -I m4
AM_CPPFLAGS = $(libevent_core_CFLAGS) $(libevent_pthreads_CFLAGS)
if TLS
AM_CPPFLAGS += $(libevent_openssl_CFLAGS)
endif

EXTRA_DIST = README.md

lib_LTLIBRARIES = libcfluid_base.la
libcfluid_base_la_SOURCES = cfluid/base/evloop.h cfluid/base/evloop.c\
							cfluid/base/ofp_buffer.h\
						    cfluid/base/base_of_conn.h cfluid/base/base_of_conn.c\
						    cfluid/base/base_of_client.h cfluid/base/base_of_client.c\
						    cfluid/of_conn.h cfluid/of_conn.c \
						    cfluid/of_settings.h cfluid/of_settings.c\
						    cfluid/base/of.h\
						    cfluid/base/timed_callback.h cfluid/base/timed_callback.c \
						    cfluid/of_client.h cfluid/of_client.c
libcfluid_base_la_LIBADD = $(libevent_core_LIBS) $(libevent_pthreads_LIBS)
if TLS
libcfluid_base_la_LIBADD += $(libevent_openssl_LIBS)
endif

libcfluid_base_includedir = $(includedir)
nobase_libcfluid_base_include_HEADERS = cfluid/base/evloop.h\
									    cfluid/base/ofp_buffer.h\
									    cfluid/base/timed_callback.h\
									    cfluid/base/vector.h\
									    cfluid/base/uthash.h\
							   		    cfluid/base/base_of_conn.h\
							   		    cfluid/base/base_of_client.h\
							   		    cfluid/of_conn.h\
							   		    cfluid/of_settings.h\
							   		    cfluid/of_client.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libcfluid_base.pc
