EXE 	 := Waffle

CC 		 := ppc-amigaos-g++
WARNINGS := -Wno-prio-ctor-dtor -Wno-unused-parameter -Wno-unused-variable -Wno-unused-value -Wno-parentheses -Wno-enum-compare
CFLAGS 	 := -mcrt=clib4 -D__USE_INLINE__ -O3 -mstrict-align -std=c++17 -I. -Iinclude -gstabs -MMD $(WARNINGS)
LDFLAGS  := -mcrt=clib4
LIBS 	 := -lftdi -lcapsimage -athread=native
STRIP	 := ppc-amigaos-strip
FLEXCAT  := flexcat

CATALOGS := Locale/italian/waffle.catalog Locale/german/waffle.catalog Locale/french/waffle.catalog Locale/dutch/waffle.catalog Locale/greek/waffle.catalog Locale/spanish/waffle.catalog Locale/polish/waffle.catalog

SOURCES := ADFWriter.cpp ArduinoInterface.cpp common.cpp ftdi_impl.cpp ibm_sectors.cpp pll.cpp RotationExtractor.cpp SerialIO.cpp locale_support.cpp
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

# Catalog compilation rules
catalogs: $(CATALOGS)

Locale/italian/waffle.catalog: Locale/waffle.cd Locale/italian/waffle.ct
	$(FLEXCAT) Locale/waffle.cd Locale/italian/waffle.ct CATALOG $@

Locale/german/waffle.catalog: Locale/waffle.cd Locale/german/waffle.ct
	$(FLEXCAT) Locale/waffle.cd Locale/german/waffle.ct CATALOG $@

Locale/french/waffle.catalog: Locale/waffle.cd Locale/french/waffle.ct
	$(FLEXCAT) Locale/waffle.cd Locale/french/waffle.ct CATALOG $@

Locale/dutch/waffle.catalog: Locale/waffle.cd Locale/dutch/waffle.ct
	$(FLEXCAT) Locale/waffle.cd Locale/dutch/waffle.ct CATALOG $@

Locale/greek/waffle.catalog: Locale/waffle.cd Locale/greek/waffle.ct
	$(FLEXCAT) Locale/waffle.cd Locale/greek/waffle.ct CATALOG $@

Locale/spanish/waffle.catalog: Locale/waffle.cd Locale/spanish/waffle.ct
	$(FLEXCAT) Locale/waffle.cd Locale/spanish/waffle.ct CATALOG $@

Locale/polish/waffle.catalog: Locale/waffle.cd Locale/polish/waffle.ct
	$(FLEXCAT) Locale/waffle.cd Locale/polish/waffle.ct CATALOG $@

clean:
	rm -f *.o *.d Waffle_NoGui Waffle_Reaction Waffle $(CATALOGS)

-include $(DEP)

%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $<
