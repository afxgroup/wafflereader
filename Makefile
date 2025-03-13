EXE 	 := Waffle

CC 		 := ppc-amigaos-g++
WARNINGS := -Wno-prio-ctor-dtor -Wno-unused-parameter -Wno-unused-variable -Wno-unused-value -Wno-parentheses -Wno-enum-compare
CFLAGS 	 := -mcrt=clib4 -D__USE_INLINE__ -O3 -mstrict-align -std=c++17 -I. -Iinclude -gstabs -MMD $(WARNINGS)
LDFLAGS  := -mcrt=clib4
LIBS 	 := -lftdi -lcapsimage -athread=native
STRIP	 := ppc-amigaos-strip

SOURCES := ADFWriter.cpp ArduinoInterface.cpp common.cpp ftdi_impl.cpp ibm_sectors.cpp pll.cpp RotationExtractor.cpp SerialIO.cpp
ifeq ($(GUI),3D)
	SOURCES += utils.cpp gui_common.cpp GUI.cpp
	CFLAGS += -DGUI
	GFXLIBS := -lraylib -lglfw3 -lGL -lpthread -lauto
else ifeq ($(GUI),REACTION)
	SOURCES += utils.cpp gui_common_reaction.cpp RAGUI.cpp
	CFLAGS += -DRAGUI
	EXE := Waffle_Reaction
	GFXLIBS := -lpthread -lauto
else
	SOURCES += Main.cpp
	EXE := Waffle_NoGui
	GFXLIBS :=
endif
OBJ		 =$(SOURCES:%.cpp=%.o)
DEP		 =$(OBJ:%.o=%.d)

all: $(EXE)

$(EXE): $(OBJ)
	$(CC) $(LDFLAGS) -o $@ $^ $(GFXLIBS) $(LIBS)
	$(STRIP) $@

clean:
	rm -f *.o *.d Waffle_NoGui Waffle_Reaction Waffle

-include $(DEP)

%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $<
