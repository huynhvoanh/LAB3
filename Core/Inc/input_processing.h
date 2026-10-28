#ifndef INC_INPUT_PROCESSING_H_
#define INC_INPUT_PROCESSING_H_
#include "main.h"
void fsm_for_input_processing(void);
uint8_t get_current_mode(void);
uint8_t get_red_edit_value(void);
uint8_t get_amber_edit_value(void);
uint8_t get_green_edit_value(void);
void processDurationButtons(void);

#endif /* INC_INPUT_PROCESSING_H_ */
