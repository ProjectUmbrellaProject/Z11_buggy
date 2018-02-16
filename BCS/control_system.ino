#include <SPI.h> 


const int leftMotorPin = 4, rightMotorPin = 7, speedPin = 3, trigPin = 9, echoPin = 8;

String inputString = "";
boolean stringComplete;

unsigned long previousPingTime;
const long pingInterval = 300;
int minimumDistance = 15;
long obstacle_distance;
bool forward;


void setup() {
  stringComplete = false;
  Serial.begin(9600); // initiate serial commubnication at 9600 baud rate
  Serial.print("+++"); //Enter xbee AT commenad mode, NB no carriage return here
  delay(1500);  // Guard time
  Serial.println("ATID 3311, CH C, CN"); // PAN ID 3311
  delay(1100);
  while(Serial.read() != -1) {}; // get rid of OK
    inputString.reserve(200);

  forward = false;

  
  pinMode(speedPin, OUTPUT);
  pinMode(leftMotorPin, OUTPUT);
  pinMode(rightMotorPin, OUTPUT);
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input

  //Set the motor states to max power but off
  analogWrite(speedPin, 120);
  digitalWrite(leftMotorPin, HIGH);
  digitalWrite(rightMotorPin, HIGH);
}

void loop() {
  unsigned long currentTime = millis();
  if (currentTime - previousPingTime >= pingInterval && forward){
    previousPingTime = currentTime;
    
    obstacle_distance = obstacleDistance();
   // Serial.println("obstacle");
    Serial.println(String(obstacle_distance));

    if (obstacle_distance <= 15){
     // moveCommand(0);
        //Serial.println("obstacle");

    }

    //To remove
    //print(obstacleDistance());

  }
  

  
  if (stringComplete) {
     moveCommand(inputString.toInt());
    
    // clear the string:
    inputString = "";
    stringComplete = false;
  }
}

void moveCommand(int command){
    switch (command){

      //Stop
      case 0:
        delay(20);
        digitalWrite(leftMotorPin, HIGH);
        digitalWrite(rightMotorPin, HIGH);
        forward = false;
        Serial.println("Stopping");
        break;
  
      //Move Forward
      case 1:
        delay(20);
        digitalWrite(leftMotorPin, LOW);
        digitalWrite(rightMotorPin, LOW);
        Serial.println("Moving forward");
        forward = true;
      
        break;
        
      default:
        Serial.println("default");
        break;

  }

  
}


long obstacleDistance(){
  long distance, duration;

  // Clears the trigPin
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  
  // Sets the trigPin on HIGH state for 10 micro seconds
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  
  // Reads the echoPin, returns the sound wave travel time in microseconds
  duration = pulseIn(echoPin, HIGH);
  
  // Calculating the distance
  distance = round(duration*0.034/2);

  return distance;

  
}

void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read();
    
    // add it to the inputString:
    inputString += inChar;
    
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}

