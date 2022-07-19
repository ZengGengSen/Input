#include <gtest/gtest.h>
#include <glog/logging.h>

#include <utility>
#include "common.hpp"
#include "input.hpp"
#include "mapping.hpp"
#include "hid.h"

using namespace sen;

static inline string ConstString(const char *str) {
  if (str) {
    return {str};
  }

  return "";
}
struct InputJoypadUdev {
  explicit InputJoypadUdev(Input& input) : input_(input) {}

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
    string device_name;
    string device_node;

    uint8_t evbit[(EV_MAX + 7) / 8] = {0};
    uint8_t keybit[(KEY_MAX + 7) / 8] = {0};
    uint8_t absbit[(ABS_MAX + 7) / 8] = {0};
    uint8_t ffbit[(FF_MAX + 7) / 8] = {0};
    uint effects = 0;

    string name{};
    string manufacturer{};
    string product{};
    string serial{};
    string vendor_id{};
    string product_id{};

    set<JoypadInput> axes;
    set<JoypadInput> hats;
    set<JoypadInput> buttons;
    bool rumble{false};
    int effect_id{-1};
  };
  vector<Joypad> joypads;

  auto Initialize() -> bool {
    context_ = udev_new();
    if (context_ == nullptr)
      return false;

    monitor_ = udev_monitor_new_from_netlink(context_, "udev");
    if (monitor_) {
      udev_monitor_filter_add_match_subsystem_devtype(monitor_, "input", nullptr);
      udev_monitor_enable_receiving(monitor_);
    }

    enumerator_ = udev_enumerate_new(context_);
    if (enumerator_) {
      udev_enumerate_add_match_property(enumerator_, "ID_INPUT_JOYSTICK", "1");
      udev_enumerate_scan_devices(enumerator_);
      devices_ = udev_enumerate_get_list_entry(enumerator_);
      for (udev_list_entry *dev = devices_; dev != nullptr; dev = udev_list_entry_get_next(dev)) {
        const char *name = udev_list_entry_get_name(dev);
        udev_device *device = udev_device_new_from_syspath(context_, name);
        const char *device_node = udev_device_get_devnode(device);
        if (device_node)
          CreateJoypad(device, device_node);
        udev_device_unref(device);
      }
    }

    return true;
  }
  auto Shutdown() -> void {
    if (enumerator_) {
      udev_enumerate_unref(enumerator_);
      enumerator_ = nullptr;
    }
  }

  auto Poll(vector<shared_ptr<HID::Device>> &devs) -> void {
    while (HotplugDevicesAvailable())
      HotplugDevice();

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

 private:
  auto CreateJoypad(udev_device *device, const string& device_node) -> void {
    Joypad jp;
    jp.device_node = device_node;

    struct stat st{};
    if (stat(device_node.c_str(), &st) < 0) return;
    jp.device = st.st_rdev;

    jp.fd = open(device_node.c_str(), O_RDWR | O_NONBLOCK);
    if (jp.fd < 0) return;

    ioctl(jp.fd, EVIOCGBIT(0, sizeof(jp.evbit)), jp.evbit);
    ioctl(jp.fd, EVIOCGBIT(EV_KEY, sizeof(jp.keybit)), jp.keybit);
    ioctl(jp.fd, EVIOCGBIT(EV_ABS, sizeof(jp.absbit)), jp.absbit);
    ioctl(jp.fd, EVIOCGBIT(EV_FF, sizeof(jp.ffbit)), jp.ffbit);
    ioctl(jp.fd, EVIOCGEFFECTS, &jp.effects);

    #define TEST_BIT(buffer, bit) (buffer[(bit) >> 3] & 1 << ((bit) & 7))
    const char *tmp;

    if (TEST_BIT(jp.evbit, EV_KEY)) {
      if (udev_device *parent = udev_device_get_parent_with_subsystem_devtype(device, "input", nullptr)) {
        jp.name = ConstString(udev_device_get_sysattr_value(parent, "name"));
        jp.vendor_id = ConstString(udev_device_get_sysattr_value(parent, "id/vendor"));
        jp.product_id = ConstString(udev_device_get_sysattr_value(parent, "id/product"));

        if (udev_device *root = udev_device_get_parent_with_subsystem_devtype(parent, "usb", "usb_device")) {
          if ((jp.vendor_id == ConstString(udev_device_get_sysattr_value(root, "idVendor")))
              && (jp.product_id == ConstString(udev_device_get_sysattr_value(root, "idProduct")))
              ) {
            jp.device_name = ConstString(udev_device_get_devpath(root));
            jp.manufacturer = ConstString(udev_device_get_sysattr_value(root, "manufacturer"));
            jp.product = ConstString(udev_device_get_sysattr_value(root, "product"));
            jp.serial = ConstString(udev_device_get_sysattr_value(root, "serial"));
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
      joypads.push_back(std::move(jp));
    }

    #undef TEST_BIT
    #undef CONST_STRING
  }
  auto RemoveJoypad(udev_device *, const string &device_node) -> void {
    auto iter = std::find_if(joypads.begin(), joypads.end(), [&](const auto &item) {
      return item.device_node == device_node;
    });

    if (iter != joypads.end()) {
      LOG(INFO) << "Remove the device of " << device_node;
      joypads.erase(iter);
    }
  }

  // 判断是否发生热插拔
  auto HotplugDevicesAvailable() const -> bool {
    pollfd fd = {0};
    fd.fd = udev_monitor_get_fd(monitor_);
    fd.events = POLLIN;
    return (::poll(&fd, 1, 0) == 1) && (fd.revents & POLLIN);
  }

  // 增加新连接设备或者删除断开连接设备
  auto HotplugDevice() -> void {
    udev_device *device = udev_monitor_receive_device(monitor_);
    if (device == nullptr) return;

    string value = ConstString(udev_device_get_property_value(device, "ID_INPUT_JOYSTICK"));
    string action = ConstString(udev_device_get_action(device));
    string device_node = ConstString(udev_device_get_devnode(device));
    if (value == "1") {
      if (action == "add") {
        CreateJoypad(device, device_node);
      }
      if (action == "remove") {
        RemoveJoypad(device, device_node);
      }
    }
  }

  static auto CreateJoypadHID(Joypad &jp) -> void {
    jp.hid->SetVendorID(std::stoi(jp.vendor_id, nullptr, 16));
    jp.hid->SetProductID(std::stoi(jp.product_id, nullptr, 16));
    jp.hid->SetPathID(Hash::CRC32::GetCRC32(jp.device_name));

    for (uint n = 0; n < jp.axes.size(); ++n) jp.hid->GetAxes().Append(std::to_string(n));
    for (uint n = 0; n < jp.hats.size(); ++n) jp.hid->GetHats().Append(std::to_string(n));
    for (uint n = 0; n < jp.buttons.size(); ++n) jp.hid->GetButtons().Append(std::to_string(n));

    jp.hid->SetRumble(jp.rumble);
  }

  auto Assign(const shared_ptr<HID::Joypad>& hid, uint groupID, uint inputID, int16_t value) -> void {
    auto &group = hid->GetGroup(groupID);

    if (group.GetInput(inputID).GetValue() == value)
      return;
    input_.DoChange(hid, groupID, inputID, group.GetInput(inputID).GetValue(), value);
    group.GetInput(inputID).SetValue(value);
  }

  Input& input_;
  udev *context_{nullptr};
  udev_monitor *monitor_{nullptr};
  udev_enumerate *enumerator_{nullptr};
  udev_list_entry *devices_{nullptr};
};

TEST(InputTest, Windows) {
  Input input;
  InputJoypadUdev input_joypad_udev{input};
  input_joypad_udev.Initialize();

  vector<shared_ptr<HID::Device>> devs;
  input_joypad_udev.Poll(devs);

  for (auto &device : devs) {
    LOG(INFO) << std::hex << device->GetID();
  }

  input_joypad_udev.Shutdown();
}

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  google::InitGoogleLogging("InputTest");
  google::SetStderrLogging(google::INFO);

  int ret = RUN_ALL_TESTS();
  google::ShutdownGoogleLogging();

  return ret;
}