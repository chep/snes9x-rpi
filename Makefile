
CXX = g++

INCLDIRS=-Iunix -Iunzip -I.
LIBDIRS=-L/usr/lib/arm-linux-gnueabihf

OPTIMISE= -D_ZAURUS -O2 -march=armv6zk -mcpu=arm1176jzf-s -mtune=arm1176jzf-s -mfpu=vfp -mfloat-abi=hard -ffast-math -fstrict-aliasing -fomit-frame-pointer 

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

LDFLAGS=-lboost_serialization -lSDL -lstdc++ -lz -lpthread -lSDL_ttf -lboost_thread -lasound

SNES9X_SRC = $(wildcard *.cpp)
SNES9X_SRC += $(wildcard unix/*.cpp)
SNES9X_SRC += $(wildcard unzip/*.cpp)
SNES9X_OBJ = $(SNES9X_SRC:.cpp=.o)

CONFJOY_SRC = $(wildcard confJoy/*.cpp)
CONFJOY_SRC += unix/joystick.cpp unix/keyboard.cpp
CONFJOY_OBJ = $(CONFJOY_SRC:.cpp=.o)

%.o: %.cpp %.d
	@echo [C++] $@
	@$(CXX) $(INCLDIRS) $(CXXFLAGS) -fPIC -c $< -o$@

%.d: %.cpp
	@$(CXX) -MM $(INCLDIRS) $^ > $@

LDLIBS =  

all: snes9x confJoyTool

snes9x: $(SNES9X_OBJ)
	@echo [LD] $@
	@$(CXX) $(LIBDIRS) $(LDFLAGS) -o $@ $^

confJoyTool: $(CONFJOY_OBJ)
	@echo [LD] $@
	@$(CXX) $(LIBDIRS) $(LDFLAGS) -o $@ $^

clean:
	find -name '*.o' -delete
	find -name '*.d' -delete
	rm -f snes9x confJoyTool

include $(SNES9X_OBJ:.o=.d)
include $(CONFJOY_OBJ:.o=.d)
