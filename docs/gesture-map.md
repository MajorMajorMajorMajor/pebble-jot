# Jot gesture keyboard — complete design map

This is a readable transcription of the **English-LH** worksheet in the original `messagease keys.xlsx` supplied during Jot's development. It captures the **intended keyboard mapping**, not a guarantee that all actions are implemented by the current watch app. The companion [CSV](gesture-map.csv) preserves every cell in a diffable, machine-readable format.

> **Design authority:** This file and its CSV companion take precedence over inferring "unused" gestures from blank cells in `src/c/ui/touch.c`. Some gestures are specified here but are not yet wired into Jot. **Do not assign blank C bindings to new features without checking this map.**

## Legend

- **Tap:** touch and release a key.
- **Swipe:** move a finger from a key in one of eight compass directions.
- **Return:** swipe outward and return to the starting key (the spreadsheet's `%` directions).
- **CW / CCW:** clockwise / counter-clockwise loops.
- **Hold:** stationary long press; recorded here even where the current recognizer does not expose it.
- **Square brackets:** named actions rather than printed characters; `[?]` means *not specified*, and `[None]` means explicitly no action.
- **Spacebar, enter, backspace, 123, hand:** special targets. **1–9** are their separate number-layout entries in the workbook; **a/n/i/h/o/r/t/e/s** are the letter-grid entries.

## Cursor assignments already present in the original workbook

| Key | Gesture | Original assigned action |
| --- | --- | --- |
| **Spacebar** | Swipe E → | [Cursor Right] |
| **Spacebar** | Swipe W ← | [Cursor Left] |
| **i** | Swipe N ↑ | [Cursor up] |
| **s** | Swipe S ↓ | [Cursor down] |
| **i** | Return N ↑ | [Navigate to start of buffer] |
| **s** | Return S ↓ | [Navigate to end of buffer] |

These are **pre-existing design assignments**. They are not available for unrelated new shortcuts, even if Jot's current binding array contains an empty string. Other planned actions in this map include forward delete, word navigation, case cycling, alternate layouts, copy/paste, and more; they should not be advertised as implemented until tested.

## Per-key reference

Choose a key to expand all eight swipe and return directions. Entries are intentionally preserved as written, including unresolved actions and unusual characters.

### Special keys

<details>
<summary><strong>spacebar · Tap: [Space] · CW: 0 · CCW: [Space] · Hold: 0</strong></summary>

| Direction | Swipe | Return swipe |
| --- | --- | --- |
| NW ↖ | [Toggle Alt] | [Switch to F1-F12 layout] |
| N ↑ | [Toggle sublabels] | [?] |
| NE ↗ | [Toggle Ctrl] | [Escape] |
| E → | [Cursor Right] | [Cursor Forward 1 Word] |
| SE ↘ | [Space] | [Space] |
| S ↓ | [Space] | [Space] |
| SW ↙ | [Space] | [Space] |
| W ← | [Cursor Left] | [Cursor Backward 1 Word] |

</details>

<details>
<summary><strong>enter · Tap: [Carriage Return] · CW: [?] · CCW: [?] · Hold: [Repeat]</strong></summary>

| Direction | Swipe | Return swipe |
| --- | --- | --- |
| NW ↖ | [Emoji Panel] | [?] |
| N ↑ | [Carriage Return] | [?] |
| NE ↗ | [Emoji Panel] | [?] |
| E → | [Carriage Return] | [?] |
| SE ↘ | [Carriage Return] | [?] |
| S ↓ | [Carriage Return] | [?] |
| SW ↙ | [Carriage Return] | [?] |
| W ← | [Carriage Return] | [?] |

</details>

<details>
<summary><strong>backspace · Tap: [Delete character backward] · CW: [Delete word backward] · CCW: [Delete word backward] · Hold: [Repeat]</strong></summary>

| Direction | Swipe | Return swipe |
| --- | --- | --- |
| NW ↖ | [Delete character backward] | [Delete word backward] |
| N ↑ | [Delete character backward] | [Delete word backward] |
| NE ↗ | [Delete character forward] | [Delete word forward] |
| E → | [Delete character forward] | [Delete word forward] |
| SE ↘ | [Delete character forward] | [Delete word forward] |
| S ↓ | [Delete character forward] | [Delete word forward] |
| SW ↙ | [Delete character backward] | [Delete word backward] |
| W ← | [Delete character backward] | [Delete word backward] |

</details>

<details>
<summary><strong>123 · Tap: [Switch to 123 layout] · CW: [?] · CCW: [?] · Hold: [Activate Tap-Drag-Hold macro overlay]</strong></summary>

| Direction | Swipe | Return swipe |
| --- | --- | --- |
| NW ↖ | [?] | [?] |
| N ↑ | [Copy] | [?] |
| NE ↗ | [Cut] | [?] |
| E → | [?] | [Cycle input language] |
| SE ↘ | [Microphone input] | [Configure voice recognition] |
| S ↓ | [Paste] | [Clipboard History] |
| SW ↙ | [Microphone input] | [Configure voice recognition] |
| W ← | [?] | [Cycle input language] |

</details>

<details>
<summary><strong>hand · Tap: [Hide keyboard] · CW: [Switch to Right Hand layout set] · CCW: [Switch to Right Hand layout set] · Hold: [Settings]</strong></summary>

| Direction | Swipe | Return swipe |
| --- | --- | --- |
| NW ↖ | [?] | [?] |
| N ↑ | [Resize up]/[Display macro bar] | [Hide macro bar] |
| NE ↗ | [?] | [?] |
| E → | [?] | [?] |
| SE ↘ | [Settings] | [Switch to Zoom layout] |
| S ↓ | [Hide macro bar]/[Resize down] | [Show macro bar] |
| SW ↙ | [Settings] | [Switch to Zoom layout] |
| W ← | [?] | [?] |

</details>

### Number-layout keys

<details>
<summary><strong>9 · Tap: 9 · CW: [?] · CCW: [?] · Hold: [None]</strong></summary>

| Direction | Swipe | Return swipe |
| --- | --- | --- |
| NW ↖ | [?] | [?] |
| N ↑ | [Cursor up] | [?] |
| NE ↗ | [Enter] | [Enter] |
| E → | [?] | [?] |
| SE ↘ | € | £ |
| S ↓ | = | ± |
| SW ↙ | [?] | [?] |
| W ← | ? | ¿ |

</details>

<details>
<summary><strong>8 · Tap: 8 · CW: ² · CCW: ² · Hold: [None]</strong></summary>

| Direction | Swipe | Return swipe |
| --- | --- | --- |
| NW ↖ | &#96; | ‘ |
| N ↑ | ^ | ˇ |
| NE ↗ | ´ | ’ |
| E → | ! | ¡ |
| SE ↘ | \ | — |
| S ↓ | [?] | [?] |
| SW ↙ | / | – |
| W ← | + | × |

</details>

<details>
<summary><strong>7 · Tap: 7 · CW: ¹ · CCW: ¹ · Hold: [None]</strong></summary>

| Direction | Swipe | Return swipe |
| --- | --- | --- |
| NW ↖ | [Compose] | [Compose] |
| N ↑ | [?] | [?] |
| NE ↗ | [?] | [?] |
| E → | - | ÷ |
| SE ↘ | [?] | [?] |
| S ↓ | [?] | [?] |
| SW ↙ | $ | ¥ |
| W ← | [?] | [?] |

</details>

<details>
<summary><strong>6 · Tap: 6 · CW: ⁿ · CCW: ⁿ · Hold: [None]</strong></summary>

| Direction | Swipe | Return swipe |
| --- | --- | --- |
| NW ↖ | \| | ¦ |
| N ↑ | [?] | [?] |
| NE ↗ | } | { |
| E → | ) | ( |
| SE ↘ | ] | [ |
| S ↓ | [?] | [?] |
| SW ↙ | @ | ª |
| W ← | [?] | [?] |

</details>

<details>
<summary><strong>5 · Tap: 5 · CW: a · CCW: a · Hold: [None]</strong></summary>

| Direction | Swipe | Return swipe |
| --- | --- | --- |
| NW ↖ | [?] | [?] |
| N ↑ | [?] | [?] |
| NE ↗ | [?] | [?] |
| E → | [?] | [?] |
| SE ↘ | [?] | [?] |
| S ↓ | [?] | [?] |
| SW ↙ | [?] | [?] |
| W ← | [?] | [?] |

</details>

<details>
<summary><strong>4 · Tap: 4 · CW: ¼ · CCW: ¼ · Hold: [None]</strong></summary>

| Direction | Swipe | Return swipe |
| --- | --- | --- |
| NW ↖ | { | } |
| N ↑ | [?] | [?] |
| NE ↗ | % | ‰ |
| E → | [?] | [?] |
| SE ↘ | _ | ¬ |
| S ↓ | [?] | [?] |
| SW ↙ | [ | ] |
| W ← | ( | ) |

</details>

<details>
<summary><strong>3 · Tap: 3 · CW: Σ · CCW: Σ · Hold: [None]</strong></summary>

| Direction | Swipe | Return swipe |
| --- | --- | --- |
| NW ↖ | [?] | [?] |
| N ↑ | & | § |
| NE ↗ | ° | 0 |
| E → | > | 3 |
| SE ↘ | [Space] | [Space] |
| S ↓ | [Cursor down] | [?] |
| SW ↙ | ; | ; |
| W ← | # | £ |

</details>

<details>
<summary><strong>2 · Tap: 2 · CW: Π · CCW: Π · Hold: [None]</strong></summary>

| Direction | Swipe | Return swipe |
| --- | --- | --- |
| NW ↖ | " | “ |
| N ↑ | [?] | [?] |
| NE ↗ | ' | ” |
| E → | [?] | [?] |
| SE ↘ | : | „ |
| S ↓ | . | … |
| SW ↙ | , | ‚ |
| W ← | [?] | [?] |

</details>

<details>
<summary><strong>1 · Tap: 1 · CW: ∫ · CCW: ∫ · Hold: [None]</strong></summary>

| Direction | Swipe | Return swipe |
| --- | --- | --- |
| NW ↖ | ~ | ˜ |
| N ↑ | ¨ | ˝ |
| NE ↗ | [?] | [?] |
| E → | * | † |
| SE ↘ | [Tab] | [Tab] |
| S ↓ | [?] | [?] |
| SW ↙ | [Backspace] | [Backspace] |
| W ← | < | ‹ |

</details>

### Letter-grid keys

<details>
<summary><strong>a · Tap: a · CW: 7 · CCW: A · Hold: 7</strong></summary>

| Direction | Swipe | Return swipe |
| --- | --- | --- |
| NW ↖ | [Compose] | [Compose] |
| N ↑ | [?] | [?] |
| NE ↗ | [?] | [?] |
| E → | - | ÷ |
| SE ↘ | v | V |
| S ↓ | [?] | [?] |
| SW ↙ | $ | ¥ |
| W ← | [?] | [?] |

</details>

<details>
<summary><strong>n · Tap: n · CW: 8 · CCW: N · Hold: 8</strong></summary>

| Direction | Swipe | Return swipe |
| --- | --- | --- |
| NW ↖ | &#96; | ‘ |
| N ↑ | ^ | ˇ |
| NE ↗ | ´ | ’ |
| E → | ! | ¡ |
| SE ↘ | \ | — |
| S ↓ | l | L |
| SW ↙ | / | – |
| W ← | + | × |

</details>

<details>
<summary><strong>i · Tap: i · CW: 9 · CCW: I · Hold: 9</strong></summary>

| Direction | Swipe | Return swipe |
| --- | --- | --- |
| NW ↖ | [?] | [?] |
| N ↑ | [Cursor up] | [Navigate to start of buffer] |
| NE ↗ | [Enter] | [Enter] |
| E → | [?] | [?] |
| SE ↘ | € | £ |
| S ↓ | = | ± |
| SW ↙ | x | X |
| W ← | ? | ¿ |

</details>

<details>
<summary><strong>h · Tap: h · CW: 4 · CCW: H · Hold: 4</strong></summary>

| Direction | Swipe | Return swipe |
| --- | --- | --- |
| NW ↖ | { | } |
| N ↑ | [?] | [?] |
| NE ↗ | % | ‰ |
| E → | k | K |
| SE ↘ | _ | ¬ |
| S ↓ | [?] | [?] |
| SW ↙ | [ | ] |
| W ← | ( | ) |

</details>

<details>
<summary><strong>o · Tap: o · CW: 5 · CCW: O · Hold: 5</strong></summary>

| Direction | Swipe | Return swipe |
| --- | --- | --- |
| NW ↖ | q | Q |
| N ↑ | u | U |
| NE ↗ | p | P |
| E → | b | B |
| SE ↘ | j | J |
| S ↓ | d | D |
| SW ↙ | g | G |
| W ← | c | C |

</details>

<details>
<summary><strong>r · Tap: r · CW: 6 · CCW: R · Hold: 6</strong></summary>

| Direction | Swipe | Return swipe |
| --- | --- | --- |
| NW ↖ | \| | ¶ |
| N ↑ | [Cycle case] | [?] |
| NE ↗ | } | { |
| E → | ) | ( |
| SE ↘ | ] | [ |
| S ↓ | [?] | [?] |
| SW ↙ | @ | ª |
| W ← | m | M |

</details>

<details>
<summary><strong>t · Tap: t · CW: 1 · CCW: T · Hold: 1</strong></summary>

| Direction | Swipe | Return swipe |
| --- | --- | --- |
| NW ↖ | ~ | ˜ |
| N ↑ | ¨ | ˝ |
| NE ↗ | y | Y |
| E → | * | † |
| SE ↘ | [Tab] | [?] |
| S ↓ | [?] | [?] |
| SW ↙ | [Backspace] | [Backspace] |
| W ← | < | ‹ |

</details>

<details>
<summary><strong>e · Tap: e · CW: 2 · CCW: E · Hold: 2</strong></summary>

| Direction | Swipe | Return swipe |
| --- | --- | --- |
| NW ↖ | " | “ |
| N ↑ | w | W |
| NE ↗ | ' | ” |
| E → | z | Z |
| SE ↘ | : | „ |
| S ↓ | . | … |
| SW ↙ | , | ‚ |
| W ← | [?] | [?] |

</details>

<details>
<summary><strong>s · Tap: s · CW: 3 · CCW: S · Hold: 3</strong></summary>

| Direction | Swipe | Return swipe |
| --- | --- | --- |
| NW ↖ | f | F |
| N ↑ | & | § |
| NE ↗ | ° | º |
| E → | > | › |
| SE ↘ | [Space] | [Space] |
| S ↓ | [Cursor down] | [Navigate to end of buffer] |
| SW ↙ | ; | ; |
| W ← | # | £ |

</details>

## Implementation notes

- Runtime gesture recognition and currently implemented text bindings: [`src/c/ui/touch.c`](../src/c/ui/touch.c).
- Keyboard artwork/layout: [`src/c/keyboard/`](../src/c/keyboard/).
- Cursor-editing discussion: [issue #3](https://github.com/MajorMajorMajorMajor/pebble-jot/issues/3).
- The original workbook has a second empty sheet. This reference represents its **English-LH** sheet only.
- A map entry is a **design requirement**, not necessarily a working behavior; unsupported platform actions (for example microphone or clipboard functions) are aspirations, not promised Pebble features.

When changing the mapping: update the CSV and Markdown together, then inspect the corresponding C bindings and tests. Distinguish consciously reassigned bindings from accidental collisions.
