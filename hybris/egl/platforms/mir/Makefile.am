pkglib_LTLIBRARIES = eglplatform_mir.la

eglplatform_mir_la_SOURCES = eglplatform_mir.cpp
eglplatform_mir_la_CXXFLAGS = \
	-I$(top_srcdir)/egl \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/common \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/egl \
	-I$(top_srcdir)/egl/platforms/common \
	$(ANDROID_HEADERS_CFLAGS) \
	$(MIR_CLIENT_CFLAGS)
	
eglplatform_mir_la_LDFLAGS = \
	-avoid-version -module -shared -export-dynamic \
	$(top_builddir)/egl/platforms/common/libhybris-eglplatformcommon.la \
	$(top_builddir)/hardware/libhardware.la
