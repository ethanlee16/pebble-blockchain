#include <pebble.h>
#include "qrencode.h"
#include "jsmn.h"

static Window *s_main_window;
static Window *s_market_window, *s_new_wallet_window, *s_view_window;
static MenuLayer *s_menu_layer, *s_view_layer;
static TextLayer *s_title_text, *s_buy_text, *s_sell_text, *s_address_text;
static ScrollLayer *s_market_layer;
static Layer *s_new_wallet_layer;
static AppSync s_sync;
static QRcode *qr;
static uint8_t s_sync_buffer[64];
static char* wallets[100];
static char* balances[100];
static int walletCount = 0;

// Key values for AppMessage Dictionary
enum {
    STATUS_KEY = 0, 
    MESSAGE_KEY = 1,
    REQ = 2
};

enum BlockKey {
    BUYPRICE = 3,
    SELLPRICE = 4,
    BLOCKSMINED = 5,
    BLOCKSTOTAL = 6,
    BLOCKSIZE = 7,
    MINERREVENUE = 8,
    ADDRESS = 9,
    WALLETARRAY = 10
};

// Write message to buffer & send
void send_message(char* req){
    DictionaryIterator *iter;
    
    app_message_outbox_begin(&iter);
    dict_write_uint8(iter, STATUS_KEY, 0x1);
    dict_write_cstring(iter, REQ, req);
    
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

static void view_draw_header_handler(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
    menu_cell_basic_header_draw(ctx, cell_layer, "Addresses");
}

static void view_draw_row_handler(GContext *ctx, const Layer *cell_layer, MenuIndex *cell_index, void *callback_context) {
    menu_cell_basic_draw(ctx, cell_layer, wallets[cell_index->row], balances[cell_index->row], NULL);
}

static void sync_error_cb(DictionaryResult dict_error, AppMessageResult app_message_error, void *context) {
  APP_LOG(APP_LOG_LEVEL_DEBUG, "App Message Sync Error: %d", app_message_error);
}

static void sync_tuple_changed_cb(const uint32_t key, const Tuple* new_tuple, const Tuple* old_tuple, void* context) {
    switch(key) {
    case BUYPRICE:
    ;
        const char* bprice = new_tuple->value->cstring;
        const char* buy = "Buy Price: $";
        char* bmsg;
        bmsg = malloc(strlen(bprice) + strlen(buy) + 1);
        strcpy(bmsg, buy);
        strcat(bmsg, bprice);
        text_layer_set_text(s_buy_text, bmsg);
        break;
    case SELLPRICE:
    ;
        const char* sprice = new_tuple->value->cstring;
        const char* sell = "Sell Price: $";
        char* smsg;
        smsg = malloc(strlen(sprice) + strlen(sell) + 1);
        strcpy(smsg, sell);
        strcat(smsg, sprice);
        text_layer_set_text(s_sell_text, smsg);
        break;
    case ADDRESS:
    ;
        APP_LOG(APP_LOG_LEVEL_DEBUG, "%s", new_tuple->value->cstring);
        const char* _address = new_tuple->value->cstring;
        char* address;
        address = malloc(strlen(_address) + 1);
        strcpy(address, _address);
        qr = QRcode_encodeString(_address, 5, QR_ECLEVEL_H, QR_MODE_8, 1);

        APP_LOG(APP_LOG_LEVEL_DEBUG, "%d", qr->width);
        APP_LOG(APP_LOG_LEVEL_DEBUG, "%d", qr->data[114]);
        
        text_layer_set_text(s_address_text, address);
        layer_mark_dirty(s_new_wallet_layer);

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

    s_buy_text = text_layer_create(GRect(5, 45, bounds.size.w, 25));
    text_layer_set_font(s_buy_text, fonts_get_system_font(FONT_KEY_GOTHIC_24));
    layer_add_child(window_layer, text_layer_get_layer(s_buy_text));

    s_sell_text = text_layer_create(GRect(5, 70, bounds.size.w, 25));
    text_layer_set_font(s_sell_text, fonts_get_system_font(FONT_KEY_GOTHIC_24));
    layer_add_child(window_layer, text_layer_get_layer(s_sell_text));

    Tuplet initial_values[] = {
        TupletCString(BUYPRICE, "Loading..."),
        TupletCString(SELLPRICE, "Loading...")
    };
    
    app_sync_init(&s_sync, s_sync_buffer, sizeof(s_sync_buffer),
        initial_values, ARRAY_LENGTH(initial_values),
        sync_tuple_changed_cb, sync_error_cb, NULL);
    
    send_message("market");
}

static void market_window_unload(Window *win) {
    scroll_layer_destroy(s_market_layer);
    text_layer_destroy(s_title_text);
    text_layer_destroy(s_buy_text);
    text_layer_destroy(s_sell_text);
}
  
static void new_wallet_update_proc(Layer *layer, GContext* ctx) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Redraw!");
    int currX = 35;
    int currY = 80;

    if(qr->data) {
        for(int y = 0; y < qr->width; y++) {
            for(int x = 0; x < qr->width; x++) {
                if(qr->data[y * qr->width + x]&0x1) {
                    graphics_draw_rect(ctx, GRect(currX, currY, 2, 2));
                    graphics_fill_rect(ctx, GRect(currX, currY, 2, 2), 0, 0);
                }
                currX += 2;
            }
            currY += 2;
            currX = 35;
        }
    }
}


static void new_wallet_window_load(Window *win) {

    Layer *window_layer = window_get_root_layer(win);
    GRect bounds = layer_get_bounds(window_layer);
    s_new_wallet_layer = layer_create(bounds);
    layer_set_update_proc(s_new_wallet_layer, new_wallet_update_proc);
    layer_add_child(window_layer, s_new_wallet_layer);
    
    s_title_text = text_layer_create(GRect(5, 10, bounds.size.w, 30));
    text_layer_set_text(s_title_text, "Create Wallet");
    text_layer_set_text_alignment(s_title_text, GTextAlignmentLeft);
    text_layer_set_font(s_title_text, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
    layer_add_child(window_layer, text_layer_get_layer(s_title_text));

    s_address_text = text_layer_create(GRect(5, 45, bounds.size.w, 20));
    text_layer_set_font(s_address_text, fonts_get_system_font(FONT_KEY_GOTHIC_14));
    layer_add_child(window_layer, text_layer_get_layer(s_address_text)); 
    
    Tuplet initial_values[] = {
        TupletCString(ADDRESS, "Creating..."),
    };
    
    app_sync_init(&s_sync, s_sync_buffer, sizeof(s_sync_buffer),
        initial_values, ARRAY_LENGTH(initial_values),
        sync_tuple_changed_cb, sync_error_cb, NULL);
    
    send_message("createWallet"); 
}

static void new_wallet_window_unload(Window *win) {
    layer_destroy(s_new_wallet_layer);
    text_layer_destroy(s_title_text);
}

static void select_callback(struct MenuLayer *s_menu_layer, MenuIndex *cell_index, void *callback_context) {
    switch (cell_index->row) {
        case 0:
            window_stack_push(s_market_window, true);
        break;
        case 1:
            window_stack_push(s_new_wallet_window, true);
        break;
        case 2:
            window_stack_push(s_view_window, true);
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



static uint16_t view_get_num_sections(MenuLayer *menu_layer, void *data) {
  return 1;
}

static int16_t view_get_header_height(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static uint16_t view_get_num_rows(MenuLayer *menu_layer, uint16_t section_index, void *data) {
  return walletCount;
}

static void view_window_load(Window *win) {
    Layer *window_layer = window_get_root_layer(win);
    GRect bounds = layer_get_bounds(window_layer);

    s_view_layer = menu_layer_create(bounds);
    menu_layer_set_callbacks(s_menu_layer, NULL, (MenuLayerCallbacks){
        .get_num_rows = view_get_num_rows,
        .get_num_sections = view_get_num_sections,
        .get_header_height = view_get_header_height,
        .draw_header = view_draw_header_handler,
        .draw_row = view_draw_row_handler
    });
    menu_layer_set_click_config_onto_window(s_menu_layer, win);
    layer_add_child(window_layer, menu_layer_get_layer(s_menu_layer));

    send_message("viewWallets");
}

static void view_window_unload(Window *win) { 
    menu_layer_destroy(s_view_layer);
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
    }

    tuple = dict_find(received, BUYPRICE);
    if(tuple) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Received BUYPRICE: %s", tuple->value->cstring);
    }

    tuple = dict_find(received, WALLETARRAY);
    if(tuple) {
        APP_LOG(APP_LOG_LEVEL_DEBUG, "Received WALLETARRAY: %s", tuple->value->cstring);
        jsmn_parser par;
        jsmn_init(&par);
        jsmntok_t tokens[256];
        const char *js;
        jsmnerr_t r;


        js = tuple->value->cstring;
        r = jsmn_parse(&par, js, strlen(js), tokens, 256);
        if(r) {
            printf("Possible error found");
        }

        for(size_t i = 0; i < sizeof(tokens); i++) {
            char *jsn = malloc(tokens[i].end - tokens[i].start + 1);
            jsmntok_t tok[10];
            strncpy(jsn, js+tokens[i].start, tokens[i].end - tokens[i].start);

            r = jsmn_parse(&par, jsn, strlen(jsn), tok, 10);
            char *address = malloc(tok[6].end - tok[6].start + 1);
            char *balance = malloc(tok[1].end - tok[1].start + 1);
            strncpy(address, jsn+tok[6].start, tok[6].end - tok[6].start);
            strncpy(balance, jsn+tok[1].start, tok[1].end - tok[1].start);
            wallets[walletCount] = address;
            balances[walletCount] = balance;
            walletCount++;
        }
    }
}

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

    s_new_wallet_window = window_create();
    window_set_window_handlers(s_new_wallet_window, (WindowHandlers) {
        .load = new_wallet_window_load,
        .unload = new_wallet_window_unload
    });

    s_view_window = window_create();
    window_set_window_handlers(s_view_window, (WindowHandlers) {
        .load = view_window_load,
        .unload = view_window_unload
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