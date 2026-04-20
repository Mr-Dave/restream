ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS= -Dsysconfdir=\"$(sysconfdir)/restream\" -DLOCALEDIR=\"$(localedir)\"

LDADD = $(LIBINTL)

bin_PROGRAMS = restream

restream_SOURCES = \
	restream.hpp     restream.cpp \
	channel.hpp      channel.cpp \
	logger.hpp       logger.cpp \
	util.hpp         util.cpp \
	conf.hpp         conf.cpp \
	infile.hpp       infile.cpp \
	pktarray.hpp     pktarray.cpp \
	webu.hpp         webu.cpp \
	webu_ans.hpp     webu_ans.cpp \
	webu_meta.hpp    webu_meta.cpp \
	webu_mpegts.hpp  webu_mpegts.cpp

###################################################################
## Create pristine directories to match exactly distributed files
###################################################################
cleanall: distclean
	@rm -rf autom4te.cache m4
	@rm -f config.h.in config.h.in~ aclocal.m4 config.sub ABOUT-NLS missing
	@rm -f compile config.guess config.rpath configure depcomp install-sh
	@rm -f Makefile.in
