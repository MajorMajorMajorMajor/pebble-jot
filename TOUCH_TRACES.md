# Touch trace collection (temporary developer mode)

This branch replaces the old touch-coordinate demo with a raw touch recorder.
The normal keyboard remains visible so gestures are performed in their real
positions. Recognition is deliberately not part of the recorder.

## On the watch

- **Up:** cycle through all 14 keys (the nine letters, SPACE, BACKSPACE,
  RETURN, 123, and KB).
- **Down:** cycle through TAP and the eight swipe directions.
- **Select:** start recording a trial; press it again to stop and dump the trial.
- **Select again:** repeat the same key/direction, with a new trial ID.

You can record a single gesture, several consecutive gestures, or no detected
touches at all between the two Select presses. For now, the trial label always
indicates the intended key and gesture, so use separate trials for different
targets. Do not change the key/direction while recording.

After each liftoff the content area shows a **provisional** direction and the
signed maximum X/Y excursions from touchdown, duration, and number of MOVE
events. `STILL` means the sensor reported no change of position; it does not
necessarily mean the user tapped. This feedback uses a short timer after
liftoff and is cancelled if the next touchdown happens first, so no screen
updates occur in the middle of a touch. The raw logged trace format is
unchanged, and feedback has no effect on its recorded values.

## On the computer

Start log capture **before** recording any trials:

```sh
pebble logs --phone PHONE_IP > touch.log
```

Use the device's normal Pebble developer connection address for `PHONE_IP`.
Wait for a trial's log dump to finish before starting the next one. The file
contains normal Pebble log prefixes in addition to `TRACE` records.

Each completed trial looks like this (the watch supplies the values):

```text
TRACE_BEGIN,42,E_RIGHT
TRACE,42,0,130,DOWN,120,80,0
TRACE,42,1,165,MOVE,126,81,0
TRACE,42,2,198,UP,129,81,0
TRACE_END,42,3,0
```

- `TRACE_BEGIN`: trial ID and the **intended** gesture.
- `TRACE`: trial ID, event sequence number (zero-based), milliseconds since
  recording began, event type, screen X/Y, and Pebble's `non_navigational` flag.
- `TRACE_END`: trial ID, event count, and count of events discarded because the
  buffer was full.

Even a zero-event attempt gets a `TRACE_BEGIN` and `TRACE_END` pair.
Each trial is limited to 256 events; any excess is counted in `TRACE_END`.
If a log line is lost during transfer, the explicit event sequence numbers and
counts allow a parser to detect that the trial is incomplete. A missing
`TRACE_END` also means the trial must not be trusted as complete.

Timestamps are taken in the touch callback. Nothing is printed or redrawn in
that callback, and the entire trace is emitted only after Select stops a trial.

The recorder captures every event **delivered to the application**, not finger
movement that the touchscreen hardware never reported.
