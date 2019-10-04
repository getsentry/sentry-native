PREMAKE_DIR := premake
PREMAKE := premake5
SOLUTION_NAME := Sentry-Native

ifeq ($(OS),Windows_NT)
  # Windows specific
else
  # Mac or Linux
  CPUS ?= $(shell getconf _NPROCESSORS_ONLN)
  INTERACTIVE := $(shell [ -t 0 ] && echo 1)
  UNAME_S := $(shell uname -s)
endif

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
	@echo "   clean-build"
	@echo ""
	@echo "LINUX ONLY:"
	@echo "   configure"
	@echo "   test"
.PHONY: help

# Dependency Download

fetch: fetch-breakpad fetch-crashpad
.PHONY: fetch

fetch-breakpad:
	breakpad/fetch_breakpad.sh
.PHONY: fetch-breakpad

fetch-crashpad:
	crashpad/fetch_crashpad.sh
.PHONY: fetch-crashpad

# Cleanup

clean: clean-breakpad clean-crashpad clean-build
.PHONY: clean

clean-breakpad:
	rm -rf breakpad/build
.PHONY: clean-breakpad

clean-crashpad:
	rm -rf crashpad/build
.PHONY: clean-crashpad

clean-build:
	git clean -xdf -e $(PREMAKE_DIR)/$(PREMAKE) -- $(PREMAKE_DIR)
.PHONY: clean-build

# Development on Linux / macOS.
# Does not work on Windows or Android

configure: $(PREMAKE_DIR)/Makefile
.PHONY: configure

test: configure
	$(MAKE) -C $(PREMAKE_DIR) -j$(CPUS) test_sentry
	$(PREMAKE_DIR)/bin/Release/test_sentry
.PHONY: test

$(PREMAKE_DIR)/Makefile: $(PREMAKE_DIR)/$(PREMAKE) $(wildcard $(PREMAKE_DIR)/*.lua)
	@cd $(PREMAKE_DIR) && ./$(PREMAKE) gmake2
	@touch $@

$(PREMAKE_DIR)/$(PREMAKE):
	@echo "Downloading premake"
	$(eval PREMAKE_DIST := $(if $(filter Darwin, $(UNAME_S)), macosx, linux))
	@curl -sL https://github.com/premake/premake-core/releases/download/v5.0.0-alpha14/premake-5.0.0-alpha14-$(PREMAKE_DIST).tar.gz | tar xz -C $(PREMAKE_DIR)

linux-build-env:
	@docker build ${DOCKER_BUILD_ARGS} -t getsentry/sentry-native .
.PHONY: linux-build-env

linux-run:
ifneq ("${SHOW_DOCKER_BUILD}","1")
	$(eval OUTPUT := >/dev/null)
endif
	@$(MAKE) linux-build-env ${OUTPUT}

ifeq ("${INTERACTIVE}","1")
	$(eval DOCKER_ARGS := -it)
endif
	$(eval CMD ?= bash)

# We need seccomp:unconfined so we can test crashing inside the container
	@docker run --rm -v ${PWD}:/work \
		--security-opt seccomp:unconfined \
		${DOCKER_ARGS} getsentry/sentry-native ${CMD}
.PHONY: linux-run

linux-shell:
	@$(MAKE) linux-run SHOW_DOCKER_BUILD=1
.PHONY: linux-shell

### Android ###

$(PREMAKE_DIR)/$(SOLUTION_NAME)_Application.mk: $(PREMAKE_DIR)/$(PREMAKE) $(wildcard $(PREMAKE_DIR)/*.lua)
	@cd $(PREMAKE_DIR) && ./$(PREMAKE) --os=android androidmk
	@touch $@

android-configure: $(PREMAKE_DIR)/$(SOLUTION_NAME)_Application.mk
.PHONY: android-configure

android-build: android-configure
	@cd $(PREMAKE_DIR) && ndk-build NDK_APPLICATION_MK=./$(SOLUTION_NAME)_Application.mk NDK_PROJECT_PATH=. PM5_CONFIG=release
.PHONY: android-build
