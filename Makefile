none:
	echo 'Nothing to do'; exit 1

fetch-breakpad:
	breakpad/fetch_breakpad.sh
	
fetch-crashpad:
	crashpad/fetch_crashpad.sh

fetch-all: fetch-breakpad fetch-crashpad

clean-breakpad:
	rm -rf breakpad/build

clean-crashpad:
	rm -rf crashpad/build

clean-all: clean-breakpad clean-crashpad
