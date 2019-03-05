libsentry.dylib:
	g++ -g -dynamiclib \
		-o libsentry.dylib src/sentry.cpp src/crashpad_wrapper.cpp src/vendor/mpack.c \
		-I ../crashpad-Darwin/include/ -I ../crashpad-Darwin/include/mini_chromium/ \
		-I ../mpack-amalgamation-1.0/ -I ../mpack-amalgamation-1.0/src/mpack \
		-I ./include \
		-fvisibility=hidden \
		-std=c++11 -L../crashpad-Darwin/lib -lclient -lbase -lutil \
		-framework Foundation -framework Security -framework CoreText \
		-framework CoreGraphics -framework IOKit -lbsm \
		-D SENTRY_CRASHPAD
example: example.c libsentry.dylib
	gcc -g -o example example.c -I ./include -L . -lsentry
build-example: example
clean:
	rm -rf example libsentry.dylib completed new pending *.dSYM *.mp *.dmp *.dat