#ifndef __CharConvert_H__
#define __CharConvert_H__

#include <stdint.h>
uint8_t CharConvert(const uint32_t arithmetic,const uint16_t parameter,const uint8_t power){
	uint8_t i;
	uint32_t multiply=1;
	for(i=0;i<power;i++){
		multiply=multiply*10;
	}
	return (arithmetic%((parameter*10)*multiply))/(parameter*multiply)+'0';
}
#endif


