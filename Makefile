
CXX = g++

INCLDIRS=-Iunix -Iunix/input -Iunzip -I.
LIBDIRS=-L/usr/lib/arm-linux-gnueabihf

OPTIMISE= -D_ZAURUS -O3 -march=armv6zk -mcpu=arm1176jzf-s -mtune=arm1176jzf-s -mfpu=vfp -mfloat-abi=hard -ffast-math -fstrict-aliasing -fomit-frame-pointer

UNZIPDEFINES=-DUNZIP_SUPPORT
SOUNDDEFINES=-DSPC700_C

CXXFLAGS = $(OPTIMISE) \
-D__linux \
-DZLIB \
-DVAR_CYCLES \
-DCPU_SHUTDOWN \
-DSPC700_SHUTDOWN \
$(SOUNDDEFINES) \
$(UNZIPDEFINES) \
-DNO_INLINE_SET_GET

CFLAGS=$(CXXFLAGS)

LDFLAGS=-lboost_serialization -lSDL2 -lstdc++ -lz -lboost_thread -lasound -lboost_system -lboost_chrono
LDFLAGS_conftool = -lSDL2 -lSDL2_ttf -lboost_serialization -L/usr/local/lib

SNES9X_SRC = $(wildcard *.cpp)
SNES9X_SRC += $(wildcard unix/*.cpp)
SNES9X_SRC += $(wildcard unix/input/*.cpp)
SNES9X_SRC += $(wildcard unzip/*.cpp)
SNES9X_OBJ = $(SNES9X_SRC:.cpp=.o)

CONFTOOL_SRC = $(wildcard confTool/*.cpp)
CONFTOOL_SRC += unix/input/joystick.cpp unix/input/inputConfig.cpp
CONFTOOL_OBJ = $(CONFTOOL_SRC:.cpp=.o)

%.o: %.cpp %.d
	@echo [C++] $@
	@$(CXX) $(INCLDIRS) $(CXXFLAGS) -fPIC -c $< -o$@

%.d: %.cpp
	@$(CXX) -MM $(INCLDIRS) $^ > $@

LDLIBS =  

all: snes9x confTool/confTool

snes9x: $(SNES9X_OBJ)
	@echo [LD] $@
	@$(CXX) -o $@ $^ $(LIBDIRS) $(LDFLAGS)

confTool/confTool: $(CONFTOOL_OBJ)
	@echo [LD] $@
	@$(CXX) -o $@ $^ $(LIBDIRS) $(LDFLAGS_conftool)

clean:
	find -name '*.o' -delete
	find -name '*.d' -delete
	rm -f snes9x confTool/confTool

install:
	install -d $(DESTDIR)/usr/bin
	install confTool/confTool $(DESTDIR)/usr/bin
	install snes9x $(DESTDIR)/usr/bin

include $(SNES9X_OBJ:.o=.d)
include $(CONFTOOL_OBJ:.o=.d)
