PORT_NAME := popclassic
ENV_PREFIX := POPCLASSIC
PORTBASE := portbase
PORT_SRCS := $(wildcard game/*.cpp)
CPPFLAGS += -Igame
include $(PORTBASE)/Makefile

FAAD_SRCS := $(wildcard third_party/faad2/libfaad/*.c)
FAAD_OBJS := $(patsubst third_party/faad2/libfaad/%.c,build/faad/%.o,$(FAAD_SRCS))
LDLIBS += build/libfaad.a
$(TARGET): build/libfaad.a
build/libfaad.a: $(FAAD_OBJS)
	$(CROSS)ar rcs $@ $^
build/faad/%.o: third_party/faad2/libfaad/%.c
	@mkdir -p $(dir $@)
	$(CROSS)gcc -O2 -fPIC -include third_party/faad2/port_config.h -Ithird_party/faad2/include -Ithird_party/faad2/libfaad -c $< -o $@

.PHONY: libs
libs: $(TARGET)
	bash tools/collect_libs.sh $(TARGET) build/libs.armhf
	bash tools/check_glibc_floor.sh $(TARGET) build/libs.armhf
