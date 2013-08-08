bin_PROGRAMS=wheeliemote
wheeliemote_SOURCES = src/main.c
wheeliemote_CPPFLAGS = $(AM_CPPFLAGS) $(UDEV_CFLAGS) $(DBUS_CFLAGS) $(BLUEZ_CFLAGS) $(LIBXWIIMOTE_CFLAGS)
wheeliemote_LDADD = -lxwiimote -lm -lbluetooth
wheeliemote_LDFLAGS = $(AM_LDFLAGS) $(UDEV_LIBS) $(DBUS_LIBS) $(BLUEZ_LIBS) $(LIBXWIIMOTE_LIBS)

ACLOCAL_AMFLAGS = -I m4

AM_CFLAGS = \
	-Wall \
	-pipe \
	-fno-common \
	-ffast-math \
	-fdiagnostics-show-option \
	-fno-strict-aliasing \
	-fvisibility=hidden \
	-ffunction-sections \
	-fdata-sections \
	-fstack-protector
AM_CPPFLAGS = \
	-include $(top_builddir)/config.h \
	-I $(srcdir)/lib \
	-D'XWII__EXPORT=__attribute__((visibility("default")))'
AM_LDFLAGS = \
	-Wall \
	-Wl,--as-needed \
	-Wl,--gc-sections \
	-Wl,-z,relro \
	-Wl,-z,now


