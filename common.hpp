#ifndef COMMON_HPP_
#define COMMON_HPP_

#include <string>
#include <memory>
#include <vector>
#include <set>
#include <functional>

#ifdef INPUT_UDEV
#include <cerrno>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <fcntl.h>
#include <libudev.h>
#include <linux/types.h>
#include <linux/input.h>
#endif

namespace sen {
using std::string;
using std::shared_ptr;
using std::unique_ptr;
using std::vector;
using std::function;
using std::set;

using uint = unsigned;
using uintptr = uintptr_t;
using intmax = intmax_t;
using uintmax = uintmax_t;
}

namespace sen {

static inline auto hex(uintmax value, long precision = '0', char padchar = '0') -> string {
  string buffer;
  buffer.resize(sizeof(uintmax) * 2);
  char *p = buffer.data();

  uint size = 0;
  do {
    uint n = value & 15;
    p[size++] = n < 10 ? ('0' + n) : ('a' + n - 10);
    value >>= 4;
  } while (value);
  buffer.resize(size);
  std::reverse(buffer.begin(), buffer.end());
  if (precision)
    buffer.resize(precision, padchar);
  return buffer;
}

template<uint bits>
inline auto sclamp(const intmax x) -> intmax {
  enum : intmax { b = 1ull << (bits - 1), m = b - 1 };
  return (x > m) ? m : (x < -b) ? -b : x;
}

namespace Hash {
struct Hash {
  virtual auto reset() -> void = 0;
  virtual auto input(uint8_t data) -> void = 0;
  virtual auto output() const -> vector<uint8_t> = 0;

  auto input(const void *data, uint64_t size) -> void {
    auto p = (const uint8_t *) data;
    while (size--) input(*p++);
  }
  auto input(const vector<uint8_t> &data) -> void {
    for (auto byte : data) input(byte);
  }
  auto input(const string &data) -> void {
    for (auto byte : data)
      input(byte);
  }

  auto digest() const -> string {
    string result;
    for (auto n : output()) result.append(hex(n, 2L));
    return result;
  }
};

struct CRC32 : Hash {
  using Hash::input;

  static uint32_t GetCRC32(const std::string &data) {
    auto crc = CRC32();
    crc.reset();
    crc.input(data);
    return crc.value();
  }

  auto reset() -> void override {
    checksum = ~0;
  }

  auto input(uint8_t value) -> void override {
    checksum = (checksum >> 8) ^ table(checksum ^ value);
  }

  auto output() const -> vector<uint8_t> override {
    vector<uint8_t> result;

    for (int i = 0; i < 4; ++i) {
      result.push_back(~checksum >> i * 8);
    }

    return result;
  }

  auto value() const -> uint32_t {
    return ~checksum;
  }

 private:
  static auto table(uint8_t index) -> uint32_t {
    static std::array<uint32_t, 256> table = {0};
    static bool initialized = false;

    if(!initialized) {
      initialized = true;
      for(auto i = 0; i < 256; ++i) {
        uint32_t crc = i;
        for(auto bit = 0; bit < 8; ++bit) {
          crc = (crc >> 1) ^ (crc & 1 ? 0xedb8'8320 : 0);
        }
        table[i] = crc;
      }
    }

    return table[index];
  }

  uint32_t checksum = 0;
};
}

}

#endif //COMMON_HPP_
