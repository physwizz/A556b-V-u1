/* drivers/gpu/drm/samsung/dpu/panel/mcd-panel-samsung-helper.c
 *
 * Samsung SoC display driver.
 *
 * Copyright (c) 2021 Samsung Electronics
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/of.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/sort.h>
//#include "panel_kunit.h"
#include <drm/drm_modes.h>
#include <exynos_drm_crtc.h>
#include "panel-samsung-drv.h"
#include "mcd-panel-samsung-helper.h"

/*
 * comparison function used to sort exynos panel mode
 * (order:descending)
 */
__visible_for_testing int compare_exynos_panel_mode(const void *a, const void *b)
{
	int v1, v2;

	/* compare width_mm */
	v1 = ((struct exynos_panel_mode *)a)->mode.width_mm;
	v2 = ((struct exynos_panel_mode *)b)->mode.width_mm;
	if (v1 != v2)
		return v2 - v1;

	/* compare height_mm */
	v1 = ((struct exynos_panel_mode *)a)->mode.height_mm;
	v2 = ((struct exynos_panel_mode *)b)->mode.height_mm;
	if (v1 != v2)
		return v2 - v1;

	/* compare horizontal display */
	v1 = ((struct exynos_panel_mode *)a)->mode.hdisplay;
	v2 = ((struct exynos_panel_mode *)b)->mode.hdisplay;
	if (v1 != v2)
		return v2 - v1;

	/* compare vertical display */
	v1 = ((struct exynos_panel_mode *)a)->mode.vdisplay;
	v2 = ((struct exynos_panel_mode *)b)->mode.vdisplay;
	if (v1 != v2)
		return v2 - v1;

	/* compare vertical refresh rate */
	v1 = drm_mode_vrefresh(&((struct exynos_panel_mode *)a)->mode);
	v2 = drm_mode_vrefresh(&((struct exynos_panel_mode *)b)->mode);
	if (v1 != v2)
		return v2 - v1;

	/* compare clock */
	v1 = ((struct exynos_panel_mode *)a)->mode.clock;
	v2 = ((struct exynos_panel_mode *)b)->mode.clock;
	if (v1 != v2)
		return v2 - v1;

	/* compare vtotal */
	v1 = ((struct exynos_panel_mode *)a)->mode.vtotal;
	v2 = ((struct exynos_panel_mode *)b)->mode.vtotal;
	if (v1 != v2)
		return v2 - v1;

	/* compare vscan */
	v1 = ((struct exynos_panel_mode *)a)->mode.vscan;
	v2 = ((struct exynos_panel_mode *)b)->mode.vscan;
	if (v1 != v2)
		return v2 - v1;

	return 0;
}

__visible_for_testing int exynos_mode_snprintf(const struct exynos_display_mode *mode, char *buf, size_t size)
{
	if (!mode || !buf || !size)
		return 0;

	return snprintf(buf, size, "Exynos Modeline " EXYNOS_MODE_FMT "\n", EXYNOS_MODE_ARG(mode));
}

void exynos_mode_debug_printmodeline(const struct exynos_display_mode *mode)
{
	char buf[128];

	exynos_mode_snprintf(mode, buf, sizeof(buf));
	pr_debug("%s", buf);
}

void exynos_mode_info_printmodeline(const struct exynos_display_mode *mode)
{
	char buf[128];

	exynos_mode_snprintf(mode, buf, sizeof(buf));
	pr_info("%s", buf);
}

__visible_for_testing int exynos_panel_mode_snprintf(const struct exynos_panel_mode *mode, char *buf, size_t size)
{
	if (!mode || !buf || !size)
		return 0;

	return snprintf(buf, size, "Exynos Panel Modeline (DRM)" DRM_MODE_FMT " (EXYNOS)" EXYNOS_MODE_FMT "\n",
			DRM_MODE_ARG(&mode->mode), EXYNOS_MODE_ARG(&mode->exynos_mode));
}

void exynos_panel_mode_debug_printmodeline(const struct exynos_panel_mode *mode)
{
	char buf[128];

	exynos_panel_mode_snprintf(mode, buf, sizeof(buf));
	pr_debug("%s", buf);
}

void exynos_panel_mode_info_printmodeline(const struct exynos_panel_mode *mode)
{
	char buf[128];

	exynos_panel_mode_snprintf(mode, buf, sizeof(buf));
	pr_info("%s", buf);
}

/**
 * exynos_drm_mode_set_name - set the name on a mode
 * @mode: name will be set in this mode
 * @refresh_mode : extension of exynos_drm_mode for mcd-panel (e.g. ns, hs, phs)
 *
 * Set the name of @mode to a exynos drm format which is
 * <hdisplay>x<vdisplay>@<vrefresh><refreshmode>.
 */
void exynos_drm_mode_set_name(struct drm_display_mode *mode, int refresh_mode)
{
	drm_mode_set_name(mode);

	snprintf(mode->name + strlen(mode->name),
			DRM_DISPLAY_MODE_LEN - strlen(mode->name), "@%d%s",
			drm_mode_vrefresh(mode), refresh_mode_to_str(refresh_mode));
}

int drm_display_mode_from_panel_display_mode(struct panel_display_mode *pdm, struct drm_display_mode *ddm)
{
	int vscan;

	if (!pdm | !ddm)
		return -EINVAL;

	ddm->hdisplay = (pdm->in_width > 0) ? pdm->in_width : pdm->width;
	ddm->hsync_start = ddm->hdisplay + pdm->panel_hporch[PANEL_PORCH_HFP];
	ddm->hsync_end = ddm->hsync_start + pdm->panel_hporch[PANEL_PORCH_HSA];
	ddm->htotal = ddm->hsync_end + pdm->panel_hporch[PANEL_PORCH_HBP];

	ddm->vdisplay = (pdm->in_height > 0) ? pdm->in_height : pdm->height;
	ddm->vsync_start = ddm->vdisplay + pdm->panel_vporch[PANEL_PORCH_VFP];
	ddm->vsync_end = ddm->vsync_start + pdm->panel_vporch[PANEL_PORCH_VSA];
	ddm->vtotal = ddm->vsync_end + pdm->panel_vporch[PANEL_PORCH_VBP];

	/*
	 * 'vscan' decide how many times real display(e.g. AMOLED-PANEL)
	 * vertical scan while one frame update.
	 */
	vscan = panel_mode_vscan(pdm);
	if (vscan > 1)
		ddm->vscan = vscan;

	ddm->clock = ddm->htotal * ddm->vtotal * pdm->refresh_rate * vscan / 1000;

	ddm->type = DRM_MODE_TYPE_DRIVER;

	exynos_drm_mode_set_name(ddm, pdm->refresh_mode);

	return 0;
}

static int exynos_display_mode_get_disp_qos_fps(struct panel_display_mode *pdm)
{
	int vscan;
	int disp_qos_fps = 0;
	unsigned int htotal = 0;
	unsigned int vtotal = 0;
	unsigned int vtotal_without_vfp = 0;
	unsigned int total_pixel_during_one_sec = 0;

	if (!pdm) {
		pr_err("%s pdm is null.\n", __func__);
		return -EINVAL;
	}

	htotal += pdm->width;
	htotal += pdm->panel_hporch[PANEL_PORCH_HBP];
	htotal += pdm->panel_hporch[PANEL_PORCH_HFP];
	htotal += pdm->panel_hporch[PANEL_PORCH_HSA];

	vtotal += pdm->height;
	vtotal += pdm->panel_vporch[PANEL_PORCH_VBP];
	vtotal += pdm->panel_vporch[PANEL_PORCH_VFP];
	vtotal += pdm->panel_vporch[PANEL_PORCH_VSA];

	vscan = panel_mode_vscan(pdm);
	total_pixel_during_one_sec = htotal * vtotal * pdm->refresh_rate * vscan;

	vtotal_without_vfp = (vtotal > pdm->panel_vporch[PANEL_PORCH_VFP]) ?
		(vtotal - pdm->panel_vporch[PANEL_PORCH_VFP]) : 0;

	disp_qos_fps = total_pixel_during_one_sec / (vtotal_without_vfp * htotal);

	pr_debug("%s (debug) %d %d %d %d %d %d\n", __func__,
		htotal, vtotal, pdm->refresh_rate, vscan, total_pixel_during_one_sec, vtotal_without_vfp);

	pr_info("%s disp_qos_fps:(%d)\n", __func__, disp_qos_fps);

	return disp_qos_fps;
}

int exynos_display_mode_from_panel_display_mode(struct panel_display_mode *pdm, struct exynos_display_mode *edm)
{
	if (!pdm | !edm)
		return -EINVAL;

	edm->bts_fps = (pdm->disp_qos_fps > 0) ?
		pdm->disp_qos_fps : exynos_display_mode_get_disp_qos_fps(pdm);
	edm->dsc.enabled = pdm->dsc_en;
	edm->dsc.dsc_count = pdm->dsc_cnt;
	edm->dsc.slice_count = pdm->dsc_slice_num;
	edm->dsc.slice_height = pdm->dsc_slice_h;
	edm->mode_flags = MIPI_DSI_CLOCK_NON_CONTINUOUS | (pdm->panel_video_mode ? MIPI_DSI_MODE_VIDEO : 0);
	edm->bpc = 8;
#ifdef CONFIG_USDM_PANEL_LPM
	edm->is_lp_mode = pdm->doze_mode;
#else
	edm->is_lp_mode = false;
#endif
	edm->scaler_type = EXYNOS_SCALER_NONE;
	if ((pdm->in_width > 0 && pdm->in_height > 0) &&
		(pdm->in_width != pdm->width || pdm->in_height != pdm->height)) {
		edm->desired_hdisplay = pdm->width;
		edm->desired_vdisplay = pdm->height;
		edm->scaler_type = EXYNOS_SCALER_AIQE;
	}

	// add to set drm_dsc_config here, from pdm->dsc_picture_parameter_set

	convert_pps_to_dsc_config(&edm->dsc_cfg, pdm->dsc_picture_parameter_set);

	return 0;
}

int exynos_panel_mode_from_panel_display_mode(struct panel_display_mode *pdm, struct exynos_panel_mode *epm)
{
	if (!pdm | !epm)
		return -EINVAL;

	drm_display_mode_from_panel_display_mode(pdm, &epm->mode);
	exynos_display_mode_from_panel_display_mode(pdm, &epm->exynos_mode);

	return 0;
}

struct panel_display_mode *exynos_panel_find_panel_mode(
		struct panel_display_modes *pdms, const struct drm_display_mode *pmode)
{
	struct drm_display_mode t_pmode;
	struct panel_display_mode *pdm = NULL;
	int i, ret;

	for (i = 0; i < pdms->num_modes; i++) {
		memset(&t_pmode, 0, sizeof(t_pmode));
		ret = drm_display_mode_from_panel_display_mode(pdms->modes[i], &t_pmode);
		if (ret < 0)
			continue;

		if (!strcmp(t_pmode.name, pmode->name)) {
			pdm = pdms->modes[i];
			break;
		}
	}

	return pdm;
}
EXPORT_SYMBOL(exynos_panel_find_panel_mode);

/**
 * exynos_panel_mode_create - create a new exynos panel mode
 * @ctx: exynos_panel
 *
 * Create a new, cleared exynos_panel_mode with kzalloc, allocate an ID for it
 * and return it.
 *
 * Returns:
 * Pointer to new mode on success, NULL on error.
 */
struct exynos_panel_mode *exynos_panel_mode_create(struct exynos_panel *ctx)
{
	struct exynos_panel_mode *nmode;

	nmode = kzalloc(sizeof(struct exynos_panel_mode), GFP_KERNEL);
	if (!nmode)
		return NULL;

	return nmode;
}
EXPORT_SYMBOL(exynos_panel_mode_create);


/**
 * exynos_panel_mode_destroy - remove a mode
 * @ctx: exynos_panel
 * @mode: mode to remove
 *
 * Release @mode's unique ID, then free it @mode structure itself using kfree.
 */
void exynos_panel_mode_destroy(struct exynos_panel *ctx, struct exynos_panel_mode *mode)
{
	if (!mode)
		return;

	kfree(mode);
}
EXPORT_SYMBOL(exynos_panel_mode_destroy);


/**
 * exynos_panel_desc_create - create a new exynos panel desc
 * @ctx: exynos_panel
 *
 * Create a new, cleared exynos_panel_desc with kzalloc, allocate an ID for it
 * and return it.
 *
 * Returns:
 * Pointer to new desc on success, NULL on error.
 */
struct exynos_panel_desc *exynos_panel_desc_create(struct exynos_panel *ctx)
{
	struct exynos_panel_desc *ndesc;

	ndesc = kzalloc(sizeof(struct exynos_panel_desc), GFP_KERNEL);
	if (!ndesc)
		return NULL;

	return ndesc;
}
EXPORT_SYMBOL(exynos_panel_desc_create);


/**
 * exynos_panel_desc_destroy - remove a desc
 * @ctx: exynos_panel
 * @desc: desc to remove
 *
 * Release @desc's unique ID, then free it @desc structure itself using kfree.
 */
void exynos_panel_desc_destroy(struct exynos_panel *ctx, struct exynos_panel_desc *desc)
{
	if (!desc)
		return;

	kfree(desc->modes);
	kfree(desc);
}
EXPORT_SYMBOL(exynos_panel_desc_destroy);


static int exynos_panel_fill_hdr_info(struct exynos_panel *ctx, struct exynos_panel_desc *desc)
{
	struct panel_device *panel;
	struct panel_hdr_info *hdr;

	if (!ctx) {
		pr_err("%s ctx is null\n", __func__);
		return -EINVAL;
	}

	if (!desc) {
		pr_err("%s desc is null\n", __func__);
		return -EINVAL;
	}

	panel = ctx->mcd_panel_dev;
	if (!panel) {
		pr_err("%s panel is null\n", __func__);
		return -EINVAL;
	}
	hdr = &panel->hdr;

	desc->hdr_formats = hdr->formats;
	desc->max_luminance = hdr->max_luma;
	desc->max_avg_luminance = hdr->max_avg_luma;
	desc->min_luminance = hdr->min_luma; /* TODO should get from mcd-panel */

	return 0;
}

struct exynos_panel_desc *
exynos_panel_desc_create_from_panel_display_modes(struct exynos_panel *ctx,
		struct panel_display_modes *pdms)
{
	struct exynos_panel_desc *desc;
	struct exynos_panel_mode *modes = NULL;
	struct exynos_panel_mode *unique_modes = NULL;
	struct exynos_panel_mode native_mode;
	struct panel_device *panel;
	int num_modes = 0, temp_num_modes = 0;
	int num_unique_modes = 0;
	int i, j;

	desc = exynos_panel_desc_create(ctx);
	if (!desc) {
		dev_err(ctx->dev, "%s: could not allocate exynos_panel_desc\n", __func__);
		return NULL;
	}

	panel = ctx->mcd_panel_dev;
	if (!panel) {
		dev_err(ctx->dev, "%s panel is null\n", __func__);
		goto modefail;
	}

	/* create exynos_panel_mode */
	unique_modes = kcalloc(pdms->num_modes, sizeof(*unique_modes), GFP_KERNEL);
	if (!unique_modes) {
		dev_err(ctx->dev, "%s: could not allocate exynos_panel_mode array\n", __func__);
		goto modefail;
	}

	/* store unique exynos_panel_mode */
	for (i = 0; i < pdms->num_modes; i++) {
		struct exynos_panel_mode mode;

		memset(&mode, 0, sizeof(mode));
		exynos_panel_mode_from_panel_display_mode(pdms->modes[i], &mode);

		/* push unique exynos_panel_mode */
		for (j = 0; j < num_unique_modes; j++)
			if (!memcmp(&unique_modes[j], &mode, sizeof(mode)))
				break;

		if (j != num_unique_modes)
			continue;

		/* copy unique mode */
		memcpy(&unique_modes[num_unique_modes++], &mode, sizeof(mode));
		num_modes++;
	}

	/* create modes array */
	modes = kcalloc(num_modes, sizeof(*modes), GFP_KERNEL);
	if (!modes) {
		dev_err(ctx->dev, "%s: could not allocate exynos_panel_mode array\n", __func__);
		goto modefail;
	}

	for (i = 0; i < num_unique_modes; i++)
		memcpy(&modes[temp_num_modes++],
				&unique_modes[i], sizeof(struct exynos_panel_mode));

	/*
	 * sorting exynos_panel_mode list
	 */
	sort(modes, num_modes, sizeof(modes[0]),
			compare_exynos_panel_mode, NULL);

	/*
	 * print sorted exynos_panel_mode list
	 */
	for (i = 0; i < num_modes; i++)
		exynos_panel_mode_info_printmodeline(&modes[i]);

	/* find default mode in sorted exynos_panel_mode */
	memset(&native_mode, 0, sizeof(native_mode));
	exynos_panel_mode_from_panel_display_mode(pdms->modes[pdms->native_mode], &native_mode);

	for (i = 0; i < num_modes; i++) {
		if (!memcmp(&modes[i], &native_mode,
					sizeof(native_mode))) {
			/* set native-mode as preferred mode */
			modes[i].mode.type |= DRM_MODE_TYPE_PREFERRED;
			desc->default_mode = &modes[i];
			break;
		}
	}

	/* set exynos_panel_desc */
	desc->brightness.max = 1023; /* TODO should get from mcd-panel */
	desc->brightness.cur = 511; /* TODO should get from mcd-panel */
	desc->modes = modes;
	desc->num_modes = num_modes;
	desc->dqe_xml_suffix = panel->panel_data.dqe_suffix;
	desc->aiqe_xml_suffix = panel->panel_data.aiqe_suffix;
	exynos_panel_fill_hdr_info(ctx, desc);

	kfree(unique_modes);

	return desc;

modefail:
	kfree(modes);
	kfree(unique_modes);
	exynos_panel_desc_destroy(ctx, desc);
	return NULL;
}
EXPORT_SYMBOL(exynos_panel_desc_create_from_panel_display_modes);

MODULE_AUTHOR("Gwanghui Lee <gwanghui.lee@samsung.com>");
MODULE_DESCRIPTION("MIPI-DSI based mcd panel helper driver");
MODULE_LICENSE("GPL");
