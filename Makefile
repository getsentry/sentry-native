libsentry.dylib:
	g++ -g -dynamiclib \
		-o libsentry.dylib src/sentry.cpp src/crashpad_wrapper.cpp src/vendor/mpack.c \
		-I ../crashpad-Darwin/include/ -I ../crashpad-Darwin/include/mini_chromium/ \
		-I ./include \
		-fvisibility=hidden \
		-std=c++14 -L../crashpad-Darwin/lib -lclient -lbase -lutil \
		-framework Foundation -framework Security -framework CoreText \
		-framework CoreGraphics -framework IOKit -lbsm \
		-D SENTRY_CRASHPAD
example: example.c libsentry.dylib
	gcc -g -o example example.c -I ./include -L . -lsentry
build-example: example
clean:
	rm -rf example libsentry.dylib crashpad-db *.dSYM *.mp

breakpad-libsentry.dylib:
	g++ -g -dynamiclib \
		-o libsentry.dylib src/sentry.cpp src/breakpad_wrapper.cpp src/vendor/mpack.c \
		-I ../breakpad-Darwin/include/ \
		-I ./include \
		-fvisibility=hidden \
		-L ../breakpad-Darwin/lib \
		-lbreakpad_client -lpthread \
		-std=c++11 \
		-framework Foundation \
		-D SENTRY_BREAKPAD
