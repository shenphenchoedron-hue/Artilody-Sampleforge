% version: 0.3.0
% date: October 2026
% tagline: Turn chromatic recordings into tuned SFZ instruments

# Introduction

Artilody Sampleforge turns a single recording of an instrument into a playable sample instrument. You play the notes one by one in ascending chromatic order, record them into one audio file and load that file into Sampleforge. The program then:

- finds every note onset and splits the recording into individual samples,
- measures the pitch of each sample and maps it to the correct key,
- shows missing notes as gaps on an 88-key keyboard,
- suggests a fine-tuning correction for every sample, which you can audition and approve,
- lets you correct boundaries manually (trim, split, merge),
- exports a standard **SFZ** instrument with **24-bit WAV** samples that works in any SFZ-capable sampler.

Sampleforge is a standalone application, not a plug-in. All processing happens locally on your computer; no network connection is needed after installation.

## System requirements

| Platform | Requirements |
|---|---|
| macOS | macOS 11 Big Sur or newer, Apple Silicon or Intel (universal app) |
| Windows | Windows 10 or 11, 64-bit (x64) |
| Linux | Debian 12 or newer, Ubuntu 22.04 or newer (amd64), X11 or XWayland desktop |
| All | Screen of at least 1024 x 700 pixels, audio output device, about 1 GB free RAM for long recordings |

# Installation

Download the file for your system from the **Releases** page on GitHub:

https://github.com/shenphenchoedron-hue/Artilody-Sampleforge/releases

| File | System |
|---|---|
| `Artilody-Sampleforge-0.3.0-macOS.dmg` | macOS (Apple Silicon and Intel) |
| `Artilody-Sampleforge-0.3.0-Windows-x64-Setup.exe` | Windows installer |
| `Artilody-Sampleforge-0.3.0-Windows-x64-Portable.zip` | Windows, no installation |
| `artilody-sampleforge_0.3.0_amd64.deb` | Debian / Ubuntu package |
| `artilody-sampleforge-0.3.0-linux-x86_64.tar.gz` | Other Linux, no installation |

## macOS

1. Double-click `Artilody-Sampleforge-0.3.0-macOS.dmg`.
2. Drag **Artilody Sampleforge** onto the **Applications** folder in the window that opens.
3. Eject the disk image and start the program from Applications or Launchpad.

> The app is not notarized by Apple. The first time, macOS may say that it cannot verify the developer. Open **System Settings > Privacy & Security**, scroll down and click **Open Anyway** next to the message about Artilody Sampleforge. On older macOS versions you can instead right-click (Ctrl-click) the app and choose **Open**. This is only necessary once.

If macOS reports that the app is "damaged", remove the download quarantine flag in Terminal:

```
xattr -dr com.apple.quarantine "/Applications/Artilody Sampleforge.app"
```

## Windows

1. Double-click `Artilody-Sampleforge-0.3.0-Windows-x64-Setup.exe`.
2. If Windows SmartScreen shows "Windows protected your PC", click **More info** and then **Run anyway**. The installer is not code-signed with a paid certificate.
3. Choose language (English or Danish), accept the GPL licence and follow the wizard. You can choose whether to install for all users or only for yourself, and whether to create a desktop shortcut.
4. Start **Artilody Sampleforge** from the Start menu. The manuals are also available in the Start menu folder.

To uninstall, use **Settings > Apps > Installed apps** or the uninstall shortcut in the Start menu.

**Portable version:** unpack `Artilody-Sampleforge-0.3.0-Windows-x64-Portable.zip` anywhere and run `Artilody Sampleforge.exe`. Nothing is written to the registry.

## Debian and Ubuntu Linux

Install the package with apt, which automatically installs the required libraries:

```
cd ~/Downloads
sudo apt install ./artilody-sampleforge_0.3.0_amd64.deb
```

Note the `./` in front of the file name. Alternatively, double-click the `.deb` file to open it in your software centre.

Start **Artilody Sampleforge** from the application menu (category Audio / Sound & Video), or run `artilody-sampleforge` in a terminal. The program is installed in `/opt/artilody-sampleforge`; the manuals are in `/usr/share/doc/artilody-sampleforge`.

To uninstall:

```
sudo apt remove artilody-sampleforge
```

> Sound on Linux: Sampleforge plays audio through ALSA. On modern desktops ALSA is routed to PipeWire or PulseAudio automatically. If you get no sound, install the bridge package `pipewire-alsa` (PipeWire) or `libasound2-plugins` (PulseAudio) and restart the program.

**Tarball:** unpack `artilody-sampleforge-0.3.0-linux-x86_64.tar.gz` and run `./artilody-sampleforge`. The libraries listed in the troubleshooting section must then be installed manually.

# Preparing a recording

The quality of the instrument depends on the recording. Sampleforge expects:

- **One note at a time** in **ascending chromatic order** (C, C#, D, D# ...). Any starting note and any number of notes are allowed; you do not need 88 notes.
- A short pause or a clear decay between notes, so that each onset can be detected. Notes must not overlap.
- Mono or stereo audio. **WAV, AIFF or FLAC** are recommended. Other formats depend on the decoders available on your system.
- At most 128 MB of decoded audio, roughly six minutes of stereo at 44.1 kHz.

Tips: record in a quiet room, keep a constant distance to the microphone, avoid strong vibrato and let each note sound for at least half a second. If you skip a note, Sampleforge leaves a gap on the keyboard instead of shifting all the following notes.

# The main window

The window is divided into these areas, from top to bottom:

| Area | Contents |
|---|---|
| Header | Title, **Import audio** and **Export...** (top right) |
| Overview | File name, format and number of samples, and the whole recording as a waveform with all detected samples marked |
| Analysis bar | **Floor dB**, **Spacing s**, **Re-analyse**, **First note** and **A4 Hz** |
| Middle, left: Samples | List of all samples. Scrolls independently |
| Middle, centre: editor | Name of the selected note, the enlarged detail waveform (click to place the split cursor), **Play**, **Stop**, **Corrected preview**, **Start s**, **End s**, **Set boundaries**, **Split at cursor** and **Merge with next** |
| Middle, right: Pitch correction | Detected frequency, deviation in cents and reliability, plus **Align range to this pitch**, **Apply tuning**, **Use original** and **Tune reliable samples**. Scrolls if the window is low |
| Keyboard | Fixed 88-key keyboard from A0 to C8, with information about missing notes |
| Status line | Messages, progress and errors |

The window adapts to the screen on start-up and can be resized freely (minimum 980 x 640 pixels).

# Step by step

## 1. Import a recording

Click **Import audio** and choose your file. Sampleforge reads the audio, detects onsets and measures the pitch of each sample in a background thread. The status line shows the progress. When the analysis is complete, the samples appear in the list, in the overview and on the keyboard (gold keys).

**A series of files:** select several files in the import dialog (for example one file per note, or a chromatic recording split into parts). Sampleforge sorts them by the pitch of their first note; if any file has an uncertain pitch, natural filename order is used instead (`Note 2` before `Note 10`). The files are joined into one recording, and onsets are detected inside each file at that file's own level, so a sample never crosses a file boundary. All files must use the same sample rate; mono files are duplicated to stereo when the series contains stereo files. The 128 MB decoded-size limit applies to all files together. **Re-analyse** keeps the file boundaries.

## 2. Check the detection

Look at the overview and the samples list:

- Too few samples (soft notes are missed): lower **Floor dB**, for example from -42 to -50, and click **Re-analyse**.
- Too many samples (noise or the decay is detected as new notes): raise **Floor dB** (for example -36) or increase **Spacing s**.
- Two notes merged into one: lower **Spacing s** or use **Split at cursor**.

| Setting | Default | Meaning |
|---|---|---|
| Floor dB | -42 | Onset threshold relative to the loudest RMS level. Allowed -70 to -12. Lower values find softer notes but also more noise |
| Spacing s | 0.18 | Minimum time in seconds between two onsets. Allowed 0.08 to 10 |
| First note | A0 | The note of the first sample. Changing it moves the whole mapping |
| A4 Hz | 440 | Tuning reference, 400 to 480 Hz |

> **Re-analyse** replaces all manual boundaries and tuning approvals. You will be asked to confirm.

## 3. Check the note mapping

Sampleforge determines the first note automatically from the measured frequency. If the whole range is shifted, select the correct note under **First note**, or select a sample whose pitch you trust and click **Align range to this pitch**. Both actions reset the tuning approvals.

Note that different programs name octaves differently. In Sampleforge MIDI note 60 is **C4**. A different octave name in another program is not in itself a reason to move the notes by an octave.

Missing notes are shown in the keyboard information line ("Missing in detected range: ..."). A stable missing note leaves a gap when the next measured note confirms the jump.

## 4. Audition

Select a sample by clicking it in the list, in the overview or on a gold key on the keyboard. Click **Play** to hear the original. Turn on **Corrected preview** and click **Play** again to hear the suggested correction. **Stop** is always available. Previewing uses a separate buffer; the imported audio is never changed.

## 5. Tune

The pitch correction panel shows the detected frequency, the deviation from the nearest equal-tempered note in cents and whether the measurement is reliable.

- **Apply tuning** approves the correction of the selected sample for export.
- **Tune reliable samples** approves all stable measurements within +/-50 cents in one go.
- Uncertain measurements within +/-50 cents can be approved manually after a confirmation. Listen to both versions first.
- **Use original** removes the approval; the sample is exported at its original pitch.

Only approved samples are pitch-corrected on export. If you change **A4 Hz**, the approvals are reset, and the new reference is rendered into the approved WAV files.

## 6. Edit boundaries

- Click in the large detail waveform to place the cursor, then click **Split at cursor** to split the sample in two. The cursor must be at least 80 ms from both edges.
- **Merge with next** joins the selected sample with the following one.
- **Start s** and **End s** show the boundaries in seconds from the beginning of the whole recording. Type new values and click **Set boundaries** to trim the sample. The sample is measured again. Neighbouring samples must not overlap and a sample must be at least 40 ms long.

Split and merge change the note assignment of the following samples and reset approvals, so check the mapping afterwards.

## 7. Export

Click **Export...** and choose one of:

- **SFZ + WAV folder** - choose a parent folder; Sampleforge creates a unique sub-folder named `Sampleforge-<date>-<time>` containing `Instrument.sfz` and the WAV files.
- **SFZ + WAV ZIP (.zip)** - saves the same files in an ordinary ZIP archive. Unpack it before loading the instrument.

The WAV files are 24-bit PCM, named in ascending order, for example `001_C1.wav`, `002_Csharp1.wav`. The original sample rate and mono/stereo are preserved and a 2 ms fade at both ends prevents clicks.

# Using the instrument

Load `Instrument.sfz` from inside an SFZ-capable sampler, for example sforzando (free, macOS/Windows), Sfizz (free, all platforms, including Linux), Decent Sampler with SFZ support, or the samplers in many DAWs. Always keep `Instrument.sfz` and the WAV files together in the same folder.

The file follows the usual SFZ template: `<control>` (`default_path` is empty because the samples are beside the file), `<global>`, one `<group>` with `loop_mode=no_loop`, and one region per sample that plays only on its own key (comments omitted here):

```
<control>
default_path=

<global>

<group>
loop_mode=no_loop

<region> lokey=24 hikey=24 pitch_keycenter=24 sample=001_C1.wav
<region> lokey=25 hikey=25 pitch_keycenter=25 sample=002_Csharp1.wav
```

- Missing notes stay silent; Sampleforge does not create new samples for empty keys.
- Approved tuning is already rendered into the WAV files, so the SFZ contains no `tune` opcodes.
- No loops, velocity layers or effects are exported.

The format is described in detail in `INSTRUMENT-FORMAT.md` in the repository.

# Limitations

- Overlapping notes cannot be separated.
- Noise, vibrato, very short notes and complex overtones can give uncertain measurements. The detector works in the range of about 24-4600 Hz.
- The pitch correction is a simple phase vocoder intended for small corrections. It can colour attacks and the stereo image, so always compare original and corrected sound.
- When the output device runs at a different sample rate, previews are resampled linearly. Exported files keep the original rate.
- No SFZ import, project saving, loop editing, velocity layers or plug-in version yet.
- An interrupted folder export can leave a partial folder. ZIP exports are written through a temporary file.

# Troubleshooting

| Problem | Solution |
|---|---|
| macOS: "cannot be opened" or "damaged" | See the macOS installation section: **Open Anyway** or the `xattr` command |
| Windows: SmartScreen warning | Click **More info** and **Run anyway** |
| Linux: no sound | Install `pipewire-alsa` or `libasound2-plugins`; check that no other program locks the sound card |
| Linux: program does not start from the tarball | `sudo apt install libasound2 libfreetype6 libfontconfig1 libx11-6 libxext6 libxinerama1 libxrandr2 libxcursor1 libxrender1 libxcomposite1` |
| "Cannot decode <file>" | Convert that file to WAV, AIFF or FLAC |
| "<file>: use mono or stereo audio" | Mix that file down to mono or stereo |
| "Use mono/stereo audio up to 128 MB" | Shorten the audio or select fewer files; the limit covers all selected files together |
| "All files must use the same sample rate" | Convert the files in the series to one sample rate |
| Notes are missed or too many are found | Adjust **Floor dB** and **Spacing s**, then **Re-analyse** |
| The whole range is an octave off | Use **First note** or **Align range to this pitch** |

Bugs and suggestions can be reported at https://github.com/shenphenchoedron-hue/Artilody-Sampleforge/issues

# Building from source

The source code is available on GitHub. Sampleforge is written in C++20 with JUCE 8.0.4, which is downloaded automatically during the first build.

| System | Command |
|---|---|
| macOS | Run `bash Build-Mac.command` in Terminal (needs Apple Command Line Tools) |
| Windows | Double-click `Build-Windows.bat` (needs Visual Studio 2022 or Build Tools with "Desktop development with C++"; Inno Setup 6 optional for the installer) |
| Debian / Ubuntu | Run `bash Build-Linux.sh` (installs the build dependencies with apt and creates a `.deb` in `dist/`) |

# Licence

Artilody Sampleforge is free software, licensed under the GNU General Public License version 3 (GPLv3). See the file `LICENSE`. JUCE is a separate dependency licensed under AGPLv3 or the commercial JUCE licence; see `JUCE-LICENSE.md`. The software is provided without any warranty.
