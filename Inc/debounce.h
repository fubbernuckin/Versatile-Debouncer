#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/*
 * Basic setup instructions:
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
 * The pin number corresponds to the pin that your function from earlier will read.
 * The threshold determines how confident the debouncer must be before considering
 * the button state to be changed. Threshold also determines the minimum number of
 * DB_Update calls required between state changes.
 * This library will use pointers to these buttons to access and identify them.
 * Note: threshold must not equal 0
 * ex:
 * DB_Button buttons[] = {
 *   {.pin = 4, .threshold = 20},
 *   {.pin = 23, .threshold = 8}
 * };
 *
 * 3. Initialize debouncer.
 * Declare an empty DB_Handle and run DB_Init using your handle, button array,
 * button array length, and the GPIO reading function you wrote in step 1.
 *
 * Note: the DB_Handle must be declared in a scope where the call to DB_Update
 * in the next step will still have access to it.
 * ex:
 * DB_Handle db; // may be declared in a higher scope
 * uint8_t count = sizeof(buttons)/sizeof(DB_Button); // calculate length of buttons array
 * DB_Init(&db, buttons, count, Read_GPIO, NULL); // optional arguments are left NULL
 *
 * 4. Call DB_Update() at a relatively consistent interval.
 * Every time DB_Update is called, it will query every button in the DB_Handle
 * you provided, and update the debounced state of each button.
 * Note: dramatically irregular or sparse timing of DB_Update calls may
 *   negatively affect debounce quality.
 * ex:
 * DB_Update(&db);
 */

/*
 * Button state:
 * You can read the debounced state of any button by calling DB_Rd and passing
 * your button struct as an argument.
 * ex:
 * bool state = DB_Rd(buttons[0]);
 */

/*
 * Events:
 * There are two ways to receive button events, callbacks and polling, depending
 * on what makes the most sense for your application.
 *
 * Polling is simpler to use, where a polling function is called and returns
 * whether or not that event has happened since the last time it was requested.
 * Available polling functions are DB_Rising, DB_Falling, and DB_Changed, which
 * check whether or not there has been a rising edge, falling edge, or state
 * change on the provided button struct. Since events are detected during
 * DB_Update, you can call these immediately after DB_Update to ensure no events
 * are missed.
 * Note: It is highly recommended to EITHER poll for rising/falling edges, OR
 * poll for state changes on any given button as polling for state changes can
 * interfere with calls to rising/falling edges and vice versa.
 * ex:
 * bool button_pressed = DB_Falling(buttons[0]);
 *
 * Callbacks are more complex to set up, but offer greater flexibility.
 *
 * 1. Create an event handler.
 * This function will be called every time an event is detected. It should take a
 * DB_Event struct as an argument and return nothing.
 * ex:
 * void Event_Handler(DB_Event ev) {
 *   if (ev.btn == &buttons[0]) {
 *     // do something
 *   }
 *   else if (ev.btn == &buttons[1]) {
 *     // do something else
 *   }
 * }
 *
 * 2. Add your event manager to DB_Init.
 * Pass a pointer to your event handler to DB_Init during your initial setup.
 * ex:
 * DB_Init(&db, buttons, count, Read_GPIO, Event_Handler);
 */

#define EVENT_QUEUE_SIZE 8

#ifndef INC_DEBOUNCE_H_
#define INC_DEBOUNCE_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Represents a mechanical button accessed through GPIO.
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
	uint8_t _state;
} DB_Button;

/*
 * Contains all possible button event types.
 */
typedef enum {
	DB_RISING_EDGE,
	DB_FALLING_EDGE
} DB_Event_Type;

/*
 * Represents a single button event for a given button.
 *
 * const DB_Button *btn: a pointer to the button where the event occurred.
 *
 * const DB_Event_Type ev_type: The type of button event that occurred.
 */
typedef struct {
	DB_Button *btn;
	DB_Event_Type ev_type;
} DB_Event;

/*
 * A function pointer to a user-defined wrapper function that takes in a
 *   uint8_t value as a pin ID and returns a boolean corresponding to the
 *   platform's GPIO driver output.
 */
typedef bool (*DB_GPIO_Read)(uint8_t pin);

/*
 * A function pointer to a user-defined event handler function that takes in
 *   a DB_Event struct.
 */
typedef void (*DB_Event_Callback)(DB_Event ev);

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
 *
 * DB_Event_Callback: Function pointer to user-defind event manager. Set to
 *   NULL to disable callbacks.
 */
typedef struct {
	DB_Button *btns;
	uint8_t count;
	DB_GPIO_Read rd;
	DB_Event_Callback cb;
} DB_Handle;

/*
 * Initialize all button states and populate a given DB_Handle structure.
 */
void DB_Init(DB_Handle *db, DB_Button *buttons, uint8_t count, DB_GPIO_Read rd, DB_Event_Callback cb);

/*
 * Update each button's state using DB_Handle's user-defined GPIO reader.
 * Performs event callbacks and sets rising and falling edge flags for
 * polling functions.
 *
 * Run on a consistent tick. NOT ISR or thread safe.
 */
void DB_Update(DB_Handle *db);

/*
 * Return the debounced state of a button as a boolean value. Returned state
 * will reflect the button state during the last DB_Update call.
 * ex:
 * bool x = DB_Rd(&buttons[1]);
 */
bool DB_Rd(const DB_Button *btn);

/*
 * Returns true if the debounced state of the button has gone from false to
 * true since the last DB_Rising call.
 * Clears the rising edge flag on the button every time it is called.
 * ex:
 * bool x = DB_Rising(&buttons[1]);
 */
bool DB_Rising(DB_Button *btn);

/*
 * Returns true if the debounced state of the button has gone from true to
 * false since the last DB_Falling call.
 * Clears the falling edge flag on the button every time it is called.
 * ex:
 * bool x = DB_Falling(&buttons[1]);
 */
bool DB_Falling(DB_Button *btn);

/*
 * Returns true if the debounced state of the button has changed since the last
 * DB_Falling call.
 * Clears the rising AND falling edge flags on the button every time it is
 * called.
 * ex:
 * bool x = DB_Falling(&buttons[1]);
 */
bool DB_Changed(DB_Button *btn);


#ifdef __cplusplus
}
#endif

#endif /* INC_DEBOUNCE_H_ */
