#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

export TARGET := dsi-camera-test
export TOPDIR := $(CURDIR)

GAME_TITLE  := DSi Kamera-Test
GAME_AUTHOR := Lernprojekt

include $(DEVKITARM)/ds_rules

.PHONY: all clean checkarm7 checkarm9

all: $(TARGET).nds

checkarm7:
	$(MAKE) -C arm7

checkarm9:
	$(MAKE) -C arm9

$(TARGET).nds: checkarm7 checkarm9
	ndstool -c $(TARGET).nds -7 arm7/$(TARGET).elf -9 arm9/$(TARGET).elf \
		-b $(CALICO)/share/nds-icon.bmp "$(GAME_TITLE);$(GAME_AUTHOR)"

clean:
	$(MAKE) -C arm7 clean
	$(MAKE) -C arm9 clean
	rm -f $(TARGET).nds
