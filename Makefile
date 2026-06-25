TARGET       ?= template
BUILD_PRESET ?= debug

FMT_FILES := $(shell find $(TARGET)/src $(TARGET)/include $(TARGET)/tests \
             \( -name "*.c" -o -name "*.h" -o -name "*.cpp" -o -name "*.hpp" \) 2>/dev/null)

.PHONY: help init clean install-hooks format format-all build build-all run test docs

help:
	@echo "-----------------------------------------------------------------------"
	@echo "Usage: make [target] [TARGET=TARGET1] [BUILD_PRESET=debug|release|profile]"
	@echo "Targets:"
	@echo "  init            | Install git hooks"
	@echo "  build           | Configure and build TARGET"
	@echo "  build-all       | Configure and build all TARGETs"
	@echo "  run             | Build and run TARGET"
	@echo "  test            | Build and run tests for TARGET"
	@echo "  clean           | Remove build artifacts"
	@echo "  format          | Format C/C++ files for TARGET"
	@echo "  format-all      | Format all C/C++ files in the repository"
	@echo "  docs            | Serve mdbook"
	@echo "-----------------------------------------------------------------------"

init:
	$(MAKE) install-hooks

install-hooks:
	git config core.hooksPath .githooks
	@echo "Git hooks installed (.githooks/pre-commit)"

build:
	cmake --preset $(BUILD_PRESET)
	cmake --build --preset $(BUILD_PRESET) --target $(TARGET)

build-all:
	cmake --preset $(BUILD_PRESET)
	cmake --build --preset $(BUILD_PRESET)

run: build
	@./build/$(BUILD_PRESET)/$(TARGET)/$(TARGET)

test:
	cmake --preset $(BUILD_PRESET)
	cmake --build --preset $(BUILD_PRESET) --target $(TARGET)-tests
	./build/$(BUILD_PRESET)/$(TARGET)/tests/$(TARGET)-tests

clean:
	rm -rf build

format:
	clang-format -i $(FMT_FILES)

format-all:
	clang-format -i \
	$(shell find . -not -path "./build/*" \
	\( -name "*.c" -o -name "*.h" -o -name "*.cpp" -o -name "*.hpp" \) 2>/dev/null)

docs:
	mdbook serve docs
