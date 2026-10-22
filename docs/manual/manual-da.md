% version: 0.3.0
% date: Oktober 2026
% tagline: Gør kromatiske optagelser til stemte SFZ-instrumenter

# Introduktion

Artilody Sampleforge gør én optagelse af et instrument til et spilbart sampleinstrument. Du spiller tonerne én efter én i stigende kromatisk rækkefølge, optager dem i én lydfil og indlæser filen i Sampleforge. Programmet:

- finder hvert anslag og deler optagelsen op i enkelte samples,
- måler tonehøjden på hvert sample og placerer det på den rigtige tangent,
- viser manglende toner som huller på et klaviatur med 88 tangenter,
- foreslår en finstemning til hvert sample, som du kan lytte til og godkende,
- lader dig rette grænserne manuelt (trimme, dele, sammenlægge),
- eksporterer et standard **SFZ**-instrument med **24-bit WAV**-samples, der virker i enhver SFZ-kompatibel sampler.

Sampleforge er et selvstændigt program, ikke et plugin. Al behandling foregår lokalt på din computer; der kræves ingen netværksforbindelse efter installationen.

## Systemkrav

| Platform | Krav |
|---|---|
| macOS | macOS 11 Big Sur eller nyere, Apple Silicon eller Intel (universel app) |
| Windows | Windows 10 eller 11, 64-bit (x64) |
| Linux | Debian 12 eller nyere, Ubuntu 22.04 eller nyere (amd64), X11- eller XWayland-skrivebord |
| Alle | Skærm på mindst 1024 x 700 pixels, lydudgang, ca. 1 GB ledig RAM til lange optagelser |

# Installation

Hent filen til dit system fra siden **Releases** på GitHub:

https://github.com/shenphenchoedron-hue/Artilody-Sampleforge/releases

| Fil | System |
|---|---|
| `Artilody-Sampleforge-0.3.0-macOS.dmg` | macOS (Apple Silicon og Intel) |
| `Artilody-Sampleforge-0.3.0-Windows-x64-Setup.exe` | Windows-installationsprogram |
| `Artilody-Sampleforge-0.3.0-Windows-x64-Portable.zip` | Windows, uden installation |
| `artilody-sampleforge_0.3.0_amd64.deb` | Debian-/Ubuntu-pakke |
| `artilody-sampleforge-0.3.0-linux-x86_64.tar.gz` | Anden Linux, uden installation |

## macOS

1. Dobbeltklik på `Artilody-Sampleforge-0.3.0-macOS.dmg`.
2. Træk **Artilody Sampleforge** over i mappen **Programmer** (Applications) i vinduet, der åbner.
3. Skub diskbilledet ud, og start programmet fra Programmer eller Launchpad.

> Appen er ikke notariseret hos Apple. Første gang kan macOS sige, at udvikleren ikke kan bekræftes. Åbn **Systemindstillinger > Anonymitet & sikkerhed**, rul ned, og klik **Åbn alligevel** ud for beskeden om Artilody Sampleforge. På ældre macOS-versioner kan du i stedet højreklikke (ctrl-klikke) på appen og vælge **Åbn**. Det er kun nødvendigt én gang.

Hvis macOS siger, at appen er "beskadiget", fjernes download-karantænen i Terminal:

```
xattr -dr com.apple.quarantine "/Applications/Artilody Sampleforge.app"
```

## Windows

1. Dobbeltklik på `Artilody-Sampleforge-0.3.0-Windows-x64-Setup.exe`.
2. Hvis Windows SmartScreen viser "Windows beskyttede din pc", klik **Flere oplysninger** og derefter **Kør alligevel**. Installationsprogrammet er ikke kodesigneret med et betalt certifikat.
3. Vælg sprog (dansk eller engelsk), acceptér GPL-licensen, og følg guiden. Du kan vælge at installere for alle brugere eller kun for dig selv, og om der skal oprettes en genvej på skrivebordet.
4. Start **Artilody Sampleforge** fra Start-menuen. Manualerne ligger også i mappen i Start-menuen.

Afinstallér via **Indstillinger > Apps > Installerede apps** eller afinstallationsgenvejen i Start-menuen.

**Portabel udgave:** pak `Artilody-Sampleforge-0.3.0-Windows-x64-Portable.zip` ud et vilkårligt sted, og kør `Artilody Sampleforge.exe`. Der skrives intet i registreringsdatabasen.

## Debian og Ubuntu Linux

Installér pakken med apt, som automatisk installerer de nødvendige biblioteker:

```
cd ~/Overførsler
sudo apt install ./artilody-sampleforge_0.3.0_amd64.deb
```

Bemærk `./` foran filnavnet. Mappen kan hedde `~/Downloads` afhængigt af sprogindstillingen. Du kan også dobbeltklikke på `.deb`-filen for at åbne den i softwarecentret.

Start **Artilody Sampleforge** fra programmenuen (kategorien Lyd / Lyd og video), eller kør `artilody-sampleforge` i en terminal. Programmet installeres i `/opt/artilody-sampleforge`; manualerne ligger i `/usr/share/doc/artilody-sampleforge`.

Afinstallation:

```
sudo apt remove artilody-sampleforge
```

> Lyd på Linux: Sampleforge afspiller gennem ALSA. På moderne skriveborde sendes ALSA automatisk videre til PipeWire eller PulseAudio. Hvis der ikke kommer lyd, så installér broen `pipewire-alsa` (PipeWire) eller `libasound2-plugins` (PulseAudio), og genstart programmet.

**Tarball:** pak `artilody-sampleforge-0.3.0-linux-x86_64.tar.gz` ud, og kør `./artilody-sampleforge`. Bibliotekerne fra afsnittet om fejlfinding skal så installeres manuelt.

# Forbered en optagelse

Instrumentets kvalitet afhænger af optagelsen. Sampleforge forventer:

- **Én tone ad gangen** i **stigende kromatisk rækkefølge** (C, C#, D, D# ...). Starttone og antal toner er frit; der kræves ikke 88 toner.
- En kort pause eller tydelig udklingning mellem tonerne, så hvert anslag kan findes. Tonerne må ikke overlappe.
- Mono- eller stereolyd. **WAV, AIFF eller FLAC** anbefales. Andre formater afhænger af systemets tilgængelige dekodere.
- Højst 128 MB dekodet lyd, cirka seks minutter stereo ved 44,1 kHz.

Gode råd: optag i et stille rum, hold samme afstand til mikrofonen, undgå kraftig vibrato, og lad hver tone klinge i mindst et halvt sekund. Springer du en tone over, efterlader Sampleforge et hul på klaviaturet i stedet for at flytte alle efterfølgende toner.

# Hovedvinduet

Vinduet er delt i disse områder, oppefra og ned:

| Område | Indhold |
|---|---|
| Top | Titel, **Import audio** og **Export...** (øverst til højre) |
| Oversigt | Filnavn, format og antal samples samt hele optagelsen som bølgeform med alle fundne samples markeret |
| Analyselinje | **Floor dB**, **Spacing s**, **Re-analyse**, **First note** og **A4 Hz** |
| Midten, venstre: Samples | Liste over alle samples. Ruller for sig selv |
| Midten, center: editor | Navnet på den valgte tone, den forstørrede detaljebølgeform (klik for at sætte klippemarkøren), **Play**, **Stop**, **Corrected preview**, **Start s**, **End s**, **Set boundaries**, **Split at cursor** og **Merge with next** |
| Midten, højre: Pitch correction | Målt frekvens, afvigelse i cent og pålidelighed samt **Align range to this pitch**, **Apply tuning**, **Use original** og **Tune reliable samples**. Ruller, hvis vinduet er lavt |
| Klaviatur | Fast klaviatur med 88 tangenter fra A0 til C8 med oplysning om manglende toner |
| Statuslinje | Beskeder, fremdrift og fejl |

Programmets knapper og felter er på engelsk; manualen bruger de engelske navne. Vinduet tilpasses skærmen ved start og kan frit ændres i størrelse (mindst 980 x 640 pixels).

# Trin for trin

## 1. Indlæs en optagelse

Klik **Import audio**, og vælg din fil. Sampleforge læser lyden, finder anslag og måler tonehøjden på hvert sample i en baggrundstråd. Statuslinjen viser fremdriften. Når analysen er færdig, vises samples i listen, i oversigten og på klaviaturet (gyldne tangenter).

**En serie filer:** vælg flere filer i importdialogen (fx én fil pr. tone eller en kromatisk optagelse delt i flere dele). Sampleforge sorterer dem efter tonehøjden på deres første tone; hvis en fil har usikker tonehøjde, bruges naturlig filnavnsrækkefølge i stedet (`Tone 2` før `Tone 10`). Filerne samles til én optagelse, og anslag findes inden for hver fil ud fra filens eget niveau, så et sample aldrig krydser en filgrænse. Alle filer skal have samme samplerate; monofiler dubleres til stereo, når serien indeholder stereofiler. Grænsen på 128 MB afkodet lyd gælder for alle filer tilsammen. **Re-analyse** bevarer filgrænserne.

## 2. Kontrollér opdelingen

Se på oversigten og samplelisten:

- For få samples (svage toner overses): sænk **Floor dB**, fx fra -42 til -50, og klik **Re-analyse**.
- For mange samples (støj eller udklingning opfattes som nye toner): hæv **Floor dB** (fx -36), eller øg **Spacing s**.
- To toner er slået sammen: sænk **Spacing s**, eller brug **Split at cursor**.

| Indstilling | Standard | Betydning |
|---|---|---|
| Floor dB | -42 | Anslagsgrænse i forhold til den højeste RMS-værdi. Tilladt -70 til -12. Lavere tal finder svagere toner, men også mere støj |
| Spacing s | 0.18 | Mindste afstand i sekunder mellem to anslag. Tilladt 0,08 til 10 |
| First note | A0 | Tonen for det første sample. Ændring flytter hele tonerækken |
| A4 Hz | 440 | Stemmetone, 400 til 480 Hz |

> **Re-analyse** erstatter alle manuelle klip og godkendelser af stemning. Du bliver bedt om at bekræfte.

## 3. Kontrollér tonetildelingen

Sampleforge bestemmer automatisk første tone ud fra den målte frekvens. Er hele rækken forskudt, så vælg den rigtige tone under **First note**, eller vælg et sample, hvis tonehøjde du stoler på, og klik **Align range to this pitch**. Begge handlinger nulstiller godkendelserne.

Bemærk, at programmer navngiver oktaver forskelligt. I Sampleforge er MIDI-tone 60 = **C4**. Et andet oktavnavn i et andet program er ikke i sig selv grund til at flytte tonerne en oktav.

Manglende toner vises i klaviaturets infolinje ("Missing in detected range: ..."). En stabil manglende tone efterlader et hul, når den næste målte tone bekræfter springet.

## 4. Aflyt

Vælg et sample ved at klikke på det i listen, i oversigten eller på en gylden tangent. Klik **Play** for at høre originalen. Slå **Corrected preview** til, og klik **Play** igen for at høre den foreslåede korrektion. **Stop** er altid tilgængelig. Aflytning bruger en separat buffer; den importerede lyd ændres aldrig.

## 5. Stem

Panelet Pitch correction viser den målte frekvens, afvigelsen fra nærmeste ligesvævende tone i cent og om målingen er pålidelig.

- **Apply tuning** godkender korrektionen af det valgte sample til eksport.
- **Tune reliable samples** godkender alle stabile målinger inden for +/-50 cent på én gang.
- Usikre målinger inden for +/-50 cent kan godkendes manuelt efter bekræftelse. Lyt til begge versioner først.
- **Use original** fjerner godkendelsen; samplet eksporteres med sin originale tonehøjde.

Kun godkendte samples pitchkorrigeres ved eksport. Ændrer du **A4 Hz**, nulstilles godkendelserne, og den nye stemmetone indbygges i de godkendte WAV-filer.

## 6. Redigér grænser

- Klik i den store detaljebølgeform for at sætte markøren, og klik **Split at cursor** for at dele samplet i to. Markøren skal være mindst 80 ms fra begge kanter.
- **Merge with next** lægger det valgte sample sammen med det næste.
- **Start s** og **End s** viser grænserne i sekunder fra begyndelsen af hele optagelsen. Skriv nye værdier, og klik **Set boundaries** for at trimme samplet. Samplet måles igen. Naboklip må ikke overlappe, og et sample skal være mindst 40 ms langt.

Deling og sammenlægning ændrer tonetildelingen for de efterfølgende samples og nulstiller godkendelser, så kontrollér tildelingen bagefter.

## 7. Eksportér

Klik **Export...**, og vælg:

- **SFZ + WAV folder** - vælg en overmappe; Sampleforge opretter en unik undermappe med navnet `Sampleforge-<dato>-<tid>` med `Instrument.sfz` og WAV-filerne.
- **SFZ + WAV ZIP (.zip)** - gemmer de samme filer i et almindeligt ZIP-arkiv. Pak det ud, før instrumentet indlæses.

WAV-filerne er 24-bit PCM og nummereres i stigende rækkefølge, fx `001_C1.wav`, `002_Csharp1.wav`. Original samplerate og mono/stereo bevares, og en fade på 2 ms i begge ender forhindrer klik.

# Brug af instrumentet

Indlæs `Instrument.sfz` inde fra en SFZ-kompatibel sampler, fx sforzando (gratis, macOS/Windows), Sfizz (gratis, alle platforme inkl. Linux), Decent Sampler med SFZ-understøttelse eller samplerne i mange DAW'er. Hold altid `Instrument.sfz` og WAV-filerne samlet i samme mappe.

Filen følger den almindelige SFZ-skabelon: `<control>` (`default_path` er tom, fordi samples ligger ved siden af filen), `<global>`, én `<group>` med `loop_mode=no_loop` og én region pr. sample, der kun spiller på sin egen tangent (kommentarer udeladt her):

```
<control>
default_path=

<global>

<group>
loop_mode=no_loop

<region> lokey=24 hikey=24 pitch_keycenter=24 sample=001_C1.wav
<region> lokey=25 hikey=25 pitch_keycenter=25 sample=002_Csharp1.wav
```

- Manglende toner forbliver tavse; Sampleforge laver ikke nye samples til tomme tangenter.
- Godkendt stemning er allerede indbygget i WAV-filerne, så SFZ-filen indeholder ingen `tune`-opcodes.
- Der eksporteres ingen loops, velocity-lag eller effekter.

Formatet er beskrevet detaljeret i `INSTRUMENT-FORMAT.md` i repositoriet.

# Begrænsninger

- Overlappende toner kan ikke adskilles.
- Støj, vibrato, meget korte toner og komplekse overtoner kan give usikre målinger. Detektoren arbejder i området ca. 24-4600 Hz.
- Pitchkorrektionen er en enkel phase vocoder beregnet til små rettelser. Den kan farve anslag og stereobillede, så sammenlign altid original og korrigeret lyd.
- Kører lydudgangen med en anden samplerate, resamples aflytningen lineært. Eksporterede filer beholder den originale samplerate.
- Endnu ingen SFZ-import, projektgemning, loopredigering, velocity-lag eller pluginudgave.
- En afbrudt mappeeksport kan efterlade en delvis mappe. ZIP-eksport skrives via en midlertidig fil.

# Fejlfinding

| Problem | Løsning |
|---|---|
| macOS: "kan ikke åbnes" eller "beskadiget" | Se afsnittet om installation på macOS: **Åbn alligevel** eller `xattr`-kommandoen |
| Windows: SmartScreen-advarsel | Klik **Flere oplysninger** og **Kør alligevel** |
| Linux: ingen lyd | Installér `pipewire-alsa` eller `libasound2-plugins`; kontrollér, at intet andet program låser lydkortet |
| Linux: programmet starter ikke fra tarball | `sudo apt install libasound2 libfreetype6 libfontconfig1 libx11-6 libxext6 libxinerama1 libxrandr2 libxcursor1 libxrender1 libxcomposite1` |
| "Cannot decode <fil>" | Konvertér den fil til WAV, AIFF eller FLAC |
| "<fil>: use mono or stereo audio" | Mix filen ned til mono eller stereo |
| "Use mono/stereo audio up to 128 MB" | Gør lyden kortere, eller vælg færre filer; grænsen gælder alle valgte filer tilsammen |
| "All files must use the same sample rate" | Konvertér filerne i serien til samme samplerate |
| Toner overses, eller der findes for mange | Justér **Floor dB** og **Spacing s**, og klik **Re-analyse** |
| Hele rækken ligger en oktav forkert | Brug **First note** eller **Align range to this pitch** |

Fejl og forslag kan indberettes på https://github.com/shenphenchoedron-hue/Artilody-Sampleforge/issues

# Byg fra kildekode

Kildekoden findes på GitHub. Sampleforge er skrevet i C++20 med JUCE 8.0.4, som hentes automatisk ved første build.

| System | Kommando |
|---|---|
| macOS | Kør `bash Build-Mac.command` i Terminal (kræver Apple Command Line Tools) |
| Windows | Dobbeltklik på `Build-Windows.bat` (kræver Visual Studio 2022 eller Build Tools med "Desktop development with C++"; Inno Setup 6 valgfrit til installationsprogrammet) |
| Debian / Ubuntu | Kør `bash Build-Linux.sh` (installerer byggeafhængigheder med apt og laver en `.deb` i `dist/`) |

# Licens

Artilody Sampleforge er fri software under GNU General Public License version 3 (GPLv3). Se filen `LICENSE`. JUCE er en separat afhængighed under AGPLv3 eller den kommercielle JUCE-licens; se `JUCE-LICENSE.md`. Programmet leveres uden nogen form for garanti.
