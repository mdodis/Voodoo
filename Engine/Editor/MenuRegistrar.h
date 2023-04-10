#pragma once
#include "Containers/Array.h"
#include "Containers/Map.h"
#include "Delegates.h"
#include "Memory/Arena.h"
#include "Str.h"

struct MenuRegistrar {
  struct MenuItem {
    Str name;
    Delegate<void> on_invoke;
    int priority = 0;
  };

  void register_item(Str name, Delegate<void> on_invoke, int priority);
  void draw();
  void init();
  int rank(const MenuItem &item);
  TArray<MenuItem> menu_items;
  TMap<Str, int> precedence;
  Arena temp_arena;
};

MenuRegistrar *get_menu_registrar();

template <typename T> struct StaticMenuItem {
  T t;
  StaticMenuItem(Str name, T t, int priority) : t(t) {
    get_menu_registrar()->register_item(
        name, Delegate<void>::create_raw(this, &StaticMenuItem::action),
        priority);
  }

  void action() { t(); }
};

template <typename T>
StaticMenuItem<T> _static_menu_item(Str name, T t, int priority) {
  return StaticMenuItem<T>(name, t, priority);
}

#define STATIC_MENU_ITEM_3(x, y) x##y
#define STATIC_MENU_ITEM_2(x, y) STATIC_MENU_ITEM_3(x, y)
#define STATIC_MENU_ITEM_1(prefix) STATIC_MENU_ITEM_2(prefix, __COUNTER__)
#define STATIC_MENU_ITEM(name, code, priority)                                 \
  static auto STATIC_MENU_ITEM_1(Static_Menu_Item_) = _static_menu_item(       \
      LIT(name), []() { code }, priority)
