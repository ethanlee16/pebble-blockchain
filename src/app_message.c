#include <pebble.h>

static Window *s_main_window;
static Window *s_market_window;
static MenuLayer *s_menu_layer;
static TextLayer *s_title_text, *s_price_text;
static ScrollLayer *s_market_layer;
static AppSync s_sync;
static uint8_t s_sync_buffer[64];

// Key values for AppMessage Dictionary
enum {
    STATUS_KEY = 0, 
    MESSAGE_KEY = 1,
    REQ = 2
};

// Write message to buffer & send
void send_message(void){
    DictionaryIterator *iter;
    
    app_message_outbox_begin(&iter);
    dict_write_uint8(iter, STATUS_KEY, 0x1);
    dict_write_cstring(iter, REQ, "market");
    
    dict_write_end(iter);
    app_message_outbox_send();
}

static void draw_header_handler(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
    menu_cell_basic_header_draw(ctx, cell_layer, "Blockchain for Pebble");
}

static void draw_row_handler(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context) {
    switch (cell_index->row) {
        case 0:
            menu_cell_basic_draw(ctx, cell_layer, "Market Price", "Current price of Bitcoin", NULL);
        break;
        case 1:
            menu_cell_basic_draw(ctx, cell_layer, "Create Wallet", "powered by Blockchain", NULL);
        break;
        case 2:
            menu_cell_basic_draw(ctx, cell_layer, "View Wallets", "View created wallets", NULL);
        break;
    }
}

static void sync_error_cb(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d", app_message_error);
}

static void sync_tuple_changed_cb(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
    switch(key) {
    case 0x0:
        text_layer_set_text(s_price_text, new_tuple->value->cstring);
        break;
    }
}

static void market_window_load(Window *win) {
    Layer *window_layer = window_get_root_layer(win);
    GRect bounds = layer_get_bounds(window_layer);
    s_market_layer = scroll_layer_create(bounds);

    s_title_text = text_layer_create(GRect(5, 10, bounds.size.w, 30));
    text_layer_set_text(s_title_text, "Market Price");
    text_layer_set_text_alignment(s_title_text, GTextAlignmentLeft);
    text_layer_set_font(s_title_text, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
    layer_add_child(window_layer, text_layer_get_layer(s_title_text));

    Tuplet initial_values[] = {
        TupletCString(0x0, "0") 
    };

    

    app_sync_init(&s_sync, s_sync_buffer, sizeof(s_sync_buffer),
        initial_values, ARRAY_LENGTH(initial_values),
        sync_tuple_changed_cb, sync_error_cb, NULL);

    send_message();
}

static void market_window_unload(Window *win) {
    scroll_layer_destroy(s_market_layer);
    text_layer_destroy(s_title_text);
    text_layer_destroy(s_price_text);
}

static void select_callback(struct MenuLayer *s_menu_layer, MenuIndex *cell_index, void *callback_context) {
    switch (cell_index->row) {
        case 0:
            window_stack_push(s_market_window, true);
        break;
        case 1:
        break;
        case 2:
        break;
    }
}

static uint16_t menu_get_num_sections(MenuLayer *menu_layer, void *data) {
  return 1;
}

static int16_t menu_get_header_height(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static uint16_t menu_get_num_rows(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return 3;
}

static void menu_window_load(Window *win) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "menu load");

    Layer *window_layer = window_get_root_layer(win);
    GRect bounds = layer_get_bounds(window_layer);

    s_menu_layer = menu_layer_create(bounds);
    menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks){
        .get_num_rows = menu_get_num_rows,
        .get_num_sections = menu_get_num_sections,
        .get_header_height = menu_get_header_height,
        .draw_header = draw_header_handler,
        .draw_row = draw_row_handler,
        .select_click = select_callback
    }); 
    menu_layer_set_click_config_onto_window(s_menu_layer, win);
    layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));
}

static void menu_window_unload(Window *win) { 
    menu_layer_destroy(s_menu_layer);
}

// Called when a message is received from PebbleKitJS
static void in_received_handler(DictionaryIterator *received, void *context) {
    Tuple *tuple;
    
    tuple = dict_find(received, STATUS_KEY);
    if(tuple) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Received Status: %d", (int)tuple->value->uint32); 
    }
    
    tuple = dict_find(received, MESSAGE_KEY);
    if(tuple) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Received Message: %s", tuple->value->cstring);
    }}

// Called when an incoming message from PebbleKitJS is dropped
static void in_dropped_handler(AppMessageResult reason, void *context) {    
}

// Called when PebbleKitJS does not acknowledge receipt of a message
static void out_failed_handler(DictionaryIterator *failed, AppMessageResult reason, void *context) {
}

void init(void) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "init");

    s_main_window = window_create();
    window_set_window_handlers(s_main_window, (WindowHandlers) {
        .load = menu_window_load,
        .unload = menu_window_unload
    });

    window_stack_push(s_main_window, true);

    s_market_window = window_create();
    window_set_window_handlers(s_market_window, (WindowHandlers) {
        .load = market_window_load,
        .unload = market_window_unload
    });
    
    // Register AppMessage handlers
    app_message_register_inbox_received(in_received_handler); 
    app_message_register_inbox_dropped(in_dropped_handler); 
    app_message_register_outbox_failed(out_failed_handler);
        
    app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
    
    //send_message();
}

void deinit(void) {
    app_message_deregister_callbacks();
    window_destroy(s_main_window);
}

int main( void ) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "start");
    init();
    app_event_loop();
    deinit();
}