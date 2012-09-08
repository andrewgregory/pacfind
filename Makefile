LDFLAGS= -lalpm
CFLAGS= -g

all: pacfind

install: all
	install -D pacfind $(DESTDIR)/usr/bin/pacfind
