#include <Wire.h>
#include <Servo.h>
#include "RTClib.h"


//Defines temperature sensor analog pin
const int tempSensor = A0;

//Defines light sensor analog pin
const int lightSensor = A1;

// Defines LED pins for when it is dark
const int secLedPin = 1;
const int minLedPin = 2;
const int hourLedPin = 3;
const int tempLedPin = 4;

// Defines Ultrasonic pins
const int trigPin = 5;
const int echoPin = 6;

// Defines Second Motor Pins
#define secDirPin 8
#define secStepPin 9

// Define Minute Motor Pins
#define minDirPin 10
#define minStepPin 11

// Define Hour Motor Pins
#define hourDirPin 12
#define hourStepPin 13

//Shortcut for using the RTC module
RTC_DS1307 rtc;

// Creating an object for the servo
Servo tempServo;

// To be used by the light sensor
int light;

// To be used for the tempurature sensor
int tempInput;
double temp;

// To be used for the servo position
int servoPos;

// To be used by the ultrasonic sensor
int duration;
int distance;

// To be used for assigning the time
int currentSec;
int currentMin;
int currentHour;

// This is used so I only need to change this value instead of two values
const int distVal = 60;

// To be used as a comparision
int previousSec = 0;

// To be used to indicate if it has reset or initialised.
bool reset = false;
bool initialise = false;

// Setup is mainly to set up the pinmodes for everything, start the serial port and the RTC module
void setup() {

  // Setting the pin modes for all the leds.
  pinMode(secLedPin, OUTPUT);
  pinMode(minLedPin, OUTPUT);
  pinMode(hourLedPin, OUTPUT);
  
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input

  // Setting pin modes for the seconds motor
  pinMode(secDirPin, OUTPUT);
  pinMode(secStepPin, OUTPUT);

  // Setting pin modes for the minutes motor
  pinMode(minDirPin, OUTPUT);
  pinMode(minStepPin, OUTPUT);

  // Setting pin modes for the hours motor
  pinMode(hourDirPin, OUTPUT);
  pinMode(hourStepPin, OUTPUT);

  //Attaching the servo outputs to pin 7
  tempServo.attach(7);

  Serial.begin(9600); // Starts the serial communication
  delay(3000); // Wait for the serial to begin

  // Check the RTC module to see if it's begun or not.
  if(! rtc.begin()){
    Serial.println("Couldn't find the RTC module. Make sure it is connected");
    while(1);
  }

  // Checks if it has been running and has a time set. If not, it'll set the time.
  if(!rtc.isrunning()){
    Serial.println("Setting time for RTC");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // Sets the time to when this code was compiled
  }
}

void loop() {

  // This chunk of code grabs the current time now, and assigns the current time to values that can be used later.
  DateTime now = rtc.now();
  currentSec = now.second();
  currentMin = now.minute();
  currentHour = now.hour();
  
  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  // Sets the trigPin on HIGH state for 10 micro seconds, then back off.
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  
  // Calculating the distance. Approx the distance in cms (only approximately).
  distance = duration*0.034/2;

  /* Using the distance value, the clock will only show the time when the 
   * user is within approximately 60 cm away from the clock. If they are too far,
   * the clock resets back to 0, meaning the user will have to be close to it for
   * it to show the time */
   
  if(distance <= distVal){
    reset = false; //Turns of the reset value

    // This will initalise the clock to go to the current time. It will only do it once.
    // Without it, it would just move the arms but not tell the current time.
    if(initialise == false){

      // These are individual commands for moving the arms a certain amount of degrees.
      secRotate(currentSec);
      minRotate(currentMin);
      hourRotate(currentHour);
      previousSec = currentSec;
      Serial.println("Hello, I see you've come to check the time. Now displaying the time");
      initialise = true;
      
    // If the clock has been initalised, it no longer needs to move to the current time, but only on step.
    }else{
      
      // This compares the current second to the previous second, so it only does it once per second.
      if(currentSec != previousSec){
        secRotate(1);

        //Nested inside this are two statements that check to see if it's a new minute or hour
        // If the second is 0, then it's a new minute
        if(currentSec = 0){
          minRotate(1);
          
          // If the minute is 0, then it's a new hour
          if(currentMin = 0){
            hourRotate(1);
          }
        }
        //Sets the current second as the previous second, so this won't run until the time changes
        previousSec = currentSec;
      }
    }

    // LIGHT SENSOR FUNCTIONALITY
    // So that it works when it is dark, there are 3 leds that light up when it's dark enough and the user is close enough.
    light = analogRead(lightSensor);
    if(light <= 300){
      digitalWrite(secLedPin, HIGH);
      digitalWrite(minLedPin, HIGH);
      digitalWrite(hourLedPin, HIGH);
      digitalWrite(tempLedPin, HIGH);
    }else{
      digitalWrite(secLedPin, LOW);
      digitalWrite(minLedPin, LOW);
      digitalWrite(hourLedPin, LOW);
      digitalWrite(tempLedPin, LOW);
    }

    // TEMPERATURE SENSOR FUNCTIONALITY
    // Adding another arm, this arm indicates the temperature.
    // It's a servo motor instead of a DC because it would be easier to read temp as it only turns 180 degrees.
    tempInput = analogRead(tempSensor);

    // This set of code is calculating the temperature value from the voltage outputted from the sensor
    // Code adapted from this tutorial: https://bc-robotics.com/tutorials/using-a-tmp36-temperature-sensor-with-arduino/
    temp = (double)tempInput / 1024;
    temp = temp * 5;
    temp = temp * 100;
    // This maps the temperature (between 5 and 50 degrees) to a position for the servo.
    servoPos = map(temp, 5, 50, 0, 180);
    
  }

  // This bit of code will reset the clock back to 0:00:00
  
  if (distance > distVal){
    initialise = false;
    if(reset == false){
      // Using maths to figure out the remaining time to 0.
      // Essentially taking the current time and subtracting it from the full number to find the amount of time remaining to 0.
      // This will then rotate the amount of degrees back to 0. 
      
      secRotate(60-currentSec);
      minRotate(60-currentMin);
      hourRotate(12-currentHour);

      digitalWrite(secLedPin, LOW);
      digitalWrite(minLedPin, LOW);
      digitalWrite(hourLedPin, LOW);
      digitalWrite(tempLedPin, LOW);

      // This resets the temp servo position. It does not need to be shown if the user isn't close.
      servoPos = 0;
      reset = true;
      Serial.println("You're too far away. Reseting everything until you come back");
    } 
  }

  // After everything, this will write the position for the servo controlling temp.
  // It's outside of everything as it will be easier to write it once instead of two times.
  tempServo.write(servoPos);
}


// This command takes the value inputted in the command.
// In the initalisation phase, it finds the current second from the RTC module and steps itself to that point.
void secRotate(float sec){
  float deg;
  // This multiplies the second value by 6 (which is the amount of degrees the arm has to turn
  if(sec != 0){
    deg = sec * 6;
  }

  // If it's 0, then it needs to rotate to 360.
  if(sec == 0){
    deg = 360;
  }

  // Calculates the amount of steps the module needs to take.
  int steps = abs(deg) * 1/0.225;
  for(int i=0; i<steps; i++){
    //Steps at the high speed.
    digitalWrite(secDirPin, HIGH);
    digitalWrite(secStepPin, HIGH);
    
  }
}

// This is the same as the second code, just a different pin to write to.
void minRotate(float minu){
  float deg;
  if(minu != 0){
    deg = minu * 6;
    
  }
  if(minu == 0){
    deg = 360;
  }
  int steps = abs(deg) * 1/0.225;
  for(int i=0; i<steps; i++){
    digitalWrite(minDirPin, HIGH);
    digitalWrite(minStepPin, HIGH);
    
  }
}

// Slightly different as the degree difference is more for it to turn (30 degrees per rotation).
// It's not an acutal clock so it doesn't adjust the arm slightly, it only ticks over when the hour changes.
void hourRotate(float hou){
  float deg;
  if(hou != 12){
    deg = hou * 30;
    
  }
  if(hou == 12){
    deg = 360;
  }
  int steps = abs(deg) * 1/0.225;
  for(int i=0; i<steps; i++){
    digitalWrite(hourDirPin, HIGH);
    digitalWrite(hourStepPin, HIGH);
    
  }
}
