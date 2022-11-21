#include <Arduino.h>
#include <DS3231_Simple.h>

#include "clock.h"

void current_TIME(){

    DateTime MyDateAndTime = Clock.read();
    Serial.print("Hour: ");   Serial.println(MyDateAndTime.Hour);
    Serial.print("Minute: "); Serial.println(MyDateAndTime.Minute);
    Serial.print("Second: "); Serial.println(MyDateAndTime.Second);
    Serial.print("Year: ");   Serial.println(MyDateAndTime.Year);
    Serial.print("Month: ");  Serial.println(MyDateAndTime.Month);
    Serial.print("Day: ");    Serial.println(MyDateAndTime.Day);
    Serial.println();
}

void set_TIME(Stream &Serial)
{

    char buffer[3] = { 0 };
    DateTime MyDateAndTime;

    Serial.println(F("Clock is set when all data is entered and you send 'Y' to confirm."));
    do
    {
        memset(buffer, 0, sizeof(buffer));
        Serial.println();
        Serial.print(F("Enter Day of Month (2 digits, 01-31): "));
        while(!Serial.available()) ; // Wait until bytes
        Serial.readBytes(buffer, 2);
        while(Serial.available()) Serial.read(); 
        MyDateAndTime.Day = atoi(buffer[0] == '0' ? buffer+1 : buffer);

        memset(buffer, 0, sizeof(buffer));
        Serial.println();
        Serial.print(F("Enter Month (2 digits, 01-12): "));
        while(!Serial.available()) ; // Wait until bytes
        Serial.readBytes(buffer, 2);
        while(Serial.available()) Serial.read(); 
        MyDateAndTime.Month = atoi(buffer[0] == '0' ? buffer+1 : buffer);

        memset(buffer, 0, sizeof(buffer));
        Serial.println();
        Serial.print(F("Enter Year (2 digits, 00-99): "));
        while(!Serial.available()) ; // Wait until bytes
        Serial.readBytes(buffer, 2);
        while(Serial.available()) Serial.read(); 
        MyDateAndTime.Year = atoi(buffer[0] == '0' ? buffer+1 : buffer);

        memset(buffer, 0, sizeof(buffer));
        Serial.println();
        Serial.print(F("Enter Hour (2 digits, 24 hour clock, 00-23): "));
        while(!Serial.available()) ; // Wait until bytes
        Serial.readBytes(buffer, 2);
        while(Serial.available()) Serial.read(); 
        MyDateAndTime.Hour = atoi(buffer[0] == '0' ? buffer+1 : buffer);

        memset(buffer, 0, sizeof(buffer));
        Serial.println();
        Serial.print(F("Enter Minute (2 digits, 00-59): "));
        while(!Serial.available()) ; // Wait until bytes
        Serial.readBytes(buffer, 2);
        while(Serial.available()) Serial.read(); 
        MyDateAndTime.Minute = atoi(buffer[0] == '0' ? buffer+1 : buffer);
            
        memset(buffer, 0, sizeof(buffer));
        Serial.println();
        Serial.print(F("Enter Second (2 digits, 00-59): "));
        while(!Serial.available()) ; // Wait until bytes
        Serial.readBytes(buffer, 2);
        while(Serial.available()) Serial.read(); 
        MyDateAndTime.Second = atoi(buffer[0] == '0' ? buffer+1 : buffer);


        memset(buffer, 0, sizeof(buffer));
        Serial.println();
        Serial.println(F("Enter Day Of Week (1 digit, 1-7, arbitrarily 1 = Mon, 7 = Sun): "));    
        while(!Serial.available()) ; // Wait until bytes
        Serial.readBytes(buffer, 1);
        while(Serial.available()) Serial.read(); 
        MyDateAndTime.Dow = atoi(buffer);

        Serial.println();
        Serial.print(F("Entered Timestamp: ")); 
        Clock.printTo(Serial, MyDateAndTime);  
        Serial.println();
            
        while(!Serial.available()) ; // Wait until bytes
        Serial.readBytes(buffer, 1);
        while(Serial.available()) Serial.read();  
        Clock.write(MyDateAndTime); 
        break;
    } while(1);   
}