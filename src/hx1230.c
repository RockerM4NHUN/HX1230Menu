#include "hx1230.h"
#include "font.h"

#define SPI_TIMEOUT 10

static SPI_HandleTypeDef* _hspi;

static void WriteSingle(uint16_t data){
	HAL_GPIO_WritePin(HX1230_nCE_GPIO_Port,HX1230_nCE_Pin,GPIO_PIN_RESET);

	HAL_SPI_Transmit(_hspi,(uint8_t*)&data,1,SPI_TIMEOUT);

	HAL_GPIO_WritePin(HX1230_nCE_GPIO_Port,HX1230_nCE_Pin,GPIO_PIN_SET);
}

void HX1230_Stream(const uint16_t* data, uint16_t length){
	HAL_GPIO_WritePin(HX1230_nCE_GPIO_Port,HX1230_nCE_Pin,GPIO_PIN_RESET);

	// WARNING: why is length staying the same? Maybe SPI implementation works like this.
	// -> it is deliberatly converted "badly"
	HAL_SPI_Transmit(_hspi,(uint8_t*)data,length,SPI_TIMEOUT);

	HAL_GPIO_WritePin(HX1230_nCE_GPIO_Port,HX1230_nCE_Pin,GPIO_PIN_SET);
}


void HX1230_Init(SPI_HandleTypeDef * hspi){
	_hspi = hspi;

	HAL_GPIO_WritePin(HX1230_nRST_GPIO_Port,HX1230_nRST_Pin,GPIO_PIN_RESET);
	HAL_GPIO_WritePin(HX1230_nCE_GPIO_Port,HX1230_nCE_Pin,GPIO_PIN_RESET);

	HAL_Delay(50);

	HAL_GPIO_WritePin(HX1230_nRST_GPIO_Port,HX1230_nRST_Pin,GPIO_PIN_SET);

	HAL_Delay(1);

	HAL_GPIO_WritePin(HX1230_nCE_GPIO_Port,HX1230_nCE_Pin,GPIO_PIN_SET);

	WriteSingle(HX1230_TURN_ON);
	WriteSingle(HX1230_INVERT_OFF);
	WriteSingle(HX1230_PIXEL_TEST_OFF);
	WriteSingle(HX1230_DISPLAY_ON);
	HX1230_Scan_Offset(0);
	HX1230_SetXY(0,0);
}

void HX1230_Command(uint8_t cmd){
	WriteSingle(cmd);
}

void HX1230_Clear(){
	uint16_t words[8] = {0x100, 0x100, 0x100, 0x100, 0x100, 0x100, 0x100, 0x100};
	HX1230_SetXY(0,0);
	for (int i = 0; i < HX1230_ROWS * 12; i++){
		HX1230_Stream(words, 8);
	}
	HX1230_SetXY(0,0);
}

void HX1230_PixelTest(){
	WriteSingle(HX1230_PIXEL_TEST_ON);
	HAL_Delay(1000);
	WriteSingle(HX1230_PIXEL_TEST_OFF);
}

void HX1230_SetXY(uint8_t x, uint8_t y){

	if (x > 95){
		x -= 96;
	}

	WriteSingle(0xB0 | (y & 0x07));
	WriteSingle(0x10 | ((x >> 4) & 0x07));
	WriteSingle(x & 0x0F);
}

void HX1230_SetX(uint8_t x){

	if (x > 95){
		x -= 96;
	}

	WriteSingle(0x10 | ((x >> 4) & 0x07));
	WriteSingle(x & 0x0F);
}

void HX1230_SetY(uint8_t y){
	WriteSingle(0xB0 | (y & 0x07));
}

void HX1230_Scan_Offset(uint8_t y){
	WriteSingle(0x40 | (y & 0x3F));
}

void HX1230_Draw(const uint8_t* data, uint16_t length){
	uint16_t words[length];
	for(int i = 0; i < length; i++){
		words[i] = 0x100 | data[i];
	}

	HX1230_Stream(words, length);
}

/* Implementation only handles breaking between characters. */
uint16_t HX1230_Print(uint8_t x, uint8_t y, const char* text, uint16_t length){
	HX1230_SetXY(x,y);
	const uint8_t* pdata;

	uint8_t row = y;
	uint8_t col = x;
	uint16_t slices = 0;

	for(uint16_t chari = 0; chari < length; chari++){
		uint8_t width = getCharData(text[chari],&pdata);
		if (col + width > HX1230_COLS){
			// Not fits in row
			row++;
			col = 0;

			if (row > HX1230_ROWS) return slices;

			HX1230_SetXY(0,row);
			slices = row * HX1230_COLS;

			// If last character is space don't display it
			if (text[chari] == ' ') continue;
		}

		slices += width;
		col += width;
		HX1230_Draw(pdata,width);

		// Spacer between characters
		if (col + 1 <= HX1230_COLS){
			HX1230_Draw((uint8_t*)"",1);
			slices++;
			col++;
		}
	}

	return slices;
}

void HX1230_PrintField(uint8_t line, uint8_t colStart, uint8_t colEnd, const char* text, uint16_t length){
	HX1230_SetXY(colStart,line);
	const uint8_t* pdata;

	uint8_t col = colStart;

	for(uint16_t chari = 0; chari < length; chari++){
		uint8_t width = getCharData(text[chari],&pdata);
		if (col + width > colEnd){
			// Not fits in row
			width = colEnd - col;
			HX1230_Draw(pdata,width);
			return;
		}

		col += width;
		HX1230_Draw(pdata,width);

		// Spacer between characters
		if (col + 1 <= colEnd){
			col++;
			HX1230_Draw((uint8_t*)"",1);
		}
	}

	while(col++ < colEnd){
		HX1230_Draw((uint8_t*)"",1);
	}
}
