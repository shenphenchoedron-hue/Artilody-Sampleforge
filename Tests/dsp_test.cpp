#include "../Source/Dsp.h"
#include "../Source/Sfz.h"
#include <cassert>
#include <fstream>
#include <iostream>
std::vector<float> tone(double hz, double seconds = 1.2) {
  std::vector<float> x(static_cast<std::size_t>(44100 * seconds));
  for (int i = 0; i < int(x.size()); ++i) {
    double t = i / 44100.;
    x[static_cast<std::size_t>(i)] =
        float(.5 * std::sin(2 * forge::pi * hz * t) * std::min(1., t / .005) *
              std::exp(-1.8 * t));
  }
  return x;
}
int main(int argc, char **argv) {
  for (double hz :
       {27.5, 55., 110., 261.625565, 443., 880., 2093.0045, 4186.009}) {
    auto x = tone(hz);
    auto p = forge::pitch(x, 44100, 0, int(x.size()));
    assert(std::abs(forge::cents(p.hz, hz)) < 10);
  }
  for (double correction : {-40., -11.8, 35.}) {
    auto x = tone(443), y = forge::shift(x, correction);
    assert(x.size() == y.size());
    auto p = forge::pitch(y, 44100, 0, int(y.size()));
    assert(std::abs(forge::cents(p.hz, 443 * std::pow(2, correction / 1200))) <
           5);
  }
  assert(forge::detect(std::vector<float>(44100), 44100).empty());
  std::vector<forge::Sample> s;
  for (int m = 24; m <= 95; ++m)
    if (m != 35) {
      forge::Sample q;
      q.pitch = {forge::frequency(m), .95};
      s.push_back(q);
    }
  int first = forge::inferFirst(s);
  assert(first == 24);
  forge::mapNotes(s, first);
  assert(s.size() == 71 && s[11].offset == 12 && s.back().offset == 71);
  // The exported map preserves actual MIDI roots and missing notes.
  forge::Recording instrument;
  instrument.first = first;
  instrument.samples = s;
  const auto sfz = forge::sfzInstrument(instrument);
  // Template structure: control, global, group, then the regions.
  const auto control = sfz.find("<control>\n"), path = sfz.find("default_path=\n"),
             global = sfz.find("<global>\n"), group = sfz.find("<group>\n"),
             loop = sfz.find("loop_mode=no_loop\n"), region = sfz.find("<region>");
  assert(control != std::string::npos && control < path && path < global &&
         global < group && group < loop && loop < region);
  assert(std::count(sfz.begin(), sfz.end(), '<') - 3 == 71);
  assert(sfz.find("<region> lokey=24 hikey=24 pitch_keycenter=24 "
                  "sample=001_C1.wav\n") != std::string::npos);
  assert(sfz.find("<region> lokey=36 hikey=36 pitch_keycenter=36 "
                  "sample=012_C2.wav\n") != std::string::npos);
  // Only opcodes that Artilody Plugweaver's importer accepts.
  for (auto opcode : {"trigger=", "cutoff=", "fil_type=", "resonance=",
                      "ampeg_", "tune=", "transpose="})
    assert(sfz.find(opcode) == std::string::npos);
  assert(sfz.find("lokey=35 ") == std::string::npos);
  assert(sfz.find("tune=") == std::string::npos);
  assert(forge::sampleFilename(1, 25) == "002_Csharp1.wav");
  assert(forge::sampleFilename(0, 0) == "001_C-1.wav");
  assert(forge::sampleFilename(127, 127) == "128_G9.wav");

  instrument.samples[0].offset = -25;
  bool rejected = false;
  try {
    (void)forge::sfzInstrument(instrument);
  } catch (const std::runtime_error &) {
    rejected = true;
  }
  assert(rejected);
  {
    // parallelFor visits every index once and rethrows a worker's exception
    // (used for cancellation).
    std::vector<std::atomic<int>> seen(500);
    forge::parallelFor(500, [&](int i) { ++seen[size_t(i)]; });
    assert(std::all_of(seen.begin(), seen.end(),
                       [](auto &v) { return v == 1; }));
    bool thrown = false;
    try {
      forge::parallelFor(500, [](int i) {
        if (i == 123)
          throw std::runtime_error("Cancelled");
      });
    } catch (const std::runtime_error &) {
      thrown = true;
    }
    assert(thrown);
  }
  {
    // A series of files, one note per file, ordered by lead pitch and
    // combined. Mono files are upmixed next to a stereo file; the quiet file
    // is still detected because each file is analysed at its own level.
    std::vector<int> midi = {64, 60, 62, 61, 63};
    std::vector<std::vector<std::vector<float>>> files;
    for (int m : midi) {
      auto x = tone(forge::frequency(m), .8);
      if (m == 62)
        for (auto &v : x)
          v *= .01f;
      if (m == 61)
        files.push_back({x, x});
      else
        files.push_back({x});
    }
    // One uncertain pitch keeps the given (filename) order.
    std::vector<forge::Pitch> lead(5, forge::Pitch{440, .95});
    lead[2].confidence = .5;
    std::vector<std::size_t> kept;
    assert(!forge::seriesOrder(lead, kept));
    assert((kept == std::vector<std::size_t>{0, 1, 2, 3, 4}));
    // The same series appended in pitch order and analysed in one go, as
    // a reference for buildSeries.
    forge::Recording reference;
    reference.rate = 44100;
    for (auto i : {1, 3, 2, 4, 0})
      forge::appendSegment(reference, files[size_t(i)], 2);
    forge::analyse(reference, -42, .18, 440);
    forge::Recording series;
    series.rate = 44100;
    bool byPitch = false;
    auto order = forge::buildSeries(series, files, -42, .18, 440, byPitch);
    assert(byPitch && (order == std::vector<std::size_t>{1, 3, 2, 4, 0}));
    assert(series.channels.size() == 2 && series.segments.size() == 5 &&
           series.channels[0].size() == size_t(5 * 35280));
    assert(series.channels == reference.channels &&
           series.segments == reference.segments &&
           series.analysisChannel == reference.analysisChannel &&
           series.first == reference.first);
    for (std::size_t i = 0; i < 5; ++i) {
      auto &a = series.samples[i], &b = reference.samples[i];
      assert(a.start == b.start && a.end == b.end && a.offset == b.offset &&
             a.pitch.hz == b.pitch.hz);
    }
    assert(series.samples.size() == 5 && series.first == 60);
    for (int i = 0; i < 5; ++i) {
      auto &q = series.samples[static_cast<std::size_t>(i)];
      assert(q.offset == i && q.start >= series.segments[size_t(i)] &&
             q.end <= series.segments[size_t(i)] + 35280);
      assert(std::abs(forge::cents(q.pitch.hz, forge::frequency(60 + i))) <
             25);
    }
    // Re-analysis of a copy keeps the file boundaries.
    forge::Recording again = series;
    forge::analyse(again, -42, .18, 440);
    assert(again.samples.size() == 5);
  }
  if (argc > 1) {
    std::ifstream f(argv[1], std::ios::binary | std::ios::ate);
    if (!f)
      throw std::runtime_error("Cannot read PCM fixture");
    size_t bytes = size_t(f.tellg());
    std::vector<float> x(bytes / 4);
    f.seekg(0);
    f.read(reinterpret_cast<char *>(x.data()), std::streamsize(bytes));
    forge::Recording r;
    r.channels.push_back(x);
    forge::analyse(r, -42, .18, 440);
    assert(r.samples.size() == 71 && r.first == 24 &&
           r.samples.back().offset == 71);
    double worst = 0;
    for (int i = 0; i < int(r.samples.size()); ++i) {
      auto &q = r.samples[static_cast<std::size_t>(i)];
      double target = forge::frequency(r.first + q.offset);
      assert(q.pitch.confidence >= .8 &&
             std::abs(forge::cents(q.pitch.hz, target)) < 50);
      auto out = forge::render(r, i, 440, true);
      auto p = forge::pitch(out[0], 44100, 0, int(out[0].size()));
      worst = std::max(worst, std::abs(forge::cents(p.hz, target)));
      assert(out[0].size() == size_t(q.end - q.start));
    }
    std::cout << "Real piano: 71 samples, C1-B6, B1 missing, max residual "
              << worst << " cents\n";
    assert(worst < 2);
  }
  std::cout << "C++ DSP tests passed\n";
}
