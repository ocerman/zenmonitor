CC := cc

ifeq ($(PREFIX),)
	PREFIX := /usr/local
endif

BUILD_FILES_COMMON := \
	src/ss/*.c \
	src/sysfs.c \
	src/zenmonitor-lib.c

BUILD_FILES_GUI := \
	$(BUILD_FILES_COMMON) \
	src/gui.c \
	src/zenmonitor.c

BUILD_FILES_CLI := \
	$(BUILD_FILES_COMMON) \
	src/zenmonitor-cli.c

build:
	$(CC) -Isrc/include `pkg-config --cflags gtk+-3.0` $(BUILD_FILES_GUI) -o zenmonitor `pkg-config --libs gtk+-3.0` -lm -no-pie -Wall $(CFLAGS)

build-cli:
	$(CC) -Isrc/include `pkg-config --cflags glib-2.0` $(BUILD_FILES_CLI) -o zenmonitor-cli `pkg-config --libs glib-2.0` -lm -no-pie -Wall $(CFLAGS)

install:
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -m 755 zenmonitor $(DESTDIR)$(PREFIX)/bin

	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	sed -e "s|@APP_EXEC@|${DESTDIR}${PREFIX}/bin/zenmonitor|" \
			data/zenmonitor.desktop.in > \
			$(DESTDIR)$(PREFIX)/share/applications/zenmonitor.desktop

install-cli:
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -m 755 zenmonitor-cli $(DESTDIR)$(PREFIX)/bin

install-polkit:
	sed -e "s|@APP_EXEC@|${DESTDIR}${PREFIX}/bin/zenmonitor|" \
			data/zenmonitor-root.desktop.in > \
			$(DESTDIR)$(PREFIX)/share/applications/zenmonitor-root.desktop

	sed -e "s|@APP_EXEC@|${DESTDIR}${PREFIX}/bin/zenmonitor|" \
			data/org.pkexec.zenmonitor.policy.in > \
			$(DESTDIR)/usr/share/polkit-1/actions/org.pkexec.zenmonitor.policy

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/zenmonitor
	rm -f $(DESTDIR)$(PREFIX)/share/applications/zenmonitor.desktop
	rm -f $(DESTDIR)$(PREFIX)/share/applications/zenmonitor-root.desktop
	rm -f $(DESTDIR)/usr/share/polkit-1/actions/org.pkexec.zenmonitor.policy

all: build build-cli

clean:
	rm -f zenmonitor
	rm -f zenmonitor-cli
	rm -f *.o
