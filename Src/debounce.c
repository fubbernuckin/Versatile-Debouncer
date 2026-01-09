#include <stdint.h>
#include <stdbool.h>
#include <debounce.h>

enum _state_bit_mask {
	curr_state = 0x01,
	falling_edge = 0x02,
	rising_edge = 0x04
};

void DB_Init(DB_Handle *db, DB_Button *buttons, uint8_t count, DB_GPIO_Read rd, DB_Event_Callback cb) {
	for (int i = 0; i < count; i++) {
		bool in = rd(buttons[i].pin);
		buttons[i]._state = in;
		buttons[i]._counter = in * buttons[i].threshold;
	}
	db->btns = buttons;
	db->count = count;
	db->rd = rd;
	db->cb = cb;
}

void DB_Update(DB_Handle *db) {
	/*
	 * _state stores different flags in its bits
	 * 0b00000abc
	 * a: rising edge latch
	 * b: falling edge latch
	 * c: current state
	 * 0: undefined
	 */
	for (int i = 0; i < db->count; i++) {
		DB_Button *btn = &(db->btns[i]);

		// perform debounce update
		if (db->rd(btn->pin)) {
			if (btn->_counter < btn->threshold) {
				btn->_counter++;
			}
			else {
				if ((btn->_state & curr_state) == 0) {
					btn->_state = btn->_state | rising_edge; // set rising edge bit
					if (db->cb != NULL) {
						// event callback
						DB_Event ev = {
							.btn = btn,
							.ev_type = DB_RISING_EDGE
						};
						db->cb(ev);
					}
				}
				btn->_state = btn->_state | curr_state; // set state bit
			}
		}
		else {
			if (btn->_counter != 0) {
				btn->_counter--;
			}
			else {
				if ((btn->_state & curr_state) == 1) {
					btn->_state = btn->_state | falling_edge; // set falling edge bit
					if (db->cb != NULL) {
						// event callback
						DB_Event ev = {
							.btn = btn,
							.ev_type = DB_FALLING_EDGE
						};
						db->cb(ev);
					}
				}
				btn->_state = btn->_state & ~curr_state; // clear state bit
			}
		}
	}
}

bool DB_Rd(const DB_Button *btn) {
	return btn->_state & curr_state;
}

bool DB_Rising(DB_Button *btn) {
	if ((btn->_state & rising_edge) != 0) {
		btn->_state = btn->_state & ~rising_edge; // clear rising edge bit
		return true;
	}
	return false;
}

bool DB_Falling(DB_Button *btn) {
	if ((btn->_state & falling_edge) != 0) {
		btn->_state = btn->_state & ~falling_edge; // clear falling edge bit
		return true;
	}
	return false;
}

bool DB_Changed(DB_Button *btn) {
	if ((btn->_state & (rising_edge | falling_edge)) != 0) {
		btn->_state = btn->_state & ~(rising_edge | falling_edge); // clear rising and falling edge bits
		return true;
	}
	return false;
}

