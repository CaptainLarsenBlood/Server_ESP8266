include	../common.mk
 
SDK := D:/ESP8266/ESP8266_RTOS_SDK
SDK_INC := $(SDK)/include
SDK_DRIVER_INC := $(SDK)/driver_lib/include
 
SDK_LIBDIR	= lib
LD_SCRIPT	= eagle.app.v6.ld
LIBS	= cirom crypto espconn espnow freertos gcc hal lwip main mesh mirom net80211 nopoll phy pp mbedtls openssl smartconfig wpa wps airkiss
SDK_INCDIR	= extra_include include driver_lib/include include/espressif include/lwip include/lwip/ipv4 include/lwip/ipv6 include/nopoll include/spiffs include/ssl include/json include/openssl
 
SDK_INCDIR	:= $(addprefix -I$(SDK)/,$(SDK_INCDIR))
SDK_LIBDIR	:= $(addprefix $(SDK)/,$(SDK_LIBDIR))
LD_SCRIPT	:= $(addprefix -T$(SDK)/ld/,$(LD_SCRIPT))
LIBS		:= $(addprefix -l,$(LIBS))
 
XTENSA := D:/ESP8266/xtensa-lx106-elf
CC := $(XTENSA)/bin/xtensa-lx106-elf-gcc
AR := $(XTENSA)/bin/xtensa-lx106-elf-ar
LD := $(XTENSA)/bin/xtensa-lx106-elf-gcc
SIZE := $(XTENSA)/bin/xtensa-lx106-elf-size
 
CC_FLAGS := -g -O0 -std=gnu99 -Wpointer-arith -Wundef -Werror -Wl,-EL \
	-fno-inline-functions -nostdlib -mlongcalls \
	-mtext-section-literals -mno-serialize-volatile -D__ets__ \
	-DICACHE_FLASH -c
 
LDFLAGS		= -nostdlib -Wl,--no-check-sections -u call_user_start -Wl,-static
 
ESPTOOL := esptool
 
SRC_DIR = src/
 
src_files := $(wildcard $(SRC_DIR)*)
obj_files := $(addprefix build/, $(addsuffix .o, $(basename $(notdir $(src_files)))))
 
define build-obj
	@echo "CC "$(notdir $@)
	@$(CC) -Iinc -I$(SDK_INC) -I$(SDK_INC)/json -I$(SDK_INC)/espressif $(SDK_INCDIR) \
		$(CC_FLAGS) -c $< -o $@
endef
 
.PHONY: all clean
 
build/%.o: src/%.c inc/%.h inc/user_config.h
	$(call build-obj)
 
build/app_app.a: $(obj_files)
	@echo "AR build/app_app.a"
	@$(AR) cru build/app_app.a $(obj_files)
 
build/app.out: build/app_app.a
	@echo "LD build/app.out"
	@$(LD) -L$(SDK_LIBDIR) $(LD_SCRIPT) $(LDFLAGS) -Wl,--start-group $(LIBS) build/app_app.a $(SDK_LIBDIR)/libdriver.a -Wl,--end-group -o build/app.out
 
all: build/app.out
	@echo "SIZE build/app.out"
	@$(SIZE) build/app.out
	@echo "ESPTOOL build/app.out-0x00000.bin build/app.out-0x10000.bin"
	@$(ESPTOOL) elf2image build/app.out
 
flash:
	@$(ESPTOOL) -p $(PORT) -b $(BAUDRATE) write_flash --no-compress -ff 40m -fm dio -fs 4MB 0x00000 build/app.out-0x00000.bin 0x20000 build/app.out-0x20000.bin
 
clean:
	@rm -v build/*.o build/app_app.a build/app.out build/app.out-0x00000.bin build/app.out-0x20000.bin