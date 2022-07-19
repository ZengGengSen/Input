#ifndef UDEV_HPP_
#define UDEV_HPP_

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

#include "keyboard/xlib.hpp"
#include "mouse/xlib.hpp"
#include "joypad/udev.hpp"

namespace sen {

struct InputUdev : InputDriver {
  InputUdev &self = *this;
  explicit InputUdev(Input &super) : InputDriver(super)/*, keyboard(super), mouse(super)*/, joypad(super) {}
  ~InputUdev() override { Terminate(); }

  auto Create() -> bool override {
    return Initialize();
  }

  auto Driver() -> string override { return "udev"; }
  auto Ready() -> bool override { return isReady; }

  auto HasContext() -> bool override { return true; }

  auto SetContext(uintptr context) -> bool override { return Initialize(); }

  // auto Acquired() -> bool override { return mouse.acquired(); }
  // auto Acquire() -> bool override { return mouse.acquire(); }
  // auto Release() -> bool override { return mouse.release(); }

  auto Poll() -> vector<shared_ptr<HID::Device>> override {
    vector<shared_ptr<HID::Device>> devices;
    // keyboard.poll(devices);
    // mouse.poll(devices);
    joypad.Poll(devices);
    return devices;
  }

  auto Rumble(uint64_t id, bool enable) -> bool override {
    return joypad.Rumble(id, enable);
  }

 private:
  auto Initialize() -> bool {
    Terminate();
    if (!self.context_) return false;
    // if (!keyboard.initialize()) return false;
    // if (!mouse.initialize(self.context_)) return false;
    if (!joypad.Initialize()) return false;
    return isReady = true;
  }

  auto Terminate() -> void {
    isReady = false;
    // keyboard.terminate();
    // mouse.terminate();
    joypad.Terminate();
  }

  bool isReady = false;
  // InputKeyboardXlib keyboard{};
  // InputMouseXlib mouse{};
  InputJoypadUdev joypad;
};

}

#endif

#endif //UDEV_HPP_
