#ifndef RAW_INPUT_HPP_
#define RAW_INPUT_HPP_

#include "common.hpp"

namespace sen {

auto CALLBACK RawInputWindowProc(HWND, UINT, WPARAM, LPARAM) -> LRESULT;

struct RawInput {
  HANDLE mutex = nullptr;
  HWND hwnd = nullptr;
  bool ready = false;
  bool initialized = false;
  function<void(RAWINPUT *)> updateKeyboard;
  function<void(RAWINPUT *)> updateMouse;

  struct Device {
    HANDLE handle = nullptr;
    string path;
    enum class Type : uint { Null, Keyboard, Mouse, Joypad } type{Type::Null};
    uint16_t vendorID = 0;
    uint16_t productID = 0;
    bool isXInputDevice = false;
  };

  vector<Device> devices;

  auto Find(uint16_t vendorID, uint16_t productID) -> Device* {
    for (auto &device : devices) {
      if (device.vendorID == vendorID && device.productID == productID)
        return &device;
    }

    return nullptr;
  }

  auto ScanDevices() -> void {
    devices.clear();

    uint deviceCount = 0;
    GetRawInputDeviceList(nullptr, &deviceCount, sizeof(RAWINPUTDEVICELIST));
    auto *list = new RAWINPUTDEVICELIST[deviceCount];
    GetRawInputDeviceList(list, &deviceCount, sizeof(RAWINPUTDEVICELIST));

    for (int i = 0; i < deviceCount; ++i) {
      wchar_t path[4096];
      uint size = sizeof(path) - 1;
      GetRawInputDeviceInfo(list[i].hDevice, RIDI_DEVICENAME, &path, &size);

      RID_DEVICE_INFO info;
      info.cbSize = size = sizeof(RID_DEVICE_INFO);
      GetRawInputDeviceInfo(list[i].hDevice, RIDI_DEVICEINFO, &info, &size);

      Device device;
      device.path = (const char *) utf8_t(path);
      device.handle = list[i].hDevice;

      if (info.dwType == RIM_TYPEKEYBOARD) {
        device.type = Device::Type::Keyboard;
        device.vendorID = 0;
        device.productID = 1;
      }

      if (info.dwType == RIM_TYPEMOUSE) {
        device.type = Device::Type::Mouse;
        device.vendorID = 0;
        device.productID = 2;
      }

      if (info.dwType == RIM_TYPEHID) {
        //verify that this is a joypad device
        if (info.hid.usUsagePage != 1 || (info.hid.usUsage != 4 && info.hid.usUsage != 5)) continue;

        device.type = Device::Type::Joypad;
        device.vendorID = info.hid.dwVendorId;
        device.productID = info.hid.dwProductId;
        if (device.path.find("IG_")) device.isXInputDevice = true;  //"IG_" is only found inside XInput device paths
      }

      devices.push_back(device);
    }

    delete[] list;
  }

  auto windowProc(HWND h_wnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
    if (msg != WM_INPUT) return DefWindowProc(h_wnd, msg, wparam, lparam);

    uint size = 0;
    GetRawInputData((HRAWINPUT) lparam, RID_INPUT, nullptr, &size, sizeof(RAWINPUTHEADER));
    auto *input = new RAWINPUT[size];
    GetRawInputData((HRAWINPUT) lparam, RID_INPUT, input, &size, sizeof(RAWINPUTHEADER));
    WaitForSingleObject(mutex, INFINITE);

    if (input->header.dwType == RIM_TYPEKEYBOARD) {
      if (updateKeyboard) updateKeyboard(input);
    }

    if (input->header.dwType == RIM_TYPEMOUSE) {
      if (updateMouse) updateMouse(input);
    }

    ReleaseMutex(mutex);
    LRESULT result = DefRawInputProc(&input, size, sizeof(RAWINPUTHEADER));
    delete[] input;
    return result;
  }

  auto main() -> void {
    WNDCLASS wc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hbrBackground = (HBRUSH) COLOR_WINDOW;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpfnWndProc = RawInputWindowProc;
    wc.lpszClassName = L"RawInputClass";
    wc.lpszMenuName = nullptr;
    wc.style = CS_VREDRAW | CS_HREDRAW;
    RegisterClass(&wc);

    hwnd = CreateWindow(L"RawInputClass", L"RawInputClass", WS_POPUP, 0, 0, 64, 64, 0, 0, GetModuleHandle(0), 0);

    ScanDevices();

    RAWINPUTDEVICE device[2];
    //capture all keyboard input
    device[0].usUsagePage = 1;
    device[0].usUsage = 6;
    device[0].dwFlags = RIDEV_INPUTSINK;
    device[0].hwndTarget = hwnd;
    //capture all mouse input
    device[1].usUsagePage = 1;
    device[1].usUsage = 2;
    device[1].dwFlags = RIDEV_INPUTSINK;
    device[1].hwndTarget = hwnd;
    RegisterRawInputDevices(device, 2, sizeof(RAWINPUTDEVICE));

    WaitForSingleObject(mutex, INFINITE);
    ready = true;
    ReleaseMutex(mutex);

    while (true) {
      MSG msg;
      GetMessage(&msg, hwnd, 0, 0);
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
};

static RawInput rawInput;

auto WINAPI RawInputThreadProc(void*) -> DWORD {
  rawInput.main();
  return 0;
}

auto CALLBACK RawInputWindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) -> LRESULT {
  return rawInput.windowProc(hwnd, msg, wparam, lparam);
}

}

#endif //RAW_INPUT_HPP_
