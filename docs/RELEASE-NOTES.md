**Artilody Sampleforge 0.3.0** for macOS, Windows and Debian/Ubuntu Linux.

Turn a recording of notes played one by one in ascending chromatic order - or a series of note files - into a tuned SFZ instrument with 24-bit WAV samples.

### What's new in 0.3.0

- **Import a series of audio files.** Select several WAV, AIFF, FLAC, MP3, M4A or OGG files at once in **Import audio**, for example one file per note or a chromatic recording split into parts. The files are combined in ascending pitch order (natural filename order if a pitch is uncertain), and notes are detected inside each file at that file's own level, so a sample never crosses a file boundary. All files must share one sample rate; mono files are duplicated to stereo next to stereo files. The 128 MB limit applies to all files together, and **Re-analyse** keeps the file boundaries.
- **Faster import and analysis.** Pitch analysis runs on all CPU cores, and files in a series are decoded in parallel. Analysing a full 88-note series is about 5x faster, and a single recording about 4x faster.
- **Structured SFZ export.** `Instrument.sfz` now follows the usual SFZ template: `<control>` (`default_path=`), `<global>`, one `<group>` with `loop_mode=no_loop`, then one `<region>` per sample. Only opcodes accepted by sforzando, sfizz and Artilody Plugweaver are written. `<control>`/`<global>` are SFZ v2 headers; all region opcodes are SFZ 1.0.
- Safer handling of damaged or crafted audio file headers.

### Downloads

| System | File |
|---|---|
| macOS 11+ (Apple Silicon & Intel, universal) | `Artilody-Sampleforge-0.3.0-macOS.dmg` |
| Windows 10/11 x64 - installer | `Artilody-Sampleforge-0.3.0-Windows-x64-Setup.exe` |
| Windows 10/11 x64 - portable | `Artilody-Sampleforge-0.3.0-Windows-x64-Portable.zip` |
| Debian 12+ / Ubuntu 22.04+ (amd64) | `artilody-sampleforge_0.3.0_amd64.deb` |
| Other Linux x86_64 | `artilody-sampleforge-0.3.0-linux-x86_64.tar.gz` |
| Manuals | `Artilody-Sampleforge-Manual-EN.pdf`, `Artilody-Sampleforge-Manual-DA.pdf` |

Linux: `sudo apt install ./artilody-sampleforge_0.3.0_amd64.deb`

**First launch:** the builds are not signed with paid certificates. On macOS use *System Settings > Privacy & Security > Open Anyway*; on Windows click *More info > Run anyway* in SmartScreen. See the manual for details.

---

**Dansk:** Version 0.3.0 kan importere en serie lydfiler på én gang (fx én fil pr. tone), analyserer hurtigere på alle CPU-kerner og eksporterer SFZ-filen i den almindelige skabelonstruktur (`<control>`, `<global>`, `<group>`, `<region>`). Hent filen til dit system ovenfor. Brugermanualen på dansk er `Artilody-Sampleforge-Manual-DA.pdf`.

SHA-256 checksums: `SHA256SUMS.txt`
