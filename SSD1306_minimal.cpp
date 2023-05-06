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

#include "SSD1306_minimal.h"

#include "TinyWireM.h"

void SSD1306_Mini::sendCommand(unsigned char command) {
  TinyWireM.beginTransmission(itsAddress);
  TinyWireM.send(GOFi2cOLED_Command_Mode);
  TinyWireM.send(command);
  TinyWireM.endTransmission();
}

void SSD1306_Mini::sendData(unsigned char data[4]) {
  TinyWireM.beginTransmission(itsAddress);
  TinyWireM.send(GOFi2cOLED_Data_Mode);
  TinyWireM.send(data[0]);
  TinyWireM.send(data[1]);
  TinyWireM.send(data[2]);
  TinyWireM.send(data[3]);
  TinyWireM.endTransmission();
}

void SSD1306_Mini::init() {
  TinyWireM.initialise();
  delay(5);	//wait for OLED hardware init

  sendCommand(GOFi2cOLED_Display_Off_Cmd);

  sendCommand(Set_Multiplex_Ratio_Cmd);
  sendCommand(0x3F);    /*duty = 1/64*/

  sendCommand(Set_Display_Offset_Cmd);
  sendCommand(0x00);

  sendCommand(0xB0); 	//set page address
  sendCommand(0x00); 	//set column lower address
  sendCommand(0x10); 	//set column higher address

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

void SSD1306_Mini::die() {
  TinyWireM.beginTransmission(itsAddress);
  TinyWireM.send(GOFi2cOLED_Data_Mode);
  TinyWireM.send(0xFF);
  TinyWireM.send(0x81);
  TinyWireM.send(0x81);
  TinyWireM.send(0xFF);
  TinyWireM.endTransmission();
}

void SSD1306_Mini::cursorTo(unsigned char row, unsigned char col) {
  sendCommand(0x00 | (0x0F & col) );  // low col = 0
  sendCommand(0x10 | (0x0F & (col>>4)) );  // hi col = 0
//  sendCommand(0x40 | (0x0F & row) ); // line #0

  sendCommand(0xb0 | (0x03 & row) );  // hi col = 0
}

void SSD1306_Mini::startScreen(){
  sendCommand(0x00 | 0x0);  // low col = 0
  sendCommand(0x10 | 0x0);  // hi col = 0
  sendCommand(0x40 | 0x0); // line #0
}

void SSD1306_Mini::clear() {
  sendCommand(0x00 | 0x0);  // low col = 0
  sendCommand(0x10 | 0x0);  // hi col = 0
  sendCommand(0x40 | 0x0); // line #0
    
  for (uint16_t i=0; i<=((128*64/8)/16); i++) {
    // send a bunch of data in one xmission
    TinyWireM.beginTransmission(itsAddress);
    TinyWireM.send(GOFi2cOLED_Data_Mode);            // data mode
    for (uint8_t k=0;k<16;k++){
      TinyWireM.send( 0 );
    }
    TinyWireM.endTransmission();
  }
}
