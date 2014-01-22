#include <pebble.h>

Window *my_window;

void handle_init(void) {
	my_window = window_create();
	window_stack_push(my_window, true /* Animated */);
}

void handle_deinit(void) {
	  window_destroy(my_window);
}

int main(void) {
	  handle_init();
	  app_event_loop();
	  handle_deinit();
}
