#pragma once
#include "Dsp.h"

namespace forge {
// Shared by the WAV writer and SFZ mapping: relative ASCII names only.
inline std::string sampleFilename(std::size_t index, int midi) {
  auto number = std::to_string(index + 1);
  if (number.size() < 3)
    number.insert(0, 3 - number.size(), '0');
  auto name = note(midi);
  if (auto sharp = name.find('#'); sharp != std::string::npos)
    name.replace(sharp, 1, "sharp");
  return number + "_" + name + ".wav";
}

// Structured like the common SFZ template (<control>, <global>, <group>,
// <region>), using only opcodes that sforzando, sfizz and Artilody
// Plugweaver's importer all accept.
inline std::string sfzInstrument(const Recording &recording) {
  const std::string rule(78, '-'), stars(77, '*');
  std::string result =
      "//" + rule + "\n" +
      "// Artilody Sampleforge - SFZ instrument\n"
      "// Keep this file beside its WAV samples.\n"
      "// Approved tuning is already rendered into the WAV files.\n"
      "// Unrecorded keys remain silent; no automatic stretching.\n"
      "//" + rule + "\n"
      "<control>\n"
      "// The samples are in the same folder as this file.\n"
      "default_path=\n"
      "\n"
      "<global>\n"
      "// Parameters that affect the whole instrument go here.\n"
      "\n"
      "// " + stars + "\n"
      "// Mapping: one region per recorded note (MIDI 60 = C4)\n"
      "// " + stars + "\n"
      "\n"
      "<group>\n"
      "// Parameters that affect all regions below.\n"
      "loop_mode=no_loop\n"
      "\n";
  for (std::size_t i = 0; i < recording.samples.size(); ++i) {
    const int midi = recording.first + recording.samples[i].offset;
    if (midi < 0 || midi > 127)
      throw std::runtime_error("SFZ note is outside the MIDI range.");
    const auto key = std::to_string(midi);
    result += "<region> lokey=" + key + " hikey=" + key +
              " pitch_keycenter=" + key + " sample=" + sampleFilename(i, midi) +
              "\n";
  }
  return result;
}
} // namespace forge
