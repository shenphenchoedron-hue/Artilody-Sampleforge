<p align="center">
  <img src="packaging/linux/icons/256.png" width="160" alt="Artilody Sampleforge ikon">
</p>

<h1 align="center">Artilody Sampleforge</h1>

<p align="center">
  <b>Gør kromatiske optagelser til stemte SFZ-instrumenter.</b><br>
  macOS &middot; Windows &middot; Debian / Ubuntu Linux
</p>

<p align="center"><a href="README.md">English version</a></p>

---

Optag et instrument, der spiller én tone ad gangen i stigende kromatisk rækkefølge, indlæs filen i Sampleforge, og få et færdigt **SFZ 1.0**-instrument med **24-bit WAV**-samples.

## Funktioner

- **Automatisk opdeling** - finder hvert anslag og deler optagelsen i enkelte samples.
- **Tonehøjdemåling** - hvert sample placeres på den rigtige tangent; manglende toner bliver til huller i stedet for at forskyde resten.
- **Finstemning** - forslag til korrektion i cent pr. sample, A4 fra 400-480 Hz, aflytning af original/korrigeret, godkend alle pålidelige samples med ét klik.
- **Redigering** - trim grænser, del ved markøren, læg sammen med næste sample.
- **Oversigt over 88 tangenter** - fast A0-C8-klaviatur med importerede, valgte og manglende toner.
- **Åbent eksportformat** - standard SFZ 1.0 + 24-bit PCM WAV som mappe eller ZIP. Virker i sforzando, Sfizz, Decent Sampler og mange DAW-samplere.
- **Native og offline** - C++20 / JUCE, ingen browsermotor, ingen netværksadgang nødvendig.
- **Virker sammen med [Artilody Plugweaver](https://github.com/shenphenchoedron-hue/Artilody-Plugweaver)** - lav og ret kromatiske instrumentoptagelser, så de er klar til brug i Artilody Plugweaver til at lave VST3-, AU- og CLAP-plugins.

## Download

Hent den nyeste udgave fra **[Releases-siden](https://github.com/shenphenchoedron-hue/Artilody-Sampleforge/releases/latest)**.

| System | Fil | Installation |
|---|---|---|
| macOS 11+ (Apple Silicon og Intel) | `Artilody-Sampleforge-<version>-macOS.dmg` | Åbn DMG-filen, og træk appen til Programmer |
| Windows 10/11 x64 | `Artilody-Sampleforge-<version>-Windows-x64-Setup.exe` | Kør installationsprogrammet (dansk / engelsk) |
| Windows 10/11 x64, portabel | `Artilody-Sampleforge-<version>-Windows-x64-Portable.zip` | Pak ud, og kør `Artilody Sampleforge.exe` |
| Debian 12+ / Ubuntu 22.04+ (amd64) | `artilody-sampleforge_<version>_amd64.deb` | `sudo apt install ./artilody-sampleforge_<version>_amd64.deb` |
| Anden Linux x86_64 | `artilody-sampleforge-<version>-linux-x86_64.tar.gz` | Pak ud, og kør `./artilody-sampleforge` |

> **Første start:** programmerne er ikke signeret med betalte certifikater.
> **macOS:** hvis appen blokeres, så åbn *Systemindstillinger > Anonymitet & sikkerhed*, og klik **Åbn alligevel** (eller kør `xattr -dr com.apple.quarantine "/Applications/Artilody Sampleforge.app"`).
> **Windows:** ved SmartScreen-advarslen klik **Flere oplysninger > Kør alligevel**.

## Manualer

- [Brugermanual (dansk, PDF)](docs/Artilody-Sampleforge-Manual-DA.pdf)
- [User Manual (English, PDF)](docs/Artilody-Sampleforge-Manual-EN.pdf)

Manualerne følger også med i alle installationspakker og ligger ved hver release.

## Kom i gang

1. Optag tonerne én efter én, stigende kromatisk, med en kort pause imellem (WAV, AIFF eller FLAC; op til ca. 6 minutter stereo).
2. **Import audio** - Sampleforge deler og analyserer optagelsen.
3. Justér **Floor dB** / **Spacing s**, og klik **Re-analyse**, hvis toner overses eller findes for mange. Ret oktaven med **First note** eller **Align range to this pitch**.
4. Vælg samples, lyt med **Play** og **Corrected preview**, og klik **Apply tuning** eller **Tune reliable samples**.
5. **Export... > SFZ + WAV folder** (eller ZIP), og indlæs `Instrument.sfz` i din SFZ-sampler.

Eksportformatet er beskrevet i [INSTRUMENT-FORMAT.md](INSTRUMENT-FORMAT.md). En kort demo-optagelse medfølger: [Demo-C4-B4-plus12cents.wav](Demo-C4-B4-plus12cents.wav).

## Byg fra kildekode

| System | Kommando | Forudsætninger |
|---|---|---|
| macOS | `bash Build-Mac.command` | Xcode Command Line Tools |
| Windows | dobbeltklik på `Build-Windows.bat` | Visual Studio 2022 / Build Tools med *Desktop development with C++*; evt. Inno Setup 6 til installationsprogrammet |
| Debian / Ubuntu | `bash Build-Linux.sh` | ingen - installerer afhængigheder med apt og laver en `.deb` i `dist/` |

JUCE 8.0.4 hentes og verificeres automatisk ved første build. Udviklingsnoter fra baseline-02 findes i [docs/UDVIKLINGSNOTER-baseline-02.md](docs/UDVIKLINGSNOTER-baseline-02.md).

## Licens

Artilody Sampleforge er fri software under [GNU General Public License v3](LICENSE). Programmet bruger [JUCE](https://juce.com), som har sin egen licens (AGPLv3 eller kommerciel JUCE-licens) - se [JUCE-LICENSE.md](JUCE-LICENSE.md).
