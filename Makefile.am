ACLOCAL_AMFLAGS = -I m4

pkgconfdir = $(libdir)/pkgconfig

pkgconf_DATA = libstaroffice-@STAROFFICE_MAJOR_VERSION@.@STAROFFICE_MINOR_VERSION@.pc

SUBDIRS = inc src build docs

EXTRA_DIST = \
	autogen.sh \
	libstaroffice.pc.in \
	libstaroffice-zip.in \
	COPYING.LGPL \
	COPYING.MPL \
	CREDITS \
	HACKING \
	INSTALL \
	NEWS \
	README

distclean-local:
	rm -rf *.cache *~ *.pc

zip: install
	sh libstaroffice-zip

dist-hook:
	git log --date=short --pretty="format:@%cd  %an  <%ae>  [%H]%n%n%s%n%n%e%b" | sed -e "s|^\([^@]\)|\t\1|" -e "s|^@||" >$(distdir)/ChangeLog

astyle:
	astyle -HU -A3 -k3 -y -n -s2 -r \*.cpp \*.h \*.?xx
