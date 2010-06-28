##
 #  ummd ( Micro MultiMedia Daemon )
 ##

nodist_bin_SCRIPTS = \
	ummdc

nodist_sysconf_DATA = \
	ummd.conf 

CLEANFILES = \
	ummd.conf \
	ummdc

EXTRA_DIST = \
	ummd.conf.tmpl
	ummdc.tmpl

subst = sed \
	-e 's,$$PACKAGE,$(PACKAGE),g' \
	-e 's,$$VERSION,$(PACKAGE_VERSION),g' \
	-e 's,$$MY_CFG_DIR,$(MY_CFG_DIR),g' \
	-e 's,$$MY_LOG_DIR,$(MY_LOG_DIR),g' \
	-e 's,$$MY_RUN_DIR,$(MY_RUN_DIR),g'

ummd.conf: ummd.conf.tmpl Makefile
	$(subst) $< > $@

ummdc: ummdc.tmpl Makefile
	$(subst) $< > $@
