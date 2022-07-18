#ifndef INPUT_HPP_
#define INPUT_HPP_

#include "common.hpp"

namespace sen {
namespace HID {
struct Device;
}

struct Input;
struct InputDriver {
  explicit InputDriver(Input &super) : super_(super) {}
  virtual ~InputDriver() = default;

  virtual auto Create() -> bool { return true; }
  virtual auto Driver() -> string { return "None"; }
  virtual auto Ready() -> bool { return true; }

  virtual auto HasContext() -> bool { return false; }

  virtual auto SetContext(uintptr_t context) -> bool { return true; }

  virtual auto Acquired() -> bool { return false; }
  virtual auto Acquire() -> bool { return false; }
  virtual auto Release() -> bool { return false; }
  virtual auto Poll() -> vector<shared_ptr<sen::HID::Device>> { return {}; }
  virtual auto Rumble(uint64_t id, bool enable) -> bool { return false; }

 protected:
  Input &super_;
  uintptr_t context_{0};

  friend struct Input;
};

struct Input {
  static auto HasDrivers() -> vector<string>;
  static auto HasDriver(const string& driver) -> bool {
    auto drivers = HasDrivers();
    return std::find(drivers.begin(), drivers.end(), driver) != drivers.end();
  }
  static auto OptimalDriver() -> string;
  static auto SafestDriver() -> string;

  Input() : self(*this) { Reset(); }
  explicit operator bool() { return instance_->Driver() != "None"; }
  auto Reset() -> void { instance_ = std::make_unique<InputDriver>(*this); }
  auto Create(string driver = "") -> bool;
  auto Driver() -> string { return instance_->Driver(); }
  auto Ready() -> bool { return instance_->Ready(); }

  auto HasContext() -> bool { return instance_->HasContext(); }

  auto Context() -> uintptr_t { return instance_->context_; }

  auto SetContext(uintptr_t context) -> bool;

  auto Acquired() -> bool;
  auto Acquire() -> bool;
  auto Release() -> bool;
  auto Poll() -> vector<shared_ptr<sen::HID::Device>>;
  auto Rumble(uint64_t id, bool enable) -> bool;

  auto OnChange(const function<void(shared_ptr<sen::HID::Device>, uint, uint, int16_t, int16_t)> &) -> void;
  auto DoChange(
      shared_ptr<sen::HID::Device> device,
      uint group,
      uint input,
      int16_t old_value,
      int16_t new_value) -> void;

 protected:
  Input &self;
  unique_ptr<InputDriver> instance_;
  function<void(shared_ptr<sen::HID::Device>, uint, uint, int16_t, int16_t)> change;
};
}

#if defined(INPUT_CARBON)
#include <ruby/input/carbon.cpp>
#endif

#if defined(INPUT_QUARTZ)
#include <ruby/input/quartz.cpp>
#endif

#if defined(INPUT_SDL)
#include <ruby/input/sdl.cpp>
#endif

#if defined(INPUT_UDEV)
#include <ruby/input/udev.cpp>
#endif

#if defined(INPUT_WINDOWS)
#include "windows.hpp"
#endif

#if defined(INPUT_XLIB)
#include <ruby/input/xlib.cpp>
#endif

#endif //INPUT_HPP_
