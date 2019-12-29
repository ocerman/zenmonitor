ifeq ($(PREFIX),)
	PREFIX := /usr/local
endif

build:
	cc -Isrc/include `pkg-config --cflags gtk+-3.0` src/*.c src/ss/*.c -o zenmonitor `pkg-config --libs gtk+-3.0` -lm -no-pie -Wall

install:
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	install -m 755 zenmonitor $(DESTDIR)$(PREFIX)/bin

	mkdir -p $(DESTDIR)$(PREFIX)/share/applications
	sed -e "s|@APP_EXEC@|${DESTDIR}${PREFIX}/bin/zenmonitor|" \
			data/zenmonitor.desktop.in > \
			$(DESTDIR)$(PREFIX)/share/applications/zenmonitor.desktop

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

clean:
	rm -f zenmonitor
