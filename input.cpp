#include "input.hpp"
#include <utility>

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
#include "udev.hpp"
#endif

#if defined(INPUT_WINDOWS)
#include "windows.hpp"
#endif

#if defined(INPUT_XLIB)
#include <ruby/input/xlib.cpp>
#endif

namespace sen {

auto Input::SetContext(uintptr_t context) -> bool {
  if (instance_->context_ == context) return true;
  if (!instance_->HasContext()) return false;
  if (!instance_->SetContext(instance_->context_ = context)) return false;
  return true;
}

auto Input::Acquired() -> bool {
  return instance_->Acquired();
}

auto Input::Acquire() -> bool {
  return instance_->Acquire();
}

auto Input::Release() -> bool {
  return instance_->Release();
}

auto Input::Poll() -> vector<std::shared_ptr<HID::Device>> {
  return instance_->Poll();
}

auto Input::Rumble(uint64_t id, bool enable) -> bool {
  return instance_->Rumble(id, enable);
}

auto Input::OnChange(const function<void(shared_ptr<sen::HID::Device>,
                                         uint,
                                         uint,
                                         int16_t,
                                         int16_t)> &on_change) -> void {
  change = on_change;
}

auto Input::DoChange(shared_ptr<HID::Device> device, uint group, uint input, int16_t old_value, int16_t new_value) -> void {
  if (change) change(std::move(device), group, input, old_value, new_value);
}

auto Input::Create(string driver) -> bool {
  self.instance_.reset();
  if (driver.empty()) driver = OptimalDriver();

  #if defined(INPUT_WINDOWS)
  if(driver == "Windows") self.instance_ = new InputWindows(*this);
  #endif

  #if defined(INPUT_QUARTZ)
  if(driver == "Quartz") self.instance_ = new InputQuartz(*this);
  #endif

  #if defined(INPUT_CARBON)
  if(driver == "Carbon") self.instance_ = new InputCarbon(*this);
  #endif

  #if defined(INPUT_UDEV)
  if (driver == "udev") self.instance_ = std::make_unique<InputUdev>(*this);
  #endif

  #if defined(INPUT_SDL)
  if(driver == "SDL") self.instance_ = new InputSDL(*this);
  #endif

  #if defined(INPUT_XLIB)
  if(driver == "Xlib") self.instance_ = new InputXlib(*this);
  #endif

  if (!self.instance_) self.instance_ = std::make_unique<InputDriver>(*this);

  return self.instance_->Create();
}

auto Input::HasDrivers() -> vector<string> {
  return {
      #if defined(INPUT_WINDOWS)
      "Windows",
      #endif

      #if defined(INPUT_QUARTZ)
      "Quartz",
      #endif

      #if defined(INPUT_CARBON)
      "Carbon",
      #endif

      #if defined(INPUT_UDEV)
      "udev",
      #endif

      #if defined(INPUT_SDL)
      "SDL",
      #endif

      #if defined(INPUT_XLIB)
      "Xlib",
      #endif

      "None"};
}

auto Input::OptimalDriver() -> string {
  #if defined(INPUT_WINDOWS)
  return "Windows";
  #elif defined(INPUT_QUARTZ)
  return "Quartz";
  #elif defined(INPUT_CARBON)
  return "Carbon";
  #elif defined(INPUT_UDEV)
  return "udev";
  #elif defined(INPUT_SDL)
  return "SDL";
  #elif defined(INPUT_XLIB)
  return "Xlib";
  #else
  return "None";
  #endif
}

auto Input::SafestDriver() -> string {
  #if defined(INPUT_WINDOWS)
  return "Windows";
  #elif defined(INPUT_QUARTZ)
  return "Quartz";
  #elif defined(INPUT_CARBON)
  return "Carbon";
  #elif defined(INPUT_UDEV)
  return "udev";
  #elif defined(INPUT_SDL)
  return "SDL";
  #elif defined(INPUT_XLIB)
  return "Xlib";
  #else
  return "none";
  #endif
}

}
