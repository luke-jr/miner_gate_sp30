/*
 * Copyright 2014 Zvi (Zvisha) Shteingart - Spondoolies-tech.com
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 3 of the License, or (at your option)
 * any later version.  See COPYING for more details.
 *
 * Note that changing this SW will void your miners guaranty
 */


#include "dc2dc.h"
#include "i2c.h"
#include "nvm.h"
#include "asic.h"
#include <time.h>
#include <pthread.h>

#include "defines.h"





int get_mng_board_temp() {
  int err;
  int temp;
  int reg;  
  i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_TEMP_SENSOR_PIN | PRIMARY_I2C_SWITCH_DEAULT);
  reg = i2c_read_word(I2C_MGMT_THERMAL_SENSOR, 0x0, &err);
  reg = (reg&0xFF)<<4 | (reg&0xF000)>>12;
  temp = (reg*625)/10000;
  i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_DEAULT);   

  return temp;
}

int get_top_board_temp() {
  int err;
   int temp;
   int reg;  
   i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_BOARD0_MAIN_PIN | PRIMARY_I2C_SWITCH_DEAULT, &err);    
#ifdef SP2x
   i2c_write(I2C_DC2DC_SWITCH_GROUP0, 0x80, &err);
#else
   i2c_write(I2C_DC2DC_SWITCH_GROUP1, 0x80, &err);
#endif
   if (err) {
    psyslog(RED "NO TOP BOARD x!\n" RESET);
   }
   vm.board_not_present[0] = (err != 0);
   reg = i2c_read_word(I2C_MAIN_THERMAL_SENSOR, 0x0, &err);
   reg = (reg&0xFF)<<4 | (reg&0xF000)>>12;
   temp = (reg*625)/10000;
#ifdef SP2x
   i2c_write(I2C_DC2DC_SWITCH_GROUP0, 0);
#else
   i2c_write(I2C_DC2DC_SWITCH_GROUP1, 0);
#endif   
   i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_DEAULT);   
   return temp;
}

int get_bottom_board_temp() {
   int err;
   int temp;
   int reg;  
   i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_BOARD1_MAIN_PIN | PRIMARY_I2C_SWITCH_DEAULT, &err);   
#ifdef SP2x
      i2c_write(I2C_DC2DC_SWITCH_GROUP0, 0x80, &err);
#else
      i2c_write(I2C_DC2DC_SWITCH_GROUP1, 0x80, &err);
#endif

   if (err) {
    psyslog(RED "NO BOTTOM BOARD x!\n" RESET);
   }   
   vm.board_not_present[1] = (err != 0);      
   reg = i2c_read_word(I2C_MAIN_THERMAL_SENSOR, 0x0, &err);
   reg = (reg&0xFF)<<4 | (reg&0xF000)>>12;
   temp = (reg*625)/10000;
#ifdef SP2x
      i2c_write(I2C_DC2DC_SWITCH_GROUP0, 0);
#else
      i2c_write(I2C_DC2DC_SWITCH_GROUP1, 0);
#endif  

   i2c_write(PRIMARY_I2C_SWITCH, PRIMARY_I2C_SWITCH_DEAULT);   
   return temp;
}




