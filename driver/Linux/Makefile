# ===========================================================================
#  File          :    Makefile
#  Entity        :    linux
#  Purpose       :    Makefile of purelifi driver
# ===========================================================================
#  Design Engineer : Angelos Spanos (DE)
#  Creation Date   : 18-Jan-2016
# ===========================================================================

TARGET=purelifi
TARGET_MODULE=$(TARGET).ko

obj-m += $(TARGET).o 

$(TARGET)-objs := $(TARGET)_chip.o $(TARGET)_mac.o $(TARGET)_usb.o
PWD = $(shell pwd)
EXTRA_CFLAGS +=-Wdeclaration-after-statement
EXTRA_CFLAGS +=-DTYPE_STA
KERNEL_PATH=/lib/modules/$(shell uname -r)/build
MODULE_INSTALLATION_PATH=/lib/modules/$(shell uname -r)/kernel/drivers/net/wireless/
FIRMWARE_BASE_PATH_GENERIC=/lib/firmware
FIRMWARE_BASE_PATH_KERNEL=$(FIRMWARE_BASE_PATH_GENERIC)/$(shell uname -r)/$(TARGET)
FIRMWARE_INSTALLATION_PATH=purelifi/li_fi_x
FIRMWARE_SOURCE_PATH=firmware
FIRMWARE_SOURCE_BINARY={fpga.bit,usb_slave_fw.hex}
RM_FLAGS=-rf
SRC+=$(wildcard *.c)
SRC+=$(wildcard *.h)
MODULE_BOOT_LAOD_FILE=/etc/modules
all: $(TARGET_MODULE)
ifneq ($(wildcard /etc/pm/config.d/config),)
ifeq ($(shell grep SUSPEND_MODULES /etc/pm/config.d/config),)
install:$(TARGET_MODULE) add_suspend_module
else
ifeq ($(shell grep SUSPEND_MODULES /etc/pm/config.d/config | grep purelifi),)
install:$(TARGET_MODULE) append_suspend_module
endif
endif
endif

$(TARGET_MODULE):$(SRC)
	@if [ ! -d "$(KERNEL_PATH)" ]; then echo "$(KERNEL_PATH) doesn't exist. Please install the kernel headers first."; false; fi
	make -C $(KERNEL_PATH) M=$(PWD) EXTRA_CFLAGS="$(EXTRA_CFLAGS)" modules

install:
	@if [ ! -d "$(MODULE_INSTALLATION_PATH)" ]; then mkdir -p $(MODULE_INSTALLATION_PATH); fi
	cp -f $(TARGET_MODULE) $(MODULE_INSTALLATION_PATH)
	@if [ "$(shell grep $(TARGET) $(MODULE_BOOT_LAOD_FILE))" != "$(TARGET)" ]; then echo $(TARGET) >> $(MODULE_BOOT_LAOD_FILE); fi
	@if [ -d "$(FIRMWARE_BASE_PATH_KERNEL)" ]; then \
		mkdir -p $(FIRMWARE_BASE_PATH_KERNEL)/$(FIRMWARE_INSTALLATION_PATH)/ ; \
	fi
	mkdir -p $(FIRMWARE_BASE_PATH_GENERIC)/$(FIRMWARE_INSTALLATION_PATH)/
	$(shell find $(FIRMWARE_BASE_PATH_GENERIC)/$(FIRMWARE_INSTALLATION_PATH) -type l -name fpga.bit -exec rm {} \;)
	cp $(FIRMWARE_SOURCE_PATH)/* $(FIRMWARE_BASE_PATH_GENERIC)/$(FIRMWARE_INSTALLATION_PATH)/
	depmod
	modprobe -r $(TARGET)
	modprobe $(TARGET)

add_suspend_module:
	echo 'SUSPEND_MODULES="purelifi"' >> /etc/pm/config.d/config

append_suspend_module:
	$(shell sed -i 's/\(SUSPEND_MODULES *= *"\)\(.*\)/\1$(TARGET) \2/g' /etc/pm/config.d/config)

tags:
	ctags -R ./ ../../mac80211/ $(KERNEL_PATH)

clean:
	@$(RM) $(RM_FLAGS) $(TARGET_MODULE) *.o .*.cmd Module.symvers modules.order $(TARGET).mod.c .tmp_versions *.ko*
