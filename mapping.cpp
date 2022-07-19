#include "mapping.hpp"
#include "hid.h"

namespace sen {

auto InputMapping::Bind(
    const shared_ptr<InputManager> &manager,
    const shared_ptr<HID::Device> &dev,
    uint new_group_id,
    uint new_input_id,
    int16_t old_value,
    int16_t new_value) -> bool {
  if (dev->IsNull()) {
    return Unbind(), true;
  }

  if (dev->IsKeyboard() && dev->GetGroup(new_group_id).GetInput(new_input_id).GetName() == "Escape") {
    return Unbind(), true;
  }

  if (dev->IsKeyboard() && old_value == 0 && new_value == 1) {
    return SetAssignment(manager, dev, new_group_id, new_input_id), true;
  }

  if (dev->IsJoypad() && new_group_id == HID::Joypad::GroupID::Button && old_value == 0 && new_value == 1) {
    return SetAssignment(manager, dev, new_group_id, new_input_id), true;
  }

  if (dev->IsJoypad() && new_group_id == HID::Joypad::GroupID::Hat && new_value < -16384) {
    return SetAssignment(manager, dev, new_group_id, new_input_id, Qualifier::Lo), true;
  }

  if (dev->IsJoypad() && new_group_id == HID::Joypad::GroupID::Hat && new_value > +16384) {
    return SetAssignment(manager, dev, new_group_id, new_input_id, Qualifier::Hi), true;
  }

  if (dev->IsJoypad() && new_group_id == HID::Joypad::GroupID::Axis && new_value < -16384) {
    return SetAssignment(manager, dev, new_group_id, new_input_id, Qualifier::Lo), true;
  }

  if (dev->IsJoypad() && new_group_id == HID::Joypad::GroupID::Axis && new_value > +16384) {
    return SetAssignment(manager, dev, new_group_id, new_input_id, Qualifier::Hi), true;
  }

  if (dev->IsJoypad() && new_group_id == HID::Joypad::GroupID::Trigger && new_value < -16384) {
    return SetAssignment(manager, dev, new_group_id, new_input_id, Qualifier::Lo), true;
  }

  if (dev->IsJoypad() && new_group_id == HID::Joypad::GroupID::Trigger && new_value > +16384) {
    return SetAssignment(manager, dev, new_group_id, new_input_id, Qualifier::Hi), true;
  }

  return false;
}

auto split(const string &source, const string &find) -> vector<string> {
  vector<string> ret;

  string::size_type first_find_pos = source.find_first_not_of(find, 0);
  string::size_type last_find_pos = source.find_first_of(find, first_find_pos);

  while (last_find_pos != string::npos || first_find_pos != string::npos) {
    ret.push_back(source.substr(first_find_pos, last_find_pos - first_find_pos));

    first_find_pos = source.find_first_not_of(find, last_find_pos);
    last_find_pos = source.find_first_of(find, first_find_pos);
  }

  return ret;
}

auto InputMapping::Bind() -> void {
  device.reset();
  device_id = 0;
  group_id = 0;
  input_id = 0;
  qualifier = Qualifier::None;

  auto p = split(assignment, "/");

  if (p.size() >= 4) {
    /*for (auto &dev : input_manager->devices) {
      if (dev->GetName() == p[0]) {
        if (dev->GetID() == std::stoi(p[1], nullptr, 16)) {

          if (auto groupID = dev->find(p[2])) {
            if (auto inputID = dev->group(*groupID).find(p[3])) {
              this->device = dev;
              this->deviceID = dev->id();
              this->groupID = *groupID;
              this->inputID = *inputID;
              this->qualifier = Qualifier::None;
              if (p.size() >= 5) {
                if (p[4] == "Lo") this->qualifier = Qualifier::Lo;
                if (p[4] == "Hi") this->qualifier = Qualifier::Hi;
              }
              break;
            }
          }
        }
      }
    }*/
  }
}
auto InputMapping::Unbind() -> void {

}
auto InputMapping::SetAssignment(
    shared_ptr<InputManager>,
    shared_ptr<HID::Device>,
    uint,
    uint,
    InputMapping::Qualifier) -> void {

}

}