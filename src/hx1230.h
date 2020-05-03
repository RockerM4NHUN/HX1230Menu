#ifndef HX1230_H_
#define HX1230_H_

#include "main.h"
#include "spi.h"

#ifdef __cplusplus
 extern "C" {
#endif

#define HX1230_TURN_ON			0x2F
#define HX1230_TURN_OFF			0x28

#define HX1230_INVERT_ON		0xA7
#define HX1230_INVERT_OFF		0xA6

#define HX1230_DISPLAY_ON		0xAF
#define HX1230_DISPLAY_OFF		0xAE

#define HX1230_PIXEL_TEST_ON	0xA5
#define HX1230_PIXEL_TEST_OFF	0xA4

#define HX1230_ROWS				9
#define HX1230_COLS				96

void HX1230_Init(SPI_HandleTypeDef * hspi);

void HX1230_Command(uint8_t cmd);
void HX1230_PixelTest();
void HX1230_Stream(const uint16_t* data, uint16_t length);
void HX1230_Clear();

void HX1230_SetXY(uint8_t x, uint8_t y);
void HX1230_SetX(uint8_t x);
void HX1230_SetY(uint8_t y);
void HX1230_Scan_Offset(uint8_t y);

void HX1230_Draw(const uint8_t* data, uint16_t length);

uint16_t HX1230_Print(uint8_t x, uint8_t y, const char* text, uint16_t length);
void HX1230_PrintField(uint8_t line, uint8_t colStart, uint8_t colEnd, const char* text, uint16_t length);

#ifdef __cplusplus
}
#endif

#endif /* HX1230_H_ */
