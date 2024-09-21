#include <portmidi.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NOTE_ON          127
#define TOUCH_NOTE       144
#define HEADER_LENGTH    7
#define EOX_LENGTH       1

int32_t last_message = -1;

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

  Pm_Terminate(); // Clean up and terminate PortMidi
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
read_midi_message (PmStream *stream, PmEvent *event) {
  bool new_message = false;
  Pm_Read(stream, event, 1); // Read one event

  // initialize last_message
  if (last_message == -1) {
    last_message = event->message;
  }

  // since launchpad can't send twice in a row the same message, checks there is a new message
  if (last_message != (int32_t) event->message && last_message != -1) {
    last_message = event->message;
    new_message = true;
  }

  return new_message;
}

void
write_midi_message (PmStream *stream, uint8_t *data, int32_t length) {
  uint8_t header[] = {240, 0, 32, 41, 2, 24, 10};
  uint8_t eox[] = {247};
  uint8_t msg[HEADER_LENGTH + EOX_LENGTH + length];
  memcpy(msg, header, HEADER_LENGTH * sizeof(uint8_t));
  memcpy(msg + HEADER_LENGTH, data, length * sizeof(uint8_t));
  memcpy(msg + HEADER_LENGTH + length, eox, EOX_LENGTH * sizeof(uint8_t));
  Pm_WriteSysEx(stream, 0, msg);
}

int32_t
main (int32_t argc, char **argv) {
  PmError result;
  result = Pm_Initialize();
  if (result != pmNoError) {
    printf("PortMidi initialization failed: %s\n", Pm_GetErrorText(result));
    return 1;
  }

  if (argc > 1 && strcmp(argv[1], "list") == 0) return list_midi_devices();

  if (argc != 3) {
    printf("Initialization failed, missing input device id and output device id args\n");
    return 0;
  }

  uint8_t input_device_id = atoi(argv[1]);
  uint8_t output_device_id = atoi(argv[2]);

  PmStream *midi_input_stream;
  PmStream *midi_output_stream;
  init_midi_input_stream(&midi_input_stream, input_device_id);
  init_midi_output_stream(&midi_output_stream, output_device_id);

  // main loop
  while (1) {
    PmEvent event;
    if (read_midi_message(midi_input_stream, &event)) {
      int32_t status = Pm_MessageStatus(event.message);
      int32_t data1 = Pm_MessageData1(event.message);
      int32_t data2 = Pm_MessageData2(event.message);

      if (status == TOUCH_NOTE && data2 == NOTE_ON) {
        uint8_t color = 48;
        uint8_t data[] = {data1, color};
        write_midi_message(midi_output_stream, data, 2);
      }
    }
  }

  Pm_Close(midi_input_stream);
  Pm_Terminate();
  return 0;
}
