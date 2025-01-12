# Chord-Maker

Chord-Maker is a MIDI utility designed for use with the Novation Launchpad MIDI controller. It enables you to create and manipulate chords and melodies intuitively using the Launchpad's grid layout.

## Features

- **Two Interactive Views**: Easily switch between Chord View and Melody View for versatile music creation.
- **Octave and Scale Adjustments**: Quickly modify octaves and scales to fit your composition.
- **Clipboard Integration**: Seamlessly copy chords and extend them into melodies.

## How It Works

```
.\midi list
```

Print a list of midi inputs and outputs and their id.

```
.\midi <launchpad_input_id> <launchpad_output_id> <target_output_id>
```



### Chord View

In Chord View, you can:
- Play chords using the grid.
- Change the octave of the last played chord using the *up* or *down* buttons.
- Select a scale using the buttons on the right side.
- Copy the last played chord to the clipboard by pressing the *mixer* button.

Switch to Chord View by pressing the *User1* button on your Launchpad.

### Melody View

In Melody View, you can:
- Extend a chord from the clipboard across a row by pressing any of the left-side buttons on the grid.
- Change the octave of the note using the *up* and *down* buttons.
- Adjust the pitch by adding or subtracting a semitone with the *left* and *right* buttons.

Switch to Melody View by pressing the *User2* button on your Launchpad.

## License

Apache-2.0
