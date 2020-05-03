
#ifndef MENU_H
#define MENU_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define NUMEL(x) (sizeof(x)/sizeof(x[0]))

typedef void(*menu_function)(void* data);

typedef enum {
  MENU_END = 0,
  TITLE,
  VALUE,
  MODE,
  MENU,
  BACK,
  BUTTON,
  TOGGLE,
  SLIDER,
} menu_option_type;

struct menu_option{
  menu_option_type type;
  const char* name;
  menu_function loop;
  void* data;
};

struct submenu_t{
  struct submenu_t* parent;
  struct menu_option* options;
  uint8_t _len;
  uint8_t _cursor;
  uint8_t _scrn_cursor;
};

struct menu_controls{
  char up;
  char down;
  char minus;
  char plus;
  char enter;
  char enter_down;
};

struct menu_slider{
  int16_t value;
  int16_t min;
  int16_t max;
  int16_t step;
};

struct menu_toggle{
  char value;
};

struct menu_button{
  menu_function action;
  void* data;
};

void menu_init(struct submenu_t* menu);

void menu_refresh_controls(struct menu_controls* controls);

void menu_redraw();

void menu_select();

void menu_deselect();

void menu_loop();

extern struct menu_controls controls;

#ifdef __cplusplus
}
#endif

#endif /* MENU_H */
