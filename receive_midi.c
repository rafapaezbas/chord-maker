#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <portmidi.h>

#define INPUT_DEVICE_ID 3
#define OUTPUT_DEVICE_ID 2
#define NOTE_ON 127
#define TOUCH_NOTE 144

int32_t last_message = -1;

int init_midi_input_stream (PmStream** stream) {
    PmError result;

    result = Pm_Initialize();
    if (result != pmNoError) {
        printf("PortMidi initialization failed: %s\n", Pm_GetErrorText(result));
        return 1;
    }
    result = Pm_OpenInput(stream, INPUT_DEVICE_ID, NULL, 512, NULL, NULL);
    if (result != pmNoError) {
        printf("Failed to open MIDI input device: %s\n", Pm_GetErrorText(result));
        Pm_Terminate();
        return 1;
    }

    return 0;
}

int init_midi_output_stream (PmStream** stream) {
    PmError result;

    result = Pm_Initialize();
    if (result != pmNoError) {
        printf("PortMidi initialization failed: %s\n", Pm_GetErrorText(result));
        return 1;
    }
    result = Pm_OpenOutput(stream, OUTPUT_DEVICE_ID, NULL, 512, NULL, NULL, 0);
    if (result != pmNoError) {
        printf("Failed to open MIDI input device: %s\n", Pm_GetErrorText(result));
        Pm_Terminate();
        return 1;
    }

    return 0;
}

bool read_midi_message (PmStream* stream, PmEvent* event) {
    bool new_message = false;
    Pm_Read(stream, event, 1);  // Read one event

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

int main(void) {
    PmStream* midi_input_stream;
    PmStream* midi_output_stream;

    init_midi_input_stream(&midi_input_stream);
    init_midi_output_stream(&midi_output_stream);

    // main loop
    while (1) {
        PmEvent event;
        if(read_midi_message(midi_input_stream, &event)) {
            int32_t status = Pm_MessageStatus(event.message);
            int32_t data1 = Pm_MessageData1(event.message);
            int32_t data2 = Pm_MessageData2(event.message);

            printf("status %d \n", status);
            printf("data1 %d \n", data1);
            printf("data2 %d \n", data2);

            if (status == TOUCH_NOTE && data2 == NOTE_ON) {
                unsigned char msg[] = { 240, 0, 32, 41, 2, 24, 10, data1, 10, 247 };
                Pm_WriteSysEx(midi_output_stream, 0, msg);
            }
        }
    }

    Pm_Close(midi_input_stream);
    Pm_Terminate();
    return 0;
}
