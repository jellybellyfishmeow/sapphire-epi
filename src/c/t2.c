#include <pebble.h>
#include <pebble.h>
#include <time.h>
#include <sys/time.h>
#include <stdio.h>

#define KEY_BUTTON    0
#define KEY_VIBRATE   1

#define BUTTON_UP     0
#define BUTTON_SELECT 1
#define BUTTON_DOWN   2

static Window *s_window;
static TextLayer *s_text_layer;
/******************************* AppMessage ***********************************/

typedef struct AppTimer AppTimer;
typedef void(* AppTimerCallback)(void* data);
enum AccelSamplingRate;

AppTimer* appTimer;

static void send(int key, int message) {
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);

  dict_write_int(iter, key, &message, sizeof(int), true);

  app_message_outbox_send();
}

static void inbox_received_handler(DictionaryIterator *iterator, void *context) {
  // Get the first pair
  Tuple *t = dict_read_first(iterator);

  // Process all pairs present
  while(t != NULL) {
    // Process this pair's key
    switch(t->key) {
      case KEY_VIBRATE:
        // Trigger vibration
        text_layer_set_text(s_text_layer, "Vibrate!");
        vibes_short_pulse();
        break;
      default:
        APP_LOG(APP_LOG_LEVEL_INFO, "Unknown key: %d", (int)t->key);
        break;
    }

    // Get next pair, if any
    t = dict_read_next(iterator);
  }
}

static void inbox_dropped_handler(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_handler(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_handler(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

/********************************* Buttons ************************************/



static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(s_text_layer, "Are you okay? Press Up button for YES");
  send(KEY_BUTTON, BUTTON_SELECT);
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
  app_timer_cancel(appTimer);

  text_layer_set_text(s_text_layer, "Great!  False Alarm.");
  send(KEY_BUTTON, BUTTON_UP);
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
  text_layer_set_text(s_text_layer, "Press SELECT key when you have recovered.");
  //printf("I'm reading");

  send(KEY_BUTTON, BUTTON_DOWN);
}





static void click_config_provider(void *context) {
  // Assign button handlers
  window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
  window_single_click_subscribe(BUTTON_ID_UP, up_click_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, down_click_handler);
}

void myCallback(void* data) {
  // code to execute after timer expires
    text_layer_set_text(s_text_layer, "Press SELECT key when you have recovered.");
}


/*static void alarmTrigger() {
    text_layer_set_text(s_text_layer, "Are you okay? Press Up button for YES");
  AppTimerCallback callback = myCallback;
  appTimer = app_timer_register(5*1000, callback, NULL);

  window_set_click_config_provider(s_window, click_config_provider);
}*/

static void prv_window_load(Window *window) {


  AppTimerCallback callback = myCallback;
  //AppTimer* app_timer_register(uint32_t timeout_ms, AppTimerCallback callback, void * callback_data)
  appTimer = app_timer_register(5*1000, callback, NULL);





  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_text_layer = text_layer_create(GRect(0, 72, bounds.size.w, 30));
  text_layer_set_text(s_text_layer, "Are you okay? Press Up button for YES");

  // Timer here



  text_layer_set_text_alignment(s_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(s_text_layer));
}

static void prv_window_unload(Window *window) {
  text_layer_destroy(s_text_layer);
}

static void prv_init(void) {
  s_window = window_create();

  window_set_click_config_provider(s_window, click_config_provider);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = prv_window_load,
    .unload = prv_window_unload,
  });
  const bool animated = true;
  window_stack_push(s_window, animated);
}

static void prv_deinit(void) {
  window_destroy(s_window);
}




/****************** Accelerometer code ************************/



// Subscribe to batched data events


//int accel_service_set_sampling_rate(AccelSamplingRate rate);
//uint32_t samples_per_update,
uint32_t num_samples = 1;

int16_t prevX = 0;

static void accel_data_handler(AccelData *data, uint32_t num_samples) {
  // Read sample 0's x, y, and z values
  int16_t x = data[0].x;
  int16_t y = data[0].y;
  int16_t z = data[0].z;

  if (prevX > 0) {
    if (abs(prevX-x) > 70) {
      //alarmToggle();
    }
  }

  // Determine if the sample occured during vibration, and when it occured
  bool did_vibrate = data[0].did_vibrate;
  uint64_t timestamp = data[0].timestamp;


  if(!did_vibrate) {
    printf("No vibration, data input");
    // Print it out
    APP_LOG(APP_LOG_LEVEL_INFO, "t: %llu, x: %d, y: %d, z: %d",
                                                          timestamp, x, y, z);

  } else {
    // Discard with a warning
    APP_LOG(APP_LOG_LEVEL_WARNING, "Vibration occured during collection");
  }
}




/*********************************************/



int main(void) {
  prv_init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", s_window);
  accel_data_service_subscribe(num_samples, accel_data_handler);
  app_event_loop();
  prv_deinit();


}
