#ifndef HID_H_
#define HID_H_

#include <string>
#include <utility>
#include <vector>
#include <algorithm>

namespace sen::HID {

class Input {
 public:
  explicit Input(std::string name) : name_(std::move(name)) {}

  auto GetName() const -> const std::string & { return name_; }
  auto GetValue() const -> int16_t { return value_; }
  auto SetValue(int16_t value) -> void { value_ = value; }

 private:
  std::string name_;
  int16_t value_{0};

  friend class Group;
};

class Group : public std::vector<Input> {
 public:
  explicit Group(std::string name) : name_(std::move(name)) {}
  auto GetInput(uint id) -> Input & { return vector::operator[](id); }
  auto Append(const std::string &name) -> void { emplace_back(name); }

  auto Find(const std::string &name) const -> uint {
    auto id = std::find_if(begin(), end(), [this, &name](const Input &input) {
      return input.name_ == name;
    });

    if (id == end()) {
      return -1;
    } else {
      return id - begin();
    }
  }

 private:
  std::string name_;
  friend class Device;
};

class Device : public std::vector<Group> {
 public:
  explicit Device(std::string name) : name_(std::move(name)) {}

  auto GetPathID() const -> uint32_t { return static_cast<uint32_t>(id_ >> 32); }
  auto GetVendorID() const -> uint16_t { return static_cast<uint32_t>(id_ >> 16); }
  auto GetProductID() const -> uint16_t { return static_cast<uint32_t>(id_ >> 0); }

  auto SetPathID   (uint32_t path_id   ) -> void { id_ = (uint64_t)path_id     << 32 | GetVendorID() << 16 | GetProductID() << 0; }
  auto SetVendorID (uint16_t vendor_id ) -> void { id_ = (uint64_t)GetPathID() << 32 | vendor_id     << 16 | GetProductID() << 0; }
  auto SetProductID(uint16_t product_id) -> void { id_ = (uint64_t)GetPathID() << 32 | GetVendorID() << 16 | product_id     << 0; }

  virtual auto IsNull() const -> bool { return false; }
  virtual auto IsKeyboard() const -> bool { return false; }
  virtual auto IsMouse() const -> bool { return false; }
  virtual auto IsJoypad() const -> bool { return false; }

  auto GetName() const -> const std::string & { return name_; }
  auto GetID() const -> uint64_t { return id_; }
  auto SetID(uint64_t id) -> void { id_ = id; }
  auto GetGroup(uint id) -> Group & { return vector::operator[](id); }
  auto Append(const std::string &name) -> void { emplace_back(name); }
  auto AppendList(const std::vector<std::string> &names) -> void { for (auto & name : names) emplace_back(name); }

  auto Find(const std::string &name) -> uint {
    auto id = std::find_if(begin(), end(), [this, &name](const Group &group) {
      return group.name_ == name;
    });

    if (id == end()) {
      return -1;
    } else {
      return id - begin();
    }
  }

 private:
  std::string name_;
  uint64_t id_{0};
};

class NullDevice : public Device {
 public:
  enum : uint16_t { GenericVendorID = 0x0000, GenericProductID = 0x0000 };
  NullDevice() : Device("Null") { Append("Null"); }

  auto IsNull() const -> bool override { return true; }
};

class Keyboard : public Device {
  enum : uint16_t { GenericVendorID = 0x0000, GenericProductID = 0x0001 };
  enum GroupID : uint { Button };

  Keyboard() : Device("Keyboard") { Append("Button"); }

  auto IsKeyboard() const -> bool override { return true; }
  auto GetButtons() -> Group& { return GetGroup(GroupID::Button); }
};

class Mouse : public Device {
  enum : uint16_t { GenericVendorID = 0x0000, GenericProductID = 0x0002 };
  enum GroupID : uint { Axis, Button };

  Mouse() : Device("Mouse") { AppendList({"Axis", "Button"}); }

  auto IsMouse() const -> bool override { return true; }
  auto GetAxes() -> Group& { return GetGroup(GroupID::Axis); }
  auto GetButtons() -> Group& { return GetGroup(GroupID::Button); }
};

class Joypad : public Device {
 public:
  enum : uint16_t { GenericVendorID = 0x0000, GenericProductID = 0x0003 };
  enum GroupID : uint { Axis, Hat, Trigger, Button };

  Joypad() : Device("Joypad") { AppendList({"Axis", "Hat", "Trigger", "Button"}); }

  auto IsJoypad() const -> bool override { return true; }
  auto GetAxes() -> Group& { return GetGroup(GroupID::Axis); }
  auto GetHats() -> Group& { return GetGroup(GroupID::Hat); }
  auto GetTriggers() -> Group& { return GetGroup(GroupID::Trigger); }
  auto GetButtons() -> Group& { return GetGroup(GroupID::Button); }

  auto GetRumble() const -> bool { return rumble_; }
  auto SetRumble(bool rumble) -> void { rumble_ = rumble; }

 private:
  bool rumble_{false};
};

}


#endif //HID_H_
