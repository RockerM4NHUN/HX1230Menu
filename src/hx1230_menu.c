#include "hx1230_menu.h"

#include "hx1230.h"

#define SCREEN_HEIGHT (8)

static struct submenu_t* menu = NULL;
static struct menu_option* selected = NULL;
static char str[30];
static uint8_t len;

static const uint8_t gfx_cursor[]  = {0x00, 0x66, 0x3C, 0x18, 0x00};
static const uint8_t gfx_btn_on[]  = {0xE0, 0xF8, 0xF8, 0xE0, 0x00};
static const uint8_t gfx_btn_off[] = {0xE0, 0xFE, 0xFE, 0xE0, 0x00};
static const uint8_t gfx_tgl_on[]  = {0xE0, 0xF0, 0xF8, 0xE6, 0x00};
static const uint8_t gfx_tgl_off[] = {0xE6, 0xF8, 0xF0, 0xE0, 0x00};
static const uint8_t gfx_slider[]  = {0x08, 0xFE, 0xFE, 0x08, 0x00};
static const uint8_t gfx_submenu[] = {0x00, 0xE0, 0x18, 0x06, 0x00};
static const uint8_t gfx_mode[]    = {0x10, 0x54, 0x38, 0x10, 0x00};
static const uint8_t gfx_back[]    = {0x10, 0x38, 0x54, 0x10, 0x00};

struct menu_controls controls = {};

static void deselect_loop(void* p){
  menu_deselect();
}

static void slider_loop(void* p);
static void slider_redraw(struct menu_slider *const slider);

#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })

#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })

// -----

void menu_init(struct submenu_t* _menu){
  if (!_menu || !_menu->options || !_menu->options[0].type) return;
  menu = _menu;
  if (!menu || menu->parent != _menu) menu->_cursor = 0;
  menu->_scrn_cursor = 0;
  selected = NULL;

  // find the number of menu items
  if (!menu->_len){
    while(menu->options[++menu->_len].type);
  }

  menu_redraw();
}

void menu_redraw(){
  // draw menu

  if (selected && (selected->type == MODE || selected->type == SLIDER)) return;
  
  HX1230_Clear();

  uint16_t cursor = menu->_cursor - menu->_scrn_cursor;
  uint8_t scrn_cursor = 0;
  struct menu_option* option;
  while(scrn_cursor < SCREEN_HEIGHT && cursor < menu->_len){
    if (menu->_scrn_cursor == scrn_cursor){
      HX1230_SetXY(0, scrn_cursor);
      HX1230_Draw(gfx_cursor, NUMEL(gfx_cursor));
    }

    option = &menu->options[cursor];
    
    HX1230_SetXY(NUMEL(gfx_cursor), scrn_cursor);

    switch(option->type){
      case VALUE:
        len = sprintf(str, option->name, *(int16_t*)option->data);
        break;
      case MODE:
        HX1230_Draw(gfx_mode, NUMEL(gfx_cursor));
        break;
      case MENU:
        HX1230_Draw(gfx_submenu, NUMEL(gfx_cursor));
        break;
      case BACK:
        HX1230_Draw(gfx_back, NUMEL(gfx_cursor));
        break;
      case BUTTON:
        HX1230_Draw((controls.enter && selected == option) ? gfx_btn_on : gfx_btn_off, NUMEL(gfx_cursor));
        break;
      case TOGGLE:
        HX1230_Draw(((struct menu_toggle*)option->data)->value ? gfx_tgl_on : gfx_tgl_off, NUMEL(gfx_cursor));
        break;
      case SLIDER:
        HX1230_Draw(gfx_slider, NUMEL(gfx_cursor));
        break;
      default:
        break;
    }

    if (option->type != VALUE) 
      len = sprintf(str, option->name);
    HX1230_PrintField(scrn_cursor, 2*NUMEL(gfx_cursor), HX1230_COLS, str, len);

    ++scrn_cursor;
    ++cursor;
  }
}

void menu_up(){
  if (menu && menu->_cursor > 0) {
    menu->_cursor--;

    if (menu->_scrn_cursor >= SCREEN_HEIGHT / 2)
      menu->_scrn_cursor--;
    
    if (menu->_cursor < menu->_scrn_cursor)
      menu->_scrn_cursor = menu->_cursor;
    
    menu_redraw();
  }
}

void menu_down(){
  if (menu && menu->options[menu->_cursor+1].type != MENU_END) {
    menu->_cursor++;

    if (menu->_scrn_cursor < SCREEN_HEIGHT / 2 || menu->_len < SCREEN_HEIGHT)
      menu->_scrn_cursor++;
    
    if (menu->_len - menu->_cursor - 1 < min(SCREEN_HEIGHT, menu->_len) - 1 - menu->_scrn_cursor)
      menu->_scrn_cursor = SCREEN_HEIGHT - 1 - (menu->_len - menu->_cursor - 1);

    menu_redraw();
  }
}

void menu_select(){

  if (!menu) return;

  selected = &menu->options[menu->_cursor];
  switch(selected->type){
    case MODE:
      break;
    case MENU:
      menu_init((struct submenu_t*)selected->data);
      break;
    case BACK:
      menu_init(menu->parent);
      break;
    case BUTTON:
      {
        struct menu_button *const button = (struct menu_button*)selected->data;
        button->action(button->data);
        selected->loop = &deselect_loop;
      }
      break;
    case TOGGLE:
      {
        struct menu_toggle *const toggle = (struct menu_toggle*)selected->data;
        toggle->value = !toggle->value;
        selected->loop = &deselect_loop;
      }
      break;
    case SLIDER:
      // loop will handle the slider implementation
      selected->loop = &slider_loop;
      slider_redraw(selected->data);
      break;
    default:
      // if not selectable, reject selecting menu item
      selected = NULL;
      break;
  }

  menu_redraw();
}

void menu_deselect(){
  
  if (!selected) return;

  selected = NULL;
  menu_redraw();
}

void menu_loop(){
  if (selected && selected->loop)
    selected->loop(selected->data);
  
  menu_refresh_controls(&controls);
  
  if (controls.enter_down == 1)
    menu_redraw();

  if (selected) return;

  if (!selected && controls.enter) menu_select();
  else {
    if (controls.up) menu_up();
    if (controls.down) menu_down();
  }

  controls.up = controls.down = controls.plus = controls.minus = controls.enter = 0;
}


static uint8_t gfx_slider_ends[] = {0xFF, 0x81, 0xFF};
static void slider_redraw(struct menu_slider *const slider){
  
  // clear space between nubmer and slide
  HX1230_PrintField(menu->_scrn_cursor, HX1230_COLS - 54 - 4, HX1230_COLS - 54, NULL, 0);

  len = sprintf(str, "%i", slider->value);
  HX1230_PrintField(menu->_scrn_cursor, 2*NUMEL(gfx_cursor), HX1230_COLS - 54 - 4, str, len);
  
  HX1230_SetXY(HX1230_COLS - 54, menu->_scrn_cursor);
  HX1230_Draw(&gfx_slider_ends[0], 2);

  uint8_t cols = 50;
  uint8_t data = 0xBD;
  uint8_t fill = ((slider->value - slider->min)*cols)/(slider->max - slider->min);
  while(fill--){
    cols--;
    HX1230_Draw(&data, 1);
  }
  data = 0x81;
  while(cols--){
    HX1230_Draw(&data, 1);
  }

  HX1230_Draw(&gfx_slider_ends[1], 2);
}

static void slider_loop(void* p){
  struct menu_slider *const slider = (struct menu_slider*)p;

  if (controls.plus){
    slider->value = min(slider->max, slider->value + slider->step);
    slider_redraw(slider);
  }

  if (controls.minus){
    slider->value = max(slider->min, slider->value - slider->step);
    slider_redraw(slider);
  }
  
  if (controls.enter) menu_deselect();
}

