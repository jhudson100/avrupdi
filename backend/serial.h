#pragma once

#define ERROR_BUFFER_OVERRUN 1
#define ERROR_FRAME_ERROR 2

void serialInit();
uint8_t serialGetError();
