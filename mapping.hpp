#ifndef MAPPING_HPP_
#define MAPPING_HPP_

#include <utility>

#include "input.hpp"

namespace sen {

struct InputManager;

struct InputMapping {
  enum class Qualifier : uint { None, Lo, Hi };

  explicit InputMapping(string name) : name(std::move(name)) {}

  auto Bind(const shared_ptr<InputManager>&, const shared_ptr<HID::Device>&, uint, uint, int16_t, int16_t) -> bool;
  auto Bind() -> void;
  auto Unbind() -> void;
  // auto icon() -> image;
  auto Text() -> string;
  auto Value() -> int16_t;

  auto ResetAssignment() -> void;
  auto SetAssignment(shared_ptr<InputManager>, shared_ptr<HID::Device>, uint, uint, Qualifier = Qualifier::None) -> void;

  const string name;

  shared_ptr<HID::Device> device;
  uint64_t device_id{};
  uint group_id{};
  uint input_id{};
  Qualifier qualifier = Qualifier::None;

  string assignment;
  shared_ptr<InputManager> input_manager;
};

struct InputButton : InputMapping {
  using InputMapping::InputMapping;
};

struct InputHotkey : InputMapping {
  using InputMapping::InputMapping;
  auto& OnPress(function<void ()> press) { return press_callback = std::move(press), *this; }
  auto& OnRelease(function<void ()> release) { return release_callback = std::move(release), *this; }

 private:
  function<void ()> press_callback;
  function<void ()> release_callback;
  int16_t state = 0;
  friend class InputManager;
};

struct VirtualPad {
  VirtualPad();

  InputButton up{"Up"}, down{"Down"}, left{"Left"}, right{"Right"};
  InputButton select{"Select"}, start{"Start"};
  InputButton a{"A"}, b{"B"};
  InputButton x{"X"}, y{"Y"};
  InputButton l{"L"}, r{"R"};

  vector<InputMapping*> mappings;
};

struct InputManager {
  auto create() -> void;
  auto bind() -> void;
  auto poll() -> void;
  auto eventInput(shared_ptr<HID::Device>, uint groupID, uint inputID, int16_t oldValue, int16_t newValue) -> void;

  //hotkeys.cpp
  auto createHotkeys() -> void;
  auto pollHotkeys() -> void;

  vector<shared_ptr<HID::Device>> devices;
  vector<InputHotkey> hotkeys;

  uint64_t pollFrequency = 5;
  uint64_t lastPoll = 0;
};

}

#endif //MAPPING_HPP_
