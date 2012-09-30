LDFLAGS = -lalpm
CFLAGS  = -g

PREFIX    ?= /usr/local
DESTDIR   ?=
MANPREFIX ?= ${PREFIX}/share/man

all: pacfind doc

doc: README.rst
	rst2man2 README.rst > pacfind.1

install: all doc
	install -D -m755 pacfind $(DESTDIR)${PREFIX}/bin/pacfind
	install -D -m644 pacfind.1 ${DESTDIR}${MANPREFIX}/man1/pacfind.1
