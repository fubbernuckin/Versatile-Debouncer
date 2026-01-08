#include <stdint.h>
#include <stdbool.h>
#include <debounce.h>

enum _state_bit_mask {
	curr_state = 0x01,
	falling_edge = 0x02,
	rising_edge = 0x04
};

void DB_Init(DB_Handle *db, DB_Button *buttons, uint8_t count, DB_GPIO_Read rd) {
	for (int i = 0; i < count; i++) {
		bool in = rd(buttons[i].pin);
		buttons[i]._state = in;
		buttons[i]._counter = in * buttons[i].threshold;
	}
	db->btns = buttons;
	db->count = count;
	db->rd = rd;
	db->front = 0;
	db->back = 0;
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
					DB_Event ev = {
							.pin = btn->pin,
							.ev_type = RISING
					};
					_DB_Push_Event(db, ev);
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
					DB_Event ev = {
							.pin = btn->pin,
							.ev_type = FALLING
					};
					_DB_Push_Event(db, ev);
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

DB_Event *DB_Pop_Event(DB_Handle *db) {
	if (db->back == db->front) {
		return 0; // TODO: check if this is handled as expected in test context.
	}
	DB_Event *ev = &db->ev_queue[db->front];
	db->front = (db->front + 1) % EVENT_QUEUE_SIZE;
	return ev;
}

void _DB_Push_Event(DB_Handle *db, DB_Event ev) {
	db->ev_queue[db->back] = ev;
	db->back = (db->back + 1) % EVENT_QUEUE_SIZE;
	if (db->back == db->front) {
		db->front = (db->back + 1) % EVENT_QUEUE_SIZE; // overflow, dump event.
	}
}

