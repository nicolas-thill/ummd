##
 #  ummd ( Micro MultiMedia Daemon )
 ##

nodist_sysconf_DATA = \
	ummd.conf 

CLEANFILES = \
	ummd.conf 

EXTRA_DIST = \
	ummd.conf.tmpl

subst = sed \
	-e 's,$$PACKAGE,@PACKAGE@,g' \
	-e 's,$$PACKAGE_VERSION,@PACKAGE_VERSION@,g' \
	-e 's,$$MY_CFG_DIR,@MY_CFG_DIR@,g' \
	-e 's,$$MY_LOG_DIR,@MY_LOG_DIR@,g' \
	-e 's,$$MY_RUN_DIR,@MY_RUN_DIR@,g'

ummd.conf: ummd.conf.tmpl Makefile
	$(subst) $< > $@
