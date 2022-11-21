#include <Arduino.h>
#include <SimpleDHT.h>
#include <DS3231_Simple.h>
#include <Wire.h>
#include <FastLED.h>
#include <EEPROM.h>

#include "led_controller.h"
#include "clock.h"


#define def_COUNT 23

#define NUM_LEDS 1

int eeAddress = 0;

CRGB leds[NUM_LEDS];
int userRED = 255;
int userGREEN = 255;
int userBLUE = 255;

struct TimeStamp {
    byte temperature;
    byte humidity;
    byte hour;
    byte minute;
    byte year;
    byte month;
    byte day;
};

byte currTemp;
byte currHumid;



#define MAX_ARGS 6 //max arguments for token buffer
#define MAX_BUF 30 // max length of string buffer

static const uint8_t version[3] = {1,0,0}; //program version

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
    {'E','O',3,char(t_EOL)}
};

unsigned long msStart = 0; //time of starting the program
unsigned long currentMillis; //current time elapsed since starting program
unsigned long msPrev = 0; //last time something was ran in proram


unsigned long interval = 500; //blink interval time 
static bool d13_status = 0; //whether or not D13 is on or off
char led_status = 'O'; //status of current led, 'O'->OFF, 'R'->RED, 'G'->GREEN
char led_Previous = 'O'; //status of last color led input, 'O'->OFF, 'R'->RED, 'G'->GREEN
static bool d13_goBlink = false; //blink booolen to tell D13 to blink or not
static bool led_goBlink = false; //blink booolen to tell LED to blink or not
static bool led_goALTERNATE = false; //alternates led colors if true
static bool ledForce = false;
static bool d13Force = false;

int RGB_brightness = 30;
static bool RGB_status = 0;
static bool RGB_goBlink = false;
static bool RGBForce = false;

uint8_t hiByte; //upper half of user input for set blink
uint8_t loByte; //lower half of user input for set blink
unsigned long combinedVal = 500; //hiByte and loByte together (hiByte + loByte)

static bool prompt = true; //determines if main loop is being run for the first time

long addedNum = 0;

int pinDHT22 = 2;
SimpleDHT22 dht22(pinDHT22);
static bool setTIME = false;
DS3231_Simple Clock;


//checks whether or not a string contains only numbers
bool isValidNumber(char str[5]){
    //bool ret = 0;
    for(byte i=0;i<5;i++)
    {
        if(isDigit(str[i])) return true;
    }
    return false;
}




//setup function
void setup(){  
    led_setup();
    Serial.begin(9600);
    Clock.begin();
    FastLED.addLeds<WS2812B,7>(leds, NUM_LEDS);
    FastLED.setBrightness(RGB_brightness);
    leds[0] = CRGB::Black;
    FastLED.show();
    Serial.println();
}

//initial prompt when program is run
void introPrompt(){
    Serial.println(F(">Welcome to Arduino Command Line Interpreter, enter 'HELP' for list of commands!\n"));
}

//clears the parsed string for next set of input
void clearParsedString(char arr[MAX_ARGS][MAX_BUF]){
  for(uint8_t i = 0; i <=MAX_ARGS; i++){
    for(uint8_t j=0; j<=MAX_BUF;j++){
      arr[i][j] = '\0';
    }
  }
  return;
}


//takes parsed string and definitions array and fills the token buffer based on the input
void fillTokenBuffer(char parsedStr[MAX_ARGS][MAX_BUF], char defsArr[def_COUNT][4]){
    bool missingWord = false;
    for (byte i = 0; i < MAX_ARGS; i++){
        for (byte j = 0; j < def_COUNT; j++){
            if ((parsedStr[i][0] == defsArr[j][0])
            && (parsedStr[i][1] == defsArr[j][1])
            && (strlen(parsedStr[i]) == defsArr[j][2])){
                tokenBuffer[i] = defsArr[j][3];
                tokenBuffer[i+1] = t_EOL;               
            }

            else if(isDigit(parsedStr[i][0]) || (tokenBuffer[0] == t_ADD && parsedStr[i][0] == '-' && isDigit(parsedStr[i][1]))){
                if (tokenBuffer[0] == t_SET && tokenBuffer[1] == t_BLINK){
                    tokenBuffer[i] = t_WORD;
                    userBlinkLong = strtol(parsedStr[i], NULL, 0);
                    if (userBlinkLong <= 65535){
                        hiByte = (userBlinkLong >> 8);
                        loByte = (userBlinkLong);
                        tokenBuffer[3] = int(hiByte);
                        tokenBuffer[4] = int(loByte);
                        tokenBuffer[5] = t_EOL;

                    }
                }
                else if (tokenBuffer[0] == t_ADD){
                    addedNum = strtol(parsedStr[i], NULL, 0);
                    if (i == 1){
                        tokenBuffer[1] = int((addedNum >> 8));
                        tokenBuffer[2] = int((addedNum));
                    }
                    else if(i == 2){
                        tokenBuffer[3] = int((addedNum >> 8));
                        tokenBuffer[4] = int((addedNum)); 
                    }
                    tokenBuffer[5] = t_EOL;                    
                }

                else if (tokenBuffer[0] == t_RGB){
                    tokenBuffer[1] = t_WORD;
                    tokenBuffer[i+1] = atoi(parsedStr[i]);
                    tokenBuffer[i+2] = t_EOL;
                }

            }
        }
    }
    
}

//applies the number entered with 'SET BLINK' to the interval variable for blinking 
void applyUserInterval(){
    combinedVal = (long(tokenBuffer[3]<<8)|long(tokenBuffer[4]));
    if ((combinedVal >= 0) && (combinedVal <= 65535)){
        interval = combinedVal;
    }
}

void ADD(){
    long printValue = 0;
    printValue = (long(tokenBuffer[1]<<8)|long(tokenBuffer[2]));
    printValue += (long(tokenBuffer[3]<<8)|long(tokenBuffer[4]));
    Serial.print("EQUALS: ");
    Serial.println(printValue);
}

//turns D13 on
void d13_ON(){
    d13_status = 1;
    digitalWrite(13,d13_status);
}

//turns D13 off
void d13_OFF(){
    d13_status = 0;
    digitalWrite(13,d13_status);
}

//makes D13 blink
void d13_BLINK()
{
    static long int timer = 0;
    
    if((timer<millis() && d13_goBlink) || d13Force == true)
    {
        d13Force = false;
        d13_status= !d13_status;
        timer= millis()+interval;
        digitalWrite(13, d13_status);
        
    }
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

//makes the led blink based off of last color it was set to
void led_BLINK(){
    static long int ledTimer = 0;
    if ((ledTimer < millis() && led_goBlink) || ledForce == true){
        if ((led_Previous == 'O' || led_Previous == 'R') && led_status == 'O'){
            led_Previous = 'R';
            led_status = 'R';
            ledTimer = millis()+interval;
            led_RED();
            ledForce = false;
        }
        else if (led_Previous == 'G' && led_status == 'O'){
            led_status = 'G';
            ledTimer = millis()+interval;
            led_GREEN();
            ledForce = false;
        }
        else if (led_status == 'R' || led_status == 'G'){
            led_status = 'O';
            ledTimer = millis()+interval;
            led_OFF();
            ledForce = false;
        } 
    }
}

void led_BLINK2(){
    static long int ledTimer = 0;
    byte ledBlinkRed = 0b01110111;
    byte ledBlinkGreen = 0b10111011;
    for (byte i = 0; i < 8; i++){
        if (ledBlinkRed & 1 == 1){
            if(ledBlinkRed >> 1 & 1 == 1){
                led_OFF();
            }
            else{
                led_RED();
            }
        }
        ledBlinkRed = ledBlinkRed >> 2;
    }
    for (byte i = 0; i < 8; i++){
        if (ledBlinkGreen & 1 == 1){
            if(ledBlinkGreen >> 1 & 1 == 1){
                led_OFF();
            }
        }
        else{
            if(ledBlinkGreen >> 1 & 1 == 1){
                led_GREEN();
            }
        }
        ledBlinkGreen = ledBlinkGreen >> 2;
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
    Serial.print(F("> D13 "));
    if (d13_goBlink){
        Serial.println(F("BLINKING"));
    }
    else if(!d13_goBlink){
        if(d13_status){
            Serial.println(F("ON"));
        }
        else{
            Serial.println(F("OFF"));
        }
    }
    Serial.print(F("> LED "));
    if (led_goBlink){
        Serial.print(F("BLINKING "));
        if(led_Previous == 'R'){
            Serial.println(F("RED"));
        }
        else{
            Serial.println(F("GREEN"));
        }
    }
    else if(!led_goBlink){
        if(led_Previous == 'R'){
            if (led_status == 'R'){
                Serial.println(F("RED"));
            }
            else{
                 Serial.println(F("OFF"));
            }
        }
        else if (led_Previous == 'G'){
            if (led_status == 'G'){
                Serial.println(F("GREEN"));
            }
            else{
                 Serial.println(F("OFF"));
            }
        }
        else{
            Serial.println(F("OFF"));
        }
    }
    
    Serial.print("> BLINK INTERVAL SET TO: ");
    Serial.println(interval);
    
}

void current_TEMP(){
    byte temperature = 0;
    byte humidity = 0;
    int err = SimpleDHTErrSuccess;
    if ((err = dht22.read(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
        Serial.print("Read DHT22 failed, err=");
        Serial.print(SimpleDHTErrCode(err));
        Serial.print(","); 
        Serial.println(SimpleDHTErrDuration(err));
    }
    Serial.print("Sample OK: ");
    Serial.print((int)temperature); Serial.print(" *C, ");
    Serial.print((int)humidity); Serial.println(" RH%");
}

void five_SECOND_TEMP(){
    static long int timer = 0;

    if (timer<millis()){
        dht22.read(&currTemp, &currHumid, NULL);
        // Serial.print((int)currTemp); Serial.print(" *C, ");
        // Serial.print((int)currHumid); Serial.println(" RH%");
        timer= millis()+5000;
    }
}

void write_TO_EEPROM(){
    static long int timer = 0;
    if (timer<millis()){
        TimeStamp curr = {
            int(currTemp),
            int(currHumid),
            Clock.read().Hour,
            Clock.read().Minute,
            Clock.read().Year,
            Clock.read().Month,
            Clock.read().Day
        };
        
        EEPROM.put(eeAddress, curr);
        eeAddress += sizeof(curr);
        timer=millis()+900000;
    }
}

void temp_HISTORY(){
    for (int i = 0; i < eeAddress; i+= 7){
        Serial.print("Temp: "); Serial.print( EEPROM[i] ); Serial.print(" *C "); 
        Serial.print("Humidity: "); Serial.print( EEPROM[i+1] ); Serial.println(" RH%");
        Serial.print("On: "); Serial.print(EEPROM[i+5]); Serial.print('/');
        Serial.print(EEPROM[i+6]); Serial.print('/');
        Serial.println(EEPROM[i+4]);
        Serial.print("At: "); Serial.print(EEPROM[i+2]); Serial.print(':');
        Serial.println(EEPROM[i+3]);
        Serial.println();
    }
}

void temp_HIGH_LOW(){
    byte high = 0;
    byte low = 1000;
    for(byte i = 0; i < eeAddress/7; i++){
        if(EEPROM[i] > high){
            high = EEPROM[i];
        }
        if(EEPROM[i] < low){
            low = EEPROM[i];
        }
    }
    Serial.print("HIGHEST TEMP: "); Serial.print(high); Serial.println(" *C ");
    Serial.print("LOWEST TEMP: "); Serial.print(low); Serial.println(" *C ");

}

void RGB_ON(){
    leds[0].red = userRED;
    leds[0].green = userGREEN;
    leds[0].blue = userBLUE;
    FastLED.show();
}

void RGB_OFF(){
    leds[0] = CRGB::Black;
    FastLED.show();
}

void change_RGB(){
    userGREEN = tokenBuffer[2];;
    userRED = tokenBuffer[3];
    userBLUE = tokenBuffer[4];
    leds[0].green = tokenBuffer[2];
    leds[0].red = tokenBuffer[3];
    leds[0].blue = tokenBuffer[4];
    FastLED.show();
}

void RGB_BLINK(){
    static long int timer = 0;
    //Serial.println("calling");
    if((timer<millis() && RGB_goBlink) || RGBForce == true){

        RGBForce = false;
        RGB_status= !RGB_status;
        timer= millis()+interval;
        if (RGB_status){
            RGB_OFF();
        }
        else{
            RGB_ON();
        }
    }
}




void showError(){
    Serial.println(F("*INVALID COMMAND"));
}

//main loop
void loop(){
    //first prompt
    if(prompt){
        introPrompt();
        prompt = false;
    }
    // leds[0] = CRGB(50, 100, 150);
    // FastLED.show();
    five_SECOND_TEMP();
    write_TO_EEPROM();
    RGB_BLINK();
    d13_BLINK(); //starts D13 blinking
    led_BLINK(); //start LED blinking
    led_ALTERNATE(); //start LED ALTERNATEping colors
    char c = '\0'; //temp value for each character the user enters
    currentMillis = millis(); //sets current time to how much time has passed in the program so far

    //takes in user input and adds it to the buffer
    while (Serial.available()){
        c = toupper(Serial.read());
        strncat(buffer, &c, 1);
        if (c == 127){ //Handle backspace on terminal
            Serial.print("\033[D");
            Serial.print(" ");
            Serial.print("\033[D");
            if (buffer != NULL){
                const unsigned int length = strlen(buffer);
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
        Serial.print("\n");
        led_BLINK2();
        //parses the string the user entered
        for(int i = 0; i < strlen(buffer); i++){
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
        
        //buffer[0] = {'\0'}; 
        memset(buffer, 0, sizeof(buffer)); //terminates buffer 

        //cheking token buffer and applying to arduino
        if (currentMillis - msStart > 500)
        {
            switch (tokenBuffer[0]){
                    case t_D13:
                        switch (tokenBuffer[1])
                        {
                            case t_ON:
                                switch (tokenBuffer[2]){
                                    case t_EOL:
                                        d13_goBlink = false;
                                        d13_ON();
                                    break;
                                    default:
                                        showError();
                                    break;
                                }
                                
                            break;
                            case t_OFF:
                                switch (tokenBuffer[2]){
                                    case t_EOL:
                                        d13_goBlink = false;
                                        d13_OFF();
                                    break;
                                    default:
                                        showError();
                                    break;
                                }
                                
                            break;
                            case t_BLINK:
                                switch (tokenBuffer[2]){
                                    case t_EOL:
                                        d13_goBlink = true;
                                        d13_BLINK();
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
                                        led_BLINK();
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
                                    Serial.println(F("The available commands are: "));
                                    Serial.println(F(">D13 ON"));
                                    Serial.println(F(">D13 OFF"));
                                    Serial.println(F(">D13 BLINK"));
                                    Serial.println(F(">LED GREEN"));
                                    Serial.println(F(">LED RED"));
                                    Serial.println(F(">LED OFF"));
                                    Serial.println(F(">LED BLINK"));
                                    Serial.println(F(">LED ALTERNATE"));
                                    Serial.println(F(">SET BLINK <number>"));
                                    Serial.println(F(">STATUS LEDS"));
                                    Serial.println(F(">VERSION"));
                                    Serial.println(F(">HELP"));
                                    Serial.println();
                                break;
                                default:
                                    showError();
                                break;
                                
                        }
                    break;
                    case t_VERSION:
                        switch (tokenBuffer[1]){
                                case t_EOL:
                                    Serial.print(F("Version: "));
                                    Serial.print(version[0]);
                                    Serial.print('.');
                                    Serial.print(version[1]);
                                    Serial.print('.');
                                    Serial.println(version[2]);
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
                                                if(d13_goBlink == true){
                                                    d13Force = true;
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
                                switch (tokenBuffer[2]){
                                    case t_EOL:
                                        setTIME = true;
                                        set_TIME(Serial);
                                        //Clock.begin();
                                        //Clock.promptForTimeAndDate(Serial);
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
                                        current_TEMP();
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
                                        change_RGB();
                                    break;
                                    default:
                                        showError();
                                    break;
                                }
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
                                }
                            break;
                        }
                    break;

                            
                    // default:
                    //     if (!setTIME){
                    //         showError();
                    //     }
                    // break;



                    
            }
        }
        
        msStart = currentMillis; //for looping
        clearParsedString(parsedString); //clearing the parsed string for next input
        memset(tokenBuffer, 0, sizeof(tokenBuffer)); // clearing the token buffer

    }
}


    







