BUILD_DIR   := build
ARTIFACTS   := $(BUILD_DIR)/OrcaPlugin_artefacts
# CMake may place artifacts directly or under Release/
AU_SRC      := $(shell find $(ARTIFACTS) -name "Orca.component" -type d 2>/dev/null | head -1)
VST3_SRC    := $(shell find $(ARTIFACTS) -name "Orca.vst3" -type d 2>/dev/null | head -1)
AU_DST      := $(HOME)/Library/Audio/Plug-Ins/Components/Orca.component
VST3_DST    := $(HOME)/Library/Audio/Plug-Ins/VST3/Orca.vst3

.PHONY: all build install install-au install-vst3 validate clean configure

all: build install

configure:
	cmake -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release

build:
	@test -d $(BUILD_DIR) || (echo "Run 'make configure' first" && exit 1)
	cmake --build $(BUILD_DIR) --config Release

install: install-au install-vst3

install-au: build
	rm -rf "$(AU_DST)"
	cp -R "$(AU_SRC)" "$(AU_DST)"
	@echo "AU installed → $(AU_DST)"

install-vst3: build
	rm -rf "$(VST3_DST)"
	cp -R "$(VST3_SRC)" "$(VST3_DST)"
	codesign --force --deep --sign - "$(VST3_DST)"
	@echo "VST3 installed → $(VST3_DST)"

validate: install-au
	auval -v aumu Orka Orca

clean:
	rm -rf $(BUILD_DIR)
