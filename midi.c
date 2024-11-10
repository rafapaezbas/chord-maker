#include <portmidi.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define NOTE_ON       144
#define NOTE_OFF      128
#define CONTROL       176
#define CLIPBOARD     111
#define HEADER_LENGTH 7
#define EOX_LENGTH    1
#define SCALES_LENGTH 6
#define SCALE_LENGTH  12
#define GRADES_LENGTH 7
#define GRADE_LENGTH  5
#define PAGE_WIDTH    8
#define PAGE_HEIGHT   8
#define ROOT          60
#define CHORD_VOICES  7
#define WIDTH         8
#define HEIGHT        8

// Color
#define PURPLE       48
#define WHITE        2
#define BLUE         39
#define COLOR_OFFSET 10

uint8_t scales[SCALES_LENGTH][SCALE_LENGTH] = {
  {1, 0, 1, 0, 1, 1, 0, 1, 0, 1, 0, 1},
  {1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 1, 0},
  {1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 0, 1},
  {1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 0, 1},
  {1, 0, 1, 0, 1, 1, 0, 1, 1, 0, 1, 0},
  {1, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1}};

uint8_t grades[GRADES_LENGTH][CHORD_VOICES] = {
  {1, 3, 5, 7, 9},
  {2, 4, 6, 8, 10},
  {3, 5, 7, 9, 11},
  {4, 6, 8, 10, 12},
  {5, 7, 9, 11, 13},
  {6, 8, 10, 12, 14},
  {7, 9, 11, 13, 15}};

PmStream *launchpad_midi_input_stream;
PmStream *launchpad_midi_output_stream;
PmStream *external_midi_output_stream;

enum Mode {
  CHORDS, // default
  MELODIES
};

typedef struct State {
  bool running;
  int32_t last_message;
  int32_t root;
  uint8_t scale;
  bool pressed[64];
  int8_t chord_modifier[64];
  int8_t last_pressed[2];
  int8_t clipboard;
  enum Mode mode;
} State;

State state;

typedef struct Point {
  uint8_t x;
  uint8_t y;
} Point;

void
handle_sigint () {
  state.running = false;
  Pm_Close(launchpad_midi_input_stream);
  Pm_Close(launchpad_midi_output_stream);
  Pm_Close(external_midi_output_stream);
  Pm_Terminate();
  exit(0);
}

Point
midi_to_point (uint8_t n) { // Converts from launchpad midi position to grid point
  struct Point p;
  p.x = (n % 10) - 1;
  p.y = ((uint8_t) n / 10) - 1;
  return p;
}

uint8_t
point_to_midi (Point p) { // Converts from grid point to launchpad midi position
  return (p.y * 10) + p.x + 11;
}

uint8_t
point_to_int (Point p) { // Converts from grid point to 1d array position
  return (p.y * 8) + p.x;
}

Point
int_to_point (uint8_t n) {
  struct Point p;
  p.x = (n % 8);
  p.y = (uint8_t) n / 8;
  return p;
}

void
get_notes (uint8_t *notes, uint8_t scale, uint8_t grade) {
  uint8_t a[12] = {ROOT, ROOT + 1, ROOT + 2, ROOT + 3, ROOT + 4, ROOT + 5, ROOT + 6, ROOT + 7, ROOT + 8, ROOT + 9, ROOT + 10, ROOT + 11};
  uint8_t b[CHORD_VOICES];

  uint8_t c_a = 0;
  for (uint8_t i = 0; i < SCALE_LENGTH; i++) {
    if (scales[scale][i] == 1) {
      b[c_a++] = a[i];
    }
  }

  for (uint8_t i = 0; i < GRADE_LENGTH; i++) {
    uint8_t offset = grades[grade][i] > 7 ? 1 : 0;
    notes[i] = b[(grades[grade][i] - 1) % CHORD_VOICES] + 12 * offset;
  }
}

void
create_page_chords (uint8_t chords[SCALES_LENGTH][WIDTH][GRADES_LENGTH][GRADE_LENGTH]) {
  for (int i = 0; i < SCALES_LENGTH; i++) {
    for (int j = 0; j < WIDTH; j++) {
      for (int k = 0; k < GRADES_LENGTH; k++) {
        uint8_t notes[GRADE_LENGTH];
        get_notes(notes, i, k);
        for (int l = 0; l < GRADE_LENGTH; l++) {
          chords[i][j][k][l] = notes[l] + j;
        }
      }
    }
  }
}

void
init_state (State *state) {
  state->last_message = -1;
  state->scale = 0;
  state->running = true;
  state->clipboard = -1;
  state->last_pressed[0] = 0;
  state->last_pressed[1] = -1;
  for (size_t i = 0; i < 64; i++)
    state->chord_modifier[i] = 0;
}

int32_t
list_midi_devices () {
  int32_t numDevices = Pm_CountDevices();

  if (numDevices == 0) {
    printf("No MIDI devices found.\n");
  } else {
    printf("\n");
    for (uint8_t i = 0; i < numDevices; ++i) {
      const PmDeviceInfo *info = Pm_GetDeviceInfo(i);
      if (info != NULL) {
        printf("Device ID: %d\n", i);
        printf("Interface: %s\n", info->interf);
        printf("Name: %s\n", info->name);
        printf("Is Input: %s\n", info->input ? "Yes" : "No");
        printf("Is Output: %s\n", info->output ? "Yes" : "No");
        printf("Opened: %s\n\n", info->opened ? "Yes" : "No");
      }
    }
  }

  Pm_Terminate();
  return 0;
}

int32_t
init_midi_input_stream (PmStream **stream, uint8_t input_device_id) {
  PmError result;
  result = Pm_OpenInput(stream, input_device_id, NULL, 512, NULL, NULL);
  if (result != pmNoError) {
    printf("Failed to open MIDI input device: %s\n", Pm_GetErrorText(result));
    Pm_Terminate();
    return 1;
  }

  return 0;
}

int32_t
init_midi_output_stream (PmStream **stream, uint8_t output_device_id) {
  PmError result;
  result = Pm_OpenOutput(stream, output_device_id, NULL, 512, NULL, NULL, 0);
  if (result != pmNoError) {
    printf("Failed to open MIDI input device: %s\n", Pm_GetErrorText(result));
    Pm_Terminate();
    return 1;
  }

  return 0;
}

bool
read_midi_message (PmStream *stream, PmEvent *event, State *state) {
  bool new_message = false;
  Pm_Read(stream, event, 1); // Read one event

  // initialize last_message
  if (state->last_message == -1) {
    state->last_message = event->message;
  }

  // since launchpad can't send twice in a row the same message, checks there is a new message
  if (state->last_message != (int32_t) event->message && state->last_message != -1) {
    state->last_message = event->message;
    new_message = true;
  }

  return new_message;
}

void
flash_clipboard (PmStream *stream, uint8_t color) {
  uint8_t msg[] = {240, 0, 32, 41, 2, 24, 40, 0, CLIPBOARD, color, 247};
  Pm_WriteSysEx(stream, 0, msg);
}

void
write_launchpad_midi_message (PmStream *stream, uint8_t *data, int32_t length) {
  uint8_t header[] = {240, 0, 32, 41, 2, 24, 10};
  uint8_t eox[] = {247};
  uint8_t msg[HEADER_LENGTH + EOX_LENGTH + length];
  memcpy(msg, header, HEADER_LENGTH * sizeof(uint8_t));
  memcpy(msg + HEADER_LENGTH, data, length * sizeof(uint8_t));
  memcpy(msg + HEADER_LENGTH + length, eox, EOX_LENGTH * sizeof(uint8_t));
  Pm_WriteSysEx(stream, 0, msg);
}

void
write_launchpad_set_all_midi_message (PmStream *stream, uint8_t color) {
  uint8_t msg[] = {240, 0, 32, 41, 2, 24, 14, color, 247};
  Pm_WriteSysEx(stream, 0, msg);
}

void
send_note_on (PmStream *stream, uint8_t note) {
  PmEvent note_on_event;
  note_on_event.message = Pm_Message(NOTE_ON, note, 127);
  note_on_event.timestamp = 0;
  Pm_WriteShort(stream, 0, note_on_event.message);
}

void
send_note_off (PmStream *stream, uint8_t note) {
  PmEvent note_on_event;
  note_on_event.message = Pm_Message(NOTE_OFF, note, 0);
  note_on_event.timestamp = 0;
  Pm_WriteShort(stream, 0, note_on_event.message);
}

void
send_chord_on (PmStream *stream, State *state, Point point, uint8_t chords[SCALES_LENGTH][WIDTH][GRADES_LENGTH][GRADE_LENGTH]) {
  uint8_t x = point.x;
  uint8_t y = point.y;
  uint8_t n = point_to_int(point);
  uint8_t modifier = state->chord_modifier[n] * 12;
  for (uint8_t i = 0; i < GRADE_LENGTH; i++) {
    send_note_on(stream, chords[state->scale][x][y][i] + modifier);
  }
}

void
send_chord_off (PmStream *stream, State *state, Point point, uint8_t chords[SCALES_LENGTH][WIDTH][GRADES_LENGTH][GRADE_LENGTH]) {
  uint8_t x = point.x;
  uint8_t y = point.y;
  uint8_t n = point_to_int(point);
  uint8_t modifier = state->chord_modifier[n] * 12;
  for (uint8_t i = 0; i < GRADE_LENGTH; i++) {
    send_note_off(stream, chords[state->scale][x][y][i] + modifier);
  }
}

void
increase_chord_modifier (State *state) {
  uint8_t n = point_to_int(midi_to_point(state->last_pressed[1]));
  state->chord_modifier[n] = MIN(state->chord_modifier[n] + 1, 2);
}

void
decrease_chord_modifier (State *state) {
  uint8_t n = point_to_int(midi_to_point(state->last_pressed[1]));
  state->chord_modifier[n] = MAX(state->chord_modifier[n] - 1, -2);
}

void
copy_to_clipboard (State *state) {
  Point point = midi_to_point(state->last_pressed[1]);
  uint8_t n = point_to_int(point);
  state->clipboard = n;
}

void
render_state (State *state) {
  size_t grid_data_length = PAGE_WIDTH * PAGE_HEIGHT * 2;
  uint8_t grid[grid_data_length];
  for (uint8_t x = 0; x < PAGE_WIDTH; x++) {
    for (uint8_t y = 0; y < PAGE_HEIGHT; y++) {
      Point p = {x, y};
      uint8_t n = point_to_midi(p);
      uint8_t color = state->pressed[point_to_int(p)] ? PURPLE : x * y + COLOR_OFFSET; // root color or plain?
      uint8_t data[] = {n, color};
      memcpy(grid + (x * PAGE_WIDTH + y) * 2, data, 2 * sizeof(uint8_t));
    }
  }

  size_t scale_data_length = HEIGHT * 2;
  uint8_t scale[scale_data_length];
  uint8_t offset = 19;
  for (uint8_t i = 0; i < HEIGHT; i++) {
    uint8_t color = state->scale == i ? WHITE : 0;
    uint8_t data[] = {(i * 10) + offset, color};
    memcpy(scale + (i * 2), data, 2 * sizeof(uint8_t));
  }

  size_t modifier_data_length = 2 * 2;
  uint8_t modifier[scale_data_length];
  uint8_t n = point_to_int(midi_to_point(state->last_pressed[1]));
  if (state->chord_modifier[n] == 2) {
    uint8_t data[] = {104, BLUE, 105, 0};
    memcpy(modifier, data, 4 * sizeof(uint8_t));
  } else if (state->chord_modifier[n] == 1) {
    uint8_t data[] = {104, WHITE, 105, 0};
    memcpy(modifier, data, 4 * sizeof(uint8_t));
  } else if (state->chord_modifier[n] == -1) {
    uint8_t data[] = {104, 0, 105, WHITE};
    memcpy(modifier, data, 4 * sizeof(uint8_t));
  } else if (state->chord_modifier[n] == -2) {
    uint8_t data[] = {104, 0, 105, BLUE};
    memcpy(modifier, data, 4 * sizeof(uint8_t));
  } else {
    uint8_t data[] = {104, 0, 105, 0};
    memcpy(modifier, data, 4 * sizeof(uint8_t));
  }

  size_t mode_data_length = 2 * 2;
  uint8_t mode[mode_data_length];
  if (state->mode == CHORDS) {
    uint8_t data[] = {106, WHITE, 107, 0};
    memcpy(mode, data, 4 * sizeof(uint8_t));
  } else {
    uint8_t data[] = {106, 0, 107, WHITE};
    memcpy(mode, data, 4 * sizeof(uint8_t));
  }

  size_t message_data_length = grid_data_length + scale_data_length + modifier_data_length + mode_data_length;
  uint8_t message[message_data_length];
  memcpy(message, grid, grid_data_length * sizeof(uint8_t));
  memcpy(message + grid_data_length, scale, scale_data_length * sizeof(uint8_t));
  memcpy(message + grid_data_length + scale_data_length, modifier, modifier_data_length * sizeof(uint8_t));
  memcpy(message + grid_data_length + scale_data_length + modifier_data_length, mode, mode_data_length * sizeof(uint8_t));

  write_launchpad_midi_message(launchpad_midi_output_stream, message, message_data_length);

  if (state->clipboard != -1) {
    uint8_t color = int_to_point(state->clipboard).x * int_to_point(state->clipboard).y + COLOR_OFFSET;
    flash_clipboard(launchpad_midi_output_stream, color);
  } else {
    flash_clipboard(launchpad_midi_output_stream, 0);
  }
}

int32_t
main (int32_t argc, char **argv) {
  // Register the signal handler for SIGINT
  signal(SIGINT, handle_sigint);

  PmError result;
  result = Pm_Initialize();
  if (result != pmNoError) {
    printf("PortMidi initialization failed: %s\n", Pm_GetErrorText(result));
    return 1;
  }

  if (argc > 1 && strcmp(argv[1], "list") == 0) return list_midi_devices();

  if (argc != 4) {
    printf("Initialization failed, missing input device id and output device id args\n");
    return 0;
  }

  uint8_t launchpad_input_device_id = atoi(argv[1]);
  uint8_t launchpad_output_device_id = atoi(argv[2]);
  uint8_t external_output_device_id = atoi(argv[3]);

  init_midi_input_stream(&launchpad_midi_input_stream, launchpad_input_device_id);
  init_midi_output_stream(&launchpad_midi_output_stream, launchpad_output_device_id);
  init_midi_output_stream(&external_midi_output_stream, external_output_device_id);

  init_state(&state);

  uint8_t chords[SCALES_LENGTH][WIDTH][GRADES_LENGTH][GRADE_LENGTH];
  create_page_chords(chords);

  render_state(&state);

  // main loop
  while (state.running) {
    PmEvent event;
    if (read_midi_message(launchpad_midi_input_stream, &event, &state)) {
      int32_t status = Pm_MessageStatus(event.message);
      int32_t data1 = Pm_MessageData1(event.message);
      int32_t data2 = Pm_MessageData2(event.message);
      printf("%d %d %d \n", status, data1, data2);

      if (data1 % 10 == 9) {
        state.scale = (uint8_t) (data1 / 10) - 1;
      }

      else if (status == NOTE_ON && data2 == 127) {
        state.pressed[point_to_int(midi_to_point(data1))] = true;
        send_chord_on(external_midi_output_stream, &state, midi_to_point(data1), chords);
        state.last_pressed[0] = status;
        state.last_pressed[1] = data1;
      }

      else if (status == NOTE_ON && data2 == 0) {
        state.pressed[point_to_int(midi_to_point(data1))] = false;
        send_chord_off(external_midi_output_stream, &state, midi_to_point(data1), chords);
      }

      else if (status == CONTROL && data1 == 104 && data2 == 127) {
        increase_chord_modifier(&state);
      }

      else if (status == CONTROL && data1 == 105 && data2 == 127) {
        decrease_chord_modifier(&state);
      }

      else if (status == CONTROL && data1 == 111 && data2 == 127) {
        copy_to_clipboard(&state);
      }

      else if (status == CONTROL && data1 == 106 && data2 == 127) {
        state.mode = CHORDS;
      }

      else if (status == CONTROL && data1 == 107 && data2 == 127) {
        state.mode = MELODIES;
      }

      render_state(&state);
    }
  }
}
