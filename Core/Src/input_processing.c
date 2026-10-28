#include "main.h"
#include "input_reading.h"
#include "input_processing.h"

static uint8_t currentMode = 1;
static uint8_t buttonWasPressed = 0;
uint8_t get_current_mode(void);

void fsm_for_input_processing(void){
	uint8_t buttonIsPressed = is_button_pressed(0);

	if (buttonIsPressed && !buttonWasPressed)
	{
		currentMode++;
		if (currentMode > 4)
		{
			currentMode = 1;
		}
	}
	buttonWasPressed = buttonIsPressed;
}

uint8_t get_current_mode(void)
{
	return currentMode;
}


static uint8_t editRed = 5;
static uint8_t editAmber = 2;
static uint8_t editGreen = 3;

static uint8_t lastTDUR = 0;
static uint8_t lastVALUE = 0;

void processDurationButtons(void)
{
    uint8_t tdur = is_button_pressed(1);
    uint8_t value = is_button_pressed(2);

    if (tdur && !lastTDUR)
    {
        switch (get_current_mode())
        {
            case 2: {
                editRed = (editRed >= 99U)
                        ? 1U : (uint8_t)(editRed + 1U);
                break;
            }

            case 3: {
                editAmber = (editAmber >= 99U)
                          ? 1U : (uint8_t)(editAmber + 1U);
                break;
            }

            case 4: {
                editGreen = (editGreen >= 99U)
                          ? 1U : (uint8_t)(editGreen + 1U);
                break;
            }

            default:
                break;
        }
    }

    if (value && !lastVALUE)
    {
        switch (get_current_mode())
        {
            case 2:
                setRedDuration(editRed);
                editRed = redDuration;
                editAmber = amberDuration;
                editGreen = greenDuration;
                break;

            case 3:
                setAmberDuration(editAmber);
                editAmber = amberDuration;
                editRed = redDuration;
                editGreen = greenDuration;
                break;

            case 4:
                setGreenDuration(editGreen);
                editGreen = greenDuration;
                editRed = redDuration;
                editAmber = amberDuration;
                break;

            default:
                break;
        }
    }

    lastTDUR = tdur;
    lastVALUE = value;
}

uint8_t get_red_edit_value(void)
{
    return editRed;
}

uint8_t get_amber_edit_value(void)
{
    return editAmber;
}

uint8_t get_green_edit_value(void)
{
    return editGreen;
}
