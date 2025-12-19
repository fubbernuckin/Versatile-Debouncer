#include <stdint.h>
#include <stdbool.h>
#include <debounce.h>

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
	for(int i = 0; i < db->count; i++) {
		DB_Button *btn = &(db->btns[i]);
		if(db->rd(btn->pin)) {
			if(btn->_counter < btn->threshold) {
				btn->_counter++;
			}
			else {
				btn->_state = true;
			}
		}
		else {
			if(btn->_counter != 0) {
				btn->_counter--;
			}
			else {
				btn->_state = false;
			}
		}
	}
}

bool DB_Rd(const DB_Button *button) {
	return button->_state;
}
