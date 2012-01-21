# Makefile for mod_proxyerrorcapture.c (gmake)
# $Id:$

APXS2=/usr/bin/apxs2

default:
	@echo mod_proxyerrorcapture:
	@echo author: john.carr@isotoma.com
	@echo
	@echo following options available:
	@echo \"make proxyerrorcapture\" to compile
	@echo \"make install\" to install
	@echo
	@echo change path to apxs if this is not it: \"$(APXS2)\"


proxyerrorcapture: mod_proxyerrorcapture.o
	@echo make done, type \"make install\" to install mod_proxyerrorcapture

mod_proxyerrorcapture.o: mod_proxyerrorcapture.c
	$(APXS2) -c -n $@ mod_proxyerrorcapture.c

mod_proxyerrorcapture.c:

install: mod_proxyerrorcapture.o
	$(APXS2) -i -n mod_proxyerrorcapture.so mod_proxyerrorcapture.la

clean:
	rm -rf *~ *.o *.so *.lo *.la *.slo .libs/
