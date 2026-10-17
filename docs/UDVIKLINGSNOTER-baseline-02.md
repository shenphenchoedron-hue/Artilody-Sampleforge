# Artilody Sampleforge — baseline-02

C++20 / JUCE 8.0.4. Denne udgave erstatter Swift/WebKit-prototypen. Både brugerflade, lydanalyse, afspilning og eksport er nu C++. Der indgår ingen HTML-/JavaScript-brugerflade eller browsermotor.

## Byg og åbn på din Mac

1. Luk den tidligere Sampleforge-app.
2. Pak ZIP-filen ud i en ny mappe. Behold gerne baseline-01 som reference.
3. Åbn Terminal, skriv `bash` og et mellemrum, og træk **Build-Mac.command** ind i Terminalen. Tryk Enter.
4. Hvis Apple Command Line Tools mangler, starter installationsdialogen. Kør buildfilen igen, når installationen er færdig.
5. Første build henter om nødvendigt en lokal CMake 3.31.6 og henter JUCE 8.0.4. Begge downloads verificeres med SHA-256. Ingen Homebrew er nødvendig. Første build kan tage flere minutter.
6. Appen åbner automatisk efter et vellykket build og selvtest.

Den færdige app ligger i:

`build-mac/Sampleforge_artefacts/Release/Artilody Sampleforge.app`

Du kan derefter trække den til Programmer. Buildet er til den Mac, du kører på: Intel eller Apple Silicon. Minimum macOS 11. Første build kræver internet; selve appen arbejder lokalt uden netværk.

## SFZ-eksport — 16. oktober 2026

Eksporten bruger nu det åbne, royaltyfrie SFZ-format med WAV-filer. Vælg **SFZ + WAV folder** eller **SFZ + WAV ZIP (.zip)**. Pak ZIP-filen ud, og åbn `Instrument.sfz` inde fra en SFZ-afspiller. Behold WAV-filerne i samme mappe. Sampleforge kan endnu ikke selv genåbne instrumentet som et projekt.

## Kodeoprydning — 16. oktober 2026

De 111 advarsler i appkoden fra den første Mac-buildlog er adresseret med eksplicitte konverteringer mellem JUCEs signed indeks og standardbibliotekets størrelsestyper, eksplicitte betingelser for flydende tal og et entydigt variabelnavn. Koden er formateret for læsbarhed. Compileradvarsler er fortsat slået til.

Den sidste advarsel stammer fra JUCE 8.0.4. CMake retter præcis én betingelse i den automatisk hentede Mac-kode: et ubrugt pointer-navn fjernes, mens funktionskald og null-kontrol bevares. Et eget checkout angivet via `JUCE_PATH` ændres ikke.

Appkoden og DSP-testen kompilerer uden advarsler med GCC og `-Wall -Wextra -Wpedantic -Wsign-conversion -Wfloat-conversion -Wfloat-equal -Wshadow -Werror`. Klavertesten giver fortsat 71 samples og 0,858784 cent maksimal intern restafvigelse. Selve Mac-rettelsen er kontrolleret som en præcis kildekodeændring; denne reviderede pakke er endnu ikke bygget med AppleClang på Mac.

## Det nye i baseline-02

- Native JUCE-brugerflade og C++-lydmotor.
- Startvinduet tilpasses skærmens tilgængelige område.
- Kompakt midterområde og et fast 88-tangenters klaviatur fra A0 til C8 nederst. Samplelisten og det lange stemmepanel ruller lokalt.
- Gold/logo med rubinrosa, violet og turkis.
- Opdeling, målt tonehøjde, forslag til finstemning og huller ved manglende toner.
- Original/korrigeret aflytning, trimning, split og sammenlægning.
- Eksport af SFZ + WAV-filer i en mappe eller samlet ZIP-fil.

## Brug

Indlæs en optagelse med toner spillet én efter én i stigende kromatisk rækkefølge. Der er ikke krav om 88 toner eller en bestemt starttone. WAV, AIFF og FLAC anbefales. Andre formater afhænger af JUCE/macOS' tilgængelige dekodere.

**Floor dB** er anslagsgrænsen relativt til den højeste RMS-værdi. Lavere tal tillader svagere anslag, men kan også finde støj. **Spacing s** er mindste afstand mellem anslag. **Re-analyse** erstatter manuelle klip og godkendelser efter bekræftelse.

**First note** kan rette hele tonerækkens placering. Stemningen bruger den faktiske frekvens og A4 = 440 Hz som standard. A4 kan ændres til 400–480 Hz. Et andet programs oktavnavn er ikke i sig selv grund til at flytte tonerne en oktav.

Vælg et sample i listen, oversigten eller klaviaturet. Klik **Play** for originalen. Slå **Corrected preview** til for at høre korrektionen. Aflytning bruger en separat buffer; den importerede lyd ændres ikke.

**Apply tuning** godkender korrektionen til eksport. **Tune reliable samples** godkender alle stabile fund inden for ±50 cent. Usikre målinger inden for dette område kan godkendes manuelt efter bekræftelse. **Use original** fjerner godkendelsen. **Align range to this pitch** flytter hele tonerækken efter den valgte målte tone og nulstiller godkendelser.

Klik i den store bølgeform for at sætte en klippemarkør. **Split at cursor** deler et sample; **Merge with next** samler to. **Start s / End s** angiver sekunder fra hele optagelsens begyndelse. **Set boundaries** ændrer klippet og måler det igen. Naboklip må ikke overlappe. Split og sammenlægning ændrer efterfølgende tonetildeling og nulstiller godkendelser; kontrollér derfor resultatet.

Klaviaturet viser altid alle 88 tangenter. Importerede toner er gyldne; den valgte tone fremhæves. En stabil manglende tone efterlader et hul, når næste målte tone bekræfter springet. Mangler og usikre målinger skal stadig kontrolleres. Toner uden for A0–C8 kan vælges fra samplelisten.

## Eksport

**Export... → SFZ + WAV folder** opretter en unik undermappe med `Instrument.sfz` og 24-bit PCM WAV. Filerne nummereres i stigende rækkefølge, fx `001_C1.wav`, `002_Csharp1.wav`. Samplerate og mono/stereo bevares. En kort 2 ms kantfade modvirker klik.

**Export... → SFZ + WAV ZIP (.zip)** gemmer samme filer i en almindelig ZIP. Pak den ud, og indlæs `Instrument.sfz` i en SFZ-afspiller. SFZ-filen og WAV-filerne skal blive sammen. Formatet beskrives i `INSTRUMENT-FORMAT.md`. Plugweaver-importen er endnu ikke bygget.

Tidligere `.artisamples`-pakker kan ikke åbnes som SFZ ved blot at omdøbe dem. Geneksportér optagelsen med denne udgave for at få SFZ.

Kun godkendte samples eksporteres med pitchkorrektion. De øvrige bevarer deres originale tonehøjde. Du får ikke automatisk et nyt sample på en manglende tangent.

## Teststatus

- SFZ-testen kontrollerer MIDI-tildeling, relative WAV-navne, manglende B1, ingen dobbelt stemmekorrektion og afvisning af noter uden for MIDI-området. Afspilning i en ekstern SFZ-sampler er endnu ikke afprøvet her.

- C++-lydmotoren er kompileret og kørt med syntetiske toner fra A0 til C8, tonekorrektioner, stilhed og manglende toner.
- Den konkrete `FNF Chromatic Scale Maker.wav` er analyseret med den nye C++-motor: 71 samples, C1–B6, manglende B1. Alle 71 opfylder stemmegrænsen. Samme detektor måler efter korrektion højst cirka 0,86 cents restafvigelse. Dette er intern konsistens, ikke uafhængig måling eller aflytning.
- JUCEs WAV-læser er testet med den uploadede stereofil. 24-bit stereo-WAV-eksport, atomisk skrivning af instrumentpakke og ZIP-genlæsning består en separat integrationstest.
- C++-appkoden er kompileret til objektkode mod de faktiske JUCE 8.0.4-headers. CMake-testprojektet og dets tests består.
- Den indsendte Mac-buildlog bekræfter, at den oprindelige baseline-02 blev linket, og at selvtesten bestod. Den opdaterede pakke skal bygges igen på Mac. GUI, lydudgang og eksportdialoger er ikke verificeret her på en Mac. Der medfølger kildekode og buildfil, ikke en færdig Mac-binary.

## Begrænsninger

Optagelser med overlappende toner kan ikke lydadskilles. Støj, vibrato, korte toner og komplekse overtoner kan give usikre målinger. Detektoren er fokuseret på cirka 24–4600 Hz. Et isoleret oktavfund flytter ikke automatisk resten af rækken.

Pitchkorrektionen er en enkel phase vocoder med resampling til oprindeligt antal frames. Den er til små rettelser og kan farve anslag og stereobillede; sammenlign original og korrigeret lyd. Ingen avanceret transient- eller formantbevaring. Ved afspilning til en anden samplerate bruges lineær resampling.

Maksimal import er 128 MB dekodet float-audio, cirka seks minutter stereo ved 44,1 kHz. Under arbejde kan der bruges flere kopier i RAM. Arbejdet udføres i en baggrundstråd, og redigeringskontroller låses imens. Stop-knappen for aflytning er altid tilgængelig. Ved afbrudt WAV-eksport kan der ligge en delvis eksportmappe; instrumentpakker skrives via en midlertidig fil.

Ingen SFZ-import, projektgemning, loopredigering, velocity-lag eller pluginhosting endnu. Appen er en selvstændig sampleeditor, ikke et plugin.

## Kode

- `Source/Main.cpp`: JUCE-vindue, layout, filvalg, lydudgang og eksport.
- `Source/Sfz.h`: standard-SFZ-kortlægning og fælles WAV-filnavne.
- `Source/Dsp.h`: anslagsdetektion, YIN-toneanalyse, kromatisk kortlægning og længdebevarende pitchkorrektion.
- `Assets/icon.svg` og `icon.png`: det farverige ikon i vektor og raster.
- `Tests/dsp_test.cpp`: selvtest af lydmotoren.
- `CMakeLists.txt`: C++20 og fastlåst JUCE-version.

JUCEs licensoplysninger følger i `JUCE-LICENSE.md`. JUCE er en separat afhængighed under sine egne licensvilkår.
