SUBDIRS = ac libsylph src plugin po manual faq nsis macosx

EXTRA_DIST = config.rpath  \
	ChangeLog.ja \
	INSTALL.ja \
	README.es \
	README.ja \
	TODO.ja \
	ChangeLog-1.0 \
	ChangeLog-1.0.ja \
	NEWS-1.0 \
	NEWS-2.0 \
	LICENSE \
	PLUGIN.txt \
	PLUGIN.ja.txt \
	zylpheed.desktop \
	zylpheed.spec \
	zylpheed.spec.in \
	zylpheed.png \
	zylpheed-128x128.png \
	zylpheed-64x64.png \
	autogen.sh \
	makeosx.sh \
	makewin32.sh \
	mime.types \
	zylpheed-mailto-protocol_admin.reg \
	zylpheed-mailto-protocol_user.reg \
	oauth2.ini.sample

BZIP2_ENV =
XZ_ENV =

release: distdir
	-chmod -R a+r $(distdir)
	GZIP=$(GZIP_ENV) $(AMTAR) chozf $(distdir).tar.gz $(distdir)
	rm -f $(distdir)/zylpheed.spec
	sed '/^Source:/s/\.tar\.gz$$/\.tar\.bz2/' zylpheed.spec \
	  > $(distdir)/zylpheed.spec
	BZIP2=$(BZIP2_ENV) $(AMTAR) --bzip2 -chof $(distdir).tar.bz2 $(distdir)
	rm -f $(distdir)/zylpheed.spec
	sed '/^Source:/s/\.tar\.gz$$/\.tar\.xz/' zylpheed.spec \
	  > $(distdir)/zylpheed.spec
	XZ_OPT=$(XZ_ENV) $(AMTAR) --xz -chof $(distdir).tar.xz $(distdir)
	-rm -rf $(distdir)

#ACLOCAL_AMFLAGS = -I m4

install-data-local:
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(DESTDIR)$(datadir)/pixmaps; \
	$(INSTALL_DATA) $(srcdir)/zylpheed-64x64.png $(DESTDIR)$(datadir)/pixmaps/zylpheed.png;

desktop_DATA = zylpheed.desktop
desktopdir = $(datadir)/applications
