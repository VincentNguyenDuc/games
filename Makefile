GAME         ?= game1
BUILD_PRESET ?= debug

FMT_FILES := $(wildcard $(GAME)/src/*.c) $(wildcard $(GAME)/src/*.h) \
             $(wildcard $(GAME)/src/*.cpp) $(wildcard $(GAME)/src/*.hpp) \
             $(wildcard $(GAME)/include/*.hpp) $(wildcard $(GAME)/tests/*.cpp)

.PHONY: help init clean install-hooks format build build-all run test

help:
	@echo "-----------------------------------------------------------------------"
	@echo "Usage: make [target] [GAME=game1] [BUILD_PRESET=debug|release|profile]"
	@echo "Targets:"
	@echo "  init            | Install git hooks"
	@echo "  build           | Configure and build GAME"
	@echo "  build-all       | Configure and build all games"
	@echo "  run             | Build and run GAME"
	@echo "  test            | Build and run tests for GAME"
	@echo "  clean           | Remove build artifacts"
	@echo "  format          | Format C/C++ files for GAME"
	@echo "-----------------------------------------------------------------------"

init:
	$(MAKE) install-hooks

install-hooks:
	git config core.hooksPath .githooks
	@echo "Git hooks installed (.githooks/pre-commit)"

build:
	cmake --preset $(BUILD_PRESET)
	cmake --build --preset $(BUILD_PRESET) --target $(GAME)

build-all:
	cmake --preset $(BUILD_PRESET)
	cmake --build --preset $(BUILD_PRESET)

run: build
	./build/$(BUILD_PRESET)/$(GAME)

test: build
	ctest --test-dir build/$(BUILD_PRESET) -R "$(GAME)" --output-on-failure

clean:
	rm -rf build

format:
	clang-format -i $(FMT_FILES)
