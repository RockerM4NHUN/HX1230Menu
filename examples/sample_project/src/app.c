#include "main.h"
#include "hx1230.h"
#include "adc.h"
#include "tim.h"

#include "hx1230_menu.h"

char str[100];
int len = 0;
ADC_ChannelConfTypeDef adc_chx = {0};
ADC_ChannelConfTypeDef adc_chy = {0};

int abs(int v){return v > 0 ? v : -v;}

void init_adc_configs(){
  
  adc_chx.Channel = ADC_CHANNEL_1;
  adc_chx.Rank = ADC_REGULAR_RANK_1;
  adc_chx.SingleDiff = ADC_SINGLE_ENDED;
  adc_chx.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  adc_chx.OffsetNumber = ADC_OFFSET_NONE;
  adc_chx.Offset = 0;
  
  adc_chy.Channel = ADC_CHANNEL_2;
  adc_chy.Rank = ADC_REGULAR_RANK_1;
  adc_chy.SingleDiff = ADC_SINGLE_ENDED;
  adc_chy.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  adc_chy.OffsetNumber = ADC_OFFSET_NONE;
  adc_chy.Offset = 0;
  
  if (HAL_ADC_ConfigChannel(&hadc1, &adc_chx) != HAL_OK)
  {
    Error_Handler();
  }

  if (HAL_ADC_ConfigChannel(&hadc1, &adc_chy) != HAL_OK)
  {
    Error_Handler();
  }
}

struct {
  uint8_t button; // if > 0, button is pressed
  int16_t x, y; // mapped values
  int16_t xoffs, yoffs; // origin in ADC space
  int16_t xdead, ydead; // dead space radius at origin
} joystick;

void read_joystick(){

  // read x
  HAL_ADC_ConfigChannel(&hadc1, &adc_chx);
  HAL_ADC_Start(&hadc1);
  if (HAL_ADC_PollForConversion(&hadc1, 1000000) == HAL_OK)
  {
    joystick.x = HAL_ADC_GetValue(&hadc1);
  }
  HAL_ADC_Stop(&hadc1);

  // read y
  HAL_ADC_ConfigChannel(&hadc1, &adc_chy);
  HAL_ADC_Start(&hadc1);
  if (HAL_ADC_PollForConversion(&hadc1, 1000000) == HAL_OK)
  {
    joystick.y = HAL_ADC_GetValue(&hadc1);
  }
  HAL_ADC_Stop(&hadc1);
}

const unsigned char debounce_cycles = 2;
void update_joystick(){
  read_joystick();
  
  // map to coordinate space
  int x = joystick.x - joystick.xoffs;
  if (abs(x) <= joystick.xdead) x = 0;
  else if (x < 0) x /= 19;
  else if (x > 0) x /= 22;
  
  int y = joystick.y - joystick.yoffs;
  if (abs(y) <= joystick.ydead) y = 0;
  else if (y < 0) y /= 18;
  else if (y > 0) y /= 24;

  joystick.x = -y;
  joystick.y = -x;

  // read button with debounce
  const char is_pressed = !HAL_GPIO_ReadPin(JOYSTICK_SW_GPIO_Port, JOYSTICK_SW_Pin);
  joystick.button = is_pressed ? debounce_cycles : (joystick.button > 0 ? (joystick.button-1) : 0);
}

void joystick_default_calibration(){
  joystick.xoffs = 1735;
  joystick.yoffs = 1712;
  joystick.xdead = 14;
  joystick.ydead = 12;
}

void calibrate_joystick(){
  unsigned int end = HAL_GetTick() + 1000;

  int xmin = 65536;
  int xmax = 0;
  int ymin = 65536;
  int ymax = 0;

  while(HAL_GetTick() < end){
    read_joystick();
    if (joystick.x < xmin) xmin = joystick.x;
    if (joystick.x > xmax) xmax = joystick.x;
    if (joystick.y < ymin) ymin = joystick.y;
    if (joystick.y > ymax) ymax = joystick.y;
  }

  // find dead space settings
  joystick.xoffs = (xmax + xmin) / 2;
  joystick.xdead = (xmax - xmin) * 3 / 4; // omit divide by 2 to have a robust dead space (optionally *3/2)

  joystick.yoffs = (ymax + ymin) / 2;
  joystick.ydead = (ymax - ymin) * 3 / 4; // omit divide by 2 to have a robust dead space (optionally *3/2)
}

void menu_refresh_controls(struct menu_controls* controls){
  // link joystick and menu

  static int16_t up_down_level = 0;
  static int16_t plus_minus_level = 0;

  up_down_level += abs(joystick.y)*joystick.y;
  plus_minus_level += abs(joystick.x)*joystick.x;

  const int16_t limit = 9000;

  controls->up = up_down_level > limit;
  controls->down = up_down_level < -limit;
  controls->plus = plus_minus_level > limit;
  controls->minus = plus_minus_level < -limit;

  if (abs(up_down_level) > limit) up_down_level = 0;
  if (abs(plus_minus_level) > limit) plus_minus_level = 0;

  static char prev_button = 0;
  controls->enter_down = joystick.button;
  controls->enter = joystick.button == debounce_cycles && !prev_button;
  prev_button = joystick.button == debounce_cycles;
}

int16_t time_value;
void menu_redraw_loop(){
  static uint32_t next = 0;
  uint32_t tick = HAL_GetTick();
  if (tick > next){
    
    // if getters are needed for menu values, they should be here
    time_value = tick & 0xFFFF;
    
    menu_redraw();
    next = tick + 250;
  }
}

void joystick_test_loop(){
  static uint8_t first = 1;
  static uint8_t refresh = 1;
  uint8_t line = 0;

  if (first){
    first = 0;
    HX1230_Clear();
  }

  if (!(refresh--)){
    refresh = 10;
    len = sprintf(str, "X: %i", joystick.x);
    HX1230_PrintField(line++, 1, 40, str, len);
    len = sprintf(str, "Y: %i", joystick.y);
    HX1230_PrintField(line++, 1, 40, str, len);
    len = sprintf(str, "sqr(X): %i", abs(joystick.x)*joystick.x);
    HX1230_PrintField(line++, 1, 70, str, len);
    len = sprintf(str, "sqr(Y): %i", abs(joystick.y)*joystick.y);
    HX1230_PrintField(line++, 1, 70, str, len);
    len = sprintf(str, "xoffs: %i", joystick.xoffs);
    HX1230_PrintField(line++, 1, 70, str, len);
    len = sprintf(str, "yoffs: %i", joystick.yoffs);
    HX1230_PrintField(line++, 1, 70, str, len);
    len = sprintf(str, "xdead: %i", joystick.xdead);
    HX1230_PrintField(line++, 1, 70, str, len);
    len = sprintf(str, "ydead: %i", joystick.ydead);
    HX1230_PrintField(line++, 1, 70, str, len);
  }

  if (controls.enter) {
    first = 1;
    menu_deselect();
  }
}

struct menu_slider setpoint_slider = {
  .value = 0,
  .min = 0,
  .max = 20000,
  .step = 10,
};

struct menu_slider setpoint_slider2 = {
  .value = -1000,
  .min = -1000,
  .max = 1000,
  .step = 100,
};

void zero_slider(){
  setpoint_slider.value = 0;
}

struct menu_button set_zero_button = {
  .action = &zero_slider,
};

void zero_slider2(){
  setpoint_slider2.value = 0;
}

struct menu_button set_zero_button2 = {
  .action = &zero_slider2,
};

struct menu_toggle test_toggle = {
  .value = 0,
};

struct menu_toggle test_toggle2 = {
  .value = 0,
};


struct submenu_t demo_menu, sub_menu;

struct submenu_t sub_menu = {
  .parent = &demo_menu,
  .options = (struct menu_option[]){
    {BACK,   ".."},
    {BUTTON, "Set zero 2",  NULL,         &set_zero_button2},
    {TOGGLE, "Toggle 2",    NULL,         &test_toggle2},
    {SLIDER, "Slider 2",    NULL,         &setpoint_slider2},
    {VALUE,  "Slider2: %i", NULL,         &setpoint_slider2.value},
    {VALUE,  "Time (ms): %i", NULL,         &time_value},
    {MENU_END},
  },
};

struct submenu_t demo_menu = {
  .options = (struct menu_option[]){
    {TITLE,  "Title"},
    {MENU,   "Submenu",         NULL,         &sub_menu},
    {BUTTON, "Button",          NULL,         &set_zero_button},
    {TOGGLE, "Toggle",          NULL,         &test_toggle},
    {SLIDER, "Slider",          NULL,         &setpoint_slider},
    {MODE,   "Custom mode",     joystick_test_loop},
    {VALUE,  "int16 value: %i", NULL,         &setpoint_slider.value},
    {TITLE,  "-------"},
    {TITLE,  "Lots"},
    {TITLE,  "of"},
    {TITLE,  "rows"},
    {TITLE,  "are"},
    {TITLE,  "possible!"},
    {TITLE,  "-------"},
    {TITLE,  "0000"},
    {TITLE,  "0001"},
    {TITLE,  "0011"},
    {TITLE,  "0010"},
    {TITLE,  "0110"},
    {TITLE,  "0111"},
    {TITLE,  "0101"},
    {TITLE,  "0100"},
    {TITLE,  "1100"},
    {TITLE,  "1101"},
    {TITLE,  "1111"},
    {TITLE,  "1110"},
    {TITLE,  "1010"},
    {TITLE,  "1011"},
    {TITLE,  "1001"},
    {TITLE,  "1000"},
    {MENU_END},
  },
};




void app_start(){
  HX1230_Init(&hspi1);
  HX1230_PixelTest();
  HX1230_Clear();
  HX1230_SetXY(0,1);

  init_adc_configs();

  // joystick calibration
  update_joystick();
  if (joystick.button) {
    HX1230_PrintField(1,1, 96,"Calibrate joystick", 50-32);
    while(joystick.button) update_joystick();
    HX1230_PrintField(2,1, 96,"Started...", 42-32);
    calibrate_joystick();
    
    HX1230_PrintField(2,1, 96,"Done", 4);
    len = sprintf(str, "xoffs: %i", joystick.xoffs);
    HX1230_PrintField(3, 1, 96, str, len);
    len = sprintf(str, "yoffs: %i", joystick.yoffs);
    HX1230_PrintField(4, 1, 96, str, len);
    len = sprintf(str, "xdead: %i", joystick.xdead);
    HX1230_PrintField(5, 1, 96, str, len);
    len = sprintf(str, "ydead: %i", joystick.ydead);
    HX1230_PrintField(6, 1, 96, str, len);

    while(!joystick.button) update_joystick();
  } else {
    joystick_default_calibration();
  }

  menu_init(&demo_menu);
  
  while(1){
    update_joystick();

    menu_loop();
    menu_redraw_loop();

    HAL_Delay(10);
  }
}
