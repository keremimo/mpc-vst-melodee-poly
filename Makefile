# Host targets (the device builds are vst/build.sh, through mpc-vst-plugins' tools/build_port.sh):
#   make test   the synth's checks under ASan and UBSan, and that vst/params.json and vst/presets.json are current
#   make json   write vst/params.json and vst/presets.json from the synth's tables (src/core/gen_json.h)
SYNTH := poly
CXX ?= c++
CXXFLAGS := -std=c++17 -O1 -g -Wall -Wextra -Isrc -fsanitize=address,undefined -fno-omit-frame-pointer
OUT := test/build

SOURCES := src/$(SYNTH).cpp
HEADERS := $(wildcard src/*.h src/core/*.h)

.PHONY: test json clean
test: $(OUT)/test_$(SYNTH) $(OUT)/gen_json
	$(OUT)/test_$(SYNTH)
	$(OUT)/gen_json --check vst/params.json vst/presets.json

json: $(OUT)/gen_json
	$(OUT)/gen_json vst/params.json vst/presets.json

$(OUT)/test_$(SYNTH): test/test_$(SYNTH).cpp $(SOURCES) $(HEADERS)
	@mkdir -p $(OUT)
	$(CXX) $(CXXFLAGS) test/test_$(SYNTH).cpp $(SOURCES) -o $@

$(OUT)/gen_json: tools/gen_json.cpp $(SOURCES) $(HEADERS)
	@mkdir -p $(OUT)
	$(CXX) $(CXXFLAGS) tools/gen_json.cpp $(SOURCES) -o $@

clean:
	rm -rf $(OUT)
