#ifndef COMMON_HPP_
#define COMMON_HPP_

#include <string>
#include <memory>
#include <vector>
#include <functional>

namespace sen {
using std::string;
using std::shared_ptr;
using std::unique_ptr;
using std::vector;
using std::function;

using uint = unsigned;
using uintptr = uintptr_t;
}

#ifdef INPUT_WINDOWS
#define UNICODE
#include <windows.h>
#endif

namespace sen {

//UTF-16 to UTF-8
struct utf8_t {
  utf8_t(const wchar_t *s = L"") { operator=(s); }
  ~utf8_t() { reset(); }

  utf8_t(const utf8_t &) = delete;
  auto operator=(const utf8_t &) -> utf8_t & = delete;

  auto operator=(const wchar_t *s) -> utf8_t & {
    reset();
    if (!s) s = L"";
    length = WideCharToMultiByte(CP_UTF8, 0, s, -1, nullptr, 0, nullptr, nullptr);
    buffer = new char[length + 1];
    WideCharToMultiByte(CP_UTF8, 0, s, -1, buffer, length, nullptr, nullptr);
    buffer[length] = 0;
    return *this;
  }

  auto reset() -> void {
    delete[] buffer;
    length = 0;
  }

  operator char *() { return buffer; }
  operator const char *() const { return buffer; }

  auto data() -> char * { return buffer; }
  auto data() const -> const char * { return buffer; }

  auto size() const -> uint { return length; }

 private:
  char *buffer = nullptr;
  uint length = 0;
};

}

#endif //COMMON_HPP_
