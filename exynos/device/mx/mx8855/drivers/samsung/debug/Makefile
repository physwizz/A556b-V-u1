# SPDX-License-Identifier: GPL-2.0

ccflags-y := -Wformat
subdir-ccflags-y += -I${ROOT_DIR}/${EXYNOS_SOC_DIR}

# softdog module
obj-$(CONFIG_SOFT_WATCHDOG_EXT)		+= softdog.o
softdog-y				+= softdog_ext.o

# Samsung Debugging Features
obj-$(CONFIG_SEC_DEBUG_BASE)		+= sec_debug.o
sec_debug-y				+= sec_debug_base.o
sec_debug-y				+= sec_dump_sink.o
obj-$(CONFIG_SEC_DEBUG_BASE)		+= sec_debug_base_early.o
obj-$(CONFIG_SEC_DEBUG_BASE)		+= sec_debug_dprm.o
obj-$(CONFIG_SEC_DEBUG_BASE)		+= sec_debug_dprt.o

obj-$(CONFIG_SEC_DEBUG_MODE)			+= sec_debug_mode.o
obj-$(CONFIG_SEC_DEBUG_EXTRA_INFO)		+= sec_debug_extra_info.o
obj-$(CONFIG_SEC_DEBUG_RESET_REASON)		+= sec_debug_reset_reason.o
obj-$(CONFIG_SEC_DEBUG_DTASK)			+= sec_debug_dtask.o
obj-$(CONFIG_SEC_DEBUG_SOFTDOG)   		+= sec_debug_softdog.o
obj-$(CONFIG_SEC_DEBUG_SCHED_INFO)		+= sec_debug_sched_info.o
obj-$(CONFIG_SEC_DEBUG_STACKTRACE)		+= sec_debug_stacktrace.o
obj-$(CONFIG_SEC_DEBUG_HARDLOCKUP_INFO)		+= sec_debug_hardlockup_info.o
obj-$(CONFIG_SEC_DEBUG_SCHED_REPORT)		+= sec_debug_sched_report.o
obj-$(CONFIG_SEC_DEBUG_CHECK_PREEMPT)		+= sec_debug_check_preempt_built.o
obj-$(CONFIG_SEC_DEBUG_KERN_ERR_REPORT)		+= sec_debug_kerror_report.o
obj-$(CONFIG_SEC_DEBUG_RESUME_PROFILE)		+= sec_debug_resume_profile.o
obj-$(CONFIG_SEC_DEBUG_SLAB_INFO)		+= sec_debug_slab_info.o

ifeq ($(CONFIG_SEC_KUNIT)$(CONFIG_CC_IS_GCC), yy)
ccflags-y := -Wno-array-bounds
KBUILD_CFLAGS += -Wno-unused-variable -Wno-unused-function

obj-$(CONFIG_SEC_KUNIT)				+= kunit_test/
GCOV_PROFILE_sec_debug_sched_info.o		:= $(CONFIG_SEC_KUNIT)
GCOV_PROFILE_sec_dump_sink.o		:= $(CONFIG_KUNIT)
else
obj-$(CONFIG_SEC_DEBUG_BASE)		+= sec_debug_test.o
obj-$(CONFIG_SEC_DEBUG_HW_PARAM)		+= sec_debug_hw_param.o
obj-$(CONFIG_SEC_DEBUG_WATCHDOGD_FOOTPRINT)	+= sec_debug_wdd_info.o
endif

