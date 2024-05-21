#include <dm/uclass.h>
#include <adc.h>
#include <amlogic/saradc.h>
#include <command.h>
#include <amlogic/zapper_boot.h>

static unsigned char key_press_index = 0;

int Zapper_get_key_info(unsigned char *key_index)
{
	if (key_index == NULL) {
		printf("[ZAPPER] The file is %s, function is %s, line is %d\n",__FILE__,__FUNCTION__,__LINE__);
		return ZAPPER_ERROR;
	}

	*key_index = key_press_index;
	return ZAPPER_SUCCESS;
}


static int front_panel_key_check(unsigned int period)
{
	struct udevice *dev;
	int ret;
	unsigned int val = 0;
	unsigned int val_1 = 0;
	unsigned int val_2 = 0;
#ifdef CONFIG_ADC
	//open saradc channel for key press check
	printf("UCLASS_ADC is %d\n", UCLASS_ADC);
	ret = uclass_get_device_by_name(UCLASS_ADC, "adc", &dev);//saradc key
	if (ret) {
		printf("uclass_get_device_by_name failed\n");
		return ADC_DEVICE_ERROR;
	}

	ret = adc_set_mode(dev, MESON_SARADC_CH0, ADC_MODE_AVERAGE);//saradc key is channel 0 ,check mode is ADC_MODE_AVERAGE
	if (ret) {
		printf("current platform does not support ADC_MODE_AVERAGE mode\n");
		return ADC_DEVICE_ERROR;
	}

	//get saradc val to know which key has been pressed
	printf("[zapper]adc_channel_single_shot_mode with CH0, AVERAGE_MOD twice\n");
	ret = adc_channel_single_shot_mode("adc", ADC_MODE_AVERAGE,
					   MESON_SARADC_CH0, &val);//saradc key is channel 0 ,check mode is ADC_MODE_AVERAGE
	if (ret) {
		printf("adc_channel_single_shot_mode failed\n");
		return ADC_DEVICE_ERROR;
	}
#endif
	//printf("SARADC channel(0) val1 is %d.\n", val);
	val_1 = val;

	udelay(period*1000);//500ms

	ret = adc_channel_single_shot_mode("adc", ADC_MODE_AVERAGE,
					   MESON_SARADC_CH0, &val);//saradc key is channel 0 ,check mode is ADC_MODE_AVERAGE
	if (ret) {
		printf("adc_channel_single_shot_mode failed\n");
		return ADC_DEVICE_ERROR;
	}

	//printf("SARADC channel(0) val2 is %d.\n", val);
	val_2 = val;

	val = abs(val_1 - val_2);

	if (val < 10) {
		printf("The key has been pressed abs(val) is %d\n", val);
		if (val_1 < 5) {
			printf("The key A has been pressed\n");
			key_press_index = ADC_KEY_A_PRESS;
			return ADC_KEY_A_PRESS;
		} else if (val_1 < 926 && val_1 > 910) {
			printf("The key B has been pressed\n");
			key_press_index = ADC_KEY_B_PRESS;
			return ADC_KEY_B_PRESS;
		} else if (val_1 < 525 && val_1 > 505) {
			printf("The key C has been pressed\n");
			key_press_index = ADC_KEY_C_PRESS;
			return ADC_KEY_C_PRESS;
		} else if (val_1 == 1023) {
			key_press_index = NO_ADC_KEY_PRESS;
			return NO_ADC_KEY_PRESS;
		} else {
			printf("Can not detect which key has been pressed\n");
			key_press_index = UNKNOWN_KEY_PRESS;
			return UNKNOWN_KEY_PRESS;
		}
	} else {
		return NO_ADC_KEY_PRESS;
	}
}

static int do_zapper_key_detect(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	printf("Hello, now we are going to do_zapper_key_detect\n");
	int ret = ZAPPER_ERROR;
	ret = front_panel_key_check((unsigned int)KEY_DETECT_PERIOD);
	printf("[ZAPPER] key_press_index = %d\n",key_press_index);
	return ret;
}


U_BOOT_CMD(
	zapper_key_detect, 1, 0, do_zapper_key_detect,"zapper key detection" ,"It will use our ADC driver"
);


