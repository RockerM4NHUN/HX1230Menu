
#ifndef _FONT_H_
#define	_FONT_H_

#include <stdint.h>

#ifdef __cplusplus
 extern "C" {
#endif

uint8_t getCharData(char, const uint8_t**);
extern const uint8_t charset[];

#ifdef __cplusplus
}
#endif

#endif	/* _FONT_H_ */

