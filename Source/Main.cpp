#include "BinaryData.h"
#include "Dsp.h"
#include "Sfz.h"
#include <atomic>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_gui_extra/juce_gui_extra.h>
#include <thread>
using namespace juce;
namespace palette {
const Colour bg(0xff180e21), panel(0xff281329), wine(0xff43192e),
    line(0xff784455), gold(0xfff4c666), text(0xffffedda), muted(0xffc6adc0);
}
// Georgia is standard on macOS and Windows but usually absent on Linux.
// JUCE 8.0.4 can crash while laying out text in a typeface that does not
// exist, so pick an installed serif and fall back to the default UI font.
static String titleTypeface() {
  const auto installed = Font::findAllTypefaceNames();
  for (auto name : {"Georgia", "DejaVu Serif", "Liberation Serif", "Noto Serif",
                    "FreeSerif"})
    if (installed.contains(name, true))
      return name;
  return Font::getDefaultSansSerifFontName();
}
class ForgeLook final : public LookAndFeel_V4 {
public:
  ForgeLook() {
    setColour(Label::textColourId, palette::text);
    setColour(TextEditor::backgroundColourId, palette::bg);
    setColour(TextEditor::textColourId, palette::text);
    setColour(TextEditor::outlineColourId, palette::line);
    setColour(ComboBox::backgroundColourId, palette::wine);
    setColour(ComboBox::textColourId, palette::text);
    setColour(ComboBox::outlineColourId, palette::line);
    setColour(PopupMenu::backgroundColourId, palette::panel);
    setColour(PopupMenu::textColourId, palette::text);
    setColour(ListBox::backgroundColourId, Colours::transparentBlack);
    setColour(ToggleButton::textColourId, palette::text);
    setColour(ToggleButton::tickColourId, palette::gold);
    setColour(ScrollBar::thumbColourId, palette::line);
  }
  void drawButtonBackground(Graphics &g, Button &b, const Colour &, bool hover,
                            bool down) override {
    auto r = b.getLocalBounds().toFloat().reduced(1);
    bool golden = bool(b.getProperties()["gold"]);
    auto c = golden ? palette::gold : palette::wine;
    if (!b.isEnabled())
      c = c.withMultipliedAlpha(.4f);
    if (hover)
      c = c.brighter(.12f);
    if (down)
      c = c.darker(.1f);
    g.setGradientFill(ColourGradient(c.brighter(.12f), 0, r.getY(),
                                     c.darker(.2f), 0, r.getBottom(), false));
    g.fillRoundedRectangle(r, 10);
    g.setColour(golden ? Colour(0xffffe7a1) : palette::line);
    g.drawRoundedRectangle(r, 10, 1);
  }
  void drawButtonText(Graphics &g, TextButton &b, bool, bool) override {
    g.setColour((bool(b.getProperties()["gold"]) ? palette::bg : palette::text)
                    .withMultipliedAlpha(b.isEnabled() ? 1.f : .4f));
    g.setFont(FontOptions(13.f));
    g.drawFittedText(b.getButtonText(), b.getLocalBounds().reduced(4),
                     Justification::centred, 2);
  }
};
class WaveView final : public Component {
public:
  std::shared_ptr<const forge::Recording> recording;
  bool overview = false;
  int selected = 0, cursor = 0;
  std::function<void(int)> clicked;
  void paint(Graphics &g) override {
    auto area = getLocalBounds().toFloat();
    g.setColour(palette::bg.withAlpha(.7f));
    g.fillRoundedRectangle(area, 10);
    g.setColour(palette::line.withAlpha(.3f));
    for (int i = 1; i < 12; ++i)
      g.drawVerticalLine(i * getWidth() / 12, 0, float(getHeight()));
    g.drawHorizontalLine(getHeight() / 2, 0, float(getWidth()));
    if (!recording || recording->channels.empty()) {
      g.setColour(palette::muted);
      g.setFont(FontOptions(14.f));
      g.drawFittedText("Import a chromatic recording", getLocalBounds(),
                       Justification::centred, 2);
      return;
    }
    const auto &r = *recording;
    const auto &x = r.channels[static_cast<std::size_t>(r.analysisChannel)];
    int start = 0, end = int(x.size());
    if (!overview && !r.samples.empty()) {
      start = r.samples[static_cast<std::size_t>(selected)].start;
      end = r.samples[static_cast<std::size_t>(selected)].end;
    }
    if (end <= start || getWidth() == 0)
      return;
    if (overview)
      for (int i = 0; i < int(r.samples.size()); ++i) {
        auto s = r.samples[static_cast<std::size_t>(i)];
        float left = float(s.start) / x.size() * getWidth(),
              right = float(s.end) / x.size() * getWidth();
        if (i == selected) {
          g.setColour(palette::gold.withAlpha(.13f));
          g.fillRect(
              Rectangle<float>(left, 0, right - left, float(getHeight())));
        }
        g.setColour(palette::line);
        g.drawVerticalLine(int(left), 0, float(getHeight()));
      }
    double peak = .01;
    for (int i = start; i < end; ++i)
      peak = std::max(peak, double(std::abs(x[static_cast<std::size_t>(i)])));
    float scale = float(.43 * getHeight() / peak);
    g.setColour(palette::gold);
    Path wave;
    for (int px = 0; px < getWidth(); ++px) {
      int a = start + int(double(px) / getWidth() * (end - start)),
          b = std::min(end,
                       std::max(a + 1, start + int(double(px + 1) / getWidth() *
                                                   (end - start))));
      float low = 0, high = 0;
      for (int j = a; j < b; ++j) {
        low = std::min(low, x[static_cast<std::size_t>(j)]);
        high = std::max(high, x[static_cast<std::size_t>(j)]);
      }
      wave.startNewSubPath(float(px), getHeight() * .5f + low * scale);
      wave.lineTo(float(px), getHeight() * .5f + high * scale);
    }
    g.strokePath(wave, PathStrokeType(1));
    if (!overview) {
      float p = float(cursor - start) / (end - start) * getWidth();
      g.setColour(Colour(0xffffedb6));
      g.drawLine(p, 0, p, float(getHeight()), 1.5f);
    }
  }
  void mouseDown(const MouseEvent &e) override {
    if (!recording || recording->channels.empty() || !clicked)
      return;
    int start = 0, end = int(recording->channels[0].size());
    if (!overview && !recording->samples.empty()) {
      start = recording->samples[static_cast<std::size_t>(selected)].start;
      end = recording->samples[static_cast<std::size_t>(selected)].end;
    }
    clicked(start +
            int(std::clamp(e.position.x / float(std::max(1, getWidth())), 0.f,
                           1.f) *
                (end - start)));
  }
};
class PianoView final : public Component {
  struct Key {
    int midi;
    bool black;
    Rectangle<float> box;
  };
  std::vector<Key> keys;
  bool black(int m) const {
    int k = m % 12;
    return k == 1 || k == 3 || k == 6 || k == 8 || k == 10;
  }
  int index(int midi) const {
    if (recording)
      for (int i = 0; i < int(recording->samples.size()); ++i)
        if (recording->first +
                recording->samples[static_cast<std::size_t>(i)].offset ==
            midi)
          return i;
    return -1;
  }

public:
  std::shared_ptr<const forge::Recording> recording;
  int selected = 0;
  std::function<void(int)> clicked;
  void resized() override {
    keys.clear();
    float w = getWidth() / 52.f;
    int white = 0;
    for (int m = 21; m <= 108; ++m) {
      bool b = black(m);
      keys.push_back({m,
                      b,
                      {(b ? white - .3f : float(white)) * w, 0,
                       w * (b ? .6f : 1.f), getHeight() * (b ? .61f : 1.f)}});
      if (!b)
        ++white;
    }
  }
  void paint(Graphics &g) override {
    for (bool blackPass : {false, true})
      for (auto k : keys) {
        if (k.black != blackPass)
          continue;
        int i = index(k.midi);
        bool mapped = i >= 0, sel = mapped && i == selected;
        Colour top = k.black ? Colour(0xff542e45) : Colour(0xffeee1cc),
               bottom = k.black ? Colour(0xff201321) : Colour(0xffcbbbaf);
        if (mapped) {
          top = k.black ? Colour(0xff986733) : Colour(0xffffeab8);
          bottom = k.black ? Colour(0xff473024) : Colour(0xffe4b75f);
        }
        if (sel) {
          top = Colour(0xfffff4bb);
          bottom = Colour(0xfff7b53e);
        }
        g.setGradientFill(
            ColourGradient(top, 0, 0, bottom, 0, k.box.getBottom(), false));
        g.fillRoundedRectangle(k.box.reduced(.5f), 3);
        g.setColour(sel ? Colour(0xffffefa4) : palette::bg);
        g.drawRoundedRectangle(k.box.reduced(.5f), 3, 1);
        if (mapped) {
          g.setColour(k.black ? palette::gold : Colour(0xff9b6225));
          g.fillEllipse(k.box.getCentreX() - 2,
                        k.box.getBottom() - (k.black ? 12.f : 22.f), 4, 4);
        }
        if (!k.black && (k.midi % 12 == 0 || k.midi == 21)) {
          g.setColour(Colour(0xff54334b));
          g.setFont(FontOptions(10.f));
          g.drawText(String(forge::note(k.midi)),
                     k.box.withTop(k.box.getBottom() - 17),
                     Justification::centred);
        }
      }
  }
  void mouseDown(const MouseEvent &e) override {
    for (bool pass : {true, false})
      for (auto k : keys)
        if (k.black == pass && k.box.contains(e.position)) {
          int i = index(k.midi);
          if (i >= 0 && clicked)
            clicked(i);
          return;
        }
  }
};
class MainComponent final : public AudioAppComponent, private ListBoxModel {
  ForgeLook look;
  std::shared_ptr<forge::Recording> recording;
  std::unique_ptr<Drawable> logo;
  std::unique_ptr<FileChooser> chooser;
  std::thread worker;
  std::atomic<bool> closing{false};
  bool busy = false;
  int selected = 0, cursor = 0;
  double reference = 440;
  ListBox list{"Samples", this};
  WaveView overview, detail;
  PianoView piano;
  Viewport inspectorViewport;
  Component inspector;
  TextButton importButton{"Import audio"}, exportButton{"Export..."},
      reanalyse{"Re-analyse"}, playButton{"Play"}, stopButton{"Stop"},
      trimButton{"Set boundaries"}, splitButton{"Split at cursor"},
      mergeButton{"Merge with next"}, alignButton{"Align range to this pitch"},
      applyButton{"Apply tuning"}, resetButton{"Use original"},
      allButton{"Tune reliable samples"};
  ToggleButton corrected{"Corrected preview"};
  ComboBox firstNote;
  TextEditor threshold, gap, ref, start, end;
  Label title, subtitle, fileName, fileInfo, count, status, selectedNote,
      selectedInfo, tuningInfo, keyboardInfo, mappingInfo;
  Label thresholdLabel, gapLabel, firstLabel, refLabel, startLabel, endLabel;
  Rectangle<int> overviewPanel, listPanel, editorPanel, tuningPanel, pianoPanel;
  CriticalSection audioLock;
  std::vector<std::vector<float>> playback;
  double position = 0, playbackRate = 44100, deviceRate = 44100;
  bool playing = false;
  void label(Label &l, const String &text, float size = 13) {
    addAndMakeVisible(l);
    l.setText(text, dontSendNotification);
    l.setFont(FontOptions(size));
    l.setColour(Label::textColourId, palette::muted);
  }
  void button(TextButton &b, bool golden = false) {
    addAndMakeVisible(b);
    b.getProperties().set("gold", golden);
  }
  void edit(TextEditor &e, const String &text) {
    addAndMakeVisible(e);
    e.setText(text);
    e.setFont(FontOptions(13.f));
    e.setInputRestrictions(12, "0123456789.-");
  }
  int noteAt(int i) const {
    return recording->first +
           recording->samples[static_cast<std::size_t>(i)].offset;
  }
  int span() const {
    return recording && !recording->samples.empty()
               ? recording->samples.back().offset + 1
               : 0;
  }
  double correction(int i) const {
    auto p = recording->samples[static_cast<std::size_t>(i)].pitch;
    return p.hz > 0
               ? -forge::cents(p.hz, forge::frequency(noteAt(i), reference))
               : 0;
  }
  bool canTune(int i) const {
    return recording->samples[static_cast<std::size_t>(i)].pitch.hz > 0 &&
           std::abs(correction(i)) <= 50;
  }
  bool reliable(int i) const {
    return canTune(i) &&
           recording->samples[static_cast<std::size_t>(i)].pitch.confidence >=
               .8;
  }
  void message(const String &text) {
    status.setText(text, dontSendNotification);
  }
  void confirm(const String &text, std::function<void()> action) {
    Component::SafePointer<MainComponent> safe(this);
    AlertWindow::showOkCancelBox(
        AlertWindow::QuestionIcon, "Sampleforge", text, "Continue", "Cancel",
        this, ModalCallbackFunction::create([safe, action](int result) {
          if (safe && result)
            action();
        }));
  }
  void begin(std::function<std::function<void()>()> job) {
    if (busy)
      return;
    if (worker.joinable())
      worker.join();
    busy = true;
    stop();
    refresh();
    Component::SafePointer<MainComponent> safe(this);
    worker = std::thread([safe, job] {
      try {
        auto done = job();
        MessageManager::callAsync([safe, done] {
          if (safe) {
            safe->busy = false;
            done();
            safe->refresh();
          }
        });
      } catch (const std::exception &e) {
        String text = e.what();
        MessageManager::callAsync([safe, text] {
          if (safe) {
            safe->busy = false;
            safe->message("Error: " + text);
            safe->refresh();
          }
        });
      }
    });
  }
  void stop() {
    const ScopedLock lock(audioLock);
    playing = false;
  }
  void approve(bool all) {
    if (!recording || recording->samples.empty() || busy)
      return;
    if (all) {
      int n = 0;
      for (int i = 0; i < int(recording->samples.size()); ++i)
        if (reliable(i)) {
          recording->samples[static_cast<std::size_t>(i)].approved = true;
          ++n;
        }
      message(String(n) + " samples approved for tuned export.");
      refresh();
    } else if (canTune(selected)) {
      auto action = [this] {
        recording->samples[static_cast<std::size_t>(selected)].approved = true;
        message("Tuning approved. Use Corrected preview to compare.");
        refresh();
      };
      if (reliable(selected))
        action();
      else
        confirm(
            "This pitch estimate is uncertain. Approve the small correction? "
            "Listen to Original and Corrected preview before export.",
            action);
    }
  }
  void chooseImport() {
    if (busy)
      return;
    chooser = std::make_unique<FileChooser>(
        "Import a chromatic recording or a series of note files", File{},
        "*.wav;*.aiff;*.aif;*.flac;*.mp3;*.m4a;*.ogg");
    Component::SafePointer<MainComponent> safe(this);
    chooser->launchAsync(FileBrowserComponent::openMode |
                             FileBrowserComponent::canSelectFiles |
                             FileBrowserComponent::canSelectMultipleItems,
                         [safe](const FileChooser &fc) {
                           Array<File> files;
                           for (auto f : fc.getResults())
                             if (f.existsAsFile())
                               files.add(f);
                           if (safe && !files.isEmpty())
                             safe->load(files);
                         });
  }
  // Decodes one file. 'budget' is the remaining decoded size in bytes shared
  // by all files of one import (which may be decoded concurrently).
  static std::vector<std::vector<float>>
  decode(AudioFormatManager &formats, const File &file, double &rate,
         std::atomic<int64> &budget) {
    std::unique_ptr<AudioFormatReader> reader(formats.createReaderFor(file));
    if (!reader)
      throw std::runtime_error(
          ("Cannot decode " + file.getFileName() +
           ". Try WAV, AIFF or FLAC.")
              .toStdString());
    if (reader->lengthInSamples <= 0 || reader->numChannels < 1 ||
        reader->numChannels > 2)
      throw std::runtime_error(
          (file.getFileName() + ": use mono or stereo audio.").toStdString());
    // Divide instead of multiplying: the length comes from the file header
    // and must not overflow the size check.
    const int64 frameBytes = int64(reader->numChannels) * 4;
    const int64 limit = 128 * 1024 * 1024;
    if (reader->lengthInSamples > limit / frameBytes ||
        budget.fetch_sub(reader->lengthInSamples * frameBytes) -
                reader->lengthInSamples * frameBytes <
            0)
      throw std::runtime_error(
          "Use mono/stereo audio up to 128 MB decoded size in total.");
    int n = int(reader->lengthInSamples);
    // Read straight into the returned channels (no intermediate copy).
    std::vector<std::vector<float>> channels(
        reader->numChannels, std::vector<float>(static_cast<std::size_t>(n)));
    std::vector<float *> pointers;
    for (auto &ch : channels)
      pointers.push_back(ch.data());
    AudioBuffer<float> buffer(pointers.data(), int(pointers.size()), n);
    if (!reader->read(&buffer, 0, n, 0, true, true))
      throw std::runtime_error(
          ("Audio read failed: " + file.getFileName()).toStdString());
    rate = reader->sampleRate;
    return channels;
  }
  void load(Array<File> files) {
    double floor = threshold.getText().getDoubleValue(),
           spacing = gap.getText().getDoubleValue(), refHz = reference;
    if (floor < -70 || floor > -12 || spacing < .08 || spacing > 10) {
      message("Use floor -70 to -12 dB and spacing 0.08 to 10 seconds.");
      return;
    }
    if (files.size() > 128) {
      message("Select at most 128 files (one per key).");
      return;
    }
    message(files.size() == 1
                ? "Reading audio and detecting notes..."
                : "Reading " + String(files.size()) +
                      " files and detecting notes...");
    begin([this, files, floor, spacing, refHz] {
      // Called from worker threads; only reads the atomic flag.
      auto cancelled = [this](int, int) {
        if (closing)
          throw std::runtime_error("Cancelled");
      };
      constexpr int64 limit = 128 * 1024 * 1024;
      std::atomic<int64> budget{limit};
      auto r = std::make_shared<forge::Recording>();
      bool pitchOrder = false;
      if (files.size() == 1) {
        AudioFormatManager formats;
        formats.registerBasicFormats();
        r->channels = decode(formats, files[0], r->rate, budget);
        r->name = files[0].getFileName().toStdString();
        forge::analyse(*r, floor, spacing, refHz, cancelled);
      } else {
        // Natural filename order (Note 2 before Note 10) is the fallback
        // order when a file's pitch is uncertain.
        auto sorted = files;
        std::stable_sort(sorted.begin(), sorted.end(),
                         [](const File &a, const File &b) {
                           return a.getFileName().compareNatural(
                                      b.getFileName()) < 0;
                         });
        // Decode all files in parallel, each worker with its own readers.
        std::vector<std::vector<std::vector<float>>> audio(
            static_cast<std::size_t>(sorted.size()));
        std::vector<double> rates(audio.size());
        forge::parallelFor(sorted.size(), [&](int i) {
          cancelled(0, 0);
          AudioFormatManager formats;
          formats.registerBasicFormats();
          audio[static_cast<std::size_t>(i)] =
              decode(formats, sorted[i], rates[static_cast<std::size_t>(i)],
                     budget);
        });
        r->rate = rates[0];
        for (std::size_t i = 1; i < rates.size(); ++i)
          if (std::abs(rates[i] - r->rate) > .5)
            throw std::runtime_error(
                ("All files must use the same sample rate. " +
                 sorted[int(i)].getFileName() + " is " + String(rates[i], 0) +
                 " Hz, " + sorted[0].getFileName() + " is " +
                 String(r->rate, 0) + " Hz.")
                    .toStdString());
        // Mono files are duplicated next to stereo files, so check the limit
        // on the combined size.
        int64 frames = 0;
        std::size_t channelCount = 1;
        for (auto &a : audio) {
          frames += int64(a[0].size());
          channelCount = std::max(channelCount, a.size());
        }
        if (frames * int64(channelCount) * 4 > limit ||
            frames > std::numeric_limits<int>::max())
          throw std::runtime_error(
              "Use mono/stereo audio up to 128 MB decoded size in total.");
        auto order = forge::buildSeries(*r, audio, floor, spacing, refHz,
                                        pitchOrder, cancelled);
        r->name = (String(sorted.size()) + " files: " +
                   sorted[int(order.front())].getFileName() + " ... " +
                   sorted[int(order.back())].getFileName())
                      .toStdString();
      }
      int fileCount = files.size();
      return [this, r, fileCount, pitchOrder] {
        recording = r;
        selected = 0;
        cursor = r->samples.empty() ? 0 : r->samples[0].start;
        message(String(r->samples.size()) + " samples found" +
                (fileCount > 1
                     ? " in " + String(fileCount) + " files (ordered by " +
                           (pitchOrder ? "pitch" : "filename") + ")"
                     : String()) +
                ". Review boundaries and any missing notes.");
      };
    });
  }
  void analyseAgain() {
    if (!recording || busy)
      return;
    double floor = threshold.getText().getDoubleValue(),
           spacing = gap.getText().getDoubleValue();
    if (floor < -70 || floor > -12 || spacing < .08 || spacing > 10) {
      message("Use floor -70 to -12 dB and spacing 0.08 to 10 seconds.");
      return;
    }
    confirm(
        "Replace manual boundaries and tuning approvals with a new analysis?",
        [this, floor, spacing] {
          auto original = recording;
          double refHz = reference;
          message("Re-analysing...");
          begin([this, original, floor, spacing, refHz] {
            auto r = std::make_shared<forge::Recording>(*original);
            forge::analyse(*r, floor, spacing, refHz, [this](int, int) {
              if (closing)
                throw std::runtime_error("Cancelled");
            });
            return [this, r] {
              recording = r;
              selected = 0;
              cursor = r->samples.empty() ? 0 : r->samples[0].start;
              message("Analysis complete.");
            };
          });
        });
  }
  void play() {
    if (busy || !recording || recording->samples.empty())
      return;
    auto r = recording;
    int index = selected;
    double refHz = reference;
    bool tune = corrected.getToggleState();
    message("Preparing preview...");
    begin([this, r, index, refHz, tune] {
      auto audio = std::make_shared<std::vector<std::vector<float>>>(
          forge::render(*r, index, refHz, tune));
      return [this, r, audio, index, tune] {
        const ScopedLock lock(audioLock);
        playback = std::move(*audio);
        position = 0;
        playbackRate = r->rate;
        playing = true;
        message(String(forge::note(
                    r->first +
                    r->samples[static_cast<std::size_t>(index)].offset)) +
                (tune ? " - corrected preview" : " - original preview"));
      };
    });
  }
  void align() {
    if (!recording || recording->samples.empty() || busy)
      return;
    auto s = recording->samples[static_cast<std::size_t>(selected)];
    if (s.pitch.hz <= 0)
      return;
    int first = forge::nearest(s.pitch.hz, reference) - s.offset;
    if (first < 0 || first + span() > 128)
      return;
    confirm("Move the complete range to start at " +
                String(forge::note(first)) +
                "? Tuning approvals will be reset.",
            [this, first] {
              recording->first = first;
              for (auto &sample : recording->samples)
                sample.approved = false;
              refresh();
              message("Range aligned. Review the other notes before tuning.");
            });
  }
  void editSample(int mode) {
    if (busy || !recording || recording->samples.empty())
      return;
    auto original = recording;
    int idx = selected, at = cursor,
        a = int(
            std::lround(start.getText().getDoubleValue() * recording->rate)),
        b = int(std::lround(end.getText().getDoubleValue() * recording->rate));
    message("Updating sample boundaries...");
    begin([this, original, idx, at, a, b, mode] {
      auto r = std::make_shared<forge::Recording>(*original);
      auto &s = r->samples[static_cast<std::size_t>(idx)];
      const auto &mono =
          r->channels[static_cast<std::size_t>(r->analysisChannel)];
      auto measure = [&](forge::Sample &q) {
        q.pitch = forge::pitch(mono, r->rate, q.start, q.end);
        q.approved = false;
      };
      if (mode == 0) {
        if (a < 0 || b > int(mono.size()) || b - a < r->rate * .04 ||
            (idx > 0 &&
             a < r->samples[static_cast<std::size_t>(idx - 1)].end) ||
            (idx + 1 < int(r->samples.size()) &&
             b > r->samples[static_cast<std::size_t>(idx + 1)].start))
          throw std::runtime_error(
              "Invalid or overlapping boundaries. Keep at least 40 ms.");
        s.start = a;
        s.end = b;
        measure(s);
      } else if (mode == 1) {
        if (r->first + r->samples.back().offset >= 127 ||
            at - s.start < r->rate * .08 || s.end - at < r->rate * .08)
          throw std::runtime_error("Place cursor at least 80 ms from both "
                                   "edges, within MIDI range.");
        forge::Sample next{at, s.end, s.offset + 1, {}, false};
        s.end = at;
        measure(s);
        measure(next);
        for (int i = idx + 1; i < int(r->samples.size()); ++i)
          r->samples[static_cast<std::size_t>(i)].offset++;
        r->samples.insert(r->samples.begin() + idx + 1, next);
        for (auto &q : r->samples)
          q.approved = false;
      } else {
        if (idx + 1 >= int(r->samples.size()))
          throw std::runtime_error("There is no next sample.");
        int removed =
            r->samples[static_cast<std::size_t>(idx + 1)].offset - s.offset;
        s.end = r->samples[static_cast<std::size_t>(idx + 1)].end;
        measure(s);
        r->samples.erase(r->samples.begin() + idx + 1);
        for (int i = idx + 1; i < int(r->samples.size()); ++i)
          r->samples[static_cast<std::size_t>(i)].offset -= removed;
        for (auto &q : r->samples)
          q.approved = false;
      }
      return [this, r, idx] {
        recording = r;
        selected = idx;
        cursor = r->samples[static_cast<std::size_t>(idx)].start;
        message("Boundaries and pitch updated. Check the note mapping.");
      };
    });
  }
  void chooseExport() {
    if (busy || !recording || recording->samples.empty())
      return;
    chooser = std::make_unique<FileChooser>(
        "Choose parent folder for exported samples",
        File::getSpecialLocation(File::userDocumentsDirectory), "");
    Component::SafePointer<MainComponent> safe(this);
    chooser->launchAsync(FileBrowserComponent::openMode |
                             FileBrowserComponent::canSelectDirectories,
                         [safe](const FileChooser &fc) {
                           if (safe && fc.getResult().isDirectory())
                             safe->exportTo(fc.getResult());
                         });
  }
  void choosePackage() {
    if (busy || !recording || recording->samples.empty())
      return;
    chooser = std::make_unique<FileChooser>(
        "Save SFZ instrument ZIP",
        File::getSpecialLocation(File::userDocumentsDirectory)
            .getChildFile("Instrument.zip"),
        "*.zip");
    Component::SafePointer<MainComponent> safe(this);
    chooser->launchAsync(
        FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles |
            FileBrowserComponent::warnAboutOverwriting,
        [safe](const FileChooser &fc) {
          if (safe && fc.getResult() != File{}) {
            auto file = fc.getResult().withFileExtension("zip");
            if (file != fc.getResult() && file.existsAsFile())
              safe->confirm(
                  "Replace existing SFZ ZIP " + file.getFileName() + "?",
                  [safe, file] {
                    if (safe)
                      safe->exportTo(
                          File::getSpecialLocation(File::tempDirectory), file);
                  });
            else
              safe->exportTo(File::getSpecialLocation(File::tempDirectory),
                             file);
          }
        });
  }
  void exportMenu() {
    PopupMenu menu;
    menu.addItem(1, "SFZ + WAV folder");
    menu.addItem(2, "SFZ + WAV ZIP (.zip)");
    Component::SafePointer<MainComponent> safe(this);
    menu.showMenuAsync(PopupMenu::Options().withTargetComponent(&exportButton),
                       [safe](int result) {
                         if (safe) {
                           if (result == 1)
                             safe->chooseExport();
                           if (result == 2)
                             safe->choosePackage();
                         }
                       });
  }
  void exportTo(File parent, File package = File{}) {
    auto r = recording;
    double refHz = reference;
    File folder = parent.getChildFile(
        "Sampleforge-" + Time::getCurrentTime().formatted("%Y%m%d-%H%M%S") +
        "-" + Uuid().toString().substring(0, 6));
    message("Rendering and exporting samples...");
    begin([this, r, refHz, folder, package] {
      struct Cleanup {
        File folder;
        bool active;
        ~Cleanup() {
          if (active)
            folder.deleteRecursively();
        }
      } cleanup{folder, package != File{}};
      auto created = folder.createDirectory();
      if (created.failed())
        throw std::runtime_error(created.getErrorMessage().toStdString());
      WavAudioFormat wav;
      for (int i = 0; i < int(r->samples.size()); ++i) {
        if (closing)
          throw std::runtime_error("Cancelled; partial export retained.");
        auto s = r->samples[static_cast<std::size_t>(i)];
        auto audio = forge::render(*r, i, refHz, s.approved);
        String filename = forge::sampleFilename(static_cast<std::size_t>(i),
                                                r->first + s.offset);
        auto stream = folder.getChildFile(filename).createOutputStream();
        if (!stream)
          throw std::runtime_error("Cannot create WAV file.");
        std::unique_ptr<AudioFormatWriter> writer(wav.createWriterFor(
            stream.get(), r->rate, unsigned(audio.size()), 24, {}, 0));
        if (!writer)
          throw std::runtime_error("Cannot create WAV writer.");
        stream.release();
        std::vector<const float *> pointers;
        for (auto &ch : audio)
          pointers.push_back(ch.data());
        pointers.push_back(nullptr);
        if (!writer->writeFromFloatArrays(pointers.data(), int(audio.size()),
                                          int(audio[0].size())))
          throw std::runtime_error("WAV write failed (disk full?).");
        writer.reset();
      }
      if (!folder.getChildFile("Instrument.sfz")
               .replaceWithText(String(forge::sfzInstrument(*r))))
        throw std::runtime_error("WAV files saved, but SFZ write failed.");
      if (package != File{}) {
        ZipFile::Builder zip;
        for (auto f : folder.findChildFiles(File::findFiles, false))
          zip.addFile(f, 9, f.getFileName());
        TemporaryFile temporary(package);
        auto stream = temporary.getFile().createOutputStream();
        if (!stream || !zip.writeToStream(*stream, nullptr))
          throw std::runtime_error("Could not write instrument package.");
        stream->flush();
        if (stream->getStatus().failed())
          throw std::runtime_error("Instrument package write failed.");
        stream.reset();
        if (!temporary.overwriteTargetFileWithTemporary())
          throw std::runtime_error("Could not commit instrument package.");
      }
      return [this, folder, r, package] {
        message(String(r->samples.size()) +
                (package != File{} ? " samples saved. Unzip, then load "
                                     "Instrument.sfz in an SFZ player."
                                   : " samples saved with Instrument.sfz. Keep "
                                     "the folder together."));
        (package != File{} ? package : folder).revealToUser();
      };
    });
  }
  void refresh() {
    bool has = recording && !recording->samples.empty();
    for (auto *b : {&importButton, &exportButton, &reanalyse, &playButton,
                    &trimButton, &splitButton, &mergeButton, &alignButton,
                    &applyButton, &resetButton, &allButton})
      b->setEnabled(!busy && (b == &importButton || has));
    stopButton.setEnabled(true);
    reanalyse.setEnabled(!busy && bool(recording));
    for (auto *e : {&threshold, &gap, &ref, &start, &end})
      e->setEnabled(!busy);
    corrected.setEnabled(!busy && has);
    firstNote.setEnabled(!busy && has);
    list.setEnabled(!busy);
    overview.setEnabled(!busy);
    detail.setEnabled(!busy);
    piano.setEnabled(!busy);
    overview.recording = detail.recording = piano.recording = recording;
    overview.selected = detail.selected = piano.selected = selected;
    detail.cursor = cursor;
    if (recording) {
      fileName.setText(recording->name, dontSendNotification);
      fileInfo.setText(
          String(recording->channels[0].size() / recording->rate, 1) +
              " s  |  " + String(recording->rate, 0) + " Hz  |  " +
              (recording->channels.size() == 1 ? "Mono" : "Stereo"),
          dontSendNotification);
    }
    count.setText(has ? String(recording->samples.size()) + " samples"
                      : "No samples",
                  dontSendNotification);
    if (has) {
      auto s = recording->samples[static_cast<std::size_t>(selected)];
      firstNote.setSelectedId(recording->first + 1, dontSendNotification);
      for (int m = 0; m < 128; ++m)
        firstNote.setItemEnabled(m + 1, m + span() <= 128);
      selectedNote.setText(forge::note(noteAt(selected)), dontSendNotification);
      selectedInfo.setText(
          String(selected + 1) + " / " + String(recording->samples.size()) +
              "   " + String((s.end - s.start) / recording->rate, 3) + " s",
          dontSendNotification);
      start.setText(String(s.start / recording->rate, 3), false);
      end.setText(String(s.end / recording->rate, 3), false);
      String nearest =
          s.pitch.hz > 0
              ? String(forge::note(forge::nearest(s.pitch.hz, reference)))
              : "Unknown";
      tuningInfo.setText(
          "Detected frequency\n" +
              (s.pitch.hz > 0 ? String(s.pitch.hz, 2) + " Hz" : "Unknown") +
              "\n\nTarget  " + String(forge::note(noteAt(selected))) + "  |  " +
              String(forge::frequency(noteAt(selected), reference), 2) +
              " Hz\nCorrection  " +
              (s.pitch.hz > 0 ? String(correction(selected), 1) + " cents"
                              : "Unknown") +
              "\nNearest note  " + nearest + "\n\n" +
              (reliable(selected) ? "Stable pitch - ready to tune"
               : canTune(selected)
                   ? "Uncertain pitch: review before approving"
                   : "Review note mapping or sample boundaries"),
          dontSendNotification);
      applyButton.setEnabled(!busy && canTune(selected));
      applyButton.setButtonText(s.approved ? "Tuning approved"
                                           : "Apply tuning");
      resetButton.setEnabled(!busy && s.approved);
      mergeButton.setEnabled(!busy &&
                             selected + 1 < int(recording->samples.size()));
      int suggested = s.pitch.hz > 0
                          ? forge::nearest(s.pitch.hz, reference) - s.offset
                          : -1;
      alignButton.setEnabled(!busy && suggested >= 0 &&
                             suggested + span() <= 128 &&
                             suggested != recording->first);
      StringArray missing;
      for (int m = recording->first; m < recording->first + span(); ++m) {
        bool found = false;
        for (auto q : recording->samples)
          if (recording->first + q.offset == m)
            found = true;
        if (!found)
          missing.add(forge::note(m));
      }
      mappingInfo.setText(
          missing.isEmpty()
              ? "Gold = imported samples. Click a marked key to audition."
              : "Missing in detected range: " + missing.joinIntoString(", "),
          dontSendNotification);
      keyboardInfo.setText(
          "88 keys  |  A0 - C8     Imported: " +
              String(forge::note(recording->first)) + " - " +
              String(forge::note(recording->first + span() - 1)),
          dontSendNotification);
    } else {
      selectedNote.setText("--", dontSendNotification);
      selectedInfo.setText("Import audio to begin", dontSendNotification);
      tuningInfo.setText("Pitch correction\n\nImport a recording of notes "
                         "played one at a time in ascending chromatic order.",
                         dontSendNotification);
      mappingInfo.setText("Any number of samples. All 88 keys stay visible.",
                          dontSendNotification);
    }
    list.updateContent();
    if (has)
      list.selectRow(selected);
    overview.repaint();
    detail.repaint();
    piano.repaint();
    repaint();
  }
  int getNumRows() override {
    return recording ? int(recording->samples.size()) : 0;
  }
  void paintListBoxItem(int row, Graphics &g, int width, int height,
                        bool active) override {
    if (!recording || row >= int(recording->samples.size()))
      return;
    if (active) {
      g.setColour(palette::gold);
      g.fillRoundedRectangle(
          Rectangle<float>(2, 1, float(width - 4), float(height - 2)), 8);
    }
    g.setColour(active ? palette::bg : palette::text);
    g.setFont(FontOptions(14.f));
    g.drawText(forge::note(noteAt(row)), 8, 0, 55, height,
               Justification::centredLeft);
    g.setFont(FontOptions(11.f));
    g.drawText(recording->samples[static_cast<std::size_t>(row)].approved
                   ? "tuned"
               : reliable(row) ? ""
                               : "review",
               60, 0, width - 66, height, Justification::centredRight);
  }
  void selectedRowsChanged(int row) override {
    if (row >= 0 && recording && row < int(recording->samples.size()) &&
        row != selected) {
      selected = row;
      cursor = recording->samples[static_cast<std::size_t>(row)].start;
      stop();
      refresh();
    }
  }

public:
  MainComponent() {
    setLookAndFeel(&look);
    logo = Drawable::createFromImageData(
        BinaryData::icon_svg,
        static_cast<std::size_t>(BinaryData::icon_svgSize));
    overview.overview = true;
    for (Component *c : std::initializer_list<Component *>{
             &list, &overview, &detail, &piano, &firstNote, &corrected,
             &inspectorViewport})
      addAndMakeVisible(c);
    inspectorViewport.setViewedComponent(&inspector, false);
    inspectorViewport.setScrollBarsShown(true, false);
    list.setRowHeight(31);
    label(title, "Artilody Sampleforge", 30);
    title.setFont(Font(FontOptions(titleTypeface(), 30.f, Font::plain)));
    title.setColour(Label::textColourId, palette::text);
    label(subtitle, "TURN RECORDINGS INTO INSTRUMENTS", 10);
    label(fileName, "Your next instrument starts here", 17);
    fileName.setColour(Label::textColourId, palette::text);
    label(fileInfo,
          "Import one recording or a series of note files (WAV, AIFF or FLAC)",
          12);
    label(count, "No recording loaded", 13);
    count.setJustificationType(Justification::centredRight);
    count.setColour(Label::textColourId, palette::gold);
    label(status, "Ready - C++ / JUCE baseline-02", 12);
    status.setColour(Label::textColourId, palette::gold);
    label(selectedNote, "--", 32);
    selectedNote.setColour(Label::textColourId, palette::text);
    label(selectedInfo, "Import audio to begin", 12);
    selectedInfo.setJustificationType(Justification::centredRight);
    label(keyboardInfo, "88 keys  |  A0 - C8", 15);
    label(mappingInfo, "Any instrument. Any chromatic range.", 11);
    label(thresholdLabel, "Floor dB");
    label(gapLabel, "Spacing s");
    label(firstLabel, "First note");
    label(refLabel, "A4 Hz");
    label(startLabel, "Start s");
    label(endLabel, "End s");
    edit(threshold, "-42");
    edit(gap, "0.18");
    edit(ref, "440");
    edit(start, "0.000");
    edit(end, "0.000");
    for (auto *b : {&importButton, &reanalyse, &stopButton, &trimButton,
                    &splitButton, &mergeButton})
      button(*b);
    for (auto *b : {&exportButton, &playButton})
      button(*b, true);
    for (auto *b : {&alignButton, &applyButton, &resetButton, &allButton}) {
      inspector.addAndMakeVisible(b);
      b->getProperties().set("gold", b == &applyButton);
    }
    inspector.addAndMakeVisible(tuningInfo);
    tuningInfo.setFont(FontOptions(15.f));
    tuningInfo.setColour(Label::textColourId, palette::text);
    tuningInfo.setJustificationType(Justification::topLeft);
    for (int m = 0; m < 128; ++m)
      firstNote.addItem(forge::note(m), m + 1);
    firstNote.setSelectedId(22, dontSendNotification);
    importButton.onClick = [this] { chooseImport(); };
    exportButton.onClick = [this] { exportMenu(); };
    reanalyse.onClick = [this] { analyseAgain(); };
    playButton.onClick = [this] { play(); };
    stopButton.onClick = [this] { stop(); };
    trimButton.onClick = [this] { editSample(0); };
    splitButton.onClick = [this] { editSample(1); };
    mergeButton.onClick = [this] { editSample(2); };
    alignButton.onClick = [this] { align(); };
    applyButton.onClick = [this] { approve(false); };
    allButton.onClick = [this] { approve(true); };
    resetButton.onClick = [this] {
      recording->samples[static_cast<std::size_t>(selected)].approved = false;
      refresh();
    };
    firstNote.onChange = [this] {
      if (recording && !busy) {
        recording->first = firstNote.getSelectedId() - 1;
        for (auto &s : recording->samples)
          s.approved = false;
        refresh();
      }
    };
    ref.onFocusLost = ref.onReturnKey = [this] {
      double value = ref.getText().getDoubleValue();
      double updated = std::clamp(value, 400., 480.);
      bool changed = std::abs(reference - updated) > .0001;
      reference = updated;
      ref.setText(String(reference, 1), false);
      if (changed && recording)
        for (auto &s : recording->samples)
          s.approved = false;
      refresh();
    };
    detail.clicked = [this](int frame) {
      if (!busy) {
        cursor = frame;
        detail.cursor = cursor;
        detail.repaint();
        message("Split cursor: " + String(frame / recording->rate, 3) + " s");
      }
    };
    overview.clicked = [this](int frame) {
      if (busy || !recording)
        return;
      for (int i = 0; i < int(recording->samples.size()); ++i) {
        auto s = recording->samples[static_cast<std::size_t>(i)];
        if (frame >= s.start && frame < s.end) {
          selected = i;
          cursor = s.start;
          stop();
          refresh();
          return;
        }
      }
    };
    piano.clicked = [this](int i) {
      if (busy)
        return;
      selected = i;
      cursor = recording->samples[static_cast<std::size_t>(i)].start;
      refresh();
      play();
    };
    setSize(1280, 800);
    setAudioChannels(0, 2);
    refresh();
  }
  ~MainComponent() override {
    closing = true;
    if (worker.joinable())
      worker.join();
    shutdownAudio();
    list.setModel(nullptr);
    inspectorViewport.setViewedComponent(nullptr, false);
    setLookAndFeel(nullptr);
  }
  void prepareToPlay(int, double sampleRate) override {
    const ScopedLock lock(audioLock);
    deviceRate = sampleRate;
  }
  void releaseResources() override {}
  void getNextAudioBlock(const AudioSourceChannelInfo &info) override {
    info.clearActiveBufferRegion();
    const ScopedLock lock(audioLock);
    if (!playing || playback.empty())
      return;
    double step = playbackRate / deviceRate;
    for (int i = 0; i < info.numSamples; ++i) {
      size_t a = size_t(position);
      if (a >= playback[0].size()) {
        playing = false;
        break;
      }
      size_t b = std::min(a + 1, playback[0].size() - 1);
      float f = float(position - a);
      for (int ch = 0; ch < info.buffer->getNumChannels(); ++ch) {
        const auto &x = playback[size_t(ch) % playback.size()];
        info.buffer->setSample(ch, info.startSample + i,
                               x[static_cast<std::size_t>(a)] * (1 - f) +
                                   x[static_cast<std::size_t>(b)] * f);
      }
      position += step;
    }
  }
  void paint(Graphics &g) override {
    g.setGradientFill(ColourGradient(palette::wine, float(getWidth()), 0,
                                     palette::bg, 0, float(getHeight()),
                                     false));
    g.fillAll();
    for (auto r :
         {overviewPanel, listPanel, editorPanel, tuningPanel, pianoPanel}) {
      g.setColour(palette::panel.withAlpha(.85f));
      g.fillRoundedRectangle(r.toFloat(), 18);
      g.setColour(palette::line);
      g.drawRoundedRectangle(r.toFloat().reduced(.5f), 18, 1);
    }
    if (logo)
      logo->drawWithin(g, Rectangle<float>(18, 13, 54, 54),
                       RectanglePlacement::centred, 1);
    Path curve;
    curve.startNewSubPath(getWidth() * .44f, 8);
    curve.cubicTo(getWidth() * .58f, 0, getWidth() * .59f, 80,
                  getWidth() * .76f, 63);
    g.setColour(palette::gold.withAlpha(.4f));
    g.strokePath(curve, PathStrokeType(1));
    g.setColour(palette::muted);
    g.setFont(FontOptions(15.f));
    g.drawText("Samples", listPanel.reduced(12).removeFromTop(20),
               Justification::centredLeft);
    g.drawText("Pitch correction", tuningPanel.reduced(12).removeFromTop(20),
               Justification::centredLeft);
  }
  void resized() override {
    int w = getWidth(), h = getHeight();
    title.setBounds(84, 14, w - 410, 36);
    subtitle.setBounds(86, 49, w - 410, 18);
    importButton.setBounds(w - 304, 23, 132, 36);
    exportButton.setBounds(w - 160, 23, 142, 36);
    int overviewH = h < 740 ? 106 : 128;
    overviewPanel = {16, 82, w - 32, overviewH};
    fileName.setBounds(30, 90, w - 280, 25);
    fileInfo.setBounds(30, 114, w - 280, 18);
    count.setBounds(w - 245, 91, 212, 25);
    overview.setBounds(30, 137, w - 60, overviewH - 63);
    int y = overviewPanel.getBottom() + 9;
    thresholdLabel.setBounds(18, y, 58, 28);
    threshold.setBounds(77, y, 54, 28);
    gapLabel.setBounds(143, y, 68, 28);
    gap.setBounds(213, y, 55, 28);
    reanalyse.setBounds(281, y, 103, 28);
    firstLabel.setBounds(400, y, 66, 28);
    firstNote.setBounds(468, y, 85, 28);
    refLabel.setBounds(570, y, 47, 28);
    ref.setBounds(619, y, 70, 28);
    pianoPanel = {16, h - 149, w - 32, 115};
    keyboardInfo.setBounds(29, h - 145, w / 2, 22);
    mappingInfo.setBounds(w / 2, h - 145, w / 2 - 30, 22);
    piano.setBounds(29, h - 117, w - 58, 74);
    status.setBounds(19, h - 29, w - 38, 24);
    int top = y + 39, bottom = pianoPanel.getY() - 10,
        midH = std::max(120, bottom - top), listW = w < 1100 ? 140 : 162,
        tuneW = w < 1100 ? 250 : 278;
    listPanel = {16, top, listW, midH};
    tuningPanel = {w - 16 - tuneW, top, tuneW, midH};
    editorPanel = {listPanel.getRight() + 10, top,
                   tuningPanel.getX() - listPanel.getRight() - 20, midH};
    list.setBounds(listPanel.reduced(8).withTrimmedTop(29));
    int ex = editorPanel.getX() + 12, ew = editorPanel.getWidth() - 24;
    selectedNote.setBounds(ex, top + 8, 105, 39);
    selectedInfo.setBounds(ex + 105, top + 12, ew - 105, 28);
    int buttonsBottom = bottom - 12;
    splitButton.setBounds(ex, buttonsBottom - 28, ew / 2 - 4, 28);
    mergeButton.setBounds(ex + ew / 2 + 4, buttonsBottom - 28, ew / 2 - 4, 28);
    int trimY = buttonsBottom - 64;
    startLabel.setBounds(ex, trimY, 47, 28);
    start.setBounds(ex + 47, trimY, 69, 28);
    endLabel.setBounds(ex + 124, trimY, 42, 28);
    end.setBounds(ex + 166, trimY, 69, 28);
    trimButton.setBounds(ex + 242, trimY, std::max(90, ew - 242), 28);
    int playY = buttonsBottom - 101;
    playButton.setBounds(ex, playY, 70, 29);
    stopButton.setBounds(ex + 78, playY, 60, 29);
    corrected.setBounds(ex + 148, playY, ew - 148, 29);
    detail.setBounds(ex, top + 51, ew, std::max(45, playY - top - 60));
    inspectorViewport.setBounds(tuningPanel.reduced(11).withTrimmedTop(27));
    int iw = inspectorViewport.getWidth() - 14;
    inspector.setSize(iw, 382);
    tuningInfo.setBounds(0, 0, iw, 212);
    alignButton.setBounds(0, 214, iw, 30);
    applyButton.setBounds(0, 251, iw, 32);
    resetButton.setBounds(0, 290, iw, 30);
    allButton.setBounds(0, 327, iw, 32);
  }
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
class SampleforgeApplication final : public JUCEApplication {
  class Window final : public DocumentWindow {
  public:
    Window()
        : DocumentWindow("Artilody Sampleforge - baseline-02 | C++ / JUCE",
                         palette::bg, allButtons) {
      setUsingNativeTitleBar(true);
      setContentOwned(new MainComponent(), true);
      setResizable(true, true);
      setResizeLimits(980, 640, 2400, 1800);
      auto *display = Desktop::getInstance().getDisplays().getPrimaryDisplay();
      auto area = display ? display->userArea : Rectangle<int>(0, 0, 1440, 900);
      int w = std::min(1280, area.getWidth() - 36),
          h = std::min(840, area.getHeight() - 48);
      centreWithSize(w, h);
      setVisible(true);
    }
    void closeButtonPressed() override {
      JUCEApplication::getInstance()->systemRequestedQuit();
    }
  };
  std::unique_ptr<Window> window;

public:
  const String getApplicationName() override { return "Artilody Sampleforge"; }
  const String getApplicationVersion() override { return "0.3.0"; }
  void initialise(const String &) override {
    window = std::make_unique<Window>();
  }
  void shutdown() override { window.reset(); }
  bool moreThanOneInstanceAllowed() override { return false; }
  void anotherInstanceStarted(const String &) override {}
};
START_JUCE_APPLICATION(SampleforgeApplication)
