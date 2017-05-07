/* _____  _____  __  __             _____  _____
  / ____||_   _||  \/  |    /\     / ____||_   _|
  | |  __   | |  | \  / |   /  \   | (___    | |
  | | |_ |  | |  | |\/| |  / /\ \   \___ \   | |
  | |__| | _| |_ | |  | | / ____ \  ____) | _| |_
  \_____||_____||_|  |_|/_/    \_\|_____/ |_____|
  (c) 2017 GIMASI SA

   tuino1_thermostat.ino

    Created on: March 11, 2017
        Author: Massimo Santoli massimo@gimasi.ch
        Brief: Tuino1 Maker's Kit IoT Thermostat Demo
        Version: 1.0

        License: it's free - do whatever you want! ( provided you leave the credits)

        Hysteresis and bug fixes by Remi Gunsett remi.gunsett@actility.com
*/

#include "gmx_lr.h"
#include "Regexp.h"
#include "SeeedOLED.h"
#include "display_utils.h"

#include <Wire.h>

// NFC
#include "M24SR.h"
#include "NdefMessage.h"
#include "NdefRecord.h"


// RegExp Engine for NFC parsing
MatchState nfc_ms;
char regexp_buf[512];

#define gpo_pin TUINO_NFC_INTERRUPT

bool writing_nfc = false;

M24SR m24sr04(gpo_pin);
// END NFC

long int timer_period_to_tx = 60000;
long int timer_millis_lora_tx = 0;
int ledState = 0;

int buttonPin = D4;
int relayPin = D5;
int tempSensorPin = A0;

float thermostat_temperature = 21.0f;
int manual_mode = 0;
int relay_status = 0;

// Temperature Sensor constants
const int B = 4275;               // B value of the thermistor
const int R0 = 100000;          // R0 = 100k

#define TEMPERATURE_SAMPLING    (20)
#define THERMOSTAT_HYSTERESIS   (1.0f)

float current_temperature = 0.0f;
float temp_temperature = 0;
int temperature_counts = 0;


// LoRa RX interrupt

bool data_received = false;

void loraRx() {
  data_received = true;
}


float readTemp() {
  int a = analogRead(tempSensorPin );

  float R = 1023.0 / ((float)a) - 1.0;
  R = 100000.0 * R;

  float temperature = 1.0 / (log(R / 100000.0) / B + 1 / 298.15) - 273.15; //convert to temperature via datasheet ;

  return temperature;
}



void writeNFC() {
  char string[32];
  String tmpString;
  String nfc_data;

  gmxLR_getDevEui(tmpString);
  nfc_data = "DevEUI:" + tmpString;
  nfc_data = nfc_data + "\n\r";

  nfc_data = nfc_data + "Temp:" + String(readTemp());
  nfc_data = nfc_data + "\n\r";

  nfc_data = nfc_data + "Setting:" + String(thermostat_temperature);
  nfc_data = nfc_data + "\n\r";

  nfc_data = nfc_data + "Mode:";
  if ( manual_mode == 1 )
    nfc_data = nfc_data + "MANUAL";
  else
    nfc_data = nfc_data + "AUTOMATIC";
  nfc_data = nfc_data + "\n\r";


  nfc_data = nfc_data + "Relay:";
  if ( relay_status == 1 )
    nfc_data = nfc_data + "ON";
  else
    nfc_data = nfc_data + "OFF";
  nfc_data = nfc_data + "\n\r";


  // WriteNFC data
  writing_nfc = true;
  NdefMessage message = NdefMessage();
  message.addTextRecord(nfc_data);
  m24sr04.writeNdefMessage(&message);
  delay(200);
  writing_nfc = false;
}


void waitButton() {

  while ( digitalRead(buttonPin) == 0 )
  {

  };

  delay(200);
}


void setup() {
  // put your setup code here, to run once:
  String DevEui;
  String AppEui;
  String AppKey;
  String _AppEui;
  String _AppKey;
  String loraClass;

  char string[64];

  String adr, dcs, dxrate;

  byte join_status;
  int join_wait;

  Wire.begin();
  Serial.begin(9600);
  Serial.println("Starting");

  // Startupp NFC
  m24sr04._setup();

  // Init Oled
  SeeedOled.init();  //initialze SEEED OLED display

  SeeedOled.clearDisplay();          //clear the screen and set start position to top left corner
  SeeedOled.setNormalDisplay();      //Set display to normal mode (i.e non-inverse mode)
  SeeedOled.setHorizontalMode();           //Set addressing mode to Page Mode

  // Init Digital Outputs
  // Button
  pinMode(buttonPin, INPUT);
  // Relay
  pinMode(relayPin, OUTPUT);


  // GMX-LR init pass callback function
  gmxLR_init(&loraRx);

  // Set AppEui and AppKey
  // Uncomment these if you want to change the default keys
  // AppEui = "00:00:00:00:00:00:00:00";
  // AppKey = "6d:41:46:39:67:4e:30:56:46:4a:62:4c:67:30:58:33";


  // Show Splash Screen on OLED
  splashScreen();
  waitButton();

  // Show LoRaWAN Params on OLED
  gmxLR_getDevEui(DevEui);
  gmxLR_getAppKey(AppKey);
  gmxLR_getAppEui(AppEui);
  displayLoraWanParams(DevEui, AppEui, AppKey);
  waitButton();

  SeeedOled.clearDisplay();
  SeeedOled.setTextXY(0, 0);
  SeeedOled.putString("Joining...");

  Serial.println("Joining...");
  join_wait = 0;
  while ((join_status = gmxLR_isNetworkJoined()) != LORA_NETWORK_JOINED) {


    if ( join_wait == 0 )
    {
      // If AppKey and/or AppEUI are specified set them
      if (AppEui.length() > 0 )
        gmxLR_setAppEui(AppEui);
      if (AppKey.length() > 0 )
        gmxLR_setAppKey(AppKey);

      // Disable Duty Cycle  ONLY FOR DEBUG!
      gmxLR_setDutyCycle("0");

      // Set LoRaWAN Class
      gmxLR_setClass("C");

      Serial.println("LoRaWAN Params:");
      gmxLR_getDevEui(DevEui);
      Serial.println("DevEui:" + DevEui);
      gmxLR_getAppEui(AppEui);
      Serial.println("AppEui:" + AppEui);
      gmxLR_getAppKey(AppKey);
      Serial.println("AppKey:" + AppKey);
      gmxLR_getClass(loraClass);
      Serial.println("Class:" + loraClass);
      adr = String( gmxLR_getADR() );
      Serial.println("ADR:" + adr);
      dcs = String( gmxLR_getDutyCycle() );
      Serial.println("DCS:" + dcs);
      gmxLR_getRX2DataRate(dxrate);
      Serial.println("RX2 DataRate:" + dxrate);

      gmxLR_Join();
    }

    SeeedOled.setTextXY(1, 0);
    sprintf(string, "Attempt: %d", join_wait);
    SeeedOled.putString(string);

    join_wait++;

    if (!( join_wait % 100 )) {
      gmxLR_Reset();
      join_wait = 0;
    }

    delay(5000);

  };

  SeeedOled.setTextXY(2, 0);
  SeeedOled.putString("Joined!");

  writeNFC();
  delay(2000);
  SeeedOled.clearDisplay();

  // Init Temperature
  current_temperature = readTemp();

}

void loop() {
  // put your main code here, to run repeatedly:

  long int delta_lora_tx;
  int temperature_int;
  char lora_data[32];
  byte tx_buf[32];
  byte rx_buf[256];
  int buf_len;

  String rx_data;
  String tx_data;
  int port;

  delta_lora_tx = millis() - timer_millis_lora_tx;

  // Transmit Period
  if (( delta_lora_tx > timer_period_to_tx) || (timer_millis_lora_tx == 0 )) {

    temperature_int = current_temperature * 100;

    Serial.println(temperature_int);

    tx_buf[0] = 0x02; // packet header - multiple data
    tx_buf[1] = (temperature_int & 0xff00 ) >> 8;
    tx_buf[2] = temperature_int & 0x00ff;
    tx_buf[3] = relay_status;
    tx_buf[4] = manual_mode;

    temperature_int = thermostat_temperature * 100;
    tx_buf[5] = (temperature_int & 0xff00 ) >> 8;
    tx_buf[6] = (temperature_int & 0x00ff );

    sprintf(lora_data, "%02X%02X%02X%02X%02X%02X%02X", tx_buf[0], tx_buf[1], tx_buf[2], tx_buf[3], tx_buf[4], tx_buf[5], tx_buf[6]);

    displayLoraTX(true);
    Serial.println(lora_data);
    tx_data = String(lora_data);
    Serial.println("TX DATA:" + tx_data);
    gmxLR_TXData(tx_data);


    timer_millis_lora_tx = millis();


    // Update NFC
    writeNFC();

    displayLoraTX(false);
  }
  else
  {
    displayTime2TX(timer_period_to_tx - delta_lora_tx);
  }
  // End Trasmission

  if (data_received)
  {
    displayLoraRX(true);

    gmxLR_RXData(rx_data, &port);
    gmxLR_StringToHex(rx_data, rx_buf, &buf_len );

    Serial.println("LORA RX DATA:" + rx_data);
    Serial.print("LORA RX LEN:");
    Serial.println(buf_len);
    Serial.print("LORA RX PORT:");
    Serial.println(port);


    if (rx_buf[0] == 0x01 ) {

      if ( rx_buf[1] == 0x01 )
      {
        manual_mode = 1;
        relay_status = 1;
      }
      else
      {
        manual_mode = 0;
        relay_status = 0;
      }

      Serial.print("Manual Mode=");
      Serial.println( relay_status );

    }


    if (rx_buf[0] == 0x02 ) {


      temperature_int = rx_buf[1];
      temperature_int = temperature_int << 8;

      temperature_int = temperature_int + rx_buf[2];
      thermostat_temperature = (float) ( temperature_int ) / 100.0;

      Serial.print("Set Temp=");
      Serial.println( thermostat_temperature );
    }

    data_received = false;

    delay(1000);
    displayLoraRX(false);
  }

  // activate actuators and display information
  digitalWrite(relayPin, relay_status);
  displayTemp(current_temperature, thermostat_temperature, manual_mode, relay_status);

  // check for buttonPressed and send a packet
  if (digitalRead(buttonPin) == 1) {
    timer_millis_lora_tx = 0;
  }

  // update temperature
  // we sample and make an average of the temperature - since in this demo we use a NTC sensor that fluctuates
  temp_temperature += readTemp();
  temperature_counts ++;

  if ( temperature_counts >= TEMPERATURE_SAMPLING )
  {
    current_temperature = temp_temperature / TEMPERATURE_SAMPLING;

    temperature_counts = 0;
    temp_temperature = 0;
  }
  
  // do thermostat logic
  if ( manual_mode == 0 ) {

    if ( ( thermostat_temperature != 0 ) && (current_temperature != 0.0f) ) {

      if (current_temperature <= (thermostat_temperature - THERMOSTAT_HYSTERESIS)) {
        relay_status = 1;
      }
      else if (current_temperature >= (thermostat_temperature + THERMOSTAT_HYSTERESIS))
      {
        relay_status = 0;
      }
    }
  }
}
