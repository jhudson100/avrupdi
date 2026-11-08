#pragma once

#ifndef __AVR_AVR32DD14__
#error Assuming AVR32DD14
#endif

#define F_CPU 24'000'000ULL

#define UPDI_PORT PORTC
#define UPDI_PIN (1<<1)
#define UPDI_PINCTRL PIN1CTRL

//PC2 is not connected

#define USBCONF_PORT PORTC
#define USBCONF_PIN (1<<3)

//PF6 is not connected
//PF7 is this chip's UPDI

#define USBRESET_PORT PORTD
#define USBRESET_PIN (1<<4)

#define UPDILED_PORT PORTD
#define UPDILED_PIN (1<<5)

//PD6 is not connected

#define USBCONFLED_PORT PORTD
#define USBCONFLED_PIN (1<<7)

//the serial port we're using
#define SERIAL_USART USART0

//interrupt vector for serial receive complete
#define USARTn_RXC_vect USART0_RXC_vect

//the port number of the serial port
#define SERIAL_PORT PORTA
#define SERIAL_TRANSMIT_PIN (1<<0)
#define SERIAL_RECEIVE_PIN (1<<1)
