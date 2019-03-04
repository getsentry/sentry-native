libsentry.dylib:
	g++ -g -dynamiclib \
		-o libsentry.dylib sentry.cpp crashpad_wrapper.cpp url.cpp \
		-I ../crashpad-Darwin/include/ -I ../crashpad-Darwin/include/mini_chromium/ \
		-I ../mpack-amalgamation-1.0/ -I ../mpack-amalgamation-1.0/src/mpack \
		-std=c++11 -L../crashpad-Darwin/lib -lclient -lbase -lutil \
		-framework Foundation -framework Security -framework CoreText \
		-framework CoreGraphics -framework IOKit -lbsm \
		-D SENTRY_CRASHPAD
example: example.c libsentry.dylib
	gcc -g -o example example.c -L . -lsentry
build-example: example
clean:
	rm -rf example libsentry.dylib completed new pending *.dSYM *.mp