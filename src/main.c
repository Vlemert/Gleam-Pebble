#include <pebble.h>

Window *splashWindow;
static BitmapLayer *splashBitmapLayer;
static GBitmap *splashBitmap;

Window *setupWindow;
static BitmapLayer *setupBitmapLayer;
static GBitmap *setupBitmap;

Window *mainWindow;
Window *groupWindow;

// Message payloads
enum {
  	GLEAM_LOADMAINMENU = 0x0,
	GLEAM_JAVASCRIPTREADY = 0x1,
	GLEAM_GROUPCOUNT = 0x2,
	GLEAM_ANYLIGHTON = 0x3,
	GLEAM_LOADGROUPMENU = 0x4,
	GLEAM_OPENGROUPMENU = 0x5,
	GLEAM_SETALLLIGHTSONSTATE = 0x6,
	GLEAM_GROUPOFFSET = 0x14, // 20
	GLEAM_PRESETOFFSET = 0x64 // 100 (this allows for 80 groups, code can currently store 40 so this should suffice)
};

// Menu types
enum {
	MENU_MAIN = 0x0,
	MENU_GROUP = 0x1
};


typedef struct {
	int presetId;
	char presetName[20];
} preset;

typedef struct {
	int groupId;
	char groupName[20];
	bool anyLightOn;
} groupState;

typedef struct {
	bool anyLightOn;
	
	groupState groupStates[40];
	int currentGroup;
	
	preset presets[40];
} appState;


appState gleamAppState;


MenuLayer *mainMenuLayer;
MenuLayer *groupMenuLayer;



char *translate_error(AppMessageResult result) {
  switch (result) {
    case APP_MSG_OK: return "APP_MSG_OK";
    case APP_MSG_SEND_TIMEOUT: return "APP_MSG_SEND_TIMEOUT";
    case APP_MSG_SEND_REJECTED: return "APP_MSG_SEND_REJECTED";
    case APP_MSG_NOT_CONNECTED: return "APP_MSG_NOT_CONNECTED";
    case APP_MSG_APP_NOT_RUNNING: return "APP_MSG_APP_NOT_RUNNING";
    case APP_MSG_INVALID_ARGS: return "APP_MSG_INVALID_ARGS";
    case APP_MSG_BUSY: return "APP_MSG_BUSY";
    case APP_MSG_BUFFER_OVERFLOW: return "APP_MSG_BUFFER_OVERFLOW";
    case APP_MSG_ALREADY_RELEASED: return "APP_MSG_ALREADY_RELEASED";
    case APP_MSG_CALLBACK_ALREADY_REGISTERED: return "APP_MSG_CALLBACK_ALREADY_REGISTERED";
    case APP_MSG_CALLBACK_NOT_REGISTERED: return "APP_MSG_CALLBACK_NOT_REGISTERED";
    case APP_MSG_OUT_OF_MEMORY: return "APP_MSG_OUT_OF_MEMORY";
    case APP_MSG_CLOSED: return "APP_MSG_CLOSED";
    case APP_MSG_INTERNAL_ERROR: return "APP_MSG_INTERNAL_ERROR";
    default: return "UNKNOWN ERROR";
  }
}

void resetGroupStates() {
	for (uint i = 0; i < sizeof(gleamAppState.groupStates)/sizeof(groupState); i++) {
		gleamAppState.groupStates[i].groupId = -1;
	}
}

void resetPresetStates() {
	for (uint i = 0; i < sizeof(gleamAppState.presets)/sizeof(preset); i++) {
		gleamAppState.presets[i].presetId = -1;
	}
}

static void in_received_handler(DictionaryIterator *iter, void *context) {
	Tuple *javascriptReadyTuple = dict_find(iter, GLEAM_JAVASCRIPTREADY);
	Tuple *loadMainMenuTuple = dict_find(iter, GLEAM_LOADMAINMENU);
	Tuple *groupCountTuple = dict_find(iter, GLEAM_GROUPCOUNT);
	Tuple *anyLightOnTuple = dict_find(iter, GLEAM_ANYLIGHTON);
	Tuple *loadGroupMenuTuple = dict_find(iter, GLEAM_LOADGROUPMENU);
	Tuple *openGroupMenuTuple = dict_find(iter, GLEAM_OPENGROUPMENU);
	
	APP_LOG(APP_LOG_LEVEL_DEBUG, "message received");
	
	if (javascriptReadyTuple) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "javascript started");
		
		switch (javascriptReadyTuple->value->uint16) {
			case 0:
				window_stack_push(setupWindow, true);
				window_stack_remove(splashWindow, false);
			break;
			case 1:
				window_stack_push(mainWindow, true);
				window_stack_remove(splashWindow, false);
				APP_LOG(APP_LOG_LEVEL_DEBUG, "menu loaded");
			break;
		}
	}
	
	if (loadMainMenuTuple) {
    	
	}
	
	if (groupCountTuple) {
		resetGroupStates();
		
		for (int i = 0; i < groupCountTuple->value->uint16; i++) {
			gleamAppState.groupStates[i].groupId = i;
			
			Tuple *groupNameTuple = dict_find(iter, GLEAM_GROUPOFFSET + i);
			
			memcpy(gleamAppState.groupStates[i].groupName, groupNameTuple->value->cstring, groupNameTuple->length);
		}
		
		 menu_layer_reload_data(mainMenuLayer);
	}
	
	if (anyLightOnTuple) {
		gleamAppState.anyLightOn = (bool)anyLightOnTuple->value->uint16;
		
		menu_layer_reload_data(mainMenuLayer);
	}
	
	if (loadGroupMenuTuple) {
		// Reload the group menu data
		resetPresetStates();
		
		for (int i = 0; i < loadGroupMenuTuple->value->uint16; i++) {
			gleamAppState.presets[i].presetId = i;
			
			Tuple *presetNameTuple = dict_find(iter, GLEAM_PRESETOFFSET + i);
			
			memcpy(gleamAppState.presets[i].presetName, presetNameTuple->value->cstring, presetNameTuple->length);
		}		
		
		menu_layer_reload_data(groupMenuLayer);
	}
	
	if (openGroupMenuTuple) {
		APP_LOG(APP_LOG_LEVEL_DEBUG, "show group menu");
		// Show the group menu
		MenuIndex index;
    	index.row = 0;
		index.section = 0;
		menu_layer_set_selected_index(groupMenuLayer, index, MenuRowAlignCenter, false);
		window_stack_push(groupWindow, true);
	}
}

static void in_dropped_handler(AppMessageResult reason, void *context) {
  	APP_LOG(APP_LOG_LEVEL_DEBUG, translate_error(reason));
}

static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
  	APP_LOG(APP_LOG_LEVEL_DEBUG, translate_error(reason));
	
	// TODO: only do this when the main menu is not loaded (splash screen)
	//loadMainMenu();
	
	// TODO: maybe build in a retry function for all messages?
}

static void out_sent_handler(DictionaryIterator *iterator, void *context) {
	APP_LOG(APP_LOG_LEVEL_DEBUG, "pebble message sent");
}

static void app_message_init(void) {
	// Register message handlers
	app_message_register_inbox_received(in_received_handler);
	app_message_register_inbox_dropped(in_dropped_handler);
	app_message_register_outbox_failed(out_failed_handler);
	app_message_register_outbox_sent(out_sent_handler);
	// Init buffers
	app_message_open(128, 64);
	APP_LOG(APP_LOG_LEVEL_DEBUG, "message init done");
}

void loadMainMenu() {
	Tuplet loadmainmenu_tuple = TupletInteger(GLEAM_LOADMAINMENU, 1);

  	DictionaryIterator *iter;
  	app_message_outbox_begin(&iter);

	if (iter == NULL) {
    	return;
	}

    dict_write_tuplet(iter, &loadmainmenu_tuple);
    dict_write_end(iter);

    app_message_outbox_send();	
}

void loadGroupMenu(int groupId) {
	gleamAppState.currentGroup = groupId;
	
	APP_LOG(APP_LOG_LEVEL_DEBUG, "request group menu");
	//app_message_init();
	
	Tuplet loadgroupmenu_tuple = TupletInteger(GLEAM_LOADGROUPMENU, groupId);
	
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	
	if (iter == NULL) {
		return;
	}
	
	dict_write_tuplet(iter, &loadgroupmenu_tuple);
	dict_write_end(iter);
	
	app_message_outbox_send();
}

void setAllLightsOnState(bool on) {
	int onState = 0;
	
	if (on) {
		onState = 1;
	}
	
	Tuplet loadgroupmenu_tuple = TupletInteger(GLEAM_SETALLLIGHTSONSTATE, onState);
	
	DictionaryIterator *iter;
	app_message_outbox_begin(&iter);
	
	if (iter == NULL) {
		return;
	}
	
	dict_write_tuplet(iter, &loadgroupmenu_tuple);
	dict_write_end(iter);
	
	app_message_outbox_send();
}

void mainMenu_select_click(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context)
{
	switch (cell_index->section) {
		case 0: // "all lights"
		// TODO: make this behavior a setting
		setAllLightsOnState(!gleamAppState.anyLightOn);
		break;
		case 1: // "groups"
		// Load selected group
		loadGroupMenu(cell_index->row);
		break;
	}
}
void mainMenu_select_long_click(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
	switch (cell_index->section) {
		case 0: // "all lights"
		// TODO: make this behavior a setting
		
		break;
		case 1: // "groups"
		break;
	}
}
void mainMenu_draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context)
{ // Adding the row number as text on the row cell.
	graphics_context_set_text_color(ctx, GColorBlack); // This is important.
	
	switch (cell_index->section) {
		case 0:
		// TODO: give user the option to change this behavior
		if (gleamAppState.anyLightOn) {
			graphics_draw_text(ctx, "Turn off", fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(20,0,layer_get_frame(cell_layer).size.w,layer_get_frame(cell_layer).size.h), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
		} else {
			graphics_draw_text(ctx, "Turn on", fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(20,0,layer_get_frame(cell_layer).size.w,layer_get_frame(cell_layer).size.h), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
		}		
		break;
		case 1:
			graphics_draw_text(ctx, gleamAppState.groupStates[cell_index->row].groupName, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(20,0,layer_get_frame(cell_layer).size.w,layer_get_frame(cell_layer).size.h), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
		break;
	}
	
	//graphics_draw_text(ctx, hex+2*cell_index->row, fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(0,0,layer_get_frame(cell_layer).size.w,layer_get_frame(cell_layer).size.h), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
	// Just saying layer_get_frame(cell_layer) for the 4th argument doesn't work.  Probably because the GContext is relative to the cell already, but the cell_layer.frame is relative to the menulayer or the screen or something.
}
void mainMenu_draw_header(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *callback_context)
{
	graphics_context_set_text_color(ctx, GColorBlack);
	
	switch (section_index) {
		case 0:
		graphics_draw_text(ctx, "all lights", fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GRect(0,0,layer_get_frame(cell_layer).size.w,layer_get_frame(cell_layer).size.h), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
		break;
		case 1:
		graphics_draw_text(ctx, "groups", fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GRect(0,0,layer_get_frame(cell_layer).size.w,layer_get_frame(cell_layer).size.h), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
		break;
	}
}
int16_t mainMenu_get_header_height(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context)
{
	return 20;
}
int16_t mainMenu_get_cell_height(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context)
{
	return 30;
}
uint16_t mainMenu_get_num_rows_in_section(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context)
{
	int groupCount = 0;
	switch (section_index) {
		case 0:
		return 1;
		break;
		case 1:
		for (uint i = 0; i < sizeof(gleamAppState.groupStates)/sizeof(groupState); i++) {
			if (gleamAppState.groupStates[i].groupId != -1) {
				groupCount += 1;
			}
		}
		return groupCount;
		break;
	}
	return 0;
}
uint16_t mainMenu_get_num_sections(struct MenuLayer *menu_layer, void *callback_context)
{
	return 2;
}


void groupMenu_select_click(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context)
{
	switch (cell_index->section) {
		case 0: // "group"
		// TODO: make this behavior a setting
		
		break;
		case 1: // "presets"
		// Launch selected preset
		break;
	}
}
void groupMenu_select_long_click(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context) {
	switch (cell_index->section) {
		case 0: // "group"
		// TODO: make this behavior a setting
		
		break;
		case 1: // "presets"
		break;
	}
}
void groupMenu_draw_row(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context)
{
	graphics_context_set_text_color(ctx, GColorBlack); // This is important.
	
	switch (cell_index->section) {
		case 0:
		// TODO: give user the option to change this behavior
		if (gleamAppState.groupStates[gleamAppState.currentGroup].anyLightOn) {
			graphics_draw_text(ctx, "Turn off", fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(20,0,layer_get_frame(cell_layer).size.w,layer_get_frame(cell_layer).size.h), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
		} else {
			graphics_draw_text(ctx, "Turn on", fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(20,0,layer_get_frame(cell_layer).size.w,layer_get_frame(cell_layer).size.h), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
		}		
		break;
		case 1:
			graphics_draw_text(ctx, gleamAppState.presets[cell_index->row].presetName, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD), GRect(20,0,layer_get_frame(cell_layer).size.w,layer_get_frame(cell_layer).size.h), GTextOverflowModeTrailingEllipsis, GTextAlignmentLeft, NULL);
		break;
	}
	
	//graphics_draw_text(ctx, hex+2*cell_index->row, fonts_get_system_font(FONT_KEY_GOTHIC_14), GRect(0,0,layer_get_frame(cell_layer).size.w,layer_get_frame(cell_layer).size.h), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
	// Just saying layer_get_frame(cell_layer) for the 4th argument doesn't work.  Probably because the GContext is relative to the cell already, but the cell_layer.frame is relative to the menulayer or the screen or something.
}
void groupMenu_draw_header(GContext *ctx, const Layer *cell_layer, uint16_t section_index, void *callback_context)
{
	graphics_context_set_text_color(ctx, GColorBlack);
	
	switch (section_index) {
		case 0:
		graphics_draw_text(ctx, gleamAppState.groupStates[gleamAppState.currentGroup].groupName, fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GRect(0,0,layer_get_frame(cell_layer).size.w,layer_get_frame(cell_layer).size.h), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
		break;
		case 1:
		graphics_draw_text(ctx, "presets", fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD), GRect(0,0,layer_get_frame(cell_layer).size.w,layer_get_frame(cell_layer).size.h), GTextOverflowModeTrailingEllipsis, GTextAlignmentCenter, NULL);
		break;
	}
}
int16_t groupMenu_get_header_height(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context)
{
	return 20;
}
int16_t groupMenu_get_cell_height(struct MenuLayer *menu_layer, MenuIndex *cell_index, void *callback_context)
{
	return 30;
}
uint16_t groupMenu_get_num_rows_in_section(struct MenuLayer *menu_layer, uint16_t section_index, void *callback_context)
{
	int presetCount = 0;
	switch (section_index) {
		case 0:
		return 1;
		break;
		case 1:
		for (uint i = 0; i < sizeof(gleamAppState.presets)/sizeof(preset); i++) {
			if (gleamAppState.presets[i].presetId != -1) {
				presetCount += 1;
			}
		}
		return presetCount;
		break;
	}
	return 0;
}
uint16_t groupMenu_get_num_sections(struct MenuLayer *menu_layer, void *callback_context)
{
	return 2;
}

MenuLayerCallbacks mainMenuCbacks;
MenuLayerCallbacks groupMenuCbacks;

void setupMenus() {	
	GRect frame;
		
	frame = layer_get_bounds(window_get_root_layer(mainWindow));

	mainMenuLayer = menu_layer_create(GRect(0,0,frame.size.w,frame.size.h));
	
	menu_layer_set_click_config_onto_window(mainMenuLayer, mainWindow);
	
	// Set all of our callbacks.
	mainMenuCbacks.get_num_sections = &mainMenu_get_num_sections;
	mainMenuCbacks.get_num_rows = &mainMenu_get_num_rows_in_section;
	//mainMenuCbacks.get_cell_height = &mainMenu_get_cell_height;
	mainMenuCbacks.get_header_height = &mainMenu_get_header_height;
	mainMenuCbacks.select_click = &mainMenu_select_click;
	mainMenuCbacks.draw_row = &mainMenu_draw_row;
	mainMenuCbacks.draw_header = &mainMenu_draw_header;
	// mainMenuCbacks.selection_changed = &func(struct MenuLayer *menu_layer, MenuIndex new_index, MenuIndex old_index, void *callback_context) // I assume this would be called whenever an up or down button was pressed.
	mainMenuCbacks.select_long_click = &mainMenu_select_long_click;
	menu_layer_set_callbacks(mainMenuLayer, NULL, mainMenuCbacks);
	
	
	frame = layer_get_bounds(window_get_root_layer(groupWindow));
	
	groupMenuLayer = menu_layer_create(GRect(0,0,frame.size.w,frame.size.h));
	
	menu_layer_set_click_config_onto_window(groupMenuLayer, groupWindow);
	
	groupMenuCbacks.get_num_sections = &groupMenu_get_num_sections;
	groupMenuCbacks.get_num_rows = &groupMenu_get_num_rows_in_section;
	//groupMenuCbacks.get_cell_height = &groupMenu_get_cell_height;
	groupMenuCbacks.get_header_height = &groupMenu_get_header_height;
	groupMenuCbacks.select_click = &groupMenu_select_click;
	groupMenuCbacks.draw_row = &groupMenu_draw_row;
	groupMenuCbacks.draw_header = &groupMenu_draw_header;
	// groupMenuCbacks.selection_changed = &func(struct MenuLayer *menu_layer, MenuIndex new_index, MenuIndex old_index, void *callback_context) // I assume this would be called whenever an up or down button was pressed.
	groupMenuCbacks.select_long_click = &groupMenu_select_long_click;
	menu_layer_set_callbacks(groupMenuLayer, NULL, groupMenuCbacks);
	
}





void splashLayerDraw(Layer *layer, GContext *ctx) {
  	GRect bounds = layer_get_frame(layer);
	splashBitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SPLASHSCREEN_WHITE);
	splashBitmapLayer = bitmap_layer_create(bounds);
	bitmap_layer_set_bitmap(splashBitmapLayer, splashBitmap);
	bitmap_layer_set_alignment(splashBitmapLayer, GAlignTop);
	layer_add_child(layer, bitmap_layer_get_layer(splashBitmapLayer));
	
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
}

void setupLayerDraw(Layer *layer, GContext *ctx) {
  	GRect bounds = layer_get_frame(layer);
	setupBitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SETUPSCREEN);
	setupBitmapLayer = bitmap_layer_create(bounds);
	bitmap_layer_set_bitmap(setupBitmapLayer, setupBitmap);
	bitmap_layer_set_alignment(setupBitmapLayer, GAlignTop);
	layer_add_child(layer, bitmap_layer_get_layer(setupBitmapLayer));
	
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, bounds, 0, GCornerNone);
}

void handle_init(void) {
	// Init state
	resetGroupStates();
	resetPresetStates();
	
	// Initialize messaging (automatically loads main menu)
	app_message_init();
	
	// Initialize all windows
	splashWindow = window_create();
	window_set_fullscreen(splashWindow, true);
	
	setupWindow = window_create();
	window_set_fullscreen(setupWindow, true);
	
	mainWindow = window_create();
	groupWindow = window_create();
	
	// Splash screen image
	Layer *splashLayer = window_get_root_layer(splashWindow);
	layer_set_update_proc(splashLayer, splashLayerDraw);
	
	// Setup screen update proc
	Layer *setupLayer = window_get_root_layer(setupWindow);
	layer_set_update_proc(setupLayer, setupLayerDraw);
	
	
	// Create menus and set up handlers
	setupMenus();
	// Add the main menu to the main window
	layer_add_child(window_get_root_layer(mainWindow), menu_layer_get_layer(mainMenuLayer));
	// Add the gruop menu to the group window
	layer_add_child(window_get_root_layer(groupWindow), menu_layer_get_layer(groupMenuLayer));
	
	// Show splash screen
	window_stack_push(splashWindow, true /* Animated */);
}

void handle_deinit(void) {
	gbitmap_destroy(splashBitmap);
	bitmap_layer_destroy(splashBitmapLayer);
	window_destroy(splashWindow);
	
	gbitmap_destroy(setupBitmap);
	bitmap_layer_destroy(setupBitmapLayer);
	window_destroy(setupWindow);
	
	menu_layer_destroy(mainMenuLayer);
	window_destroy(mainWindow);
	
	menu_layer_destroy(groupMenuLayer);
	window_destroy(groupWindow);
}

int main(void) {
	  handle_init();
	  app_event_loop();
	  handle_deinit();
}
