#pragma once
#include <algorithm>
#include <atomic>
#include <cmath>
#include <complex>
#include <cstddef>
#include <exception>
#include <functional>
#include <map>
#include <mutex>
#include <numeric>
#include <span>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
namespace forge {
constexpr double pi = 3.14159265358979323846;
struct Pitch {
  double hz = 0, confidence = 0;
};
struct Sample {
  int start = 0, end = 0, offset = 0;
  Pitch pitch;
  bool approved = false;
};
struct Recording {
  std::vector<std::vector<float>> channels;
  double rate = 44100;
  std::string name;
  std::vector<Sample> samples;
  int first = 21;
  int analysisChannel = 0;
  // Start frame of every imported file when a series of files is combined
  // into one recording. Empty (or a single entry) for a single recording.
  // Notes are then detected per file, so no note crosses a file boundary.
  std::vector<int> segments;
};
inline double frequency(int midi, double reference = 440) {
  return reference * std::pow(2.0, (midi - 69) / 12.0);
}
inline int nearest(double hz, double ref = 440) {
  return int(std::lround(69 + 12 * std::log2(hz / ref)));
}
inline std::string note(int midi) {
  static const char *names[] = {"C",  "C#", "D",  "D#", "E",  "F",
                                "F#", "G",  "G#", "A",  "A#", "B"};
  return std::string(names[(midi % 12 + 12) % 12]) +
         std::to_string(int(std::floor(midi / 12.0)) - 1);
}
inline double cents(double hz, double target) {
  return 1200 * std::log2(hz / target);
}
inline Pitch yin(const std::vector<float> &x, double rate) {
  const int maxLag = std::min(int(rate / 24), int(x.size() / 2) - 1),
            minLag = std::max(2, int(rate / 4600));
  if (maxLag <= minLag)
    return {};
  const int count = std::min(2048, int(x.size()) - maxLag);
  double energy = 0;
  for (int i = 0; i < count; ++i)
    energy +=
        double(x[static_cast<std::size_t>(i)]) * x[static_cast<std::size_t>(i)];
  if (energy / count < 1e-10)
    return {};
  std::vector<double> d(static_cast<std::size_t>(maxLag + 1));
  for (int t = 1; t <= maxLag; ++t) {
    double sum = 0;
    for (int j = 0; j < count; ++j) {
      double v =
          x[static_cast<std::size_t>(j)] - x[static_cast<std::size_t>(j + t)];
      sum += v * v;
    }
    d[static_cast<std::size_t>(t)] = sum;
  }
  double sum = 0;
  for (int t = 1; t <= maxLag; ++t) {
    sum += d[static_cast<std::size_t>(t)];
    d[static_cast<std::size_t>(t)] =
        sum > 0.0 ? d[static_cast<std::size_t>(t)] * t / sum : 1;
  }
  int found = -1;
  for (int t = minLag; t < maxLag; ++t)
    if (d[static_cast<std::size_t>(t)] < .14) {
      while (t + 1 < maxLag && d[static_cast<std::size_t>(t + 1)] <
                                   d[static_cast<std::size_t>(t)])
        ++t;
      found = t;
      break;
    }
  if (found < 0)
    return {};
  double a = d[static_cast<std::size_t>(found - 1)],
         b = d[static_cast<std::size_t>(found)],
         c = d[static_cast<std::size_t>(found + 1)], den = a - 2 * b + c,
         lag = found + (std::abs(den) > 0.0 ? .5 * (a - c) / den : 0);
  return {rate / lag, 1 - b};
}
inline Pitch pitch(std::span<const float> x, double sr, int start, int end) {
  int step = std::max(1, int(sr / 12000));
  double rate = sr / step;
  std::vector<Pitch> values;
  for (double offset : {.07, .18, .32}) {
    int begin = start + int(sr * offset),
        len = std::min(4096, (end - begin) / step);
    if (len < int(std::ceil(rate / 24)) * 2)
      continue;
    std::vector<float> y(static_cast<std::size_t>(len));
    for (int i = 0; i < len; ++i) {
      double sum = 0;
      for (int j = 0; j < step; ++j)
        sum += x[static_cast<std::size_t>(begin + i * step + j)];
      y[static_cast<std::size_t>(i)] = float(sum / step);
    }
    auto p = yin(y, rate);
    if (p.hz > 700) {
      std::vector<float> hi(x.begin() + begin,
                            x.begin() + std::min(end, begin + 4096));
      auto h = yin(hi, sr);
      if (h.hz > 0)
        p = h;
    }
    if (p.hz >= 24 && p.hz <= 4600)
      values.push_back(p);
  }
  if (values.empty())
    return {};
  std::sort(values.begin(), values.end(),
            [](auto a, auto b) { return a.hz < b.hz; });
  auto p = values[values.size() / 2];
  int agree = 0;
  for (auto v : values)
    if (std::abs(cents(v.hz, p.hz)) < 25)
      ++agree;
  p.confidence *= double(agree) / values.size();
  return p;
}
// Runs fn(0) ... fn(n - 1) spread over the CPU cores. fn must be safe to call
// concurrently for different indices. After all workers have stopped, the
// first exception thrown by fn is rethrown; remaining indices are skipped.
inline void parallelFor(int n, const std::function<void(int)> &fn) {
  if (n <= 0)
    return;
  int workers = std::min(n, int(std::max(1u, std::thread::hardware_concurrency())));
  std::atomic<int> next{0};
  std::atomic<bool> failed{false};
  std::exception_ptr error;
  std::mutex errorLock;
  auto run = [&] {
    for (int i; !failed && (i = next++) < n;) {
      try {
        fn(i);
      } catch (...) {
        std::lock_guard<std::mutex> lock(errorLock);
        if (!error)
          error = std::current_exception();
        failed = true;
      }
    }
  };
  std::vector<std::thread> pool;
  for (int t = 1; t < workers; ++t) {
    try {
      pool.emplace_back(run);
    } catch (...) {
      break; // Could not start another thread: continue with fewer.
    }
  }
  run();
  for (auto &t : pool)
    t.join();
  if (error)
    std::rethrow_exception(error);
}
inline std::vector<Sample> detect(std::span<const float> x, double sr,
                                  double db = -42, double gap = .18) {
  int hop = std::max(1, int(std::lround(sr * .005)));
  std::vector<double> env;
  double peak = 0;
  for (int i = 0; i < int(x.size()); i += hop) {
    double sum = 0;
    int end = std::min(i + hop, int(x.size()));
    for (int j = i; j < end; ++j)
      sum += double(x[static_cast<std::size_t>(j)]) *
             x[static_cast<std::size_t>(j)];
    double r = std::sqrt(sum / (end - i));
    env.push_back(r);
    peak = std::max(peak, r);
  }
  if (peak < 1e-7)
    return {};
  double floor = peak * std::pow(10, db / 20);
  std::vector<int> starts;
  double last = -1e10;
  for (int i = 0; i < int(env.size()); ++i) {
    double base = 0;
    int n = 0;
    for (int j = std::max(0, i - 8); j < i; ++j) {
      base += env[static_cast<std::size_t>(j)];
      ++n;
    }
    if (n)
      base /= n;
    if (env[static_cast<std::size_t>(i)] > floor &&
        (base < floor * .65 || env[static_cast<std::size_t>(i)] > base * 2.5) &&
        i * hop - last > sr * gap) {
      int k = i;
      while (k > 0 && k > i - 8 &&
             env[static_cast<std::size_t>(k - 1)] > floor * .25 &&
             env[static_cast<std::size_t>(k - 1)] <
                 env[static_cast<std::size_t>(k)])
        --k;
      starts.push_back(std::max(0, k * hop - int(std::lround(sr * .003))));
      last = i * hop;
    }
  }
  std::vector<Sample> result;
  for (int i = 0; i < int(starts.size()); ++i)
    result.push_back({starts[static_cast<std::size_t>(i)],
                      i + 1 < int(starts.size())
                          ? starts[static_cast<std::size_t>(i + 1)]
                          : int(x.size()),
                      i,
                      {},
                      false});
  return result;
}
inline int inferFirst(const std::vector<Sample> &samples,
                      double reference = 440, int fallback = 21) {
  struct Candidate {
    int index, note;
    double confidence;
  };
  std::vector<Candidate> all, early;
  for (int i = 0; i < int(samples.size()); ++i) {
    auto p = samples[static_cast<std::size_t>(i)].pitch;
    if (p.hz > 0 && p.confidence >= .7) {
      int n = nearest(p.hz, reference) - i;
      if (n >= 0 && n + int(samples.size()) <= 128) {
        Candidate c{i, n, p.confidence};
        all.push_back(c);
        if (i < 8)
          early.push_back(c);
      }
    }
  }
  if (all.empty())
    return std::clamp(fallback, 0, 128 - int(samples.size()));
  auto pool = early.empty() ? all : early;
  if (pool.size() > 8)
    pool.resize(8);
  std::map<int, std::pair<double, int>> scores;
  for (auto c : pool) {
    auto it = scores.find(c.note);
    if (it == scores.end())
      scores[c.note] = {c.confidence, c.index};
    else
      it->second.first += c.confidence;
  }
  int best = pool[0].note;
  for (auto [n, s] : scores)
    if (s.first > scores[best].first ||
        (!(s.first < scores[best].first) && s.second < scores[best].second))
      best = n;
  return best;
}
inline void mapNotes(std::vector<Sample> &samples, int first,
                     double reference = 440) {
  std::vector<int> notes;
  for (auto s : samples)
    notes.push_back(s.pitch.hz > 0 && s.pitch.confidence >= .8
                        ? nearest(s.pitch.hz, reference)
                        : -1);
  int expected = first;
  for (int i = 0; i < int(samples.size()); ++i) {
    int n = notes[static_cast<std::size_t>(i)];
    if (i > 0 && n > expected && n - expected <= 12 &&
        n + int(samples.size()) - i <= 128 && i + 1 < int(notes.size()) &&
        notes[static_cast<std::size_t>(i + 1)] == n + 1)
      expected = n;
    samples[static_cast<std::size_t>(i)].offset = expected - first;
    ++expected;
  }
}
// Onsets within one file of a series, detected at that file's own level so a
// quiet file is not lost next to a loud one. A file in which no onset is
// found (but which is not silent) becomes one whole sample.
inline std::vector<Sample> detectSegment(std::span<const float> x, double rate,
                                         double threshold, double gap) {
  if (double(x.size()) < rate * .04)
    return {};
  auto found = detect(x, rate, threshold, gap);
  if (found.empty() && std::any_of(x.begin(), x.end(), [](float v) {
        return std::abs(v) > 1e-5f;
      }))
    found.push_back({0, int(x.size()), 0, {}, false});
  return found;
}
// progress (if set) may be called from worker threads.
inline void analyse(Recording &r, double threshold, double gap,
                    double reference,
                    std::function<void(int, int)> progress = {}) {
  if (r.channels.empty())
    throw std::runtime_error("Empty recording");
  double best = -1;
  for (int ch = 0; ch < int(r.channels.size()); ++ch) {
    double e = 0;
    for (size_t i = 0; i < r.channels[static_cast<std::size_t>(ch)].size();
         i += 32)
      e += double(r.channels[static_cast<std::size_t>(ch)][i]) *
           r.channels[static_cast<std::size_t>(ch)][i];
    if (e > best) {
      best = e;
      r.analysisChannel = ch;
    }
  }
  const auto &x = r.channels[static_cast<std::size_t>(r.analysisChannel)];
  if (r.segments.size() > 1) {
    // A series of files: detect onsets within each file at its own level.
    r.samples.clear();
    for (std::size_t k = 0; k < r.segments.size(); ++k) {
      int a = std::clamp(r.segments[k], 0, int(x.size())),
          b = k + 1 < r.segments.size()
                  ? std::clamp(r.segments[k + 1], a, int(x.size()))
                  : int(x.size());
      for (auto s : detectSegment(std::span<const float>(x).subspan(
                                      static_cast<std::size_t>(a),
                                      static_cast<std::size_t>(b - a)),
                                  r.rate, threshold, gap)) {
        s.start += a;
        s.end += a;
        r.samples.push_back(s);
      }
    }
  } else {
    r.samples = detect(x, r.rate, threshold, gap);
  }
  if (r.samples.size() > 128)
    throw std::runtime_error(
        "More than 128 attacks. Increase minimum spacing or detection floor.");
  std::atomic<int> done{0};
  const int total = int(r.samples.size());
  parallelFor(total, [&](int i) {
    auto &s = r.samples[static_cast<std::size_t>(i)];
    s.pitch = pitch(x, r.rate, s.start, s.end);
    if (progress)
      progress(++done, total);
  });
  r.first = inferFirst(r.samples, reference, r.first);
  mapNotes(r.samples, r.first, reference);
}
// Order in which a series of files is combined, given the lead pitch of each
// file in its current (filename) order. Ascending pitch when every file has a
// reliable first note; otherwise the current order is kept. Returns whether
// pitch order was used.
inline bool seriesOrder(const std::vector<Pitch> &lead,
                        std::vector<std::size_t> &order) {
  order.resize(lead.size());
  std::iota(order.begin(), order.end(), std::size_t(0));
  bool reliable = std::all_of(lead.begin(), lead.end(), [](const Pitch &p) {
    return p.hz > 0 && p.confidence >= .8;
  });
  if (reliable)
    std::stable_sort(order.begin(), order.end(),
                     [&](std::size_t a, std::size_t b) {
                       return lead[a].hz < lead[b].hz;
                     });
  return reliable;
}
// Appends one file to a recording built from a series of files and records
// where it starts. Mono files are duplicated when the series has stereo files.
inline void appendSegment(Recording &r,
                          const std::vector<std::vector<float>> &channels,
                          std::size_t channelCount) {
  if (channels.empty() || channelCount < channels.size())
    throw std::runtime_error("Invalid channel layout");
  if (r.channels.empty())
    r.channels.resize(channelCount);
  if (r.channels.size() != channelCount)
    throw std::runtime_error("Invalid channel layout");
  r.segments.push_back(int(r.channels[0].size()));
  for (std::size_t ch = 0; ch < channelCount; ++ch) {
    const auto &src = channels[std::min(ch, channels.size() - 1)];
    r.channels[ch].insert(r.channels[ch].end(), src.begin(), src.end());
  }
}
// Combines a series of decoded files into r (r.rate must be set) and analyses
// it, giving the same result as appending the files and calling analyse(),
// but every note is measured only once: the pitch of each file's first note
// also decides the order (seriesOrder). Files are released as they are
// appended. Returns the order used; pitchOrder tells whether it is by pitch.
// progress (if set) may be called from worker threads.
inline std::vector<std::size_t>
buildSeries(Recording &r, std::vector<std::vector<std::vector<float>>> &files,
            double threshold, double gap, double reference, bool &pitchOrder,
            const std::function<void(int, int)> &progress = {}) {
  std::size_t channelCount = 1, frames = 0;
  for (const auto &f : files) {
    if (f.empty() || f[0].empty())
      throw std::runtime_error("Empty file in series");
    channelCount = std::max(channelCount, f.size());
    frames += f[0].size();
  }
  // Same rule as analyse(): the channel with the most energy overall.
  std::vector<double> energy(channelCount);
  for (const auto &f : files)
    for (std::size_t ch = 0; ch < channelCount; ++ch) {
      const auto &x = f[std::min(ch, f.size() - 1)];
      for (std::size_t i = 0; i < x.size(); i += 32)
        energy[ch] += double(x[i]) * x[i];
    }
  r.analysisChannel = int(std::max_element(energy.begin(), energy.end()) -
                          energy.begin());
  auto channel = [&](std::size_t k) -> const std::vector<float> & {
    const auto &f = files[k];
    return f[std::min(std::size_t(r.analysisChannel), f.size() - 1)];
  };
  std::vector<std::vector<Sample>> local(files.size());
  std::vector<std::pair<std::size_t, std::size_t>> jobs;
  for (std::size_t k = 0; k < files.size(); ++k) {
    local[k] = detectSegment(channel(k), r.rate, threshold, gap);
    for (std::size_t i = 0; i < local[k].size(); ++i)
      jobs.emplace_back(k, i);
  }
  if (jobs.size() > 128)
    throw std::runtime_error(
        "More than 128 attacks. Increase minimum spacing or detection floor.");
  std::atomic<int> done{0};
  const int total = int(jobs.size());
  parallelFor(total, [&](int j) {
    auto [k, i] = jobs[static_cast<std::size_t>(j)];
    auto &s = local[k][i];
    s.pitch = pitch(channel(k), r.rate, s.start, s.end);
    if (progress)
      progress(++done, total);
  });
  std::vector<Pitch> lead;
  for (const auto &found : local)
    lead.push_back(found.empty() ? Pitch{} : found[0].pitch);
  std::vector<std::size_t> order;
  pitchOrder = seriesOrder(lead, order);
  r.channels.assign(channelCount, {});
  for (auto &ch : r.channels)
    ch.reserve(frames);
  r.segments.clear();
  r.samples.clear();
  for (auto k : order) {
    int base = int(r.channels[0].size());
    appendSegment(r, files[k], channelCount);
    files[k].clear();
    files[k].shrink_to_fit();
    for (auto s : local[k]) {
      s.start += base;
      s.end += base;
      r.samples.push_back(s);
    }
  }
  r.first = inferFirst(r.samples, reference, r.first);
  mapNotes(r.samples, r.first, reference);
  return order;
}
inline void fft(std::vector<std::complex<double>> &a, bool inverse = false) {
  size_t n = a.size();
  for (size_t i = 1, j = 0; i < n; ++i) {
    size_t bit = n >> 1;
    for (; j & bit; bit >>= 1)
      j ^= bit;
    j ^= bit;
    if (i < j)
      std::swap(a[static_cast<std::size_t>(i)], a[static_cast<std::size_t>(j)]);
  }
  for (size_t len = 2; len <= n; len <<= 1) {
    double angle = (inverse ? 2 : -2) * pi / len;
    auto root = std::polar(1.0, angle);
    for (size_t i = 0; i < n; i += len) {
      std::complex<double> w = 1;
      for (size_t j = 0; j < len / 2; ++j) {
        auto u = a[static_cast<std::size_t>(i + j)],
             v = a[static_cast<std::size_t>(i + j + len / 2)] * w;
        a[static_cast<std::size_t>(i + j)] = u + v;
        a[static_cast<std::size_t>(i + j + len / 2)] = u - v;
        w *= root;
      }
    }
  }
  if (inverse)
    for (auto &v : a)
      v /= double(n);
}
inline std::vector<float> shift(const std::vector<float> &x,
                                double correction) {
  if (std::abs(correction) < .02)
    return x;
  double ratio = std::pow(2, correction / 1200);
  constexpr int N = 2048, H = 256;
  std::vector<float> input(x.size() + 2 * N);
  std::copy(x.begin(), x.end(), input.begin() + N);
  std::vector<double> window(N);
  for (int i = 0; i < N; ++i)
    window[static_cast<std::size_t>(i)] = .5 - .5 * std::cos(2 * pi * i / N);
  struct Frame {
    std::vector<double> mag, phase;
  };
  std::vector<Frame> frames;
  for (int pos = 0; pos + N <= int(input.size()); pos += H) {
    std::vector<std::complex<double>> a(N);
    for (int i = 0; i < N; ++i)
      a[static_cast<std::size_t>(i)] =
          input[static_cast<std::size_t>(pos + i)] *
          window[static_cast<std::size_t>(i)];
    fft(a);
    Frame f;
    f.mag.resize(N / 2 + 1);
    f.phase.resize(N / 2 + 1);
    for (int k = 0; k <= N / 2; ++k) {
      f.mag[static_cast<std::size_t>(k)] =
          std::abs(a[static_cast<std::size_t>(k)]);
      f.phase[static_cast<std::size_t>(k)] =
          std::arg(a[static_cast<std::size_t>(k)]);
    }
    frames.push_back(std::move(f));
  }
  if (frames.size() < 2)
    return x;
  int length = int(std::ceil(input.size() * ratio)) + N;
  std::vector<double> out(static_cast<std::size_t>(length)),
      norm(static_cast<std::size_t>(length)), phase = frames[0].phase;
  int outPos = 0;
  for (double t = 0; t < double(frames.size() - 1);
       t += 1 / ratio, outPos += H) {
    int idx = int(t);
    double alpha = t - idx;
    auto &a = frames[static_cast<std::size_t>(idx)];
    auto &b = frames[static_cast<std::size_t>(idx + 1)];
    std::vector<std::complex<double>> z(N);
    for (int k = 0; k <= N / 2; ++k) {
      double mag = a.mag[static_cast<std::size_t>(k)] * (1 - alpha) +
                   b.mag[static_cast<std::size_t>(k)] * alpha;
      z[static_cast<std::size_t>(k)] =
          std::polar(mag, phase[static_cast<std::size_t>(k)]);
      if (k > 0 && k < N / 2)
        z[static_cast<std::size_t>(N - k)] =
            std::conj(z[static_cast<std::size_t>(k)]);
      double expected = 2 * pi * H * k / N,
             delta = b.phase[static_cast<std::size_t>(k)] -
                     a.phase[static_cast<std::size_t>(k)] - expected;
      delta -= 2 * pi * std::round(delta / (2 * pi));
      phase[static_cast<std::size_t>(k)] += expected + delta;
    }
    fft(z, true);
    for (int i = 0; i < N && outPos + i < length; ++i) {
      out[static_cast<std::size_t>(outPos + i)] +=
          z[static_cast<std::size_t>(i)].real() *
          window[static_cast<std::size_t>(i)];
      norm[static_cast<std::size_t>(outPos + i)] +=
          window[static_cast<std::size_t>(i)] *
          window[static_cast<std::size_t>(i)];
    }
  }
  for (int i = 0; i < length; ++i)
    if (norm[static_cast<std::size_t>(i)] > 1e-8)
      out[static_cast<std::size_t>(i)] /= norm[static_cast<std::size_t>(i)];
  std::vector<float> y(x.size());
  for (int i = 0; i < int(y.size()); ++i) {
    double pos = (i + N) * ratio;
    int j = int(pos);
    double f = pos - j;
    y[static_cast<std::size_t>(i)] =
        float((j < length ? out[static_cast<std::size_t>(j)] : 0) * (1 - f) +
              (j + 1 < length ? out[static_cast<std::size_t>(j + 1)] : 0) * f);
  }
  return y;
}
inline std::vector<std::vector<float>> render(const Recording &r, int index,
                                              double ref, bool corrected) {
  const auto &s = r.samples.at(static_cast<std::size_t>(index));
  double target = frequency(r.first + s.offset, ref),
         correction = s.pitch.hz > 0 ? -cents(s.pitch.hz, target) : 0;
  if (corrected && (s.pitch.hz <= 0 || std::abs(correction) > 50))
    throw std::runtime_error(
        "Review note mapping before tuning (maximum 50 cents).");
  std::vector<std::vector<float>> out;
  for (const auto &ch : r.channels) {
    std::vector<float> x(ch.begin() + s.start, ch.begin() + s.end);
    if (corrected)
      x = shift(x, correction);
    int fade = std::min(int(std::lround(r.rate * .002)), int(x.size() / 4));
    for (int j = 0; j < fade; ++j) {
      x[static_cast<std::size_t>(j)] *= float(j) / fade;
      x[x.size() - 1 - static_cast<std::size_t>(j)] *= float(j) / fade;
    }
    out.push_back(std::move(x));
  }
  return out;
}
} // namespace forge
