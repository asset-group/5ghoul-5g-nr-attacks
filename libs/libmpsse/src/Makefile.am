SUBDIRS = . examples

AM_CFLAGS = -Wall -Wextra -fPIC -fno-strict-aliasing -g -O2 $(CFLAGS)
AM_CPPFLAGS = $(PYTHON_INCLUDE) $(CPPFLAGS)

ACLOCAL_AMFLAGS = -I m4

if BUILD_PYTHON_WRAPPERS
PYTHON_MPSSE_LA = libMpssePython.la
PYLIBMPSSE = pylibmpsse.py
CREATE_LINK = $(LN_S) ${prefix}/lib/libMpssePython.so.0.0.0 ${pythondir}/_pylibmpsse.so
else !BUILD_PYTHON_WRAPPERS
PYTHON_MPSSE_LA =
PYLIBMPSSE =
CREATE_LINK = true
endif !BUILD_PYTHON_WRAPPERS

lib_LTLIBRARIES = libmpsse.la $(PYTHON_MPSSE_LA)
libmpsse_la_SOURCES = mpsse.c support.c fast.c

EXTRA_DIST = mpsse.i
python_PYTHON = mpsse.py $(PYLIBMPSSE)

libMpssePython_la_SOURCES = mpsse_wrap_python.c pyswig_mpsse.c pyswig_support.c
BUILT_SOURCES = pyswig_support.c pyswig_mpsse.c
CLEANFILES = pyswig_mpsse.c pyswig_support.c mpsse_wrap_python.c
libMpssePython_la_CPPFLAGS = -DSWIGPYTHON $(AM_CPPFLAGS)

mpsse_wrap_python.c: mpsse.i
	$(SWIG) -python -o $@ $<

pyswig_mpsse.c: mpsse.c
	cp -f $< $@

pyswig_support.c: support.c
	cp -f $< $@

install-exec-hook:
	-rm ${pythondir}/_pylibmpsse.so
	$(CREATE_LINK)

uninstall-hook:
	-rm ${pythondir}/_pylibmpsse.so
