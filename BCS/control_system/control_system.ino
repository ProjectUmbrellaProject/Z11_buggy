#include <SPI.h> 

const int leftMotorPin = 4, rightMotorPin = 7, speedPin = 3, trigPin = 9, echoPin = 8, gantryIRPIN = 2, leftOvr = 5, rightOvr = 6;

String inputString = "";

unsigned long previousPingTime;
const int pingInterval = 400; //Determines how frequently the distance is measured from the ultrasonic sensor
const short minimumDistance = 15; //Determines how close an object must be to stop the buggy
const int gantryWaitTime = 1500; //Determines how long the buggy waits after detecting a gantry
const int motorPower = 170;
bool forward, objectDetected, stringComplete;

bool gantryDetected; //true if gantry is detected
unsigned long gantryDuration; //duration of the gantry pulse
int pulsecounter; //how many pulses have been recorded
int maxPulse; //maximum pulse length recorded


void setup() {
  //Declare output and input pins
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  pinMode(speedPin, OUTPUT);
  pinMode(leftMotorPin, OUTPUT);
  pinMode(rightMotorPin, OUTPUT);
  pinMode(leftOvr, OUTPUT);
  pinMode(rightOvr, OUTPUT);
  pinMode(gantryIRPIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(gantryIRPIN), gantryInterrupt, CHANGE);


  //Set the motor states to max power but off
  analogWrite(speedPin, motorPower);
  digitalWrite(leftMotorPin, HIGH);
  digitalWrite(rightMotorPin, HIGH);

  digitalWrite(leftOvr, LOW);
  digitalWrite(rightOvr, LOW);
    
  stringComplete = false;
  objectDetected = false;
  forward = false;
  gantryDetected = false;

  pulsecounter = 0;
  maxPulse = 0; 
  
  Serial.begin(9600); // initiate serial commubnication at 9600 baud rate
  Serial.print("+++"); //Enter xbee AT commenad mode, NB no carriage return here
  delay(1500);  // Guard time
  Serial.println("ATID 3311, CH C, CN"); // PAN ID 3311
  delay(1100);
  while(Serial.read() != -1) {}; // get rid of OK
    inputString.reserve(200);

  //Send status to monitoring program
  Serial.println("Buggy: Setup Complete.");
    
}

void loop() {
  
  unsigned long currentTime = millis(); //Update the time variable with the current time
  if (currentTime - previousPingTime >= pingInterval){
    previousPingTime = currentTime; 

    handleObjectDetection();
  }
  
  if (stringComplete) {
    //The stringComplete bool indicates whether or not a new command has been recieved
     moveCommand(inputString.toInt());
    
    // clear the string:
    inputString = "";
    stringComplete = false;
  }

  readPulse();
}

void moveCommand(int command){
    switch (command){

      //Stop
      case 0:
        delay(20);
        analogWrite(speedPin, 0);
        Serial.println("~4");
        Serial.print("~8");
        Serial.println(0);
        forward = false;
        break;
  
      //Move Forward
      case 1:
        delay(20);
        analogWrite(speedPin, motorPower);
        Serial.println("~9");
        Serial.print("~8");
        Serial.println(motorPower);
        forward = true;
      
        break;
        
      default:
        Serial.println("~6");
        break;
  }
  
}
//Moved from main loop to improve readability and reduce loop lenght
void handleObjectDetection(){
    long distance = obstacleDistance();

    //The command to stop should only be called if the control program has told the buggy to move and an object hasnt already been detected
    //i.e. the buggy isnt already stationary
    if (!objectDetected && distance <= minimumDistance && forward){
      
      objectDetected = true; //The objectDetected boolean prevents the if statement from being repeatedly executed while the object is still present
      moveCommand(0);
      forward = true; //Calling move command 0 also sets forward to false which is unintended in this case
      Serial.print("~5");
      Serial.println(distance);

    }
    else if (objectDetected && distance > minimumDistance && forward){
        objectDetected = false;
        moveCommand(1);
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

void gantryInterrupt(){
  //Interrupts have to be as short as possible to avoid slowing done the program. The flag updated in this function will allow the loop to handle the gantrydetection
  gantryDetected = true;
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

void readPulse(){
  
  if(gantryDetected && pulsecounter < 10){
    Serial.println("~7");
    moveCommand("0");
    
    //geting the duration of the pulse
    gantryDuration = pulseInLong(gantryIRPIN, LOW);
    
      if(gantryDuration > maxPulse){
        maxPulse = gantryDuration;
      }    
    gantryDetected = false;
    delay(100); //adjust to get faster timing if needed
    pulsecounter++;
    
  }else if(pulsecounter ==10){    
    int gantryNum = determineGantry();
    
    if(gantryNum == -1){
      Serial.println("undetermined gantry");
    }else{
     Serial.print("gantry number is ");
     Serial.println(gantryNum);
    }
    
    moveCommand("1");
    delay(500); //Allow the buggy to move away from the gantry before checking the IR receiver again
    pulsecounter = 0;
    maxPulse = 0; 
    gantryDetected = false;
    
  }
}

int determineGantry(){  
  if(maxPulse < 1250 && maxPulse > 750){
    return 1;
  }else if(maxPulse < 2250 && maxPulse > 1750){
    return 2;
  }else if(maxPulse < 3250 && maxPulse > 2750){
    return 3;
  }else return -1;
  
}


