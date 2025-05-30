/*
 * Copyright (c) 2023 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "hts_etc.h"

int hts_etc_probing_finish(struct hts_drvdata *drvdata)
{
	if (drvdata == NULL)
		return -EINVAL;

	drvdata->etc.probed = 1;

	return 0;
}
