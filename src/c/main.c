#include <pebble.h>

// pointer to main window
static Window *s_main_window;

// pointer to main window layer
static Layer *s_main_window_layer;

// pointer to canvas layer
static Layer *s_canvas_layer;

// pointer to bitmap layer
static BitmapLayer *s_layer;

// pointer to bitmap
static GBitmap *s_bitmap;

// function to redraw the watch
static void canvas_update_proc(Layer *this_layer, GContext *ctx) {

  // Text buffers to hold the date and time strings
  static char s_time_text[] = "00:00 XX";
  static char s_date_text[] = "Xxxxxxxxx, Xxx 00";

  // Get the current time
  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);
  
  // Get the formatted time string
  char *time_format = clock_is_24h_style() ? "%R" : "%I:%M %p";
  strftime(s_time_text, sizeof(s_time_text), time_format, tick_time);

  // Handle lack of non-padded hour format string for twelve hour clock.
  if (!clock_is_24h_style() && (s_time_text[0] == '0')) {
    memmove(s_time_text, &s_time_text[1], sizeof(s_time_text) - 1);
  }
  
  // Get the formatted date string
  strftime(s_date_text, sizeof(s_date_text), "%A, %b %e", tick_time);

  // Set some offsets based on watch shape
  uint8_t time_y_offset = PBL_IF_ROUND_ELSE(50, 30);
  uint8_t date_y_offset = PBL_IF_ROUND_ELSE(70, 40);
  
  // Time to start drawing!
  
  // Get the current unobstructed bounds
  GRect unobstructed_bounds = layer_get_unobstructed_bounds(this_layer);
  
  // prep colors
  GColor8 outer = PBL_IF_COLOR_ELSE(GColorBlueMoon, GColorWhite);
  GColor8 inner = PBL_IF_COLOR_ELSE(GColorRed, GColorLightGray);
  
  // only need BatteryChargeState if we display the battery level
  #ifndef PBL_BW
    BatteryChargeState bcs = battery_state_service_peek();
    if(bcs.is_charging){
      outer = GColorIcterine;
    }else if(bcs.charge_percent == 100){
      outer = GColorBlueMoon;
    }else if(bcs.charge_percent <= 20){
      outer = GColorDarkCandyAppleRed;
    }else if(bcs.charge_percent <= 50){
      outer = GColorOrange;
    }else if(bcs.charge_percent <= 90){
      outer = GColorDarkGreen;
    }
  #else
    outer = GColorWhite;
  #endif
  inner = PBL_IF_COLOR_ELSE(connection_service_peek_pebble_app_connection() ? GColorDukeBlue : GColorRed,GColorLightGray);
  
  #if defined(PBL_ROUND)
    // outer
    graphics_context_set_fill_color(ctx, outer);
    graphics_fill_radial(ctx, grect_crop(unobstructed_bounds, 4), GOvalScaleModeFitCircle, 4, DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(360));
    // inner
    graphics_context_set_fill_color(ctx, inner);
    graphics_fill_radial(ctx, grect_crop(unobstructed_bounds, 12), GOvalScaleModeFitCircle, 4, DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(360));
  #else
    // No stroke width so have to draw 2 rectangles inside one another
    // outer
    graphics_context_set_fill_color(ctx, outer);
    graphics_fill_rect(ctx, GRect(0, 0,unobstructed_bounds.size.w, unobstructed_bounds.size.h), 0, 0);
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, GRect(4, 4,unobstructed_bounds.size.w-8, unobstructed_bounds.size.h-8), 0, 0);
    // inner
    graphics_context_set_fill_color(ctx, inner);
    graphics_fill_rect(ctx, GRect(8, 8, unobstructed_bounds.size.w-16, unobstructed_bounds.size.h-16), 0, 0);
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, GRect(12, 12, unobstructed_bounds.size.w-25, unobstructed_bounds.size.h-25), 0, 0);
  #endif
  
  // Use larger font for 24 hour time and smaller for 12 hour to accomodate for AM/PM
  GFont time_font = clock_is_24h_style() ? fonts_get_system_font(FONT_KEY_LECO_36_BOLD_NUMBERS) : fonts_get_system_font(FONT_KEY_LECO_26_BOLD_NUMBERS_AM_PM);
  
  // Draw the Time
  graphics_context_set_text_color(ctx, GColorWhite);
  graphics_draw_text( ctx, s_time_text, time_font, GRect(unobstructed_bounds.origin.x, unobstructed_bounds.origin.y+time_y_offset, unobstructed_bounds.size.w, unobstructed_bounds.size.h-time_y_offset), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
    
  // Draw the Date
  graphics_draw_text(ctx, s_date_text, fonts_get_system_font(FONT_KEY_GOTHIC_18), GRect(unobstructed_bounds.origin.x, unobstructed_bounds.size.h-date_y_offset, unobstructed_bounds.size.w, unobstructed_bounds.size.h-date_y_offset), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
  
}


// Tick handler to force update of watchface
static void tick_handler(struct tm *tick_time, TimeUnits units_changed){
   
  // Force canvas layer to redraw
  layer_mark_dirty(s_canvas_layer);
  
}

// Bluetooth handler to force update of watchface
static void bluetooth_handler(bool connected){
  // Force canvas layer to redraw
  layer_mark_dirty(s_canvas_layer);
}

// Battery handler to force update of watchface
static void battery_handler(BatteryChargeState charge_state){
  // Force canvas layer to redraw
  layer_mark_dirty(s_canvas_layer);
}

static void main_window_load(Window *window) {
  
  // get the main window layer
  s_main_window_layer = window_get_root_layer(s_main_window);
  
  // get main window bounds
  GRect bounds = layer_get_bounds(s_main_window_layer);

  // set the main window background color
  window_set_background_color(s_main_window, GColorBlack);
  
  // Create the layer we will draw on
  s_canvas_layer = layer_create(layer_get_bounds(s_main_window_layer));

  // Set the update procedure for our layer
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);

  // Add the layer to our main window layer
  layer_add_child(s_main_window_layer, s_canvas_layer);  
  
  // bitmap
  s_bitmap = gbitmap_create_with_resource(RESOURCE_ID_CENTER_PIECE_BLACK);
  GPoint center = grect_center_point(&bounds);
  GSize image_size = gbitmap_get_bounds(s_bitmap).size;

  GRect image_frame = GRect(center.x, center.y, image_size.w, image_size.h);
  image_frame.origin.x -= image_size.w / 2;
  image_frame.origin.y -= image_size.h / 2 - 15;
  
  s_layer = bitmap_layer_create(image_frame);
  bitmap_layer_set_bitmap(s_layer, s_bitmap);
  layer_add_child(s_main_window_layer, bitmap_layer_get_layer(s_layer));
  
  // Subscribe to event services
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  // no point in subscribing if we won't display it...
  #ifndef PBL_BW
    battery_state_service_subscribe(battery_handler);
    connection_service_subscribe((ConnectionHandlers) {
      .pebble_app_connection_handler = bluetooth_handler
    });
  #endif
}


static void main_window_unload(Window *window) {
  bitmap_layer_destroy(s_layer);
  gbitmap_destroy(s_bitmap);
  // Unsubscribe from event services
  tick_timer_service_unsubscribe();
  // not subscribed if in BW
  #ifndef PBL_BW
    battery_state_service_unsubscribe();
    connection_service_unsubscribe();
  #endif
}


static void init(void) {
  
  // Create the main window
  s_main_window = window_create();
    
  // set the window load and unload handlers
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload,
  });
  
  // show the window on screen
  window_stack_push(s_main_window, true);
  
}


static void deinit(void) {
  
  // Destroy the main window
  window_destroy(s_main_window);
  
}


int main(void) {
  
  init();
  app_event_loop();
  deinit();
  
}
