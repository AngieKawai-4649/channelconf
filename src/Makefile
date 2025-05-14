
CC	= gcc
CFLAGS	= -O2 -Wall -pthread -D_LARGEFILE_SOURCE
OBJCHTOOL	= chtool.o
OBJSEMTOOL	= semtool.o
OBJLIB		= channel_cnf.o

LIBS	+= -lpthread
TARGETCHTOOL	= chtool
TARGETSEMTOOL	= semtool
TARGETLIB		= libchannelcnf.so
CONF = channel.cnf
HEADER	= channel_cnf.h
PKGCONF	= $(TARGETLIB:.so=.pc)
PREFIX = /usr/local

ifeq ($(shell getconf LONG_BIT),32)
CFLAGS	+= -D_FILE_OFFSET_BITS=64
endif

all: $(TARGETLIB) $(TARGETCHTOOL) $(TARGETSEMTOOL)

clean:
	rm -f $(OBJCHTOOL) $(OBJSEMTOOL) $(OBJLIB) $(TARGETCHTOOL) $(TARGETSEMTOOL) $(TARGETLIB)

uninstall:
	rm -vf $(PREFIX)/bin/$(TARGETCHTOOL) $(PREFIX)/bin/$(TARGETSEMTOOL) $(PREFIX)/bin/$(CONF) $(PREFIX)/lib/$(TARGETLIB) $(PREFIX)/lib/pkgconfig/$(PKGCONF)
	rm -vfr $(PREFIX)/include/channelcnf
	ldconfig

install: uninstall
	install -m 755 $(TARGETCHTOOL) $(PREFIX)/bin
	install -m 755 $(TARGETSEMTOOL) $(PREFIX)/bin
	install -m 755 $(TARGETLIB) $(PREFIX)/lib
	ldconfig
	install -m 644 $(CONF) $(PREFIX)/bin
	if [ ! -d $(PREFIX)/lib/pkgconfig ]; then  mkdir $(PREFIX)/lib/pkgconfig; fi
	install -m 644 $(PKGCONF) $(PREFIX)/lib/pkgconfig
	install -m 644 -D $(HEADER) $(PREFIX)/include/channelcnf/$(HEADER)

$(TARGETCHTOOL): $(OBJCHTOOL)
	$(CC) -o $@ $^ -Wl,-rpath,'$$ORIGIN' $(TARGETLIB) $(LIBS)

$(TARGETSEMTOOL): $(OBJSEMTOOL)
	$(CC) -o $@ $^ $(LIBS)

$(TARGETLIB): $(OBJLIB)
	$(CC) -o $@ -fPIC -shared $^

$(OBJLIB): $(OBJLIB:.o=.c)
	$(CC) -fPIC $(CFLAGS) $(STD_B25) -o $@ -c $(@:.o=.c)

depend:
	$(CC) -MM $(OBJLIB:.o=.c) > Makefile.dep
	$(CC) -MM $(OBJCHTOOL:.o=.c) >> Makefile.dep
	$(CC) -MM $(OBJSEMTOOL:.o=.c) >> Makefile.dep

-include Makefile.dep

