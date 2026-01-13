# Versatile Debouncer

Robust, simple, platform independent, fast button debouncer.

Written in C, this library uses a configurable integrator algorithm to handle button chatter and noise in software. 

## Features
- Entirely hardware agnostic design.
- Inline documentation.
- Integrator-based debouncing algorithm for fast, reliable debouncing.
- Button polling.
- Event polling for easy event handling.
- Event callbacks for more sophisticated event handling.

## Basic setup

**Further documentation and instruction is available in `debounce.h`**

1. Define a GPIO read function

This function acts as a wrapper for your platform's GPIO read driver. It should accept a uint8_t input representing a pin and output a bool representing the state of that pin.

ex:
```C
bool Read_GPIO(uint8_t pin) {
	if (pin < 16) {
		return (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0 << pin) == GPIO_PIN_SET);
	} else {
		return (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0 << (pin - 16)) == GPIO_PIN_SET);
	}
}
```

2. Define a `DB_Button` array

Define an array of `DB_Button` structures, and enter the pin and threshold fields. The pin number corresponds to the pin that your function from earlier will read. The threshold determines how confident the debouncer must be before considering the button state to be changed. Threshold also determines the minimum number of `DB_Update` calls required between state changes. This library will use pointers to these buttons to access and identify them.

*Note: threshold must not equal 0*

ex:
```C
DB_Button buttons[] = {
	{.pin = 4, .threshold = 20},
	{.pin = 23, .threshold = 8}
};
```

3. Initialize debouncer

Declare an empty `DB_Handle` and run `DB_Init` using your handle, button array, button array length, and the GPIO reading function you wrote in step 1.

*Note: the `DB_Handle` must be declared in a scope where the call to `DB_Update` in the next step will still have access to it.*

ex:

```C
DB_Handle db; // may be declared in a higher scope
uint8_t count = sizeof(buttons)/sizeof(DB_Button); //calculate length of buttons array
DB_Init(&db, buttons, count, Read_GPIO, NULL); // optional arguments are left NULL
```

4. Call `DB_Update` at a relatively consistent interval.

Every time `DB_Update` is called, it will query every button in the `DB_Handle` you provided, and update the debounced state of each button. 

*Note: dramatically irregular or sparse timing of `DB_Update` calls may negatively affect debounce quality.*

ex:

```C
DB_Update(&db);
```