shield:
	g++ shield.cpp -std=c++11 -lpthread -o shield.app

install:
	cp shield.app shield
	cp shield /usr/bin/
	rm shield
	chmod +x /usr/bin/shield

uninstall:
	rm /usr/bin/shield

clean:
	if [ -f shield.app ] ; then rm shield.app ; fi
	if ls *.o >/dev/null 2>&1 ; then rm *.o ; fi