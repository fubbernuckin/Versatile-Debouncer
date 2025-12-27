#include <stdint.h>
#include <stdbool.h>
#include <debounce.h>

enum _state_bit_mask {
	curr_state = 0x01,
	prev_state = 0x02,
	falling_edge = 0x04,
	rising_edge = 0x08
};

void DB_Init(DB_Handle *db, DB_Button *buttons, uint8_t count, DB_GPIO_Read rd) {
	for(int i = 0; i < count; i++) {
		bool in = rd(buttons[i].pin);
		buttons[i]._state = in;
		buttons[i]._counter = in * buttons[i].threshold;
	}
	db->btns = buttons;
	db->count = count;
	db->rd = rd;
}

void DB_Update(DB_Handle *db) {
	/*
	 * _state stores different flags in its bits
	 * 0b0000abcd
	 * a: rising edge latch
	 * b: falling edge latch
	 * c: previous state
	 * d: current state
	 * 0: undefined
	 */
	for(int i = 0; i < db->count; i++) {
		DB_Button *btn = &(db->btns[i]);

		btn->_state = (((btn->_state & curr_state) << 1) | (btn->_state & (~prev_state))); // set history bit while maintaining other bits

		// perform debounce update
		if(db->rd(btn->pin)) {
			if(btn->_counter < btn->threshold) {
				btn->_counter++;
			}
			else {
				btn->_state = btn->_state | curr_state; // set state bit
			}
		}
		else {
			if(btn->_counter != 0) {
				btn->_counter--;
			}
			else {
				btn->_state = btn->_state & ~curr_state; // clear state bit
			}
		}

		//set rising and falling edge flags
		if((btn->_state & curr_state) !=0) {
			if((btn->_state & prev_state) == 0) {
				// rising edge
				btn->_state = btn->_state | rising_edge;
			}
		}
		else {
			if((btn->_state & prev_state) !=0) {
				// falling edge
				btn->_state = btn->_state | falling_edge;
			}
		}
	}
}

bool DB_Rd(const DB_Button *btn) {
	return btn->_state & curr_state;
}

bool DB_Rising(DB_Button *btn) {
	if((btn->_state & rising_edge) != 0) {
		btn->_state = btn->_state & ~rising_edge; // clear rising edge bit
		return true;
	}
	return false;
}

bool DB_Falling(DB_Button *btn) {
	if((btn->_state & falling_edge) != 0) {
		btn->_state = btn->_state & ~falling_edge; // clear falling edge bit
		return true;
	}
	return false;
}
