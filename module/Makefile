ifneq ($(KERNELRELEASE),)

obj-m	:= aravis-module.o

else

ifeq ($(PREFIX),)
PREFIX	:= /usr/local
endif

MODULE    := aravis-module
KDIR      := /lib/modules/$(shell uname -r)/build 
INCLUDE   := -isystem $(KDIR)/include
MODCFLAGS := -DMODULE -D__KERNEL__ -Wall $(INCLUDE)

KMAKE	  := $(MAKE) -C $(KDIR) SUBDIRS=$$PWD

modules: $(MODULE).o

insert_module: install
	modprobe -r aravis-module
	modprobe aravis-module

ifneq ($(shell (uname -r | grep 2.6) > /dev/null ; echo -n $$?),0)
	echo A 2.6 kernel is required; exit 1
endif

# build module

$(MODULE).o: $(MODULE).c $(MODULE).h
	$(KMAKE) modules


# Automake rules, as per "Third-Party Makefiles" in the automake manual

all: $(MODULE).o

distdir:
	cp aravis-module.c $(distdir)
	cp aravis-module.h $(distdir)
	cp Makefile $(distdir)

install:
	$(KMAKE) modules_install
	[ -e /sbin/depmod ] && /sbin/depmod

install-data:
install-exec:
uninstall:

install-info:
installdirs:
check:
installcheck:
mostlyclean:
clean:
	rm -f aravis-module.ko
	rm -f aravis-module.o
	rm -f aravis-module.mod.o
	rm -f aravis-module.mod.c
	rm -f modules.order
	rm -f Module.symvers
distclean: clean
maintainer-clean:
dvi:
pdf:
info:
html:
tags:
ctags:

endif

