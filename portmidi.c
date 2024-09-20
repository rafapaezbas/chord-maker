#include <stddef.h>
#include <stdio.h>
#include <portmidi.h>

int main(void) {
    Pm_Initialize(); // Initialize PortMidi

    int numDevices = Pm_CountDevices(); // Get number of devices
    if (numDevices == 0) {
        printf("No MIDI devices found.\n");
    } else {
        printf("Number of MIDI devices: %d\n\n", numDevices);

        for (int i = 0; i < numDevices; ++i) {
            const PmDeviceInfo *info = Pm_GetDeviceInfo(i); // Get device info

            if (info != NULL) {
                // Check if the device is an input device
                if (info->input) {
                    printf("Device ID: %d\n", i);
                    printf("Interface: %s\n", info->interf);
                    printf("Name: %s\n", info->name);
                    printf("Is Input: %s\n", info->input ? "Yes" : "No");
                    printf("Is Output: %s\n", info->output ? "Yes" : "No");
                    printf("Opened: %s\n\n", info->opened ? "Yes" : "No");
                }
            }
        }
    }

    Pm_Terminate(); // Clean up and terminate PortMidi
    return 0;
}
