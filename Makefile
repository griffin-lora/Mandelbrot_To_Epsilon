TARGET := app

GLSLC := glslc

SOURCES := $(wildcard src/*.c) $(wildcard src/gfx/*.c) $(wildcard src/voxel/*.c) $(wildcard src/link/*.c) $(wildcard src/link/*.cpp) $(wildcard lib/*.c)
SHADER_SOURCES := $(wildcard shader/*.vert) $(wildcard shader/*.frag) $(wildcard shader/*.comp) $(wildcard shader/*.mesh) $(wildcard shader/*.task)
LIBS := -lvulkan -lglfw -lm -lpthread
OBJECTS := $(patsubst %.c,%.o,$(patsubst %.cpp,%.o,$(SOURCES)))
DEPENDS := $(patsubst %.c,%.d,$(patsubst %.cpp,%.d,$(SOURCES)))
SHADER_OBJECTS := $(patsubst %.vert,%.spv,$(patsubst %.frag,%.spv,$(patsubst %.comp,%.spv,$(patsubst %.mesh,%.spv,$(patsubst %.task,%.spv,$(SHADER_SOURCES))))))

CFLAGS = -O2 -std=c2x -Wall -Wextra -Wpedantic -Wconversion -Wno-override-init -Wno-pointer-arith -Werror -Wfatal-errors -g -Isrc -Ilib -DGLFW_INCLUDE_VULKAN -DCGLM_FORCE_DEPTH_ZERO_TO_ONE
CXXFLAGS = -O2 -Isrc -Ilib

.PHONY: build run clean

build: $(OBJECTS) $(SHADER_OBJECTS)
	$(CXX) $(CFLAGS) -o $(TARGET).elf $(OBJECTS) $(LIBS)

run: build
	@./$(TARGET).elf

clean:
	$(RM) $(OBJECTS) $(DEPENDS) $(SHADER_OBJECTS)

-include $(DEPENDS)

%.o: %.c Makefile
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

%.o: %.cpp Makefile
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

%.spv: %.vert Makefile
	$(GLSLC) $< -o $@

%.spv: %.frag Makefile
	$(GLSLC) $< -o $@

%.spv: %.comp Makefile
	$(GLSLC) $< -o $@

%.spv: %.mesh Makefile
	$(GLSLC) --target-env=vulkan1.2 $< -o $@

%.spv: %.task Makefile
	$(GLSLC) --target-env=vulkan1.2 $< -o $@