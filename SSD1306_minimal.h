/*
  SSD1306_minimal.h - SSD1306 OLED Driver Library
  2015 Copyright (c) CoPiino Electronics All right reserved.

  Original Author: GOF Electronics Co. Ltd.
  Modified by: CoPiino Electronics

  CoPiino Electronics invests time and resources providing this open source code,
	please support CoPiino Electronics and open-source hardware by purchasing
	products from CoPiino Electronics!

  What is it?	
    This library is derived from GOFi2cOLED library, only for SSD1306 in I2C Mode.
    As the original library only supports Frame Buffered mode which requires to have
    at least 1024bytes of free RAM for a 128x64px display it is too big for smaller devices.

    So this a SSD1306 library that works great with ATTiny85 devices :)


  It is a free software; you can redistribute it and/or modify it
  under the terms of BSD license, check license.txt for more information.
	All text above must be included in any redistribution.
*/


#ifndef __SSD1306_MINIMAL_H__
#define __SSD1306_MINIMAL_H__

#include "TinyWireM.h"
#include <Arduino.h>


//Fundamental Command (more than one bytes command)
#define Set_Contrast_Cmd                      0x81     //Double byte command to select 1 out of 256 contrast steps.Default(RESET = 0x7F)
#define Entire_Display_On_Resume_Cmd          0xA4     //Resume to RAM content display(RESET), Output follows RAM content
#define Entire_Display_On_Cmd                 0xA5     //Entire display ON, Output ignores RAM content
#define GOFi2cOLED_Normal_Display_Cmd	      0xA6     //Normal display (RESET)
#define GOFi2cOLED_Inverse_Display_Cmd	      0xA7     //Inverse display
#define GOFi2cOLED_Display_Off_Cmd	      0xAE     //sleep mode(RESET)
#define GOFi2cOLED_Display_On_Cmd	      0xAF     //normal mode

//Scrolling Command (more than one bytes command)
#define Right_Horizontal_Scroll_Cmd           0x26
#define Left_Horizontal_Scroll_Cmd            0x27
#define Vertical_Right_Horizontal_Scroll_Cmd  0x29
#define Vertical_Left_Horizontal_Scroll_Cmd   0x2A
#define Activate_Scroll_Cmd                   0x2F
#define Deactivate_Scroll_Cmd                 0x2E
#define Set_Vertical_Scroll_Area_Cmd          0xA3

//Addressing Setting Command (more than one bytes command)
#define Set_Memory_Addressing_Mode_Cmd        0x20
#define HORIZONTAL_MODE			      0x00
#define VERTICAL_MODE			      0x01
#define PAGE_MODE			      0x02       //Default(reset)
#define Set_Column_Address_Cmd                0x21       //Setup column start and end address. This command is only for horizontal or vertical addressing mode.
#define Set_Page_Address_Cmd                  0x22       //Setup page start and end address. This command is only for horizontal or vertical addressing mode.

//Hardware Configuration (Panel resolution & layout related) Command (more than one bytes command please refer to SSD1306 datasheet for details)
#define Segment_Remap_Cmd                     0xA1       //column address 127 is mapped to SEG0
#define Segment_Normal_map_Cmd                0xA0       //Default. column address 0 is mapped to SEG0(RESET)
#define Set_Multiplex_Ratio_Cmd               0xA8       //Set MUX ratio to N+1 MUX
#define COM_Output_Normal_Scan_Cmd            0xC0       //Normal mode (RESET). Scan from COM0 to COM[N �C1]
#define COM_Output_Remap_Scan_Cmd             0xC8       //Remapped mode. Scan from COM[N-1] to COM0
#define Set_Display_Offset_Cmd                0xD3       //Set vertical shift by COM from 0d~63d. The value is reset to 00h after RESET.
#define Set_COM_Pins_Hardware_Config_Cmd      0xDA

//Timing & Driving Scheme Setting Command (more than one bytes command)
#define Set_Display_Clock_Divide_Ratio_Cmd    0xD5
#define Set_Precharge_Period_Cmd              0xD9
#define Set_VCOMH_Deselect_Level_Cmd          0xDB
#define No_Operation_Cmd                      0xE3

#define Charge_Pump_Setting_Cmd      	      0x8D
#define Charge_Pump_Enable_Cmd	              0x14
#define Charge_Pump_Disable_Cmd               0x10     //default

#define Scroll_Left			      0x00
#define Scroll_Right			    0x01

#define Scroll_2Frames			      0x7
#define Scroll_3Frames			      0x4
#define Scroll_4Frames			      0x5
#define Scroll_5Frames			      0x0
#define Scroll_25Frames			      0x6
#define Scroll_64Frames			      0x1
#define Scroll_128Frames		      0x2
#define Scroll_256Frames		      0x3

template <uint8_t ADDRESS>
class SSD1306_Mini {
  private:
    static void sendCommand(uint8_t command) {
      uint8_t buf[] = { prefix_to_send(), 0x80, command };
      TinyWireM::transmit(buf, sizeof buf);
    }

  public:
    static void init() {
      USI_TWI_Master_Initialise();
      delay(5);  //wait for OLED hardware init

      sendCommand(GOFi2cOLED_Display_Off_Cmd);

      sendCommand(Set_Multiplex_Ratio_Cmd);
      sendCommand(0x3F);    /*duty = 1/64*/

      sendCommand(Set_Display_Offset_Cmd);
      sendCommand(0x00);

      sendCommand(0xB0);  //set page address

      sendCommand(Set_Memory_Addressing_Mode_Cmd);
      sendCommand(HORIZONTAL_MODE);

      sendCommand(0x40);    /*set display starconstructort line*/

      sendCommand(Set_Contrast_Cmd);
      sendCommand(0xcf);

      sendCommand(Segment_Remap_Cmd);

      sendCommand(COM_Output_Remap_Scan_Cmd);

      sendCommand(GOFi2cOLED_Normal_Display_Cmd);

      sendCommand(Set_Display_Clock_Divide_Ratio_Cmd);
      sendCommand(0x80);

      sendCommand(Set_Precharge_Period_Cmd);
      sendCommand(0xf1);

      sendCommand(Set_COM_Pins_Hardware_Config_Cmd);
      sendCommand(0x12);

      sendCommand(Set_VCOMH_Deselect_Level_Cmd);
      sendCommand(0x30);

      sendCommand(Deactivate_Scroll_Cmd);

      sendCommand(Charge_Pump_Setting_Cmd);
      sendCommand(Charge_Pump_Enable_Cmd);

      sendCommand(GOFi2cOLED_Display_On_Cmd);
    }

    // First byte of a buffer to be sent.
    static uint8_t prefix_to_send() {
      return TinyWireM::prefix_to_send(ADDRESS);
    }
    
    // First byte of a buffer with data to be sent.
    static uint8_t prefix_to_send_data() {
      return 0x40 | 0x0; // Set Display Start Line to 0
    }
    
    // Sends off buffer and returns 0 on success or error code.
    static uint8_t send(uint8_t buf[], uint8_t len) {
      return TinyWireM::transmit(buf, len);
    }

    static void startScreenUpload() {
      sendCommand(0x40 | 0x0); // line #0
    }

    static void clear() {
      startScreenUpload();
      uint8_t buf[2 + 64] = { prefix_to_send(), prefix_to_send_data() };
      for (uint16_t i = 0; i < 128*64/8/64; ++i) {
        TinyWireM::transmit(buf, sizeof buf);
      }
    }

    static void cursorTo(uint8_t row, uint8_t col) {
      sendCommand(0x00 | (0x0F & col) );  // low col = 0
      sendCommand(0x10 | (0x0F & (col>>4)) );  // hi col = 0
      //  sendCommand(0x40 | (0x0F & row) ); // line #0
      sendCommand(0xb0 | (0x03 & row) );  // hi col = 0
    }
};

#endif
