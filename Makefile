# paths
PREFIX = /usr/local
MANPREFIX = $(PREFIX)/share/man

ff2avs = -lm

install:
	mkdir -p "$(DESTDIR)$(MANPREFIX)/man1"
	cp -f *.1 "$(DESTDIR)$(MANPREFIX)/man1"
	for m in *.1; do chmod 644 "$(DESTDIR)$(MANPREFIX)/man1/$$m"; done
	mkdir -p "$(DESTDIR)$(MANPREFIX)/man7"
	cp -f *.7 "$(DESTDIR)$(MANPREFIX)/man7"
	for m in *.7; do chmod 644 "$(DESTDIR)$(MANPREFIX)/man7/$$m"; done
	@echo -e '\nThis Makefile only installs the man pages.'
	@echo -e '\nTo compile the programs run:\n   $$ for FILE in ff-*.c ff2*.c *2ff.c; do sh $$FILE; done'
	@echo -e "\nThis will install the programs under the user directory ~/bin/ by default."
	@echo -e "Some programs may depend on additional libraries to compile."
echo "ninstall:
	for m in *.1; do rm -f "$(DESTDIR)$(MANPREFIX)/man1/$$m"; done
	for m in *.7; do rm -f "$(DESTDIR)$(MANPREFIX)/man7/$$m"; done
