#include <Arduino.h>
#include <SimpleDHT.h>
#include <DS3231_Simple.h>
#include <Wire.h>
#include <FastLED.h>
#include <EEPROM.h>
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SPI.h>
#include <LiquidCrystal_I2C.h>

#include "keypad.h"
#include "led_controller.h"

#define def_COUNT 24 //number of defines

#define NUM_LEDS 4 //number of leds

byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};
IPAddress ip(192,168,1,177);
unsigned int localPort = 8888;      // local port to listen on
char packetBuffer[30];  // buffer to hold incoming packet,
EthernetUDP Udp;

IPAddress subnet(255,255,255,0);
IPAddress gateway(192,168,1,1);

LiquidCrystal_I2C lcd(0x27, 16, 2);

int eeAddress = EEPROM[12]; //element tracker for EEPROM

CRGB leds[NUM_LEDS]; //initilizing led
unsigned int userRED = 255; //setting led red value
unsigned int userGREEN = 255; //setting led green value
unsigned int userBLUE = 255; //setting led blue value


//struct for holding data in EEPROM
struct TimeStamp {
    short temperature;
    byte humidity;
    byte hour;
    byte minute;
    byte year;
    byte month;
    byte day;
};

float currTemp; //holds current temperature reading
float currHumid; //holds current humidity reading
bool showTemp = false; //bool to determine whether or not to loop five second temp





#define MAX_ARGS 9 //max arguments for token buffer
#define MAX_BUF 30 // max length of string buffer

static const uint8_t version[3] = {1,3,0}; //program version

unsigned long userBlinkLong; //number the user sets blink to

byte tokenBuffer[MAX_ARGS]; //token buffer
char buffer[MAX_BUF]; //string buffer

char parsedString[MAX_ARGS][MAX_BUF]; //hold the parsed string
uint8_t wordNum = 0; //number of words in parsed string
uint8_t charNum = 0; //number of letters for each word in parsed string

//definitions array for checking input
static const char definesArr[def_COUNT][4] = {
    {'D','1',3,t_D13},
    {'O','N',2,t_ON},
    {'O','F',3,t_OFF},
    {'B','L',5,t_BLINK},
    {'L','E',3,t_LED},
    {'G','R',5,t_GREEN},
    {'R','E',3,t_RED},
    {'S','E',3,t_SET},
    {'S','T',6,t_STATUS},
    {'L','E',4,t_LEDS},
    {'V','E',7,t_VERSION},
    {'H','E',4,t_HELP},
    {'W', 'O', 4, t_WORD},
    {'A', 'L', 9, t_ALTERNATE},
    {'T', 'I', 4, t_TIME},
    {'C', 'U', 7, t_CURRENT},
    {'T', 'E', 4, t_TEMP},
    {'A', 'D', 3, t_ADD},
    {'R', 'G', 3, t_RGB},
    {'H', 'I', 7, t_HISTORY},
    {'H', 'I', 4, t_HIGH},
    {'L', 'O', 3, t_LOW},
    {'C', 'L', 5, t_CLEAR},
    {'E','O',3,char(t_EOL)}
};

unsigned long msStart = 0; //time of starting the program
unsigned long currentMillis; //current time elapsed since starting program
unsigned long msPrev = 0; //last time something was ran in proram


unsigned long interval = 500; //blink interval time 
char led_status = 'O'; //status of current led, 'O'->OFF, 'R'->RED, 'G'->GREEN
char led_Previous = 'R'; //status of last color led input, 'O'->OFF, 'R'->RED, 'G'->GREEN
static bool led_goBlink = false; //blink booolen to tell LED to blink or not
static bool led_goALTERNATE = false; //alternates led colors if true
static bool ledForce = false; //forces led blink

int RGB_brightness = 30; //preset brightness for RGB
static bool RGB_status = 0; //rgb status
static bool RGB_goBlink = false; //whether or not RGB is blinking
static bool RGBForce = false; //forces rgb blink

byte hiByte; //upper half of user input for set blink
byte loByte; //lower half of user input for set blink
unsigned int combinedVal = 500; //hiByte and loByte together (hiByte + loByte)

long addedNum = 0; //final value for add command

byte pinDHT22 = 2; //pin for DHT
SimpleDHT22 dht22(pinDHT22); //initializing dht
DS3231_Simple Clock; //initilizing clock
DateTime MyDateAndTime; //initializing MyDateAndTime for changing clock

//initializers for changing clock
byte month = 1;
byte day = 1;
byte year = 0;
byte hour = 0;
byte minute = 0;
byte second = 0;

//UDP packet alarm info
byte alarm = 0;
IPAddress ipRemote(192,168,1,180);
unsigned int remotePort = 52334;
bool receivedPacket = false;
bool thresholdSent = false;


byte ledBlinkRed = 0b01110111; //red led blinking byte
byte ledBlinkGreen = 0b10111011; //green led blinking byte


//LCD vairables
int menu;
int command;
int currentMenu = 5;
char statsBuffer[100];
byte packetReceived = 0;
byte packetSent = 0;



//checks whether or not a string contains only numbers
bool isValidNumber(char str[5]){
    //bool ret = 0;
    for(byte i=0;i<5;i++)
    {
        if(isDigit(str[i])) return true;
    }
    return false;
}

//initial prompt when program is run
void introPrompt(){
    Serial.println(F(">Welcome to Arduino Command Line Interpreter, enter 'HELP' for list of commands!\n"));
}

byte majUnd = 60;
byte lowMinUnd = 61;
byte highMinUnd = 70;
byte lowCom = 71;
byte highCom = 80;
byte lowMinOve = 81;
byte highMinOve = 90;
byte majOve = 91;

//setup function
void setup(){  
    led_setup();
    Ethernet.begin(mac, ip);
    Serial.begin(9600);
    Clock.begin();
    FastLED.addLeds<WS2812B,7>(leds, NUM_LEDS);
    FastLED.addLeds<WS2812B,7>(leds, NUM_LEDS);
    FastLED.addLeds<WS2812B,7>(leds, NUM_LEDS);
    FastLED.addLeds<WS2812B,7>(leds, NUM_LEDS);
    FastLED.setBrightness(RGB_brightness);
    leds[0] = CRGB::Black;
    leds[1] = CRGB::Black;
    leds[2] = CRGB::Black;
    leds[3] = CRGB::Red;

    FastLED.show();
    introPrompt();
    while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
    }
    // start UDP
    Udp.begin(localPort);

    // initialize the LCD
	lcd.begin();
	// Turn on the blacklight and print a message.
	lcd.backlight();
    menu = 5;
    // EEPROM[0] = ip[0];
    // EEPROM[1] = ip[1];
    // EEPROM[2] = ip[2];
    // EEPROM[3] = ip[3];
    // EEPROM[4] = subnet[0];
    // EEPROM[5] = subnet[1];
    // EEPROM[6] = subnet[2];
    // EEPROM[7] = subnet[3];
    // EEPROM[8] = gateway[0];
    // EEPROM[9] = gateway[1];
    // EEPROM[10] = gateway[2];
    // EEPROM[11] = gateway[3];
    // EEPROM[12] = 12;  

    
}



//clears the parsed string for next set of input
void clearParsedString(char arr[MAX_ARGS][MAX_BUF]){
  for(byte i = 0; i <=MAX_ARGS; i++){
    for(uint8_t j=0; j<=MAX_BUF;j++){
      arr[i][j] = 0;
    }
  }
  return;
}

//my atoi function
int myAtoi(char *str){
    int res = 0; 

    for (byte i = 0; str[i] != '\0'; ++i) {
        if (str[i]> '9' || str[i]<'0')
            return -1;
        res = res*10 + str[i] - '0';
    }

    return res;
}


//my atol function
long myAtol(char *str){
    long res = 0; 
    bool negative = false;

    for (byte i = 0; str[i] != '\0'; ++i) {
        if (str[i] == '-'){
            negative = true;
            i++;
        }
        if (str[i]> '9' || str[i]<'0'){
            return -1;
        }
            
        res = res*10 + str[i] - '0';
    }
    if (negative){
        res = (-1)*res;
    }

    return res;
}

//return string length
byte myStrlen(char *str){
    byte count = 0;
    while (str[count] != '\0'){
        count++;
    }
    return count;
}




//takes parsed string and definitions array and fills the token buffer based on the input
void fillTokenBuffer(char parsedStr[MAX_ARGS][MAX_BUF], char defsArr[def_COUNT][4]){
    bool num1 = false; //for checking if add number 1 is in range
    bool num2 = false; //for checking if add number 2 is in range
    for (byte i = 0; i < MAX_ARGS; i++){
        for (byte j = 0; j < def_COUNT; j++){
            if ((parsedStr[i][0] == defsArr[j][0]) //for commands that don't contain numbers
            && (parsedStr[i][1] == defsArr[j][1])
            && (myStrlen(parsedStr[i]) == defsArr[j][2])){
                tokenBuffer[i] = defsArr[j][3];
                tokenBuffer[i+1] = t_EOL;               
            }

            //for commands that contain numbers
            else if(isDigit(parsedStr[i][0]) || (tokenBuffer[0] == t_ADD && parsedStr[i][0] == '-' && isDigit(parsedStr[i][1]))){
                if (tokenBuffer[0] == t_SET && tokenBuffer[1] == t_BLINK){
                    tokenBuffer[i] = t_WORD;
                    userBlinkLong = myAtol(parsedStr[i]);
                    if (userBlinkLong <= 65535){
                        hiByte = userBlinkLong >> 8;
                        loByte = userBlinkLong;
                        tokenBuffer[3] = hiByte;
                        tokenBuffer[4] = loByte;
                        tokenBuffer[5] = t_EOL;

                    }
                }
                
                //adding to token buffer for add
                else if (tokenBuffer[0] == t_ADD){
                    if (i == 1 && (myAtol(parsedStr[i]) <= 32767 && myAtol(parsedStr[i]) >=  -32767)){
                        addedNum = myAtol(parsedStr[i]);
                        tokenBuffer[1] = addedNum >> 8;
                        tokenBuffer[2] = addedNum;
                        num1 = true;
                    }
                    else if(i == 2 && (myAtol(parsedStr[i]) <= 32767 && myAtol(parsedStr[i]) >=  -32767)){
                        addedNum = myAtol(parsedStr[i]);
                        tokenBuffer[3] = addedNum >> 8;
                        tokenBuffer[4] = addedNum;
                        num2 = true;
                    }
                    if (num1 && num2){
                        tokenBuffer[5] = t_EOL;
                    }
                                       
                }


                //adding to token buffer for rgb
                else if (tokenBuffer[0] == t_RGB){
                    tokenBuffer[1] = t_WORD;
                    if (myAtoi(parsedStr[i]) <= 255){
                        tokenBuffer[i+1] = myAtoi(parsedStr[i]);
                        tokenBuffer[i+2] = t_EOL;
                    }
                    else{
                        tokenBuffer[0] = t_EOL;
                    }
                    
                }

                //adding to token buffer for set time
                else if (tokenBuffer[0] == t_SET && tokenBuffer[1] == t_TIME){
                    if (i == 2){
                        
                        if (myAtoi(parsedStr[i]) <= 12 && myAtoi(parsedStr[i]) > 0){
                            month = myAtoi(parsedStr[i]);
                            
                        }

                        else{
                            tokenBuffer[0] = t_EOL;
                        }
                        tokenBuffer[i] = month;
                        
                    }
                    else if (i == 3){
                        
                        if (myAtoi(parsedStr[i]) <= 31 && myAtoi(parsedStr[i]) > 0){
                            day = myAtoi(parsedStr[i]);
                            
                        }
                        else{
                            tokenBuffer[0] = t_EOL;
                        }
                        tokenBuffer[i] = day;
                    }
                    else if (i == 4){
                        
                        if (myAtoi(parsedStr[i]) <= 99 && myAtoi(parsedStr[i]) >= 0){
                            year = myAtoi(parsedStr[i]);
                            
                        }
                        else{
                            tokenBuffer[0] = t_EOL;
                        }
                        tokenBuffer[i] = year;
                    }
                    else if (i == 5){
                        
                        if (myAtoi(parsedStr[i]) <=23 && myAtoi(parsedStr[i]) >= 0){
                            hour = myAtoi(parsedStr[i]);
                            
                        }
                        else{
                            tokenBuffer[0] = t_EOL;
                        }
                        tokenBuffer[i] = hour;
                    }
                    else if (i == 6){
                        
                        if (myAtoi(parsedStr[i]) < 60 && myAtoi(parsedStr[i]) >= 0){
                            minute = myAtoi(parsedStr[i]);
                            
                        }
                        else{
                            tokenBuffer[0] = t_EOL;
                        }
                        tokenBuffer[i] = minute;
                    }
                    else if (i == 7){
                        
                        if (myAtoi(parsedStr[i]) < 60 && myAtoi(parsedStr[i]) >= 0){
                            second = myAtoi(parsedStr[i]);
                            
                        }
                        else{
                            tokenBuffer[0] = t_EOL;
                        }
                        tokenBuffer[i] = second;
                    }
                    tokenBuffer[8] = t_EOL;
                }
            }
        }
    }
}

//applies the number entered with 'SET BLINK' to the interval variable for blinking 
void applyUserInterval(){
    combinedVal = tokenBuffer[4] | (tokenBuffer[3] << 8);
    if ((combinedVal >= 0) && (combinedVal <= 65535)){
        interval = combinedVal;
    }
}


//adds two numbers
void ADD(){
    long hi = 0;
    long lo;
    hi = tokenBuffer[2] | (tokenBuffer[1] << 8);
    lo = tokenBuffer[4] | (tokenBuffer[3] << 8);
    long printvalue = hi + lo;
    Serial.print(F("EQUALS: "));
    Serial.println(printvalue);
    Udp.beginPacket(ipRemote, remotePort);
    Udp.print(F("EQUALS: "));
    Udp.print(printvalue);
    Udp.endPacket();

}


//turns the led green
void led_GREEN(){
    led_status = 'G';
    led_Previous = 'G';
    setColor(led_status);
}

//turns the led red
void led_RED(){
    led_status = 'R';
    led_Previous = 'R';
    setColor(led_status);
}

//turns the led off
void led_OFF(){
    led_status = 'O';
    setColor(led_status);
}



//blinks the led using byte rotation
void led_BLINK2(){
    static long int ledTimer = 0;
    if ((ledTimer < millis() && led_goBlink && (led_Previous == 'R')) //if led was red
     || ((ledForce == true) && (led_Previous == 'R'))){
        if (ledBlinkRed & 1 == 1){ 
            if(ledBlinkRed >> 1 & 1 == 1){ //if '11'
                led_status = 'O';
                ledTimer = millis()+interval;
                led_OFF();
                ledForce = false;
            }
            else{ //if '10'
                led_status = 'R';
                ledTimer = millis()+interval;
                led_RED();
                ledForce = false;
            }
            
        }
        ledBlinkRed = ledBlinkRed >> 2;
        
        if(ledBlinkRed == 0){
            ledBlinkRed = 0b01110111;
        }
    }

    if ((ledTimer < millis() && led_goBlink && (led_Previous == 'G')) //if led was green
     || ((ledForce == true) && (led_Previous == 'G'))){

        if (ledBlinkGreen & 1 == 1){
            if(ledBlinkGreen >> 1 & 1 == 1){ //if '11'
                led_status = 'O';
                ledTimer = millis()+interval;
                led_OFF();
                ledForce = false;
            }
        }
        else{
            if(ledBlinkGreen >> 1 & 1 == 1){ //if '01'
                led_status = 'G';
                ledTimer = millis()+interval;
                led_GREEN();
                ledForce = false;
            }
        }
        ledBlinkGreen = ledBlinkGreen >> 2;
        if(ledBlinkGreen == 0){
            ledBlinkGreen = 0b10111011;
        }

    }
}

//leds color ALTERNATEs between red and green at the interval 
void led_ALTERNATE(){
    
    static long int ledTimer = 0;
    if (ledTimer < millis() && led_goALTERNATE){
        if ((led_Previous == 'O' || led_Previous == 'R') && led_status == 'O'){
            led_Previous = 'R';
            led_status = 'R';
            ledTimer = millis()+interval;
            led_RED();
        }
        else if(led_Previous == 'R' && led_status == 'R'){
            led_Previous = 'G';
            led_status = 'G';
            ledTimer = millis()+interval;
            led_GREEN();
        }
        else if(led_Previous == 'G' && (led_status == 'G' || led_status == 'O')){
            led_Previous = 'R';
            led_status = 'R';
            ledTimer = millis()+interval;
            led_RED();
        }
    }
}

//prints the status of both leds
void status_LEDS(){
    Udp.beginPacket(ipRemote, remotePort);
    Serial.print(F("> LED "));
    Udp.print(F("> LED "));
    if (led_goBlink  && !led_goALTERNATE){
        Serial.print(F("BLINKING "));
        Udp.print(F("BLINKING "));
        if(led_Previous == 'R'){
            Serial.println(F("RED"));
            Udp.print(F("RED "));
        }
        else{
            Serial.println(F("GREEN"));
            Udp.print(F("GREEN "));
        }
    }
    else if(!led_goBlink && !led_goALTERNATE){
        if(led_Previous == 'R'){
            if (led_status == 'R'){
                Serial.println(F("RED"));
                Udp.print(F("RED "));
            }
            else{
                Serial.println(F("OFF"));
                Udp.print(F("OFF "));
            }
        }
        else if (led_Previous == 'G'){
            if (led_status == 'G'){
                Serial.println(F("GREEN"));
                Udp.print(F("GREEN "));
            }
            else{
                Serial.println(F("OFF"));
                Udp.print(F("OFF "));
            }
        }
        else{
            Serial.println(F("OFF"));
            Udp.print(F("OFF "));
        }
    }
    else if(led_goALTERNATE){
        Serial.println(F("ALTERNATING"));
        Udp.print(F("ALTERNATING "));
    }

    if (RGB_goBlink){
        // Serial.print(F("> RGB BLINKING ("));
        // Serial.print(userGREEN); Serial.print(' ');
        // Serial.print(userRED); Serial.print(' ');
        // Serial.print(userBLUE);
        // Serial.println(')');
        Udp.print(F("> RGB BLINKING ("));
        Udp.print(userGREEN); Udp.print(' ');
        Udp.print(userRED); Udp.print(' ');
        Udp.print(userBLUE);
        Udp.print(') ');

    }
    else if(!RGB_goBlink){
        Serial.print(F("> RGB "));
        Udp.print(F("> RGB "));
        if (RGB_status){
            // Serial.print(F("ON ("));
            // Serial.print(userGREEN); Serial.print(", ");
            // Serial.print(userRED); Serial.print(", ");
            // Serial.print(userBLUE);
            // Serial.println(')');
            Udp.print("ON (");
            Udp.print(userGREEN); Udp.print(", ");
            Udp.print(userRED); Udp.print(", ");
            Udp.print(userBLUE);
            Udp.print(') ');
        }
        else{
            Serial.println(F("OFF"));
            Udp.print("OFF ");
        }
    }
    Serial.print(F("> BLINK INTERVAL SET TO: "));
    Serial.println(interval);
    Udp.print("> BLINK INTERVAL SET TO: ");
    Udp.print(interval);
    Udp.endPacket();
}

//returns the temp in farenheit
short celsiusToFarenheit(float cel){
    return (cel*1.8)+32;
}


//prints the temp and humidity every 5 seconds
void five_SECOND_TEMP(){
    static long int timer = 0;

    if (timer<millis() && showTemp == true){
        dht22.read2(&currTemp, &currHumid, NULL);
        // Serial.print((float)currTemp); Serial.print(F(" *C, "));
        // Serial.print(celsiusToFarenheit(currTemp)); Serial.print(F(" *F, "));
        // Serial.print((float)currHumid); Serial.println(F(" RH%"));
        
        Udp.beginPacket(ipRemote, remotePort);
        Udp.print(float(currTemp)); Udp.print(F(" *C, "));
        Udp.print(celsiusToFarenheit(currTemp)); Udp.print(F(" *F, "));
        Udp.print((float)currHumid); Udp.print(F(" RH%"));
        Udp.endPacket();
        timer= millis()+5000;
        packetSent++;
        

    }
}

//assigns the current temperature and humidity
void current_TEMP(){
    static long int timer = 0;
    if (timer<millis()){
        dht22.read2(&currTemp, &currHumid, NULL);
        timer= millis()+5000;
    }
    
}


//writes time and temp data to EEPROM every 15 minutes
void write_TO_EEPROM(){
    if ((Clock.read().Minute == 0 || Clock.read().Minute == 15
     || Clock.read().Minute == 30 || Clock.read().Minute == 45) && Clock.read().Second == 0){
        dht22.read2(&currTemp, &currHumid, NULL);
        TimeStamp curr = {
            int(currTemp),
            int(currHumid),
            Clock.read().Hour,
            Clock.read().Minute,
            Clock.read().Year,
            Clock.read().Month,
            Clock.read().Day
        };
        
        EEPROM.put(eeAddress+1, curr);
        eeAddress += sizeof(curr);
        EEPROM.write(12, eeAddress);
        delay(1100);
    }
    
}

//prints all data in EEPROM
void temp_HISTORY(){

    for (int i = 13; i < eeAddress; i+= 8){
        short printTemp = EEPROM[i] | (EEPROM[i+1] << 8);
        // Serial.print(F("Temp: ")); Serial.print( printTemp ); Serial.print(F(" *C ")); 
        // Serial.print(F("Humidity: ")); Serial.print( EEPROM[i+2] ); Serial.println(F(" RH%"));
        // Serial.print(F("On: ")); Serial.print(EEPROM[i+6]); Serial.print('/');
        // Serial.print(EEPROM[i+7]); Serial.print('/');
        // Serial.println(EEPROM[i+5]);
        // Serial.print(F("At: ")); Serial.print(EEPROM[i+3]); Serial.print(':');
        // Serial.println(EEPROM[i+4]);
        // Serial.println();

        Udp.beginPacket(ipRemote, remotePort);
        Udp.print(F("Temp: ")); Udp.print( printTemp ); Udp.print(F(" *C ")); 
        Udp.print(F("Humidity: ")); Udp.print( EEPROM[i+2] ); Udp.print(F(" RH% "));
        Udp.print(F("On: ")); Udp.print(EEPROM[i+6]); Udp.print('/');
        Udp.print(EEPROM[i+7]); Udp.print('/');
        Udp.print(EEPROM[i+5]);
        Udp.print(F(" At: ")); Udp.print(EEPROM[i+3]); Udp.print(':');
        Udp.print(EEPROM[i+4]);
        Udp.endPacket();
        packetSent++;
    }
}

//clears all data in EEPROM
void clearEEPROM(){
    for (int i = 13 ; i < EEPROM.length() ; i++) {
        EEPROM.write(i, 0);
    }
    eeAddress = 12;
    EEPROM[12] = 12;
}

//prints the current time
void current_TIME(){
    MyDateAndTime = Clock.read();
    // Serial.print(MyDateAndTime.Month); Serial.print('/');
    // Serial.print(MyDateAndTime.Day); Serial.print('/');
    // Serial.print(MyDateAndTime.Year); Serial.print(' ');
    // Serial.print(MyDateAndTime.Hour); Serial.print(':');
    // Serial.print(MyDateAndTime.Minute); Serial.print(':');
    // Serial.print(MyDateAndTime.Second); Serial.println('\n');

    Udp.beginPacket(ipRemote, remotePort);
    Udp.print(MyDateAndTime.Month); Udp.print('/');
    Udp.print(MyDateAndTime.Day); Udp.print('/');
    Udp.print(MyDateAndTime.Year); Udp.print(' ');
    Udp.print(MyDateAndTime.Hour); Udp.print(':');
    Udp.print(MyDateAndTime.Minute); Udp.print(':');
    Udp.print(MyDateAndTime.Second);
    Udp.endPacket();  
}

//set the current time
void set_TIME()
{
    MyDateAndTime.Month = tokenBuffer[2];
    MyDateAndTime.Day = tokenBuffer[3];
    MyDateAndTime.Year = tokenBuffer[4];
    MyDateAndTime.Hour = tokenBuffer[5];
    MyDateAndTime.Minute = tokenBuffer[6];
    MyDateAndTime.Second = tokenBuffer[7];
    Clock.write(MyDateAndTime);
}

//prints the highest and lowest temp from EEPROM data
void temp_HIGH_LOW(){
    int high = 0;
    int low = 1000;
    byte j = 13;
    for(byte i = 1; i < eeAddress/8; i++){
        if(EEPROM[j] > high){
            high = EEPROM[j];
        }
        if(EEPROM[j] < low){
            low = EEPROM[j];
        }
        j += 8;
    }
    Serial.print(F("HIGHEST TEMP: ")); Serial.print(high); Serial.println(F(" *C "));
    Serial.print(F("LOWEST TEMP: ")); Serial.print(low); Serial.println(F(" *C "));

    Udp.beginPacket(ipRemote, remotePort);
    Udp.print(F("HIGHEST TEMP: ")); Udp.print(high); Udp.print(F(" *C "));
    Udp.print(F("LOWEST TEMP: ")); Udp.print(low); Udp.print(F(" *C "));
    Udp.endPacket();  

}

//turns RGB on
void RGB_ON(){
    leds[0].red = userRED;
    leds[0].green = userGREEN;
    leds[0].blue = userBLUE;
    RGB_status = 1;
    FastLED.show();
}

//turns RGB off
void RGB_OFF(){
    leds[0] = CRGB::Black;
    RGB_status = 0;
    FastLED.show();
}

//changes the color of the RGB
void change_RGB(){
    userGREEN = tokenBuffer[2];;
    userRED = tokenBuffer[3];
    userBLUE = tokenBuffer[4];
    leds[0].green = tokenBuffer[2];
    leds[0].red = tokenBuffer[3];
    leds[0].blue = tokenBuffer[4];
    RGB_status = 1;
    FastLED.show();
}


//blinks the RGB
void RGB_BLINK(){
    static long int timer = 0;
    if((timer<millis() && RGB_goBlink) || RGBForce == true){
        
        RGBForce = false;
        timer= millis()+interval;
        if (RGB_status){
            
            RGB_status = 0;
            RGB_OFF();
        }
        else{
            RGB_status = 1;
            RGB_ON();
        }
    }
}

//clears the token buffer
void clearTokenBuffer(){
    tokenBuffer[0] = 0;
    tokenBuffer[1] = 0;
    tokenBuffer[2] = 0;
    tokenBuffer[3] = 0;
    tokenBuffer[4] = 0;
    tokenBuffer[5] = 0;
    tokenBuffer[6] = 0;
    tokenBuffer[7] = 0;
    tokenBuffer[8] = 0;

}

//shows error
void showError(){
    Serial.println(F("*INVALID COMMAND"));
    Udp.beginPacket(ipRemote, remotePort);
    Udp.print(F("*INVALID COMMAND"));
    Udp.endPacket();
}

//clears the packetbuffer
void clearPacketBuffer(char clearing[30]){
    for (byte i = 0; i < 30; i++){
        clearing[i] = '\0';
    }
}

//my strcpy
void myStrcpy(char og[30], char str[30]){
    byte i = 0;
    for (i = 0; str[i] != '\0'; i++){
        og[i] = str[i];
    }
    og[i] = '\0';
}

//returns the current threshold 
byte tempThresholdState(){
    short faren = celsiusToFarenheit(currTemp);
    if (faren <= majUnd){
        return 5;
    }
    else if(faren >= lowMinUnd && faren <= highMinUnd){
        return 6;
    }
    else if(faren >= lowCom && faren <= highCom){
        return 7;
    }
    else if(faren >= lowMinOve && faren <= highMinOve){
        return 8;
    }
    else if(faren >= majOve){
        return 9;
    }

}

//sends single alarm packet for each threshold change
void sendAlarmPacket(byte tempState){
    if (tempState != alarm){
        Udp.beginPacket(ipRemote, remotePort);
        if (tempState == 5){
            alarm = 5;
            Udp.print(F("Major Under"));
            leds[2].green = 255;
            leds[2].red = 0;
            leds[2].blue = 255;
        }
        else if(tempState == 6){
            alarm = 6;
            Udp.print(F("Minor Under"));
            leds[2] = CRGB::Blue;
        }
        else if(tempState == 7){
            alarm = 7;
            Udp.print(F("Comfortable"));
            leds[2] = CRGB::Red; //actually green
        }
        else if(tempState == 8){
            alarm = 8;
            Udp.print(F("Minor Over"));
            leds[2].green = 255;
            leds[2].red = 75;
            leds[2].blue = 0;
        }
        else if(tempState == 9){
            alarm = 9;
            Udp.print(F("Major Over"));
            leds[2] = CRGB::Green; //actually red
        }
        packetSent++;
        Udp.endPacket();
        FastLED.show();
    }
}


//takes in udp packets and add them to the string buffer
void receivePackets(){
    int packetSize = Udp.parsePacket();
    if (packetSize) {
        receivedPacket = true;
        // read the packet into packetBufffer
        Udp.read(packetBuffer, 30);
        Serial.print(packetBuffer);
        Serial.print(F(" *PACKET RECEIVED"));
        myStrcpy(buffer, packetBuffer);
        // send a reply to the IP address and port that sent us the packet we received
    }
}

//my strncat function
void myStrncat(char dest[], char src[], byte length)
{
    byte i = 0;
    byte len = myStrlen(dest);
    for(i=0; i < length; i++)
    {
        dest[len+i] = src[i];
        dest[len+i+1] = 0;
    }
}

//changes menu from home
int changeMenu(){
    static long int timer = 0;
    int button = readButtons();
    if (timer < millis()){
        if (button){ 
            button = readButtons();
            menu = button;
            timer = millis()+500;
        }
        else{
            timer = millis()+100;
        }
    }
}
//commands contoroller for when inside menus
int changeCommand(){
    static long int timer = 0;
    int button = readButtons();
    if (timer < millis()){
        if (button){ 
            button = readButtons();
            command = button;
            timer = millis()+500;
        }
        else{
            timer = millis()+100;
        }
    }
}

//home/starting menu
void homeMenu(){
    lcd.clear();
    lcd.print(F("Temp: "));
    lcd.print(currTemp);
    lcd.print(F(" *C "));
    lcd.setCursor(0,1);
    lcd.print(F("Humid: "));
    lcd.print(currHumid);
    lcd.print(F(" RH%"));
    menu = 0;
}

//history menu
void historyMenu(byte log){
    lcd.clear();
    //int sec = 0;
    short printTemp = EEPROM[log] | (EEPROM[log+1] << 8);
    lcd.setCursor(0, 0);
    lcd.print(printTemp); lcd.print(F(" *C ")); lcd.print(short(EEPROM[log+2])); lcd.print(F(" RH% "));
    lcd.setCursor(0, 1);
    lcd.print(EEPROM[log+6]); lcd.print('/');
    lcd.print(EEPROM[log+7]); lcd.print('/');
    lcd.print(EEPROM[log+5]); lcd.print(' ');
    lcd.print(EEPROM[log+3]); lcd.print(':');
    lcd.print(EEPROM[log+4]); 
    //sec += 14;
    
    menu = 0;
}

//stats menu
void statsMenu(byte sent, byte received){
    lcd.clear();
    lcd.print("Sent: ");
    lcd.print(sent);
    lcd.setCursor(0,1);
    lcd.print("Received: ");
    lcd.print(received);
    menu = 0;
}

//settings menu
void settingsMenu(){
    lcd.clear();
    lcd.print(F(" ^ IP/Subnet/Gateway"));
    lcd.setCursor(0,1);
    lcd.print(F(" < Erase > Temp Thresholds"));
    menu = 0;
}

//menu to confirm erasing history
void eraseHistoryMenu(){
    lcd.clear();
    lcd.print(F("Are You Sure?"));
    lcd.setCursor(0,1);
    lcd.print(F("Yes(->) No(Home)"));
    menu = 0;
}

//menu to change ip
void changeIPMenu(){
    lcd.clear();
    for (byte i = 0; i < 12; i++){
        lcd.print(EEPROM[i]);
        if (i == 3 || i == 7 || i == 11){
            lcd.print(' ');
        }
        else{
            lcd.print('.');
        }
    }
    menu = 0;
}

//menu to change temp thresholds
void tempThresholdMenu(){
    lcd.clear();
    lcd.print(F("<="));
    lcd.print(majUnd); lcd.print(' ');
    lcd.print(lowMinUnd); lcd.print('-'); lcd.print(highMinUnd); lcd.print(' ');
    lcd.print(lowCom); lcd.print('-'); lcd.print(highCom); lcd.print(' ');
    lcd.print(lowMinOve); lcd.print('-'); lcd.print(highMinOve); lcd.print(' ');
    lcd.print(F(">=")); lcd.print(majOve); 
    menu = 0;
}
//changes just the 100 digit place
int replace100(int old, int dig){
    int ret = old - old%1000 + old%100 + dig*100;
    return ret;
}
//changes just the 10 digit place
int replace10(int old, int dig){
    int ret = old - old%100 + old%10 + dig*10;
    return ret;
}
//changes just the 1 digit place
int replace1(int old, int dig){
    int ret = old - old%10 + old%1 + dig*1;
    return ret;
}

//for menu navigation
byte cursorI = 6;
byte cycle = 0;
byte history = 13;


//main loop
void loop(){
    current_TEMP();
    five_SECOND_TEMP();
    write_TO_EEPROM();
    RGB_BLINK();
    led_BLINK2(); //start LED blinking
    led_ALTERNATE(); //start LED ALTERNATEping colors
    //menuSystem();
    if (currentMenu == 5){ //home menu
        changeMenu();
        if (menu == 5){
            homeMenu();
            cursorI = 6;
        }
        else if (menu == 1){
            historyMenu(13);
            currentMenu = 1;
        }
        else if (menu == 2){
            statsMenu(packetSent, packetReceived);
            currentMenu = 2;
        }
        else if (menu == 3){
            settingsMenu();
            currentMenu = 3;
        }
    }
    if (currentMenu == 1){ //history menu
        changeCommand();
        lcd.noCursor();
        if (command == 1){
            //lcd.scrollDisplayRight();
        }
        else if (command == 4){
            history += 8;
            if (history > eeAddress){
                history = 13;
            }
            historyMenu(history);
            //lcd.scrollDisplayLeft();
        }
        else if (command == 5){
            currentMenu = 5;
            
        }
        command = 0;
    }
    if (currentMenu == 2){ //stats menu
        changeCommand();
        lcd.noCursor();
        if (command == 1){
            lcd.scrollDisplayRight();
        }
        else if (command == 4){
            lcd.scrollDisplayLeft();
        }
        else if (command == 5){
            currentMenu = 5;
            
        }
        command = 0;
    }
    if (currentMenu == 3){ //settings menu
        changeCommand();
        lcd.noCursor();
        if (command == 4){
            //temp threshold
            currentMenu = 8;
            tempThresholdMenu();
        }
        else if (command == 2){
            //change ip
            currentMenu = 7;
            changeIPMenu();

        }
        else if (command == 1){
            //erase history
            currentMenu = 6;
            eraseHistoryMenu();
        }
        else if (command == 3){
            lcd.scrollDisplayLeft();
        }
        else if (command == 5){
            currentMenu = 5; 
        }
        command = 0;
    }
    if (currentMenu == 6){ //erase confirm menu
        changeCommand();
        if (command == 4){
            clearEEPROM();
            currentMenu = 5;
            menu = 5;
        }
        else if (command == 5){
            currentMenu = 5;
        }
        command = 0;
    }
    if (currentMenu == 7){ //change ip menu
        changeCommand();
        
        if (command == 1){
            lcd.scrollDisplayRight();
            if (cursorI == 0){
                cursorI = 40;
            }
            cursorI--;
            lcd.setCursor(cursorI, 1);
            cycle = 1;
            
        }
        else if (command == 2){
            lcd.noCursor();
            lcd.setCursor(cursorI, 0);
            lcd.print(cycle);
            if (cursorI == 0){EEPROM[0] = replace100(EEPROM[0], cycle);}
            else if (cursorI == 1){EEPROM[0] = replace10(EEPROM[0], cycle);}
            else if (cursorI == 2){EEPROM[0] = replace1(EEPROM[0], cycle);}
        
            else if (cursorI == 4){EEPROM[1] = replace100(EEPROM[1], cycle);}
            else if (cursorI == 5){EEPROM[1] = replace10(EEPROM[1], cycle);}
            else if (cursorI == 6){EEPROM[1] = replace1(EEPROM[1], cycle);}
            
            else if (cursorI == 8){EEPROM[2] = replace1(EEPROM[2], cycle);}

            else if (cursorI == 10){EEPROM[3] = replace100(EEPROM[3], cycle);}
            else if (cursorI == 11){EEPROM[3] = replace10(EEPROM[3], cycle);}
            else if (cursorI == 12){EEPROM[3] = replace1(EEPROM[3], cycle);}
            
            cycle++;
            if (cycle == 10){
                cycle = 0;
            }
        }
        else if (command == 3){
            lcd.setCursor(cursorI, 1);
            lcd.cursor();
            cycle = 0;
        }
        else if (command == 4){
            lcd.scrollDisplayLeft();
            if (cursorI == 40){
                cursorI = 0;
            }
            cursorI++;
            lcd.setCursor(cursorI, 1);
            cycle = 0;
        }
        
        else if (command == 5){
            currentMenu = 5;
        }
        command = 0;
    }
    if (currentMenu == 8){ //change temp thresholds
        changeCommand();
        if (command == 1){
            lcd.scrollDisplayRight();
            if (cursorI == 0){
                cursorI = 40;
            }
            cursorI--;
            lcd.setCursor(cursorI, 1);
            cycle = 1;
        }
        else if (command == 2){
            lcd.noCursor();
            lcd.setCursor(cursorI, 0);
            lcd.print(cycle);
            if (cursorI == 2){majUnd = replace10(majUnd, cycle);}
            else if (cursorI == 3){majUnd = replace1(majUnd, cycle);}
        
            else if (cursorI == 5){lowMinUnd = replace10(lowMinUnd, cycle);}
            else if (cursorI == 6){lowMinUnd = replace1(lowMinUnd, cycle);} 
            else if (cursorI == 8){highMinUnd = replace10(highMinUnd, cycle);}
            else if (cursorI == 9){highMinUnd = replace1(highMinUnd, cycle);} 

        
            else if (cursorI == 11){lowCom = replace10(lowCom, cycle);}
            else if (cursorI == 12){lowCom = replace1(lowCom, cycle);} 
            else if (cursorI == 14){highCom = replace10(highCom, cycle);}
            else if (cursorI == 15){highCom = replace1(highCom, cycle);} 
            
        
            else if (cursorI == 17){lowMinOve = replace10(lowMinOve, cycle);}
            else if (cursorI == 18){lowMinOve = replace1(lowMinOve, cycle);} 
            else if (cursorI == 20){highMinOve = replace10(highMinOve, cycle);}
            else if (cursorI == 21){highMinOve = replace1(highMinOve, cycle);} 
            
        
            else if (cursorI == 25){majOve = replace10(majOve, cycle);}
            else if (cursorI == 26){majOve = replace1(majOve, cycle);}
            
            cycle++;
            if (cycle == 10){
                cycle = 0;
            }
        }

        else if (command == 3){
            lcd.setCursor(cursorI, 1);
            lcd.cursor();
            cycle = 0;
        }
        else if (command == 4){
            lcd.scrollDisplayLeft();
            if (cursorI == 40){
                cursorI = 0;
            }
            cursorI++;
            lcd.setCursor(cursorI, 1);
            cycle = 0;
        }
        else if (command == 5){
            currentMenu = 5;
        }
        command = 0;
    }

    char c = '\0'; //temp value for each character the user enters
    currentMillis = millis(); //sets current time to how much time has passed in the program so far
    
    if (receivedPacket){
        c = 13;
        receivedPacket = false;
    }
    
    // Check for Ethernet hardware present
    if (Ethernet.hardwareStatus() == EthernetNoHardware || Ethernet.linkStatus() == LinkOFF) {
        leds[1] = CRGB::Green;
        FastLED.show();
    }
    else{
        leds[1] = CRGB::Red;
        FastLED.show();
        receivePackets();
        //alarmPacket();
        sendAlarmPacket(tempThresholdState());

        
    }


    //takes in user input and adds it to the buffer
    while (Serial.available()){
        c = toupper(Serial.read());
        myStrncat(buffer, &c, 1);
        
        if (c == 127){ //Handle backspace on terminal
            Serial.print("\033[D");
            Serial.print(" ");
            Serial.print("\033[D");
            if (buffer != NULL){
                byte length = myStrlen(buffer);
                if ((length > 0) && (buffer[length-1] == 127)) {
                    buffer[length-1] = '\0';
                    buffer[length-2] = '\0';
                }
            }   
        
        }
        Serial.print(c);
    }

    
    //if 'enter' is clicked, checks what user typed and applies to arduino
    if (c == 13){
        Serial.println();
        //parses the string the user entered
        char right = '>';
        myStrncat(statsBuffer, &right, 1);
        packetReceived++;
        for(byte i = 0; i < myStrlen(buffer); i++){
            myStrncat(statsBuffer, &buffer[i], 1);
            
            if(!isspace(buffer[i])){
                parsedString[wordNum][charNum] = buffer[i];
                charNum++; 
            }
            else if (!isspace(buffer[i-1])){
                parsedString[wordNum][charNum] = '\0';
                wordNum++;
                charNum = 0;
            }
        }
        
        

        fillTokenBuffer(parsedString, definesArr); //filling token buffer based off parsed string
        
        memset(buffer, 0, sizeof(buffer)); //terminates buffer 
        
        //cheking token buffer and applying to arduino
        if (currentMillis - msStart > 500)
        {
            showTemp = false;
            switch (tokenBuffer[0]){
                    break;
                    case t_LED:
                        switch (tokenBuffer[1])
                        {
                            case t_GREEN:
                                switch (tokenBuffer[2]){
                                    case t_EOL:
                                        led_goALTERNATE = false;
                                        led_goBlink = false;
                                        led_GREEN();
                                    break;
                                    default:
                                        showError();
                                    break;
                                }
                            break;
                            case t_RED:
                                switch (tokenBuffer[2]){
                                    case t_EOL:
                                        led_goALTERNATE = false;
                                        led_goBlink = false;
                                        led_RED();
                                    break;
                                    default:
                                        showError();
                                    break;
                                }
                            break;
                            case t_OFF:
                                switch (tokenBuffer[2]){
                                    case t_EOL:
                                        led_goALTERNATE = false;
                                        led_goBlink = false;
                                        led_OFF();
                                    break;
                                    default:
                                        showError();
                                    break;
                                }
                                
                            break;
                            case t_BLINK:
                                switch (tokenBuffer[2]){
                                    case t_EOL:                                     
                                        led_goALTERNATE = false;
                                        led_goBlink = true;
                                        led_BLINK2();
                                    break;
                                    default:
                                        showError();
                                    break;
                                }
                            break;
                            case t_ALTERNATE:
                                switch (tokenBuffer[2]){
                                    case t_EOL:
                                        led_goBlink = false;
                                        led_goALTERNATE = true;
                                        led_ALTERNATE();
                                    break;
                                    default:
                                        showError();
                                    break;
                                }
                                
                            break;
                            default:
                                showError();
                            break;

                        }
                    break;
                    case t_STATUS:
                        switch (tokenBuffer[1])
                        {
                            case t_LEDS:
                                switch (tokenBuffer[2]){
                                        case t_EOL:
                                           status_LEDS(); 
                                        break;
                                        default:
                                            showError();
                                        break;
                                }
                            break;
                            default:
                                showError();
                            break;
                        }
                    break;
                    case t_HELP:
                        switch (tokenBuffer[1]){
                                case t_EOL:
                                    // Serial.println(F("The available commands are: "));
                                    // Serial.println(F("->LED GREEN"));
                                    // Serial.println(F("->LED RED"));
                                    // Serial.println(F("->LED OFF"));
                                    // Serial.println(F("->LED BLINK"));
                                    // Serial.println(F("->LED ALTERNATE"));
                                    // Serial.println(F("->SET BLINK <number>"));
                                    // Serial.println(F("->STATUS LEDS"));
                                    // Serial.println(F("->CURRENT TIME"));
                                    // Serial.println(F("->SET TIME <month> <day> <year> <hour> <minute> <second>"));
                                    // Serial.println(F("->CURRENT TEMP"));
                                    // Serial.println(F("->TEMP HISTORY"));
                                    // Serial.println(F("->TEMP HIGH LOW"));
                                    // Serial.println(F("->ADD <number1> <number2>"));
                                    // Serial.println(F("->RGB <red> <green> <blue>"));
                                    // Serial.println(F("->RGB ON"));
                                    // Serial.println(F("->RGB OFF"));
                                    // Serial.println(F("->RGB BLINK"));
                                    // Serial.println(F("->VERSION"));
                                    // Serial.println(F("->HELP"));
                                    // Serial.println();

                                    // Udp.beginPacket(ipRemote, remotePort);
                                    // Udp.print(F("The available commands are: "));
                                    // Udp.print(F("->LED GREEN"));
                                    // Udp.print(F("->LED RED"));
                                    // Udp.print(F("->LED OFF"));
                                    // Udp.print(F("->LED BLINK"));
                                    // Udp.print(F("->LED ALTERNATE"));
                                    // Udp.print(F("->BLINK <number>"));
                                    // Udp.print(F("->STATUS LEDS"));
                                    // Udp.print(F("->CURRENT TIME"));
                                    // Udp.print(F("->SET TIME <month> <day> <year> <hour> <minute> <second>"));
                                    // Udp.print(F("->CURRENT TEMP"));
                                    // Udp.print(F("->TEMP HISTORY"));
                                    // Udp.print(F("->TEMP HIGH LOW"));
                                    // Udp.print(F("->ADD <number1> <number2>"));
                                    // Udp.print(F("->RGB <red> <green> <blue>"));
                                    // Udp.print(F("->RGB ON"));
                                    // Udp.print(F("->RGB OFF"));
                                    // Udp.print(F("->RGB BLINK"));
                                    // Udp.print(F("->VERSION"));
                                    // Udp.print(F("->HELP"));
                                    // Udp.endPacket();

                                break;
                                default:
                                    showError();
                                break;
                                
                        }
                    break;
                    case t_VERSION:
                        switch (tokenBuffer[1]){
                                case t_EOL:
                                    // Serial.print(F("Version: "));
                                    // Serial.print(version[0]);
                                    // Serial.print('.');
                                    // Serial.print(version[1]);
                                    // Serial.print('.');
                                    // Serial.println(version[2]);
                                    Udp.beginPacket(ipRemote, remotePort);
                                    Udp.print(F("Version: "));
                                    Udp.print(version[0]);
                                    Udp.print('.');
                                    Udp.print(version[1]);
                                    Udp.print('.');
                                    Udp.print(version[2]);
                                    Udp.endPacket();
                                break;
                                default:
                                    showError();
                                break;
                        }
                    break;

                    case t_SET:
                        switch (tokenBuffer[1])
                        {
                            case t_BLINK:
                                switch (tokenBuffer[2])
                                {
                                    case t_WORD:

                                        switch (tokenBuffer[5]){
                                            case t_EOL:
                                                if(led_goBlink == true){
                                                    ledForce = true;
                                                }
                                                if(RGB_goBlink){
                                                    RGBForce = true;
                                                }
                                                applyUserInterval();
                                            break;
                                            default:
                                                showError();
                                            break;
                                            
                                        }
                                    break;
                                    default:
                                        showError();
                                    break;
                                }
                            break;
                            case t_TIME:
                                switch (tokenBuffer[8]){
                                    case t_EOL:
                                        set_TIME();
                                    break;
                                    default:
                                        showError();
                                    break;
                                }
                                
                            break;
                            default:
                                showError();
                            break;
                        }
                        
                    break;
                    

                    case t_CURRENT:
                        switch (tokenBuffer[1]){
                            case t_TEMP:
                                switch(tokenBuffer[2]){
                                    case t_EOL:
                                        showTemp = true;
                                    break;
                                    default:
                                        showError();
                                    break;
                                }
                            break;
                            case t_TIME:
                                switch(tokenBuffer[2]){
                                    case t_EOL:
                                        current_TIME();
                                    break;
                                    default:
                                        showError();
                                    break;
                                }
                            break;
                            default:
                                showError();
                            break;
                        }
                    break;

                    case t_ADD:
                        switch(tokenBuffer[5]){
                            case t_EOL:
                                ADD();
                            break;
                            default:
                                showError();
                            break;
                        }
                    break;
                    case t_RGB:
                        switch (tokenBuffer[1])
                        {
                            case t_ON:
                                switch (tokenBuffer[2]){
                                    case t_EOL:
                                        RGB_goBlink = false;
                                        RGB_ON();
                                    break;
                                    default:
                                        showError();
                                    break;
                                }
                                
                            break;
                            case t_OFF:
                                switch (tokenBuffer[2]){
                                    case t_EOL:
                                        RGB_goBlink = false;
                                        RGB_OFF();
                                    break;
                                    default:
                                        showError();
                                    break;
                                }    
                            break;
                            case t_WORD:
                                switch (tokenBuffer[5]){
                                    case t_EOL:
                                        RGB_goBlink = false;
                                        change_RGB();
                                    break;
                                    default:
                                        showError();
                                    break;
                                }
                            break;
                            case t_BLINK:
                                switch (tokenBuffer[2]){
                                    case t_EOL:
                                        RGB_goBlink = true;
                                        RGB_BLINK();
                                    break;
                                    default:
                                        showError();
                                    break;
                                }
                            break;
                            default:
                                showError();
                            break; 

                            
                                     
                        }
                        break;
                        default:
                            showError();
                        break; 
                    
                    case t_TEMP:
                        switch (tokenBuffer[1]){
                            case t_HISTORY:
                                switch (tokenBuffer[2]){
                                    case t_EOL:
                                        temp_HISTORY();
                                        if (EEPROM[12] == 12){
                                            Serial.println(F("*NO DATA YET"));
                                        }
                                    break;
                                    default:
                                        showError();
                                    break;
                                }
                            break;
                            case t_HIGH:
                                switch (tokenBuffer[2]){
                                    case t_LOW:
                                        switch (tokenBuffer[3]){
                                            case t_EOL:
                                                temp_HIGH_LOW();
                                            break;
                                            default:
                                                showError();
                                            break;
                                        }
                                    break;
                                        default:
                                            showError();
                                        break;
 
                                }
                            break;
                            default:
                                showError();
                            break; 
                        }
                    break;
                    case t_CLEAR:
                        switch (tokenBuffer[1]){
                            case t_HISTORY:
                                switch (tokenBuffer[2]){
                                    case t_EOL:
                                        clearEEPROM();
                                        
                                    break;
                                    default:
                                        showError();
                                    break;
                                }
                            break;
                            default:
                                showError();
                            break;
                        }
                    break;                    
            }
        }
        
        clearPacketBuffer(packetBuffer);
        
        msStart = currentMillis; //for looping
        clearParsedString(parsedString); //clearing the parsed string for next input
        clearTokenBuffer(); // clearing the token buffer
        


    }
}


    







