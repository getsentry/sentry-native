libsentrypad.dylib:
	g++ -dynamiclib \
		-o libsentrypad.dylib sentrypad.cpp crashpad_wrapper.cpp \
		-I ../crashpad-Darwin/include/ -I ../crashpad-Darwin/include/mini_chromium/ \
		-std=c++11 -L../crashpad-Darwin/lib -lclient -lbase -lutil \
		-framework Foundation -framework Security -framework CoreText \
		-framework CoreGraphics -framework IOKit -lbsm
example: example.c libsentrypad.dylib
	gcc -o example example.c -L . -lsentrypad
build-example: example
clean:
	rm -rf example libsentrypad.dylib completed new pending