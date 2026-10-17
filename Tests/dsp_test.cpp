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
  assert(sfz.find("<region> lokey=24 hikey=24 pitch_keycenter=24 "
                  "loop_mode=no_loop sample=001_C1.wav") != std::string::npos);
  assert(sfz.find("<region> lokey=36 hikey=36 pitch_keycenter=36 "
                  "loop_mode=no_loop sample=012_C2.wav") != std::string::npos);
  assert(sfz.find("lokey=35 ") == std::string::npos);
  assert(sfz.find("tune=") == std::string::npos);
  assert(forge::sampleFilename(1, 25) == "002_Csharp1.wav");
  assert(forge::sampleFilename(0, 0) == "001_C-1.wav");
  assert(forge::sampleFilename(127, 127) == "128_G9.wav");
  assert(std::count(sfz.begin(), sfz.end(), '<') == 71);
  instrument.samples[0].offset = -25;
  bool rejected = false;
  try {
    (void)forge::sfzInstrument(instrument);
  } catch (const std::runtime_error &) {
    rejected = true;
  }
  assert(rejected);
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
