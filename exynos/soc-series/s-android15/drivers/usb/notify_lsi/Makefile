USB_HDRS="-I${ROOT_DIR}/${KERNEL_DIR}/drivers/usb/core"

subdir-ccflags-y += $(USB_HDRS)

# usb notify driver
obj-$(CONFIG_USB_NOTIFY_LAYER_MODULE)			+= usb_notify_layer.o
usb_notify_layer-y				:= usb_notify.o  usb_notify_sysfs.o dock_notify.o
usb_notify_layer-$(CONFIG_USB_HOST_NOTIFY)			+= host_notify_class.o
usb_notify_layer-$(CONFIG_USB_EXTERNAL_NOTIFY)		+= external_notify.o
usb_notify_layer-$(CONFIG_USB_NOTIFY_PROC_LOG)		+= usblog_proc_notify.o
obj-$(CONFIG_USB_NOTIFIER)			+= usb_notifier.o
obj-$(CONFIG_VBUS_NOTIFIER)			+= vbus_notifier.o
