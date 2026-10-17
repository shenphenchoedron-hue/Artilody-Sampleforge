# Instrument export: SFZ 1.0 + WAV

Sampleforge exports a standard SFZ text file with 24-bit PCM WAV samples.
Choose a folder or an ordinary `.zip` archive containing that same folder content.
The ZIP is for transport: extract it before opening `Instrument.sfz` in an SFZ player.
Keep the SFZ and all WAV files together. There is no required custom manifest.

## Layout and mapping

`Instrument.sfz` and `001_C1.wav`, `002_Csharp1.wav`, etc. are siblings.
Filenames are relative ASCII paths, independent of the original recording's name.
Each sample has one SFZ 1.0 region, for example:

```sfz
<region> lokey=24 hikey=24 pitch_keycenter=24 loop_mode=no_loop sample=001_C1.wav
```

- MIDI integers avoid differences in octave naming between players. MIDI 60 = C4 in Sampleforge.
- `lokey` and `hikey` restrict playback to the imported key. Missing notes remain silent.
- `pitch_keycenter` sets that same key as the sample's untransposed root.
- `loop_mode=no_loop` disables looping. Normal SFZ note-off/envelope behaviour applies; this is not one-shot playback.
- No velocity layers, effects or player-specific extensions are exported.
- Approved pitch corrections are already rendered into the WAV. No `tune` or `transpose` opcode reapplies them.
- Ungodkendte toner beholder deres originale tonehøjde. En anden A4-reference end 440 Hz er også allerede indbygget i de godkendte WAV-filer.
- Original sample rate, mono/stereo channels and the short edge fades are preserved.

SFZ is an open, royalty-free instrument definition format, usable for free and commercial software.
Specification: https://sfzformat.com/legacy/
Format overview: https://www.plogue.com/products/sforzando.html

## Compatibility and earlier files

Sampleforge currently exports SFZ; it does not import SFZ or reopen exported instruments as projects.
Load `Instrument.sfz` from inside an SFZ-capable sampler. A Finder double-click needs an associated application.
Plugweaver's future importer should read the SFZ and referenced WAVs; it has not been implemented here.

Earlier `.artisamples` files remain ordinary ZIP archives with WAVs and a custom manifest.
They are not SFZ files and cannot be converted by renaming their extension to `.sfz`.
To produce SFZ, re-export the recording from this updated Sampleforge.
The SFZ export is an instrument, not an editable Sampleforge project.
