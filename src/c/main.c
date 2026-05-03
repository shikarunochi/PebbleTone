#include <pebble.h>
static Window *s_main_window;
static Layer *s_canvas_layer;
static TextLayer *s_status_layer;

static BitmapLayer *s_wave_layer;
static GBitmap *s_wave_bitmap[4];

static SpeakerWaveform speakerWaveform;

typedef struct {
    int16_t  x;
    int16_t  y;
    GColor8 color;
    int16_t frequency_hz;
} NoteInfo;

static const NoteInfo posList[8] = {
  {50,50,GColorRed,262},{100,50,GColorOrange,294},{150,50,GColorYellow,330},
  {70,100,GColorGreen,350},{130,100,GColorVeryLightBlue,392},
  {50,150,GColorBlue,440},{100,150,GColorPurple,494},{150,150,GColorRed,523}
};

static const SpeakerWaveform SpeakerWaveformList[5] = {
   SpeakerWaveformSine,
   SpeakerWaveformSquare,
   SpeakerWaveformTriangle,
   SpeakerWaveformSawtooth,
};
int16_t speakerWaveIndex = 1;

// Vibe pattern: ON for 200ms, OFF for 100ms, ON for 400ms:
static const uint32_t segments[] = { 50 };
VibePattern pat = {
  .durations = segments,
  .num_segments = ARRAY_LENGTH(segments),
};


static void music(int16_t frequency_hz){
  //speaker_stop();
  int16_t duration_ms = 80;
  if(speakerWaveIndex == 0 || speakerWaveIndex == 2){
    duration_ms = duration_ms * 3;
  }
  speaker_play_tone(frequency_hz, 200, duration_ms, speakerWaveform);
}

static void canvas_update_proc(Layer *layer, GContext *ctx) {
  // Custom drawing happens here!
  for(int i = 0;i < 8;i++){
    graphics_context_set_fill_color(ctx, posList[i].color);
    GPoint center = GPoint(posList[i].x, posList[i].y);
    uint16_t radius = 20;
    // Fill a circle
    graphics_fill_circle(ctx, center, radius);
  }
}


static void touch_handler(const TouchEvent *event, void *context) {
  switch (event->type) {
    case TouchEvent_Touchdown:
      //APP_LOG(APP_LOG_LEVEL_DEBUG, "Touchdown at %d, %d", event->x, event->y);
      for(int i = 0;i < 8;i++){
        if( event->x >= posList[i].x - 25 &&  event->x <= posList[i].x + 25 &&
          event->y >= posList[i].y - 25 &&  event->y <= posList[i].y + 25){
          //text_layer_set_text(s_status_layer,
          //             "Touch !");
          //vibes_short_pulse();
          vibes_enqueue_custom_pattern(pat);
          music(posList[i].frequency_hz);
          break;
        } 
      }
      break;
     case TouchEvent_PositionUpdate:
       //APP_LOG(APP_LOG_LEVEL_DEBUG, "Move to %d, %d", event->x, event->y);
       break;
     case TouchEvent_Liftoff:
       //APP_LOG(APP_LOG_LEVEL_DEBUG, "Liftoff at %d, %d", event->x, event->y);
       break;
  }
}

static void main_window_load(Window *window) {
  if (!touch_service_is_enabled()) {
    text_layer_set_text(s_status_layer,
                        "Touch is disabled. Enable it in Settings → Display.");
    return;
  }

  // Touch is available - subscribe and start the touch UI
  touch_service_subscribe(touch_handler, NULL);
  
  // Get information about the Window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);

  s_canvas_layer = layer_create(bounds);
  // Assign the custom drawing procedure
  layer_set_update_proc(s_canvas_layer, canvas_update_proc);

  // Add to Window
  layer_add_child(window_get_root_layer(window), s_canvas_layer);
  
   s_status_layer = text_layer_create(GRect(0, 200 ,200 , 28));
   text_layer_set_background_color(s_status_layer, GColorClear);
   text_layer_set_text_color(s_status_layer, GColorWhite);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_status_layer));

  s_wave_layer = bitmap_layer_create(GRect(50, 190, 100, 20));
  bitmap_layer_set_bitmap(s_wave_layer, s_wave_bitmap[speakerWaveIndex]);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_wave_layer));
}

static void main_window_unload(Window *window) {
  speaker_stop();
  layer_destroy(s_canvas_layer);
  text_layer_destroy(s_status_layer);
  touch_service_unsubscribe();
  gbitmap_destroy(s_wave_bitmap[0]);
  gbitmap_destroy(s_wave_bitmap[1]);
  gbitmap_destroy(s_wave_bitmap[2]);
  gbitmap_destroy(s_wave_bitmap[3]);
  bitmap_layer_destroy(s_wave_layer);
}

static void deinit() {
  // Destroy Window
  window_destroy(s_main_window);
}

static void click_up_handler(ClickRecognizerRef recognizer, void *context) {
  speakerWaveIndex = speakerWaveIndex - 1;
  if(speakerWaveIndex < 0){
    speakerWaveIndex = 3;
  }
  speakerWaveform = SpeakerWaveformList[speakerWaveIndex];
  bitmap_layer_set_bitmap(s_wave_layer, s_wave_bitmap[speakerWaveIndex]);
  layer_mark_dirty(bitmap_layer_get_layer(s_wave_layer));
}
static void click_down_handler(ClickRecognizerRef recognizer, void *context) {
  speakerWaveIndex = speakerWaveIndex + 1;
  if(speakerWaveIndex > 3){
    speakerWaveIndex = 0;
  }
  speakerWaveform = SpeakerWaveformList[speakerWaveIndex];
  bitmap_layer_set_bitmap(s_wave_layer, s_wave_bitmap[speakerWaveIndex]);
  layer_mark_dirty(bitmap_layer_get_layer(s_wave_layer));
}

static void click_config_provider(void *context) {
  window_single_click_subscribe(BUTTON_ID_UP, click_up_handler);
  window_single_click_subscribe(BUTTON_ID_DOWN, click_down_handler);
}


static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set the background color
  window_set_background_color(s_main_window, GColorBlack);

  s_wave_bitmap[0] = gbitmap_create_with_resource(RESOURCE_ID_WAVE_SINE);
  s_wave_bitmap[1] = gbitmap_create_with_resource(RESOURCE_ID_WAVE_SQUARE);
  s_wave_bitmap[2] = gbitmap_create_with_resource(RESOURCE_ID_WAVE_TRIANGLE);
  s_wave_bitmap[3] = gbitmap_create_with_resource(RESOURCE_ID_WAVE_SAWTOOTH);
  
  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  window_set_click_config_provider(s_main_window, click_config_provider);
  
  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  speakerWaveform = SpeakerWaveformList[speakerWaveIndex];
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
