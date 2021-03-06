// Standard includes
#include "pebble.h"

// --------- Global constants
static const int kSpace = 4;
static const int kDotRadius = 12;

static const int kMonthColumn = 0;
static const int kDayOfMonthColumn = 1;
static const int kHourColumn = 2;
static const int kMinuteColumn = 3;
static const int kSecondColumn = 4;

static const int kWeekDayLine = 5;  // 6th line
// --------- End of global constants

// --------- Globals
static Window *s_window;

// Layer containing all the circles (LEDs)
static Layer *s_time_layer;

// Layer showing the battery status
static Layer *s_battery_layer;

// Layer showing the connected status
static Layer *s_connected_layer;

// Current state of the battery (default 0%, not charging, not plugged)
static BatteryChargeState s_battery_current = {
  .charge_percent = 0,
  .is_charging = false,
  .is_plugged = false
};

// Current state of the phone connection (default false)
static bool s_connected_current = false;
// --------- End of globals

// --------- Function definitions
// Updates the layer containing all LEDs when the time changes
static void time_update(Layer *me, GContext *ctx);

// Updates the battery layer when the battery changes
static void battery_update(Layer *me, GContext *ctx);

// Updates the connection status
static void connected_update(Layer *me, GContext *ctx);

// Gets the center of a specific LED
static GPoint getCenter(int col, int row);

// Changes the state of a given LED to on (on=true) or off (on=false)
static void toggle_led(GContext *ctx, int col, int row, bool on);
// --------- End of function definitions

// Called once per second
static void handle_second_tick(struct tm* tick_time, TimeUnits units_changed) {
  layer_mark_dirty(s_time_layer);
}

// Called when battery changes
static void handle_battery(BatteryChargeState charge_state) {
  s_battery_current = charge_state;
  layer_mark_dirty(s_battery_layer);
}

// Called when connection status changes
static void handle_connected(bool connected) {
  s_connected_current = connected;
  layer_mark_dirty(s_connected_layer);
}

// Handle the start-up of the app
static void init(void) {
  // Create our app's base window
  s_window = window_create();
  window_stack_push(s_window, true /* Animated */);
  window_set_background_color(s_window, GColorBlack);

  Layer *root_layer = window_get_root_layer(s_window);
  GRect root_frame = layer_get_frame(root_layer);

  s_time_layer = layer_create(root_frame);
  s_battery_layer = layer_create(root_frame);
  s_connected_layer = layer_create(root_frame);

  layer_add_child(root_layer, s_time_layer);
  layer_add_child(root_layer, s_battery_layer);
  layer_add_child(root_layer, s_connected_layer);

  layer_set_update_proc(s_time_layer, &time_update);
  layer_set_update_proc(s_connected_layer, &connected_update);
  layer_set_update_proc(s_battery_layer, &battery_update);

  tick_timer_service_subscribe(SECOND_UNIT, &handle_second_tick);
  battery_state_service_subscribe(&handle_battery);
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = &handle_connected
  });

  // Ensures time is displayed immediately (will break if NULL tick event accessed).
  handle_second_tick(NULL, SECOND_UNIT);

  // Check initial battery status
  handle_battery(battery_state_service_peek());

  // Check initial connected status
  handle_connected(connection_service_peek_pebble_app_connection());

  // Before the battery changes, lets see what its value is:
  // handle_connected();
}

// Handle the destruction of the app
static void deinit(void) {
  bluetooth_connection_service_unsubscribe();
  battery_state_service_unsubscribe();
  tick_timer_service_unsubscribe();

  layer_destroy(s_connected_layer);
  layer_destroy(s_battery_layer);
  layer_destroy(s_time_layer);

  window_destroy(s_window);
}

// The main event/run loop for our app
int main(void) {
  init();
  app_event_loop();
  deinit();
}

// See top of this file
static void time_update(Layer *me, GContext *ctx) {
  // Gets the time
  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  bool           is_24h_style = clock_is_24h_style();
  unsigned short hour         = is_24h_style ? t->tm_hour : t->tm_hour % 12;
                 hour         = !is_24h_style && 0 == hour ? 12 : hour;
  bool           pm           = t->tm_hour / 12;
  unsigned short month        = t->tm_mon + 1;
  unsigned short week_day     = 0 != t->tm_wday ? t->tm_wday : 7;  // 0 as 7

  toggle_led(ctx, kSecondColumn, 0, t->tm_sec & 1);
  toggle_led(ctx, kSecondColumn, 1, t->tm_sec & 2);
  toggle_led(ctx, kSecondColumn, 2, t->tm_sec & 4);
  toggle_led(ctx, kSecondColumn, 3, t->tm_sec & 8);
  toggle_led(ctx, kSecondColumn, 4, t->tm_sec & 16);
  toggle_led(ctx, kSecondColumn, 5, t->tm_sec & 32);

  toggle_led(ctx, kMinuteColumn, 0, t->tm_min & 1);
  toggle_led(ctx, kMinuteColumn, 1, t->tm_min & 2);
  toggle_led(ctx, kMinuteColumn, 2, t->tm_min & 4);
  toggle_led(ctx, kMinuteColumn, 3, t->tm_min & 8);
  toggle_led(ctx, kMinuteColumn, 4, t->tm_min & 16);
  toggle_led(ctx, kMinuteColumn, 5, t->tm_min & 32);

  toggle_led(ctx, kHourColumn, 0, hour & 1);
  toggle_led(ctx, kHourColumn, 1, hour & 2);
  toggle_led(ctx, kHourColumn, 2, hour & 4);
  toggle_led(ctx, kHourColumn, 3, hour & 8);
  toggle_led(ctx, kHourColumn, 4, is_24h_style ? hour & 16 : pm);
  // last bit of hour is week_day lowest bit
  toggle_led(ctx, kHourColumn, kWeekDayLine, week_day & 1);

  toggle_led(ctx, kDayOfMonthColumn, 0, t->tm_mday & 1);
  toggle_led(ctx, kDayOfMonthColumn, 1, t->tm_mday & 2);
  toggle_led(ctx, kDayOfMonthColumn, 2, t->tm_mday & 4);
  toggle_led(ctx, kDayOfMonthColumn, 3, t->tm_mday & 8);
  toggle_led(ctx, kDayOfMonthColumn, 4, t->tm_mday & 16);
  // last bit of hour is week_day middle bit
  toggle_led(ctx, kDayOfMonthColumn, kWeekDayLine, week_day & 2);

  toggle_led(ctx, kMonthColumn, 0, month & 1);
  toggle_led(ctx, kMonthColumn, 1, month & 2);
  toggle_led(ctx, kMonthColumn, 2, month & 4);
  toggle_led(ctx, kMonthColumn, 3, month & 8);
  // last bit of month is week_day highest bit
  toggle_led(ctx, kMonthColumn, kWeekDayLine, week_day & 4);

}

// See top of this file
static void battery_update(Layer *me, GContext *ctx) {
  // charge_state.charge_percent
  // charge_state.is_charging
}

// See top of this file
static void connected_update(Layer *me, GContext *ctx) {
  toggle_led(ctx, 0, 4, s_connected_current);
//  graphics_context_set_fill_color(ctx, GColorWhite);
//  GPoint center = GPoint(0,0);
//  graphics_fill_circle(ctx)
}

// See top of this file
static GPoint getCenter(int col, int row) {
  return GPoint(kDotRadius * (1 + 2 * col) + kSpace * col + kSpace,
                kDotRadius * (1 + 2 * row) + kSpace * row + kSpace / 2);
}

// See top of this file
static void toggle_led(GContext *ctx, int col, int row, bool on) {
  graphics_context_set_fill_color(ctx, on ? GColorWhite : GColorBlack);
  graphics_fill_circle(ctx, getCenter(col, row), kDotRadius);
}
