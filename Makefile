help:
	@echo "Usage: make [target]"
	@echo ""
	@echo "TARGETS:"
	@echo "   fetch"
	@echo "   fetch-breakpad"
	@echo "   fetch-crashpad"
	@echo "   clean"
	@echo "   clean-breakpad"
	@echo "   clean-crashpad"
	@echo ""
.PHONY: help

fetch: fetch-breakpad fetch-crashpad
.PHONY: fetch

fetch-breakpad:
	breakpad/fetch_breakpad.sh
.PHONY: fetch-breakpad

fetch-crashpad:
	crashpad/fetch_crashpad.sh
.PHONY: fetch-crashpad

clean-breakpad:
	rm -rf breakpad/build
.PHONY: clean-breakpad

clean-crashpad:
	rm -rf crashpad/build
.PHONY: clean-crashpad

clean-sentry:
	[ -f "premake/Makefile" ] && $(MAKE) -C premake clean || true
	rm -rf ./premake/{bin,obj}
.PHONY: clean-sentry

clean: clean-breakpad clean-crashpad clean-sentry
.PHONY: clean

configure:
	cd premake && premake5 gmake2

test:
	cd premake && make sentry_tests && ./bin/Release/sentry_tests
.PHONY: test
