#ifndef JOYPAD_UDEV_HPP_
#define JOYPAD_UDEV_HPP_

#include <cstring>
#include "../hid.h"
namespace sen {
struct InputJoypadUdev {
  Input &input;

  explicit InputJoypadUdev(Input &input) : input(input) {}

  udev *context = nullptr;
  udev_monitor *monitor = nullptr;
  udev_enumerate *enumerator = nullptr;
  udev_list_entry *devices = nullptr;
  udev_list_entry *item = nullptr;

  struct JoypadInput {
    int code{0};
    uint id{0};
    int16_t value{0};
    input_absinfo info{};

    JoypadInput() = default;
    explicit JoypadInput(int code) : code(code) {}
    JoypadInput(int code, uint id) : code(code), id(id) {}
    bool operator<(const JoypadInput &source) const { return code < source.code; }
    bool operator==(const JoypadInput &source) const { return code == source.code; }
  };

  struct Joypad {
    shared_ptr<HID::Joypad> hid{new HID::Joypad};

    int fd = -1;
    dev_t device = 0;
    string deviceName;
    string deviceNode;

    uint8_t evbit[(EV_MAX + 7) / 8] = {0};
    uint8_t keybit[(KEY_MAX + 7) / 8] = {0};
    uint8_t absbit[(ABS_MAX + 7) / 8] = {0};
    uint8_t ffbit[(FF_MAX + 7) / 8] = {0};
    uint effects = 0;

    string name;
    string manufacturer;
    string product;
    string serial;
    string vendorID;
    string productID;

    set<JoypadInput> axes;
    set<JoypadInput> hats;
    set<JoypadInput> buttons;
    bool rumble = false;
    int effectID = -1;
  };
  vector<Joypad> joypads;

  auto Assign(const shared_ptr<HID::Joypad>& hid, uint groupID, uint inputID, int16_t value) -> void {
    auto &group = hid->GetGroup(groupID);

    if (group.GetInput(inputID).GetValue() == value)
      return;
    input.DoChange(hid, groupID, inputID, group.GetInput(inputID).GetValue(), value);
    group.GetInput(inputID).SetValue(value);
  }

  auto Poll(vector<shared_ptr<HID::Device>> &devs) -> void {
    while (HotplugDevicesAvailable()) HotplugDevice();

    for (auto &jp : joypads) {
      input_event events[32];
      int64_t length = 0;
      while ((length = read(jp.fd, events, sizeof(events))) > 0) {
        length /= sizeof(input_event);
        for (uint i = 0; i < length; ++i) {
          int code = events[i].code;
          int type = events[i].type;
          int value = events[i].value;

          if (type == EV_ABS) {
            auto iter_axes = jp.axes.find(JoypadInput{code});

            if (iter_axes != jp.axes.end()) {
              int range = iter_axes->info.maximum - iter_axes->info.minimum;
              value = (value - iter_axes->info.minimum) * 65535 / range - 32767;
              Assign(jp.hid, HID::Joypad::GroupID::Axis, iter_axes->id, int16_t(sclamp<16>(value)));
            } else {
              auto iter_hat = jp.hats.find(JoypadInput{code});
              if (iter_hat != jp.hats.end()) {
                int range = iter_hat->info.maximum - iter_hat->info.minimum;
                value = (value - iter_hat->info.minimum) * 65535 / range - 32767;
                Assign(jp.hid, HID::Joypad::GroupID::Hat, iter_hat->id, int16_t(sclamp<16>(value)));
              }
            }
          } else if (type == EV_KEY) {
            if (code >= BTN_MISC) {
              auto iter_button = jp.axes.find(JoypadInput{code});
              if (iter_button != jp.buttons.end()) {
                Assign(jp.hid, HID::Joypad::GroupID::Button, iter_button->id, (bool) value);
              }
            }
          }
        }
      }

      devs.push_back(jp.hid);
    }
  }

  auto Rumble(uint64_t id, bool enable) -> bool {
    for (auto &jp : joypads) {
      if (jp.hid->GetID() != id) continue;
      if (!jp.hid->GetRumble()) continue;

      if (!enable) {
        if (jp.effectID == -1) return true;  //already stopped?

        ioctl(jp.fd, EVIOCRMFF, jp.effectID);
        jp.effectID = -1;
      } else {
        if (jp.effectID != -1) return true;  //already started?

        ff_effect effect{};
        memset(&effect, 0, sizeof(ff_effect));
        effect.type = FF_RUMBLE;
        effect.id = -1;
        effect.u.rumble.strong_magnitude = 65535;
        effect.u.rumble.weak_magnitude = 65535;
        ioctl(jp.fd, EVIOCSFF, &effect);
        jp.effectID = effect.id;

        input_event play{};
        memset(&effect, 0, sizeof(ff_effect));
        play.type = EV_FF;
        play.code = jp.effectID;
        play.value = enable;
        (void) write(jp.fd, &play, sizeof(input_event));
      }

      return true;
    }

    return false;
  }

  auto Initialize() -> bool {
    context = udev_new();
    if (context == nullptr) return false;

    monitor = udev_monitor_new_from_netlink(context, "udev");
    if (monitor) {
      udev_monitor_filter_add_match_subsystem_devtype(monitor, "input", nullptr);
      udev_monitor_enable_receiving(monitor);
    }

    enumerator = udev_enumerate_new(context);
    if (enumerator) {
      udev_enumerate_add_match_property(enumerator, "ID_INPUT_JOYSTICK", "1");
      udev_enumerate_scan_devices(enumerator);
      devices = udev_enumerate_get_list_entry(enumerator);
      for (udev_list_entry *iter = devices; iter != nullptr; iter = udev_list_entry_get_next(iter)) {
        string name = udev_list_entry_get_name(iter);
        udev_device *device = udev_device_new_from_syspath(context, name.c_str());
        string device_node = udev_device_get_devnode(device);
        if (!device_node.empty()) CreateJoypad(device, device_node);
        udev_device_unref(device);
      }
    }

    return true;
  }

  auto Terminate() -> void {
    if (enumerator) {
      udev_enumerate_unref(enumerator);
      enumerator = nullptr;
    }
  }

 private:
  auto HotplugDevicesAvailable() const -> bool {
    pollfd fd = {0};
    fd.fd = udev_monitor_get_fd(monitor);
    fd.events = POLLIN;
    return (::poll(&fd, 1, 0) == 1) && (fd.revents & POLLIN);
  }

  auto HotplugDevice() -> void {
    udev_device *device = udev_monitor_receive_device(monitor);
    if (device == nullptr) return;

    string value = udev_device_get_property_value(device, "ID_INPUT_JOYSTICK");
    string action = udev_device_get_action(device);
    string deviceNode = udev_device_get_devnode(device);
    if (value == "1") {
      if (action == "add") {
        CreateJoypad(device, deviceNode);
      }
      if (action == "remove") {
        RemoveJoypad(device, deviceNode);
      }
    }
  }

  auto CreateJoypad(udev_device *device, const string &device_node) -> void {
    Joypad jp;
    jp.deviceNode = device_node;

    struct stat st{};
    if (stat(device_node.c_str(), &st) < 0) return;
    jp.device = st.st_rdev;

    jp.fd = open(device_node.c_str(), O_RDWR | O_NONBLOCK);
    if (jp.fd < 0) return;

    // uint8_t evbit[(EV_MAX + 7) / 8] = {0};
    // uint8_t keybit[(KEY_MAX + 7) / 8] = {0};
    // uint8_t absbit[(ABS_MAX + 7) / 8] = {0};

    ioctl(jp.fd, EVIOCGBIT(0, sizeof(jp.evbit)), jp.evbit);
    ioctl(jp.fd, EVIOCGBIT(EV_KEY, sizeof(jp.keybit)), jp.keybit);
    ioctl(jp.fd, EVIOCGBIT(EV_ABS, sizeof(jp.absbit)), jp.absbit);
    ioctl(jp.fd, EVIOCGBIT(EV_FF, sizeof(jp.ffbit)), jp.ffbit);
    ioctl(jp.fd, EVIOCGEFFECTS, &jp.effects);

    #define TEST_BIT(buffer, bit) (buffer[(bit) >> 3] & 1 << ((bit) & 7))

    if (TEST_BIT(jp.evbit, EV_KEY)) {
      if (udev_device *parent = udev_device_get_parent_with_subsystem_devtype(device, "input", nullptr)) {
        jp.name = udev_device_get_sysattr_value(parent, "name");
        jp.vendorID = udev_device_get_sysattr_value(parent, "id/vendor");
        jp.productID = udev_device_get_sysattr_value(parent, "id/product");
        if (udev_device *root = udev_device_get_parent_with_subsystem_devtype(parent, "usb", "usb_device")) {
          if (jp.vendorID == udev_device_get_sysattr_value(root, "idVendor")
              && jp.productID == udev_device_get_sysattr_value(root, "idProduct")
              ) {
            jp.deviceName = udev_device_get_devpath(root);
            jp.manufacturer = udev_device_get_sysattr_value(root, "manufacturer");
            jp.product = udev_device_get_sysattr_value(root, "product");
            jp.serial = udev_device_get_sysattr_value(root, "serial");
          }
        }
      }

      uint axes = 0;
      uint hats = 0;
      uint buttons = 0;
      for (int i = 0; i < ABS_MISC; i++) {
        if (TEST_BIT(jp.absbit, i)) {
          if (i >= ABS_HAT0X && i <= ABS_HAT3Y) {
            auto hat = jp.hats.insert({i, hats++});
            if (hat.second) {
              ioctl(jp.fd, EVIOCGABS(i), &hat.first->info);
            }
          } else {
            auto axis = jp.axes.insert({i, axes++});
            if (axis.second) {
              ioctl(jp.fd, EVIOCGABS(i), &axis.first->info);
            }
          }
        }
      }
      for (int i = BTN_JOYSTICK; i < KEY_MAX; i++) {
        if (TEST_BIT(jp.keybit, i)) {
          jp.buttons.insert({i, buttons++});
        }
      }
      for (int i = BTN_MISC; i < BTN_JOYSTICK; i++) {
        if (TEST_BIT(jp.keybit, i)) {
          jp.buttons.insert({i, buttons++});
        }
      }
      jp.rumble = jp.effects >= 2 && TEST_BIT(jp.ffbit, FF_RUMBLE);

      CreateJoypadHID(jp);
      joypads.push_back(jp);
    }

    #undef TEST_BIT
  }

  static auto CreateJoypadHID(Joypad &jp) -> void {
    jp.hid->SetVendorID(std::stoi(jp.vendorID));
    jp.hid->SetProductID(std::stoi(jp.productID));
    jp.hid->SetPathID(Hash::CRC32::GetCRC32(jp.deviceName));

    for (uint n = 0; n < jp.axes.size(); ++n) jp.hid->GetAxes().Append(std::to_string(n));
    for (uint n = 0; n < jp.hats.size(); ++n) jp.hid->GetHats().Append(std::to_string(n));
    for (uint n = 0; n < jp.buttons.size(); ++n) jp.hid->GetButtons().Append(std::to_string(n));
    jp.hid->SetRumble(jp.rumble);
  }

  auto RemoveJoypad(udev_device *, const string &device_node) -> void {
    for (uint n = 0; n < joypads.size(); ++n) {
      if (joypads[n].deviceNode == device_node) {
        close(joypads[n].fd);
        joypads.erase(joypads.begin() + n);
        return;
      }
    }
  }
};

}

#endif //JOYPAD_UDEV_HPP_
