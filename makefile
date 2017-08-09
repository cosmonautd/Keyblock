keyblock:
	g++ keyblock.cpp -std=c++11 -lpthread -o keyblock.app

install:
	cp keyblock.app keyblock
	cp keyblock /usr/bin/
	rm keyblock
	chmod +x /usr/bin/keyblock

uninstall:
	rm /usr/bin/keyblock

clean:
	if [ -f keyblock.app ] ; then rm keyblock.app ; fi
	if ls *.o >/dev/null 2>&1 ; then rm *.o ; fi
