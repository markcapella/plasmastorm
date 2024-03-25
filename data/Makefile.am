EXTRA_DIST = plasmastorm.appdata.xml.in plasmastorm.desktop.in
CLEANFILES = plasmastorm.appdata.xml plasmastorm.desktop

plasmastorm.appdata.xml: $(top_srcdir)/data/plasmastorm.appdata.xml.in
	sed "s/plasmastorm_VERSION/$(PACKAGE_VERSION)/;s/plasmastorm_DATE/$(DATE)/" $(top_srcdir)/data/plasmastorm.appdata.xml.in > $@

plasmastorm.desktop: $(top_srcdir)/data/plasmastorm.desktop.in
	sed "s/plasmastorm_VERSION/$(PACKAGE_VERSION)/;s/plasmastorm_DATE/$(DATE)/" $(top_srcdir)/data/plasmastorm.desktop.in > $@

appdatadir = $(datadir)/metainfo
appdata_DATA = plasmastorm.appdata.xml

desktopdir = $(datadir)/applications
desktop_DATA = plasmastorm.desktop
