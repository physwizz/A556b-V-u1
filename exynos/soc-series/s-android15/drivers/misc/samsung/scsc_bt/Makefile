# Needed since this subdir is symlinked in the main Kernel tree
# without this our samsung subdir is NOT cleaned.
clean-files := *.o *.ko

ifeq ($(CONFIG_SCSC_BT_KUNIT_TEST),y)
obj-$(CONFIG_SCSC_BT_KUNIT_TEST) += ./kunit/
else
#
# Maxwell BT
obj-$(CONFIG_SCSC_BT) += scsc_bt.o
scsc_bt-y += scsc_bt_module.o scsc_shm.o scsc_avdtp_detect.o
scsc_bt-$(CONFIG_SCSC_QOS) += scsc_bt_qos.o
scsc_bt-$(CONFIG_SCSC_BT_BLUEZ) += scsc_bluez.o
scsc_bt-$(CONFIG_SCSC_ANT) += scsc_ant.o
endif # CONFIG_SCSC_BT_KUNIT_TEST

scsc_bt-$(CONFIG_SCSC_BT_USE_HCI_UART) += \
				scsc_bt_hci_uart.o scsc_bt_virtual_uart.o

ifneq ($(filter $(CONFIG_SCSC_LOG_COLLECTION), y m),)
ccflags-y += -DCONFIG_SCSC_LOG_COLLECTION
endif

ifneq ($(filter $(CONFIG_SCSC_INDEPENDENT_SUBSYSTEM), y m),)
ccflags-y += -DCONFIG_SCSC_INDEPENDENT_SUBSYSTEM
endif

ccflags-y += $(CONFIG_SAMSUNG_MAXWELL_EXTRA)
## See sibling scsc/ Makefile for an explanation of the reasons of
## the following ifeq/else
ifeq ($(CONFIG_SCSC_LOGRING), m)
ccflags-y += -DCONFIG_SCSC_PRINTK
else
ccflags-$(CONFIG_SCSC_LOGRING) += -DCONFIG_SCSC_PRINTK
endif

ifeq ($(CONFIG_SCSC_BT_BLUEZ), m)
ccflags-y += -DCONFIG_SCSC_BT_BLUEZ
else
ccflags-$(CONFIG_SCSC_BT_BLUEZ) += -DCONFIG_SCSC_BT_BLUEZ
endif

ifeq ($(CONFIG_SCSC_ANT), m)
ccflags-y += -DCONFIG_SCSC_ANT
else
ccflags-$(CONFIG_SCSC_ANT) += -DCONFIG_SCSC_ANT
endif

ifneq ($(CONFIG_SCSC_SEP_VERSION),0)
ccflags-y += -DSCSC_SEP_VERSION=$(CONFIG_SCSC_SEP_VERSION)
endif

ifeq ($(CONFIG_SCSC_WLBTD), m)
ccflags-y += -DCONFIG_SCSC_WLBTD
endif
