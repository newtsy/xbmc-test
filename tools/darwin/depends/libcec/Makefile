include ../Makefile.include
include ../config.site.mk

# lib name, version
LIBNAME=libcec
<<<<<<< HEAD
VERSION=1.6.2
=======
VERSION=1.5.2
>>>>>>> 1495cbeb771bb5dde20a83a50d23c89a50e6f5c1
SOURCE=$(LIBNAME)-$(VERSION)
ARCHIVE=$(SOURCE).tar.bz2

# configuration settings
CONFIGURE=./configure --prefix=$(PREFIX)

<<<<<<< HEAD
LIBDYLIB=$(LIBNAME)/.libs/$(LIBNAME).dylib
=======
LIBDYLIB=$(SOURCE)/lib/.libs/$(LIBNAME).dylib
>>>>>>> 1495cbeb771bb5dde20a83a50d23c89a50e6f5c1

all: $(LIBDYLIB) .installed

$(TARBALLS_LOCATION)/$(ARCHIVE):
	$(RETRIEVE_TOOL) $(RETRIEVE_TOOL_FLAGS) $(BASE_URL)/$(ARCHIVE)

$(LIBNAME): $(TARBALLS_LOCATION)/$(ARCHIVE)
<<<<<<< HEAD
	rm -rf $(LIBNAME)
=======
	rm -rf $(SOURCE)
>>>>>>> 1495cbeb771bb5dde20a83a50d23c89a50e6f5c1
	$(ARCHIVE_TOOL) $(ARCHIVE_TOOL_FLAGS) $(TARBALLS_LOCATION)/$(ARCHIVE)
	echo $(LIBNAME) > .gitignore
	cd $(LIBNAME); autoreconf -vif
	cd $(LIBNAME); $(CONFIGURE)

$(LIBDYLIB): $(LIBNAME)
<<<<<<< HEAD
	make -j 1 -C $(LIBNAME)
=======
	make -j 1 -C $(SOURCE)
>>>>>>> 1495cbeb771bb5dde20a83a50d23c89a50e6f5c1

.installed:
	make -C $(LIBNAME) install
	touch $@

clean:
	rm -rf $(LIBNAME) .installed

distclean::
	rm -rf $(LIBNAME) .installed

