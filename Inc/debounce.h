#include <stdint.h>
#include <stdbool.h>

/* Instructions:
 *
 * 1. Create GPIO reading function.
 * This function acts as a wrapper for your platform's GPIO read driver.
 * It should accept a uint8_t input representing a pin and output a bool
 * representing the state of that pin.
 * ex:
 * bool Read_GPIO(uint8_t pin) {
 *   if (pin < 16) {
 *     return (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0 << pin) == GPIO_PIN_SET);
 *   } else {
 *     return (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0 << (pin - 16)) == GPIO_PIN_SET);
 *   }
 * }
 *
 * 2. Create DB_Button array.
 * Create an array of DB_Button structures, and enter the pin and threshold fields.
 * The pin number corresponds to the pin your function from earlier will read.
 * The threshold determines how confident the debouncer must be before considering
 * the button state to be changed.
 * Note: threshold must not equal 0
 * ex:
 * DB_Button buttons[] = {
 *   {.pin = 4, .threshold = 20},
 *   {.pin = 23, .threshold = 1} // threshold of 1 means button is not debounced
 * };
 *
 * 3. Initialize debouncer.
 * Declare an empty DB_Handle and run DB_Init using your handle, button array,
 * button array length, and the pin reading function you wrote in step 1.
 * Note: the DB_Handle must be declared in a scope where the call to DB_Update
 * in the next step will still have access to it.
 * ex:
 * DB_Handle db;
 * uint8_t count = sizeof(buttons)/sizeof(DB_Button); //calculate length of buttons array
 * DB_Init(&db, buttons, count, Read_GPIO);
 *
 * 4. Call DB_Update() on a consistent tick, like a main loop or hardware timer.
 * Every time DB_Update is called, it will query every button in the DB_Handle
 * you provided, and update the debounced state of each button.
 * Note: dramatically irregular timing of DB_Update calls may negatively affect debounce quality.
 * ex:
 * DB_Update(&db);
 *
 * 5. Read button states as needed
 * bool x = DB_Rd(&buttons[1]);
 */

#ifndef INC_DEBOUNCE_H_
#define INC_DEBOUNCE_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	// user-defined
	const uint8_t pin;
	const uint_fast8_t threshold;

	// private
	uint_fast8_t _counter;
	volatile bool _state;
} DB_Button;

typedef bool (*DB_GPIO_Read)(uint8_t pin);

typedef struct {
	DB_Button *btns;
	uint8_t count;
	DB_GPIO_Read rd;
} DB_Handle;

// Initialize all button states and establish a handle structure
void DB_Init(DB_Handle *db, DB_Button *buttons, uint8_t count, DB_GPIO_Read rd);

// Update all buttons' states using buttons' user-defined readers
void DB_Update(DB_Handle *db);

// Read debounced state of button
bool DB_Rd(const DB_Button *button);

#ifdef __cplusplus
}
#endif

#endif /* INC_DEBOUNCE_H_ */
