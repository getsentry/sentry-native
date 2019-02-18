build:
	g++ -o crashpad_wrapper crashpad_wrapper.cpp -I ../crashpad-Darwin/include/ -I ../crashpad-Darwin/include/mini_chromium/ -std=c++11 -L../crashpad-Darwin/lib -lclient -lbase -lutil -framework Foundation -framework Security -framework CoreText -framework CoreGraphics -framework IOKit -lbsm
clean:
	rm -rf crashpad_wrapper completed new pending