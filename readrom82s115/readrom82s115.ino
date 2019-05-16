// Based on https://github.com/impressivemachines/Arduino/blob/master/readrom.ino
// by Simon Winder described at his blog http://simonwinder.com/2015/05/how-to-read-old-eproms-with-the-arduino/

#define SDCARD 0

#if SDCARD
#include <SD.h>

// The SD Chip Select pin is tied to pin 4 on the Arduino Wireless SD Shield
const int chipSelect = 4;
#define LOGFILE "DIAB"
String logFile = "";
#endif

int pin_d0 = 9;
int pin_d1 = 8;
int pin_d2 = 7;
int pin_d3 = 6;
int pin_d4 = 5;
int pin_d5 = 13;
int pin_d6 = 3;
int pin_d7 = 2;

int pin_sel_low = 10;
int pin_sel_high = 11;
int pin_cs = 12;

void set_port_as_input()
{
  pinMode(pin_d0, INPUT);
  pinMode(pin_d1, INPUT);
  pinMode(pin_d2, INPUT);
  pinMode(pin_d3, INPUT);
  pinMode(pin_d4, INPUT);
  pinMode(pin_d5, INPUT);
  pinMode(pin_d6, INPUT);
  pinMode(pin_d7, INPUT);
}

void set_port_as_output()
{
  pinMode(pin_d0, OUTPUT);
  pinMode(pin_d1, OUTPUT);
  pinMode(pin_d2, OUTPUT);
  pinMode(pin_d3, OUTPUT);
  pinMode(pin_d4, OUTPUT);
  pinMode(pin_d5, OUTPUT);
  pinMode(pin_d6, OUTPUT);
  pinMode(pin_d7, OUTPUT);
}

void set_port_value(byte value)
{
  digitalWrite(pin_d0, (value & 1) ? HIGH : LOW);
  digitalWrite(pin_d1, (value & 2) ? HIGH : LOW);
  digitalWrite(pin_d2, (value & 4) ? HIGH : LOW);
  digitalWrite(pin_d3, (value & 8) ? HIGH : LOW);
  digitalWrite(pin_d4, (value & 16) ? HIGH : LOW);
  digitalWrite(pin_d5, (value & 32) ? HIGH : LOW);
  digitalWrite(pin_d6, (value & 64) ? HIGH : LOW);
  digitalWrite(pin_d7, (value & 128) ? HIGH : LOW);
}

byte read_port_value()
{
  byte val = 0;
  if (digitalRead(pin_d0))
    val |= 1;
  if (digitalRead(pin_d1))
    val |= 2;
  if (digitalRead(pin_d2))
    val |= 4;
  if (digitalRead(pin_d3))
    val |= 8;
  if (digitalRead(pin_d4))
    val |= 16;
  if (digitalRead(pin_d5))
    val |= 32;
  if (digitalRead(pin_d6))
    val |= 64;
  if (digitalRead(pin_d7))
    val |= 128;

  return val;
}

byte read_cycle(short address)
{
  digitalWrite(pin_cs, HIGH);
  digitalWrite(pin_sel_low, LOW);
  digitalWrite(pin_sel_high, LOW);

  set_port_as_output();
  set_port_value(address & 0xff);
  digitalWrite(pin_sel_low, HIGH);
  delay(1);
  digitalWrite(pin_sel_low, LOW);
  delay(1);

  set_port_value((address & 0xff00) >> 8);
  digitalWrite(pin_sel_high, HIGH);
  delay(1);
  digitalWrite(pin_sel_high, LOW);
  delay(1);

  set_port_as_input();
  digitalWrite(pin_cs, LOW);
  delay(1);
  byte val = read_port_value();
  delay(1);
  digitalWrite(pin_cs, HIGH);

  return val;
}

char cmdbuf[64];
int cmd_index;
String dataString = "";

void setup()
{
  set_port_as_input();

  pinMode(pin_sel_low, OUTPUT);
  digitalWrite(pin_sel_low, LOW);

  pinMode(pin_sel_high, OUTPUT);
  digitalWrite(pin_sel_high, LOW);

  pinMode(pin_cs, OUTPUT);
  digitalWrite(pin_cs, HIGH);

  Serial.begin(115200);

  cmd_index = 0;
  cmdbuf[0] = 0;
  dataString.reserve(80);
#if SDCARD
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  Serial.print("Initializing SD card...");
  pinMode(chipSelect, OUTPUT);

  // see if the card is present and can be initialized:
  if (SD.begin(chipSelect))
  {
    Serial.println("Card initialized.");
    int loggNo = 0;
    while (SD.exists(LOGFILE + String(loggNo, DEC) + ".bin"))
    {
      loggNo++;
    }
    logFile = LOGFILE +  String(loggNo, DEC) + ".bin";
    Serial.println("Logfile: " + logFile);
  }
  else
  {
    Serial.println("Card failed, or not present!");
  }
#endif
}

void loop()
{
  // put your main code here, to run repeatedly:
  if (Serial.available() > 0)
  {
    char b = Serial.read();
    if (b != '\n')
    {
      if (cmd_index < 63)
        cmdbuf[cmd_index++] = b;
    }
    else
    {
      cmdbuf[cmd_index] = 0;
      if (cmd_index > 0)
      {
        if (cmdbuf[0] == 'm' && cmdbuf[1] == ' ')
        {
          int addr_start;
          int addr_end;
          int num = sscanf(cmdbuf + 2, "%x%x", &addr_start, &addr_end);
          if (num == 0)
            Serial.println('?');
          else
          {
            if (num == 1)
              addr_end = addr_start + 16;
            int addr = addr_start;
            int n = 0;
            dataString = "";
            while (addr <= addr_end)
            {
              if (n == 0)
              {
                //char buf[16];
                //sprintf(buf, "[%04x]:", addr);
                Serial.println(dataString);
                dataString = "";
                // Pad the address with leading zeroes for nice fixed width output
                if (addr <= 0x000f) dataString += "0";
                if (addr <= 0x00ff) dataString += "0";
                if (addr <= 0x0fff) dataString += "0";
                if (addr <= 0xffff) dataString += "0";
                dataString += String(addr, HEX) + " ";
                //while (Serial.availableForWrite() < 8) ;
                //Serial.println('z');
                //Serial.print(buf);
              }
              n++;
              if (n == 16)
              {
                n = 0;
              }

              byte data = read_cycle(addr);
#if SDCARD
              File dataFile = SD.open(logFile, FILE_WRITE);
              if (dataFile)
              {
                // Print data to the file.
                dataFile.print(data);
                dataFile.close();
                Serial.print(".");
              }
#endif
              //char buf[16];
              //sprintf(&buf[0], "|%02x|-", data);
              // Pad the data byte with a leading zero for nice fixed width output
              if (data <= 0x0f) dataString += "0";
              dataString += String(data, HEX) + " ";
              //while (Serial.availableForWrite() < 8) ;
              //Serial.print(buf);
              addr++;
            }
          }
        }
        else
        {
          Serial.println('?');
        }

        cmd_index = 0;
      }
    }
  }
}
