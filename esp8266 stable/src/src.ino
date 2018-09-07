/*
 * Copyright (c) 2015-2016 Chris Howell
 *
 * -------------------------------------------------------------------
 *
 * Additional Adaptation of OpenEVSE ESP Wifi
 * by Trystan Lea, Glyn Hudson, OpenEnergyMonitor
 * All adaptation GNU General Public License as below.
 *
 * -------------------------------------------------------------------
 *
 * This file is part of Open EVSE.
 * Open EVSE is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * Open EVSE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with Open EVSE; see the file COPYING.  If not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#define ENABLE_DEBUG
#define ENABLE_DEBUG_RAPI

#include <Arduino.h>
#include <ArduinoOTA.h>               // local OTA update from Arduino IDE

#include "emonesp.h"
#include "config.h"
#include "wifi.h"
#include "web_server.h"
#include "ohm.h"
#include "input.h"
#include "emoncms.h"
#include "mqtt.h"
#include "divert.h"
#include "ota.h"
#include "lcd.h"

#include "RapiSender.h"

RapiSender rapiSender(&Serial);

unsigned long Timer1; // Timer for events once every 30 seconds
unsigned long Timer3; // Timer for events once every 2 seconds

boolean rapi_read = 0; //flag to indicate first read of RAPI status

byte CMA_server[] = { 192, 168, 0, 103 }; // Google

WiFiClient cmaclient;
int cma_state = 1;
int cma_pending_state = 0;
char cma_message[50];
long cma_timer = 1000;
long cma_id = 10001;
int cma_retries = 0;

// searches for the string sfind in the string str
// returns position of last byte + 1
//0 if not found
int StrContains(char *str, char *sfind)
{
    char found = 0;
    char index = 0;
    char len;

    len = strlen(str);
    
    if (strlen(sfind) > len) {
        return 0;
    }
    while (index < len) {
        if (str[index] == sfind[found]) {
            found++;
            if (strlen(sfind) == found) {
                return index+1;
            }
        }
        else {
            found = 0;
        }
        index++;
    }

    return 0;
}

int myrapisend(char * command)
{
 if (0 == rapiSender.sendCmd(command))
  {
    if(rapiSender.getTokenCnt() >= 3)
    {
      const char *val = rapiSender.getToken(1);
      DBUGVAR(val);
      state = strtol(val, NULL, 10);
      DBUGVAR(state);
    }
    return 0;
  }
  
    DBUGLN("OpenEVSE not responding or not connected");

  
   return 1; 
 }

 


void cma_update()
{
  char buf[100];
  int i,count,reply=999;


if( cma_timer > millis())
    return;

 
 cma_timer = millis() + 1000;


//first read any pending bytes
if(cmaclient.connected()) 
 {

    count = cmaclient.available();
   
    if(count >0)
     {
      for(i=0;count-- >0;i++)
        buf[i] = cmaclient.read();
   
      buf[i] = 0;
    
     
      if(i = StrContains(buf,"OK"))
       {
        reply = atoi(&buf[i+1]);
        cma_pending_state = 0;  
        cma_retries = 0;
       }
     
      if(i = StrContains(buf,"SC"))
       {
          /// max charging amps
          reply = atoi(&buf[i+1]);
          sprintf(buf,"$SC %d",reply);
          if(myrapisend(buf))
           {
            
            rapiSender.sendCmd("$FP 1 1 error");

            sprintf(cma_message,"myev|1.0|%ld|5|OK 1|",cma_id);
            cmaclient.println(cma_message); 

           }
          
           sprintf(cma_message,"myev|1.0|%ld|5|OK 0|",cma_id);
            cmaclient.println(cma_message); 

       }    

      if(i = StrContains(buf,"SHB"))
       {
          /// max charging amps
          reply = atoi(&buf[i+1]);
          if(reply > 0 && reply < 999)
            cma_timer = reply * 1000;

          sprintf(cma_message,"myev|1.0|%ld|5|OK 0|",cma_id);
             cmaclient.println(cma_message); 
  
       }     
     
      if(i = StrContains(buf,"FD"))
       {
          /// max charging amps
          reply = atoi(&buf[i+1]);
          sprintf(buf,"$FD");
           if(myrapisend(buf))
           rapiSender.sendCmd("$FP 1 1 error");

          sprintf(cma_message,"myev|1.0|%ld|5|OK 0|",cma_id);
            cmaclient.println(cma_message); 

           
       }    

      if(i = StrContains(buf,"FE"))
       {
          /// max charging amps
          reply = atoi(&buf[i+1]);
          sprintf(buf,"$FE");
           if(myrapisend(buf))
           rapiSender.sendCmd("$FP 1 1 error");
          sprintf(cma_message,"myev|1.0|%ld|5|OK 0|",cma_id);
            cmaclient.println(cma_message); 
 
       
       } 

      
       if(i = StrContains(buf,"FR"))
       {
          /// max charging amps
          reply = atoi(&buf[i+1]);
          sprintf(buf,"$FR");
           if(myrapisend(buf))
          rapiSender.sendCmd("$FP 1 1 error");

          sprintf(cma_message,"myev|1.0|%ld|5|OK 0|",cma_id);
             cmaclient.println(cma_message); 
      
       } 
       

       if(i = StrContains(buf,"CS"))
       {
          /// max charging amps
          reply = atoi(&buf[i+1]);
          sprintf(buf,"$S4 0");
           if(myrapisend(buf))
           rapiSender.sendCmd("$FP 1 1 error");
          sprintf(cma_message,"myev|1.0|%ld|5|OK 0|",cma_id);
            cmaclient.println(cma_message); 

       }    
        
   
      if(i = StrContains(buf,"CP"))
       {
          /// max charging amps
          reply = atoi(&buf[i+1]);
          sprintf(buf,"$FD");
          if(myrapisend(buf))
           rapiSender.sendCmd("$FP 1 1 error");
          sprintf(cma_message,"myev|1.0|%ld|5|OK 0|",cma_id);
             cmaclient.println(cma_message); 
      
       
       }    

      if(i = StrContains(buf,"CR"))
       {
          /// max charging amps
          reply = atoi(&buf[i+1]);
          sprintf(buf,"$FE");
           if(myrapisend(buf))
          rapiSender.sendCmd("$FP 1 1 error");

          sprintf(cma_message,"myev|1.0|%ld|5|OK 0|",cma_id);
            cmaclient.println(cma_message); 
         

       }    

 
      if(i = StrContains(buf,"CK"))
       {
          /// max charging amps
          reply = atoi(&buf[i+1]);
          sprintf(buf,"$FD");
           if(myrapisend(buf))
           rapiSender.sendCmd("$FP 1 1 error");

           sprintf(buf,"$S4 1");
          if(myrapisend(buf))
           rapiSender.sendCmd("$FP 1 1 error");
       }    
   
    }
    
 }

 
 
if(!cmaclient.connected())
{
  cmaclient.stop();
  cmaclient.connect(CMA_server, 5000);
}


 
 if(cmaclient.connected())   
  {  

// Serial.println("cma connected");
  if(state != cma_state)
  {
 
    if(state == 2)
    {
      cma_state = 2;
       cma_pending_state = state;
       sprintf(cma_message,"myev|1.0|%ld|2|CC",cma_id);
    }
   
    if(state == 1)
    {
      sprintf(buf,"$S4 1");
      rapiSender.sendCmd(buf);

      cma_state = 1;
      cma_pending_state = state;
      sprintf(cma_message,"myev|1.0|%ld|2|CD",cma_id);
    }

    cma_retries = 10;
  
   }  
     
    if(cma_pending_state && cma_retries)
      {
        cmaclient.println(cma_message);
        
        if(cma_retries >0)
          cma_retries--;
                
      }



   
  }


return;

  }




// -------------------------------------------------------------------
// SETUP
// -------------------------------------------------------------------
void setup() {
  delay(2000);
  Serial.begin(115200);
  pinMode(0, INPUT);

  DEBUG_BEGIN(115200);

  DEBUG.println();
  DEBUG.print("OpenEVSE WiFI ");
  DEBUG.println(ESP.getChipId());
  DEBUG.println("Firmware: " + currentfirmware);

  DEBUG.printf("Free: %d\n", ESP.getFreeHeap());

  lcd_display(F("OpenEVSE WiFI"), 0, 0, 0, LCD_CLEAR_LINE);
  lcd_display(currentfirmware, 0, 1, 5 * 1000, LCD_CLEAR_LINE);
  lcd_loop();

  cma_timer = millis() + 5000;//delay cma for x seconds to give wifi a chance to connect
  cma_pending_state = 1;
  sprintf(cma_message,"myev|1.0|%ld|2|BU",cma_id);
  cma_retries = -1;
  rapiSender.sendCmd("$S4 1");
  
  // Read saved settings from the config
  config_load_settings();
  DBUGF("After config_load_settings: %d", ESP.getFreeHeap());

  // Initialise the WiFi
  wifi_setup();
  DBUGF("After wifi_setup: %d", ESP.getFreeHeap());

  // Bring up the web server
  web_server_setup();
  DBUGF("After web_server_setup: %d", ESP.getFreeHeap());

#ifdef ENABLE_OTA
  ota_setup();
  DBUGF("After ota_setup: %d", ESP.getFreeHeap());
#endif

  rapiSender.setOnEvent(on_rapi_event);
  rapiSender.enableSequenceId(0);

  // Check state the OpenEVSE is in.
  if (0 == rapiSender.sendCmd("$GS"))
  {
    if(rapiSender.getTokenCnt() >= 3)
    {
      const char *val = rapiSender.getToken(1);
      DBUGVAR(val);
      state = strtol(val, NULL, 10);
      DBUGVAR(state);
    }
  } else {
    DBUGLN("OpenEVSE not responding or not connected");
  }
} // end setup

// -------------------------------------------------------------------
// LOOP
// -------------------------------------------------------------------
void
loop() {
  Profile_Start(loop);

  lcd_loop();
  web_server_loop();
  wifi_loop();
#ifdef ENABLE_OTA
  ota_loop();
#endif
  rapiSender.loop();
  divert_current_loop();

  // Gives OpenEVSE time to finish self test on cold start
  if ( (millis() > 5000) && (rapi_read==0) ) {
    DBUGLN("first read RAPI values");
    handleRapiRead(); //Read all RAPI values
    rapi_read=1;
  }
  // -------------------------------------------------------------------
  // Do these things once every 2s
  // -------------------------------------------------------------------
  if ((millis() - Timer3) >= 2000) {
    DEBUG.printf("Free: %d\n", ESP.getFreeHeap());
    update_rapi_values();
    
    Timer3 = millis();
    
  }

  if(wifi_client_connected())
  {


    cma_update();
    if (config_mqtt_enabled()) {
      mqtt_loop();
    }

    // -------------------------------------------------------------------
    // Do these things once every 30 seconds
    // -------------------------------------------------------------------
    if ((millis() - Timer1) >= 30000) {
      DBUGLN("Time1");

      create_rapi_json(); // create JSON Strings for EmonCMS and MQTT
      if (config_emoncms_enabled()) {
        emoncms_publish(url);
      }
      if (config_mqtt_enabled()) {
        mqtt_publish(data);
      }
      if(config_ohm_enabled()) {
        ohm_loop();
      }
      Timer1 = millis();
    }
  } // end WiFi connected

  Profile_End(loop, 10);
} // end loop


void event_send(String event)
{
  web_server_event(event);

  if (config_mqtt_enabled()) {
    mqtt_publish(event);
  }
}
