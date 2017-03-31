#include <pebble.h>

// pointer to main window
static Window *s_main_window;

// pointer to main window layer
static Layer *s_main_window_layer;

// pointer to canvas layer
static Layer *s_canvas_layer;

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
  GColor8 outer = GColorBlueMoon;
  GColor8 inner = GColorRed;
  BatteryChargeState bcs = battery_state_service_peek();
  if(bcs.is_charging){
    outer = GColorIcterine;
  }else if(bcs.charge_percent == 100){
    outer = GColorBlueMoon;
  }else if(bcs.charge_percent <= 10){
    outer = GColorDarkCandyAppleRed;
  }else if(bcs.charge_percent <= 30){
    outer = GColorOrange;
  }else if(bcs.charge_percent <= 50){
    outer = GColorYellow;
  }else if(bcs.charge_percent <= 70){
    outer = GColorGreen;
  }else if(bcs.charge_percent <= 90){
    outer = GColorDarkGreen;
  }
  inner = connection_service_peek_pebble_app_connection() ? GColorDukeBlue : GColorRed;
  
  // warning removal hax
  if(outer == GColorRed){}
  if(inner == GColorRed){}
  
  // Draw the outer Border
  #if defined(PBL_ROUND)
    graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(outer, GColorWhite));
    graphics_fill_radial(ctx, grect_crop(unobstructed_bounds, 4), GOvalScaleModeFitCircle, 4, DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(360));
  #else
    // No stroke width so have to draw 2 rectangles inside one another
    graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(outer, GColorWhite));
    graphics_fill_rect(ctx, GRect(0, 0,unobstructed_bounds.size.w, unobstructed_bounds.size.h), 0, 0);
    graphics_context_set_fill_color(ctx, GColorBlack);
    graphics_fill_rect(ctx, GRect(4, 4,unobstructed_bounds.size.w-8, unobstructed_bounds.size.h-8), 0, 0);
  #endif
  
  // Draw the inner Border
  #if defined(PBL_ROUND)
    graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(inner, GColorLightGray));
    graphics_fill_radial(ctx, grect_crop(unobstructed_bounds, 12), GOvalScaleModeFitCircle, 4, DEG_TO_TRIGANGLE(0), DEG_TO_TRIGANGLE(360));
  #else
    // No stroke width so have to draw 2 rectangles inside one another
    graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(inner, GColorLightGray));
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

static void handle_bluetooth(bool connected){
  layer_mark_dirty(s_canvas_layer);
}

static void handle_battery(BatteryChargeState charge_state){
  layer_mark_dirty(s_canvas_layer);
}

static void main_window_load(Window *window) {
  
  // get the main window layer
  s_main_window_layer = window_get_root_layer(s_main_window);

  // set the main window background color
  window_set_background_color(s_main_window, GColorBlack);
  
  // Create the layer we will draw on
  s_canvas_layer = layer_create(layer_get_bounds(s_main_window_layer));

  // Set the update procedure for our layer
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);

  // Add the layer to our main window layer
  layer_add_child(s_main_window_layer, s_canvas_layer);  
  
  // Subscribe to event services
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  battery_state_service_subscribe(handle_battery);
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = handle_bluetooth
  });
}


static void main_window_unload(Window *window) {
  
  // Unsubscribe from event services
  tick_timer_service_unsubscribe();
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
