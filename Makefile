
# detect current os and set the correct path
ifeq ($(OS),Windows_NT)
	OS=Windows
else
	OS=$(shell uname -s)
endif

-include build/Makefile_source

ifeq ($(OS),Windows)
	CCFLAGS = -pedantic -std=c11 -g -O0 -DOS_WINDOWS
	PLATFORM=OS_WINDOWS
	include build_tool/win/Makefile
else ifeq ($(OS),Linux)
	CCFLAGS = -pedantic -std=c11 -g -O0 -DOS_LINUX
	PLATFORM=OS_LINUX
	include build_tool/linux/Makefile
else ifeq ($(OS),Darwin)
	CCFLAGS = -pedantic -std=c11 -g -O0 -DOS_OSX
	PLATFORM=OS_OSX
	include build_tool/osx/Makefile
else
	$(error OS not supported)
endif

CCFLAGS += -DSCRIPT_BACKEND_JS

collect:
	python build_tool/collect_source.py $(PLATFORM)

convert:
	python build_tool/convert_shader.py $(PLATFORM)