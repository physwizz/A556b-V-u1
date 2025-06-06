#include "../../comm/shub_comm.h"
#include "../../sensor/pressure.h"
#include "../../sensormanager/shub_sensor.h"
#include "../../sensormanager/shub_sensor_manager.h"
#include "../../utility/shub_utility.h"
#include "pressure_factory.h"

#include <linux/slab.h>

/*************************************************************************/
/* factory Sysfs                                                         */
/*************************************************************************/

#define LPS25H_NAME   "LPS25H"

ssize_t pressure_lps25h_temperature_show(char *buf)
{
	s32 temperature = 0;
	s32 float_temperature = 0;
	s32 temp = 0;
	struct pressure_event *sensor_value = (struct pressure_event *)(get_sensor_event(SENSOR_TYPE_PRESSURE)->value);

	temp = (s32)(sensor_value->temperature);
	temperature = (4250) + ((temp / (120 * 4)) * 100); //(42.5f) + (temperature/(120 * 4));
	float_temperature = ((temperature % 100) > 0 ? (temperature % 100) : -(temperature % 100));

	return sprintf(buf, "%d.%02d\n", (temperature / 100), float_temperature);
}

struct pressure_factory_chipset_funcs pressure_lps25h_ops = {
	.temperature_show = pressure_lps25h_temperature_show,
};

struct pressure_factory_chipset_funcs *get_pressure_lps25h_chipset_func(char *name)
{
	if (strcmp(name, LPS25H_NAME) != 0)
		return NULL;

	return &pressure_lps25h_ops;
}
