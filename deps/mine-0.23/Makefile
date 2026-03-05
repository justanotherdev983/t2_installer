
# CDB Sub-Package
#
CDB_VER  = 0.75
CDB_DIR  = cdb-$(CDB_VER)
CDB_OBJ  = $(CDB_DIR)/cdb.a  $(CDB_DIR)/alloc.a $(CDB_DIR)/buffer.a
CDB_OBJ += $(CDB_DIR)/byte.a $(CDB_DIR)/unix.a

# LibBzip2 Sub-Package
#
BZIP2_VER = 1.0.2
BZIP2_DIR = bzip2-$(BZIP2_VER)
BZIP2_OBJ = $(BZIP2_DIR)/libbz2.a

# LibTar Sub-Package
#
LIBTAR_VER = 1.2.11
LIBTAR_DIR = libtar-$(LIBTAR_VER)
LIBTAR_OBJ = $(LIBTAR_DIR)/lib/libtar.a

# Mine Itself
#
MINE_VER  = 0.23
MINE_OBJ  = check.o create.o install.o remove.o pkglist.o mkpdb.o
MINE_OBJ += showfile.o showkey.o memdb.o md5.o md5sum.o mine.o

# GAS
#
GAS_VER = $(MINE_VER)
GAS_OBJ = gasgui.o readdb.o selector.o memdb.o

# Let's use AVL for the memory DB
#
USE_AVL = 1

# Use hashes to optimize key comparisons
#
USE_HASHOPT = 1

# Let's build the gasgui tool
#
USE_GAS = 1

# The usual list of build targets
#
MINE_ALL_OBJS = $(MINE_OBJ) $(CDB_OBJ) $(BZIP2_OBJ) $(LIBTAR_OBJ)

# Set and configure the c-compiler
#
CFLAGS  = -I$(CDB_DIR) -I$(BZIP2_DIR) -I$(LIBTAR_DIR)/lib -I. -Wall -Os
CFLAGS += -I$(LIBTAR_DIR)/listhash -DMINE_VERSION=\"$(MINE_VER)\" -ggdb
CFLAGS += -DGEMCACHE=\"/var/cache/gem\" -DMINECURLOPT=\"/etc/mine.curlopt\"
CFLAGS += -std=gnu89 -w -I/usr/include/dialog -DMAX_LEN=2048
ifeq ($(USE_AVL), 1)
  CFLAGS   += -DUSE_AVL=1
  MINE_OBJ += avl.o
  GAS_OBJ  += avl.o
endif
ifeq ($(USE_HASHOPT), 1)
  CFLAGS   += -DUSE_HASHOPT=1
endif
CC = gcc

# Set binutils tools
#
AR = ar
RANLIB = ranlib

# What do we want to build
#
targets = mine mine.static
ifeq ($(USE_GAS), 1)
  targets += gasgui
endif

# Set This options when cross-compiling
#
CONFOPT =
BUILDCC = $(CC)

prefix = /usr
sysprefix = /

all: $(targets)

mine: $(MINE_ALL_OBJS)
	$(CC) $(MINE_ALL_OBJS) -o mine

mine.static:
	$(CC) $(MINE_ALL_OBJS) -static -o mine.static

gasgui: $(GAS_OBJ)
	$(CC) $(GAS_OBJ) -ldialog -lcurses -lm -o gasgui

install: all
	install mine $(sysprefix)/bin/mine
	install tarbz2gem.sh $(prefix)/sbin/tarbz2gem
	install rocket.sh $(prefix)/sbin/rocket
ifeq ($(USE_GAS), 1)
	install gasgui $(prefix)/sbin/gasgui
endif
	[ -f $(sysprefix)/etc/rocket.conf ] || cp rocket.conf $(sysprefix)/etc/

$(MINE_OBJ): $(CDB_OBJ) $(BZIP2_OBJ) $(LIBTAR_OBJ)

$(CDB_OBJ):
	$(BUILDCC) -std=gnu89 -w -o $(CDB_DIR)/auto-str $(CDB_DIR)/auto-str.c
	echo "gcc -w -Wno-incompatible-pointer-types -Wno-int-conversion -Wno-implicit-int -Wno-implicit-function-declaration" > $(CDB_DIR)/conf-cc
	$(MAKE) 'AR=$(AR)' 'RANLIB=$(RANLIB)' -C $(CDB_DIR)

$(BZIP2_OBJ):
	$(MAKE) -C $(BZIP2_DIR) libbz2.a

$(LIBTAR_OBJ):
	cd $(LIBTAR_DIR) && ./configure --without-zlib $(CONFOPT)
	$(MAKE) -C $(LIBTAR_DIR)

patchfiles:
	sh xdiff.sh $(CDB_DIR)/ > $(CDB_DIR).patch
#	sh xdiff.sh (BZIP2_DIR)/ > $(BZIP2_DIR).patch
	sh xdiff.sh $(LIBTAR_DIR)/ > $(LIBTAR_DIR).patch

clean:
	@for x in $(CDB_DIR)/* ; do \
		[ $$x != $${x%.orig} ] && continue ; \
		grep -qx "$${x#$(CDB_DIR)/}" $(CDB_DIR)/FILES || rm -v $$x ; \
	done
	@rm -vf *.o mine mine.static gasgui core
	-@make -C $(BZIP2_DIR) distclean
	-@make -C $(LIBTAR_DIR) distclean
	echo "GEM MINE $(MINE_VER)" > VERSION

