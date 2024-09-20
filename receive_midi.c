#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <portmidi.h>

#define INPUT_DEVICE_ID 3  // Change this to the actual ID of your MIDI input device
//

int32_t last_message = -1;

bool read_note (PmStream* midiStream, PmEvent* event) {
    bool new_message = false;
    Pm_Read(midiStream, event, 1);  // Read one event

    if (last_message == -1) { // initialize last_message
        last_message = event->message;
    }

    if (last_message != (int32_t) event->message && last_message != -1) { // since launchpad can't send twice in a row the same message, checks there is a new message
        last_message = event->message;
        new_message = true;
    }

    return new_message;
}

int init_midi_stream (PmStream* midiStream) {
    return 0;
}

int main(void) {
    PmStream* midiStream;
    PmError result;
    result = Pm_Initialize();

    if (result != pmNoError) {
        printf("PortMidi initialization failed: %s\n", Pm_GetErrorText(result));
        return 1;
    }
    result = Pm_OpenInput(&midiStream, INPUT_DEVICE_ID, NULL, 512, NULL, NULL);
    if (result != pmNoError) {
        printf("Failed to open MIDI input device: %s\n", Pm_GetErrorText(result));
        Pm_Terminate();
        return 1;
    }
    printf("Waiting for MIDI input...\n");
    while (1) {
        PmEvent event;
        if(read_note(midiStream, &event)) {
            printf("Status: %02X, Data1: %d, Data2: %d\n",
                   Pm_MessageStatus(event.message),
                   Pm_MessageData1(event.message),
                   Pm_MessageData2(event.message));
        }
    }

    Pm_Close(midiStream);
    Pm_Terminate();
    return 0;
}
