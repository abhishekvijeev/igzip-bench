# compiler
CC = gcc

# binary name
APP = igzip-bench

# all sources are stored in SRCS-y
SRCS-y := igzip-bench.c

PKGCONF ?= pkg-config
PC_FILE := $(shell $(PKGCONF) --path libisal 2>/dev/null)
CFLAGS += -O3 $(shell $(PKGCONF) --cflags libisal)
LDFLAGS = $(shell $(PKGCONF) --libs libisal)
LDFLAGS += -lz

# Build using pkg-config variables if possible
ifneq ($(shell $(PKGCONF) --exists libisal && echo 0),0)
$(error "no installation of Intel ISA-L found")
endif

all: build/$(APP)

build/$(APP): | build
	$(CC) $(CFLAGS) $(SRCS-y) -o $@ $(LDFLAGS)

build:
	@mkdir -p $@

.PHONY: clean
clean:
	rm -f build/$(APP)
	test -d build && rmdir -p build || true

