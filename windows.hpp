#ifndef WINDOWS_HPP_
#define WINDOWS_HPP_

#include <xinput.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

// #include "shared/rawinput.cpp"
// #include "keyboard/rawinput.cpp"
// #include "mouse/rawinput.cpp"
// #include "joypad/xinput.cpp"
// #include "joypad/directinput.cpp"
#include "input.hpp"
#include "windows-raw-input.hpp"

namespace sen {

struct InputWindows : InputDriver {
  InputWindows& self{*this};

  explicit InputWindows(Input& super) : InputDriver(super)/*, keyboard(super), mouse(super), joypadXInput(super), joypadDirectInput(super)*/ {}
  ~InputWindows() override { Terminate(); }

  auto Create() -> bool override {
    return Initialize();
  }

  auto Driver() -> string override { return "Windows"; }
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
    // joypadXInput.poll(devices);
    // joypadDirectInput.poll(devices);
    return devices;
  }

  auto Rumble(uint64_t id, bool enable) -> bool override {
    // if(joypadXInput.rumble(id, enable)) return true;
    // if(joypadDirectInput.rumble(id, enable)) return true;
    return false;
  }

 private:
  auto Initialize() -> bool {
    Terminate();
    if(!self.context_) return false;

    //TODO: this won't work if Input is recreated post-initialization; nor will it work with multiple Input instances
    if(!rawInput.initialized) {
      rawInput.initialized = true;
      rawInput.mutex = CreateMutex(nullptr, false, nullptr);
      CreateThread(nullptr, 0, RawInputThreadProc, nullptr, 0, nullptr);

      do {
        Sleep(1);
        WaitForSingleObject(rawInput.mutex, INFINITE);
        ReleaseMutex(rawInput.mutex);
      } while(!rawInput.ready);
    }

    DirectInput8Create(GetModuleHandle(0), DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&directInputContext, 0);
    if(!directInputContext) return false;

    // if(!keyboard.Initialize()) return false;
    // if(!mouse.Initialize(self.context)) return false;
    // bool xinputAvailable = joypadXInput.Initialize();
    // if(!joypadDirectInput.Initialize(self.context, directInputContext, xinputAvailable)) return false;
    return isReady = true;
  }

  auto Terminate() -> void {
    isReady = false;

    // keyboard.terminate();
    // mouse.terminate();
    // joypadXInput.terminate();
    // joypadDirectInput.terminate();

    if(directInputContext) {
      directInputContext->Release();
      directInputContext = nullptr;
    }
  }

  bool isReady = false;
  // InputKeyboardRawInput keyboard;
  // InputMouseRawInput mouse;
  // InputJoypadXInput joypadXInput;
  // InputJoypadDirectInput joypadDirectInput;
  LPDIRECTINPUT8 directInputContext = nullptr;
};

}

#endif //WINDOWS_HPP_
