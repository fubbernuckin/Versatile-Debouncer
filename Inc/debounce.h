#include <stdint.h>
#include <stdbool.h>

/*
 * Setup instructions:
 *
 * 1. Define GPIO read function.
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
 * 2. Define a DB_Button array.
 * Define an array of DB_Button structures, and enter the pin and threshold fields.
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
 * DB_Handle db; // may be declared in a higher scope
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
 * ex:
 * bool x = DB_Rd(&buttons[1]);
 */

#ifndef INC_DEBOUNCE_H_
#define INC_DEBOUNCE_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Represents a GPIO input pin.
 *
 * const uint8_t pin: An integer pin ID. Pin ID should correspond to which pin
 *   will be read by the DB_GPIO_Read function provided by the user in
 *   DB_Handle.
 *
 * const uint_fast8_t threshold: The threshold required to change the debounced
 *   state of the DB_Button. Higher values respond more slowly, but are more
 *   tolerant to chatter and noise.
 */
typedef struct {
	// user-defined
	const uint8_t pin;
	const uint_fast8_t threshold;

	// private
	uint_fast8_t _counter;
	volatile uint8_t _state;
} DB_Button;

/*
 * A function pointer to a user-defined wrapper function that takes in a
 *   uint8_t value as a pin ID and returns a boolean corresponding to the
 *   platform's GPIO driver output.
 */
typedef bool (*DB_GPIO_Read)(uint8_t pin);

/*
 * Debouncer handle, used to keep track of buttons and update debounced states.
 *
 * DB_Button *btns: An array of DB_Button structures corresponding to all
 *   pins the user wants debounced.
 *
 * uint8_t count: The number of DB_Button structures in the btns array.
 *
 * DB_GPIO_Read rd: Function pointer to the user-defind GPIO read wrapper for
 *   the platform's GPIO driver.
 */
typedef struct {
	DB_Button *btns;
	uint8_t count;
	DB_GPIO_Read rd;
} DB_Handle;

/*
 * Initialize all button states and populate a given DB_Handle structure.
 */
void DB_Init(DB_Handle *db, DB_Button *buttons, uint8_t count, DB_GPIO_Read rd);

/*
 * Update each button's state using DB_Handle's user-defined GPIO reader.
 *
 * Run on a consistent tick. May be called from an ISR but is NOT generally
 * thread safe. Debounce updates follow a single-writer multi-reader pattern.
 */
void DB_Update(DB_Handle *db);

/*
 * Return the debounced state of a button as a boolean value. Returned state
 * will reflect the button state during the last DB_Update call.
 */
bool DB_Rd(const DB_Button *button);

/*
 * Returns true if the debounced state of the button has gone from false to
 * true since the last DB_Rising call.
 * Clears the rising edge flag on the button every time it is called.
 */
bool DB_Rising(DB_Button *button);

/*
 * Returns true if the debounced state of the button has gone from true to
 * false since the last DB_Falling call.
 * Clears the falling edge flag on the button every time it is called.
 */
bool DB_Falling(DB_Button *button);

#ifdef __cplusplus
}
#endif

#endif /* INC_DEBOUNCE_H_ */
