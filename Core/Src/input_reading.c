#include "main.h"
#include "input_reading.h"

#define BUTTON_COUNT 3
#define BUTTON_PRESSED GPIO_PIN_RESET
#define BUTTON_RELEASED GPIO_PIN_SET
static volatile GPIO_PinState buttonBuffer[BUTTON_COUNT] = {
    BUTTON_RELEASED, BUTTON_RELEASED, BUTTON_RELEASED
};

static GPIO_PinState debounceButtonBuffer1[BUTTON_COUNT] = {
		BUTTON_RELEASED, BUTTON_RELEASED, BUTTON_RELEASED
};

static GPIO_PinState debounceButtonBuffer2[BUTTON_COUNT] = {
		BUTTON_RELEASED, BUTTON_RELEASED, BUTTON_RELEASED
};

static GPIO_PinState readButtonPin(uint8_t index)
{
	switch (index)
	{
	case 0:
		return HAL_GPIO_ReadPin(MODE_GPIO_Port, MODE_Pin);

	case 1:
		return HAL_GPIO_ReadPin(TDUR_GPIO_Port, TDUR_Pin);

	case 2:
		return HAL_GPIO_ReadPin(VALUE_GPIO_Port, VALUE_Pin);

	default:
		return BUTTON_RELEASED;
	}
}
void button_reading(void)
{
	for (uint8_t i = 0; i < BUTTON_COUNT; i++) {
		debounceButtonBuffer2[i] = debounceButtonBuffer1[i];
		debounceButtonBuffer1[i] = readButtonPin(i);
		if (debounceButtonBuffer1[i] == debounceButtonBuffer2[i])
		{
			buttonBuffer[i] = debounceButtonBuffer1[i];
		}
	}
}

uint8_t is_button_pressed(uint8_t index)
{
    if (index >= BUTTON_COUNT)
    {
        return 0;
    }

    return buttonBuffer[index] == BUTTON_PRESSED;
}
