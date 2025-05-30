// SPDX-License-Identifier: GPL-2.0
/*
 * Samsung Exynos SoC series Pablo driver
 *
 * Copyright (c) 2021 Samsung Electronics Co., Ltd
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/of_platform.h>
#include <linux/io.h>

#include "is-hw-api-csi.h"
#include "is-hw-api-csis_pdp_top.h"
#include "pablo-debug.h"
#include "pablo-device-camif-subblks.h"
#include "api/is-hw-api-csis_pdp_top.h"
#include "pablo-irq.h"
#include "is-device-csi.h"
#include "is-device-sensor.h"

/* CSIS_PDP_TOP */
static struct pablo_camif_csis_pdp_top *pablo_csis_pdp_top = NULL;

struct pablo_camif_csis_pdp_top *pablo_camif_csis_pdp_top_get(void)
{
	return pablo_csis_pdp_top;
}

void pablo_camif_csis_pdp_top_dump(u32 __iomem *base_reg)
{
	csi_pdp_top_dump(base_reg);
}

static void pablo_camif_csis_pdp_top_s_link_vc_list(struct pablo_camif_csis_pdp_top *top, struct pablo_camif_otf_info *otf_info, u32 otf_out_id)
{
	csis_pdp_top_s_link_vc_list(otf_info->link_vc_list[otf_out_id],
				    &otf_info->mux_val_base[otf_out_id],
				    otf_info->max_vc_num, otf_out_id);
}

static void pablo_camif_csis_pdp_top_s_otf_out_mux(struct pablo_camif_csis_pdp_top *top,
		struct pablo_camif_otf_info *otf_info, u32 otf_out_id, bool en)
{
	unsigned long flag;
	u32 csi_ch, otf_out_ch;

	if (otf_out_id >= CAMIF_OTF_OUT_ID_NUM) {
		err("[CAMIF] Not support CSI OTF OUT ID %d", otf_out_id);
		return;
	}

	csi_ch = otf_info->csi_ch;
	otf_out_ch = otf_info->otf_out_ch[otf_out_id];

	spin_lock_irqsave(&pablo_csis_pdp_top->csi_ch_slock, flag);

	if ((en && pablo_csis_pdp_top->csi_ch[otf_out_ch] != CSI_ID_MAX) ||
			(!en && pablo_csis_pdp_top->csi_ch[otf_out_ch] != csi_ch)) {
		err("[CAMIF][OTF_OUT%d] Occupied by CSI%d\n",
				otf_out_ch, pablo_csis_pdp_top->csi_ch[otf_out_ch]);
		goto exit;
	}

	/* Set CSIS Link OTF out LC */
	if (en) {
		csis_pdp_top_s_otf_lc(top->regs, otf_out_ch, otf_info->otf_lc[otf_out_id]);
		pablo_csis_pdp_top->csi_ch[otf_out_ch] = csi_ch;
	} else {
		pablo_csis_pdp_top->csi_ch[otf_out_ch] = CSI_ID_MAX;
	}

	/* Set CSIS Link OTF out mux */
	csis_pdp_top_s_otf_out_mux(top->regs, csi_ch, otf_out_ch,
		otf_info->link_vc[otf_out_id][CAMIF_VC_IMG],
		otf_info->mux_val_base[otf_out_id], en);

exit:
	spin_unlock_irqrestore(&pablo_csis_pdp_top->csi_ch_slock, flag);
}

static void pablo_camif_csis_pdp_top_set_ibuf(struct pablo_camif_csis_pdp_top *top,
		struct pablo_camif_otf_info *otf_info, u32 otf_out_id, struct is_sensor_cfg *sensor_cfg, bool csi_potf)
{
	csis_pdp_top_set_ibuf(top->regs, otf_info, otf_out_id, sensor_cfg, csi_potf);

	csis_pdp_top_irq_msk(top->regs, true);
}

static void pablo_camif_csis_pdp_top_enable_ibuf_ptrn_gen(struct pablo_camif_csis_pdp_top *top,
							  struct is_sensor_cfg *sensor_cfg,
							  struct pablo_camif_otf_info *otf_info,
							  u32 clk, u32 fps, bool on)
{
	csis_pdp_top_enable_ibuf_ptrn_gen(top->regs, sensor_cfg, otf_info, clk, fps, on);
}

static irqreturn_t pablo_camif_csis_pdp_top_isr(int irq, void *data)
{
	int ret;

	ret = csis_pdp_top_irq_src(pablo_csis_pdp_top->regs);
	if (ret == -EIO) {
		warn("IBUF OVERFLOW");
		is_debug_event_count(IS_EVENT_OVERFLOW_IBUF, 0, 0);
	}

	return IRQ_HANDLED;
}

static void pablo_camif_handle_csi_nfi_isr(enum exynos_csi_id ch)
{
	struct is_device_csi *csi = get_csi(ch);
	struct is_device_sensor *ids = v4l2_get_subdev_hostdata(*csi->subdev);

	ids->nfi_toggle = csi->nfi_toggle = !csi->nfi_toggle;
}

static irqreturn_t pablo_camif_nfi_isr(int irq, void *data)
{
	ulong str = 0, end = 0;
	enum exynos_csi_id ch;

	csis_pdp_top_nfi_irq_src(pablo_csis_pdp_top->regs, &str, &end);

	for (ch = CSI_ID_A; ch < CSI_ID_MAX && str; ch++) {
		if (test_and_clear_bit(ch, &str)) {
			info("[NFI] CSI%d N-1 start", ch);
			pablo_camif_handle_csi_nfi_isr(ch);
		}
	}

	for (ch = CSI_ID_A; ch < CSI_ID_MAX && end; ch++) {
		if (test_and_clear_bit(ch, &end)) {
			info("[NFI] CSI%d N-1 end", ch);
			/* TODO */
		}
	}

	return IRQ_HANDLED;
}

static void pablo_camif_csis_pdp_top_request_irq(void)
{
	int ret;

	ret = pablo_request_irq(pablo_csis_pdp_top->irq, pablo_camif_csis_pdp_top_isr,
		"CSI_TOP", IRQF_TRIGGER_NONE, NULL);

	if (ret)
		err("failed to request IRQ for CSI_TOP(%d): %d", pablo_csis_pdp_top->irq, ret);

	ret = pablo_request_irq(
		pablo_csis_pdp_top->irq_nfi, pablo_camif_nfi_isr, "NFI", IRQF_TRIGGER_NONE, NULL);
	if (ret)
		err("failed to request IRQ for NFI(%d): %d", pablo_csis_pdp_top->irq_nfi, ret);

	return;
}

static void pablo_camif_csis_pdp_top_free_irq(void)
{
	pablo_free_irq(pablo_csis_pdp_top->irq, NULL);
	pablo_free_irq(pablo_csis_pdp_top->irq_nfi, NULL);
}

static void pablo_camif_set_irq_msk(struct pablo_camif_csis_pdp_top *top, u32 csi_ch, bool en)
{
	csis_pdp_top_nfi_irq_msk(top->regs, csi_ch, en);
}

static const struct pablo_camif_csis_pdp_top_ops camif_top_ops = {
	.s_otf_out_mux = pablo_camif_csis_pdp_top_s_otf_out_mux,
	.s_link_vc_list = pablo_camif_csis_pdp_top_s_link_vc_list,
	.set_ibuf = pablo_camif_csis_pdp_top_set_ibuf,
	.enable_ibuf_ptrn_gen = pablo_camif_csis_pdp_top_enable_ibuf_ptrn_gen,
	.set_irq_msk = pablo_camif_set_irq_msk,
};

static int pablo_camif_csis_pdp_top_probe(struct platform_device *pdev,
		struct pablo_camif_subblks_data *data)
{
	struct device *dev = &pdev->dev;
	struct resource *res;
	int ret;
	u32 otf_ch;

	pablo_csis_pdp_top = devm_kzalloc(dev, sizeof(*pablo_csis_pdp_top), GFP_KERNEL);
	if (!pablo_csis_pdp_top) {
		dev_err(dev, "failed to alloc memory\n");
		return -ENOMEM;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(dev, "failed to get memory resource for CSIS_PDP_TOP");
		ret = -ENODEV;
		goto err_get_res;
	}

	pablo_csis_pdp_top->irq = platform_get_irq(pdev, 0);
	if (pablo_csis_pdp_top->irq < 0) {
		dev_err(dev, "failed to get IRQ for TOP: %d\n", pablo_csis_pdp_top->irq);
		ret = pablo_csis_pdp_top->irq;
		goto err_get_irq;
	}

	pablo_csis_pdp_top->irq_nfi = platform_get_irq(pdev, 1);
	if (pablo_csis_pdp_top->irq_nfi < 0)
		dev_err(dev, "failed to get IRQ for NFI: %d\n", pablo_csis_pdp_top->irq_nfi);

	pablo_csis_pdp_top->regs = devm_ioremap(dev, res->start, resource_size(res));
	if (IS_ERR(pablo_csis_pdp_top->regs)) {
		dev_err(dev, "failed to get & ioremap for CSIS_PDP_TOP control\n");
		ret = PTR_ERR(pablo_csis_pdp_top->regs);
		goto err_get_ioremap_ctl;
	}

	pablo_camif_csis_pdp_top_request_irq();

	pablo_csis_pdp_top->ops = &camif_top_ops;

	for (otf_ch = 0; otf_ch < MAX_NUM_CSIS_OTF_CH; otf_ch++)
		pablo_csis_pdp_top->csi_ch[otf_ch] = CSI_ID_MAX;

	spin_lock_init(&pablo_csis_pdp_top->csi_ch_slock);

	dev_info(dev, "%s done\n", __func__);

	return 0;

err_get_irq:
err_get_ioremap_ctl:
err_get_res:
	devm_kfree(dev, pablo_csis_pdp_top);
	pablo_csis_pdp_top = NULL;

	return ret;
}

static int pablo_camif_csis_pdp_top_remove(struct platform_device *pdev,
		struct pablo_camif_subblks_data *data)
{
	pablo_camif_csis_pdp_top_free_irq();
	return 0;
}

static struct pablo_camif_subblks_data csis_pdp_top_data = {
	.probe	= pablo_camif_csis_pdp_top_probe,
	.remove	= pablo_camif_csis_pdp_top_remove,
};

/* BNS */
static struct pablo_camif_bns *pablo_bns;

#if IS_ENABLED(CONFIG_PABLO_CAMIF_BNS)
struct pablo_camif_bns *pablo_camif_bns_get(void)
{
	return pablo_bns;
}

void pablo_camif_bns_cfg(struct pablo_camif_bns *bns,
		struct is_sensor_cfg *sensor_cfg,
		u32 ch)
{
	bns->en = csi_hw_s_bns_cfg(bns->regs, sensor_cfg,
			&bns->width, &bns->height);
	if (!bns->en)
		return;

	csi_hw_s_bns_ch(bns->mux_regs, ch);

	if (unlikely(is_get_debug_param(IS_DEBUG_PARAM_CSI)))
		csi_hw_bns_dump(bns->regs);
}

void pablo_camif_bns_reset(struct pablo_camif_bns *bns)
{
	if (!bns->en)
		return;

	csi_hw_reset_bns(bns->regs);
	bns->en = false;

	if (unlikely(is_get_debug_param(IS_DEBUG_PARAM_CSI)))
		csi_hw_bns_dump(bns->regs);
}
#endif

static int pablo_camif_bns_probe(struct platform_device *pdev,
		struct pablo_camif_subblks_data *data)
{
	int ret;
	struct device *dev = &pdev->dev;
	struct device_node *dnode = dev->of_node;
	struct resource *res;
	u32 elems;

	pablo_bns = devm_kzalloc(dev, sizeof(*pablo_bns), GFP_KERNEL);
	if (!pablo_bns) {
		dev_err(dev, "failed to allocate drv_data\n");
		return -ENOMEM;
	}

	pablo_bns->dev = &pdev->dev;
	pablo_bns->data = data;
	pablo_bns->en = false;

	/* Get BNS base address */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "ctl");
	if (!res) {
		dev_err(dev, "failed to get ctl IORESOURCE_MEM");
		ret = -ENOMEM;
		goto ioremap_ctl_err;
	}

	pablo_bns->regs = devm_ioremap(dev, res->start, resource_size(res));
	if (IS_ERR_OR_NULL(pablo_bns->regs)) {
		dev_err(dev, "failed to ioremap for ctl\n");
		ret = -ENOMEM;
		goto ioremap_ctl_err;
	}

	/* Get BNS In/Out mux address */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "mux");
	if (!res) {
		dev_err(dev, "failed to get mux IORESOURCE_MEM");
		ret = -ENOMEM;
		goto ioremap_mux_err;
	}

	pablo_bns->mux_regs = devm_ioremap(dev, res->start, resource_size(res));
	if (IS_ERR_OR_NULL(pablo_bns->regs)) {
		dev_err(dev, "failed to ioremap for ctl\n");
		ret = -ENOMEM;
		goto ioremap_mux_err;
	}

	/* Get BNS In/Out mux values */
	ret = of_property_count_u32_elems(dnode, "mux");
	if (ret < 0) {
		dev_err(dev, "failed to get mux_val\n");
		goto mux_val_err;
	}

	elems = ret;
	pablo_bns->mux_val = devm_kcalloc(dev, elems, sizeof(*pablo_bns->mux_val), GFP_KERNEL);
	if (!pablo_bns->mux_val) {
		dev_err(dev, "failed to allocate mux_val memory\n");
		ret = -ENOMEM;
		goto mux_val_err;
	}

	ret = of_property_read_u32_array(dnode, "mux", pablo_bns->mux_val, elems);
	if (ret) {
		dev_err(dev, "failed to get mux_val resource\n");
		goto mux_val_read_err;
	}

	ret = of_property_read_u32(dnode, "dma_mux", &pablo_bns->dma_mux_val);
	if (ret) {
		dev_err(dev, "failed to get dma_mux_val resource\n");
		goto mux_val_read_err;
	}

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "mux");

	platform_set_drvdata(pdev, pablo_bns);

	dev_info(dev, "%s done\n", __func__);

	return 0;

mux_val_read_err:
	devm_kfree(dev, pablo_bns->mux_val);
mux_val_err:
	devm_iounmap(dev, pablo_bns->mux_regs);
ioremap_mux_err:
	devm_iounmap(dev, pablo_bns->regs);
ioremap_ctl_err:
	devm_kfree(dev, pablo_bns);

	return ret;
}

static int pablo_camif_bns_remove(struct platform_device *pdev,
		struct pablo_camif_subblks_data *data)
{
	return 0;
}

static struct pablo_camif_subblks_data bns_data = {
	.slock	= __SPIN_LOCK_UNLOCKED(bns_data.slock),
	.probe	= pablo_camif_bns_probe,
	.remove	= pablo_camif_bns_remove,
};

/* MCB */
static struct pablo_camif_mcb *camif_mcb;

struct pablo_camif_mcb *pablo_camif_mcb_get(void)
{
	return camif_mcb;
}

static int pablo_camif_mcb_probe(struct platform_device *pdev,
		struct pablo_camif_subblks_data *data)
{
	struct device *dev = &pdev->dev;
	struct resource *res;
	int ret;

	camif_mcb = devm_kzalloc(dev, sizeof(*camif_mcb), GFP_KERNEL);
	if (!camif_mcb) {
		dev_err(dev, "failed to alloc memory\n");
		return -ENOMEM;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		probe_err("failed to get memory resource for MCB");
		ret = -ENODEV;
		goto err_get_res;
	}

	camif_mcb->regs = devm_ioremap_resource(dev, res);
	if (IS_ERR(camif_mcb->regs)) {
		dev_err(dev, "failed to get & ioremap for MCB control\n");
		ret = PTR_ERR(camif_mcb->regs);
		goto err_get_ioremap_ctl;
	}

	camif_mcb->active_ch = 0;
	mutex_init(&camif_mcb->lock);

	dev_info(dev, "%s done\n", __func__);

	return 0;

err_get_ioremap_ctl:
err_get_res:
	devm_kfree(dev, camif_mcb);

	return ret;
}

static int pablo_camif_mcb_remove(struct platform_device *pdev,
		struct pablo_camif_subblks_data *data)
{
	return 0;
}

static struct pablo_camif_subblks_data mcb_data = {
	.probe	= pablo_camif_mcb_probe,
	.remove	= pablo_camif_mcb_remove,
};

/* EBUF */
static struct pablo_camif_ebuf *camif_ebuf;

struct pablo_camif_ebuf *pablo_camif_ebuf_get(void)
{
	return camif_ebuf;
}

static int pablo_camif_ebuf_probe(struct platform_device *pdev,
		struct pablo_camif_subblks_data *data)
{
	struct device *dev = &pdev->dev;
	struct device_node *dnode = dev->of_node;
	struct resource *res;
	int ret;

	camif_ebuf = devm_kzalloc(dev, sizeof(*camif_ebuf), GFP_KERNEL);
	if (!camif_ebuf) {
		dev_err(dev, "failed to alloc memory\n");
		return -ENOMEM;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		probe_err("failed to get memory resource for EUBF");
		ret = -ENODEV;
		goto err_get_res;
	}

	camif_ebuf->regs = devm_ioremap_resource(dev, res);
	if (IS_ERR(camif_ebuf->regs)) {
		dev_err(dev, "failed to get & ioremap for EBUF control\n");
		ret = PTR_ERR(camif_ebuf->regs);
		goto err_get_ioremap_ctl;
	}

	camif_ebuf->irq = platform_get_irq(pdev, 0);
	if (camif_ebuf->irq < 0) {
		dev_err(dev, "failed to get IRQ for EBUF: %d\n", camif_ebuf->irq);
		ret = camif_ebuf->irq;
		goto err_get_irq;
	}

	ret = of_property_read_u32(dnode, "num_of_ebuf", &camif_ebuf->num_of_ebuf);
	if (ret) {
		dev_err(dev, "failed to get num_of_ebuf property\n");
		goto err_get_num_of_ebuf;
	}

	ret = of_property_read_u32(dnode, "fake_done_offset", &camif_ebuf->fake_done_offset);
	if (ret) {
		dev_err(dev, "failed to get fake_done_offset property\n");
		goto err_get_fake_done_offset;
	}

	mutex_init(&camif_ebuf->lock);

	dev_info(dev, "%s done\n", __func__);

	return 0;

err_get_fake_done_offset:
err_get_num_of_ebuf:
err_get_irq:
	devm_iounmap(dev, camif_ebuf->regs);

err_get_ioremap_ctl:
err_get_res:
	devm_kfree(dev, camif_ebuf);

	return ret;
}

static int pablo_camif_ebuf_remove(struct platform_device *pdev,
		struct pablo_camif_subblks_data *data)
{
	return 0;
}

static struct pablo_camif_subblks_data ebuf_data = {
	.probe	= pablo_camif_ebuf_probe,
	.remove	= pablo_camif_ebuf_remove,
};

/* COMMON */
static const struct of_device_id pablo_camif_subblks_of_table[] = {
	{
		.name = "camif-top",
		.compatible = "samsung,camif-top",
		.data = &csis_pdp_top_data,
	},
	{
		.name = "camif-bns",
		.compatible = "samsung,camif-bns",
		.data = &bns_data,
	},
	{
		.name = "camif-mcb",
		.compatible = "samsung,camif-mcb",
		.data = &mcb_data,
	},
	{
		.name = "camif-ebuf",
		.compatible = "samsung,camif-ebuf",
		.data = &ebuf_data,
	},
	{},
};
MODULE_DEVICE_TABLE(of, pablo_camif_subblks_of_table);

static int pablo_camif_subblks_suspend(struct device *dev)
{
	return 0;
}

static int pablo_camif_subblks_resume(struct device *dev)
{
	return 0;
}

int pablo_camif_subblks_runtime_suspend(struct device *dev)
{
	return 0;
}

int pablo_camif_subblks_runtime_resume(struct device *dev)
{
	return 0;
}

static const struct dev_pm_ops pablo_camif_subblks_pm_ops = {
	.suspend		= pablo_camif_subblks_suspend,
	.resume			= pablo_camif_subblks_resume,
	.runtime_suspend	= pablo_camif_subblks_runtime_suspend,
	.runtime_resume		= pablo_camif_subblks_runtime_resume,
};

static int pablo_camif_subblks_pdev_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	const struct of_device_id *of_id;
	struct pablo_camif_subblks_data *data;

	of_id = of_match_device(of_match_ptr(pablo_camif_subblks_of_table), dev);
	if (!of_id)
		return -EINVAL;

	data = (struct pablo_camif_subblks_data *)of_id->data;

	return data->probe(pdev, data);
}

static int pablo_camif_subblks_pdev_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	const struct of_device_id *of_id;
	struct pablo_camif_subblks_data *data;

	of_id = of_match_device(of_match_ptr(pablo_camif_subblks_of_table), dev);
	if (!of_id)
		return -EINVAL;

	data = (struct pablo_camif_subblks_data *)of_id->data;

	return data->remove(pdev, data);
}

static struct platform_driver pablo_camif_subblks_driver = {
	.probe = pablo_camif_subblks_pdev_probe,
	.remove = pablo_camif_subblks_pdev_remove,
	.driver = {
		.name		= "pablo-camif-subblks",
		.owner		= THIS_MODULE,
		.of_match_table	= pablo_camif_subblks_of_table,
		.pm		= &pablo_camif_subblks_pm_ops,
	}
};

struct platform_driver *pablo_camif_subblks_get_platform_driver(void)
{
	return &pablo_camif_subblks_driver;
}

#ifndef MODULE
static int __init pablo_camif_subblks_init(void)
{
	int ret;

	ret = platform_driver_probe(&pablo_camif_subblks_driver,
		pablo_camif_subblks_pdev_probe);
	if (ret)
		pr_err("failed to probe %s driver: %d\n",
			"pablo-camif-subblks", ret);

	return ret;
}

device_initcall_sync(pablo_camif_subblks_init);
#endif

MODULE_DESCRIPTION("Samsung EXYNOS SoC Pablo CAMIF Sub-blocks driver");
MODULE_ALIAS("platform:samsung-pablo-camif-subblks");
MODULE_LICENSE("GPL v2");
