#include "platform_mif.h"

struct platform_mif;

void platform_mif_suspend_reg_handler(struct scsc_mif_abs *interface,
	int (*suspend)(struct scsc_mif_abs *abs, void *data),
	void (*resume)(struct scsc_mif_abs *abs, void *data),
	void *data);
void platform_mif_suspend_unreg_handler(struct scsc_mif_abs *interface);
void platform_recovery_disabled_reg(struct scsc_mif_abs *interface, bool (*handler)(void));
void platform_recovery_disabled_unreg(struct scsc_mif_abs *interface);
void platform_mif_irq_default_handler(int irq, void *data);
void platform_mif_irq_reset_request_default_handler(int irq, void *data);
irqreturn_t platform_mbox_pmu_isr(int irq, void *data);
void platform_mif_intr_handler_api_init(struct platform_mif *platform);
