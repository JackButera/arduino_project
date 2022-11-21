# ![N|Solid](https://digitize-inc.com/images/home/logo150.webp)

## _Project 1 - Clear Box_
>> _Jack Butera_ 
![Build Status](https://travis-ci.org/joemccann/dillinger.svg?branch=master)
version 1.0.0


_Clear Box_ is the first project at Digitize's engineering bootcmap.
It purpose is to give the new developers a sense of what the work will be.
_Clear Box_ is a board that holds light and lets the user to control it with specific set of commands.

##### _Clear Box_ containing:
- Circuit Board
- Arduino IDE
- LED

## How is it working
* _Clear Box_ has a uniqe set of commands that holds up to three values:
<Value1> <Value2> <Value3>
* The program will react according to the first value and then the 2nd one.
* Once the user enters the right values, the system reads it and executes.
#### Set of Commands:

| &darr;1stValue / &rarr;2ndValue | ON |  OFF | BLINK | RED | GREEN | ALT | LEDS |
| :--- | :---: | :---: | :---: | :---: | :---: | :---: | :---: |
| __D13__ | V | V | V | -  | -| - | -|
| __LED__ | - | V | V | V | V | V | - |
| __SET__ | - | - | V | - | - | - | - | 
| __STATUS__ | - | - | - | - | - | - | V |
| __VERSION__ | - | - | - | - | - | - | - |
| __HELP__ | - | - | - | - | - | - | - |

* D13
    * __ON__ - Turns on the light on D13.
    * __OFF__ - Turns off the light on D13.
    * __BLINK__ - Makes the light on D13 blink.
* LED
    * __RED__ - Turns on the light on the LED to RED.
    * __GREEN__ - Turns on the light on the LED to GREEN.
    * __BLINK__ - Makes the light on the LED blink.
    * __OFF__ - Turns off the LED light.
    * __ALT__ - Makes the light on the LED changes color between RED and GREEN.
* SET
    * __BLINK__ - Sets the value of the interval of blinking.
        > Needs a 3rd value as a number.
* STATUS
    * __LEDS__ - Returns a status of all LEDs.
* VERSION
    * Returns the Verion of the program.
* HELP
    * Return the list of commands.

### How to run

- Make sure the circuit board is connected to the computer.
- Make sure Arduino IDE (Ver 1.8 and above) is installed.
-  Run in terminal with the following commands:

-> `make`
-> `make upload`
-> `picocom -b 9600 /dev/ttyACM0`


