#include <Pixy.h>
#include <SPI.h> 

//#define is a pre-compiler directive that results in 0 RAM use
#define gantryIRPIN 2
#define speedPin 3  //controls the speed of the motors
#define leftMotorPin 4
#define leftOverride 5
#define rightOverride 6
#define rightMotorPin 7
#define echoPin 8 //this pin recieves the ultrasonic pulse
#define trigPin 9 //writing this pin high will pulse the ultrasonic


String inputString = ""; //used to store assess commands recieved from processing
short motorPower = 170;
const short reducedSpeed = 135; //variable for the reduced speed once buggy sees marker to slow down
const short maxSpeed = 170; //maximum speed of the buggy (not set to the maximum possible speed to allow time to read markers/gantries)
bool forward; //indicates if the buggy is currently moving forward
bool stringComplete; //bool indicates whether or not a new command has been recieved

bool turndirection = true; // false  = left, true  = right

//Object detection variables
unsigned long previousPingTime;
const short pingInterval = 400; //Determines how frequently the distance is measured from the ultrasonic sensor
const short minimumDistance = 15; //Determines how close an object must be to stop the buggy
bool objectDetected; //bool is true if obstacle is infront of buggy within (minimumDistance) cm.

//Gantry detection variables
bool gantry_detected; //true if gantry is detected
unsigned long duration; //duration of the gantry pulse
int pulsecounter; //how many pulses have been recorded
int maxPulse; //maximum pulse length recorded
const int gantryCounter = 3; //variable to cap the amount of gantry pulses taken for each gantry.

//Pixy variables
Pixy pixy;
const int minimumDetections = 2; //minimum number of detections needed to eliminate false positives
int previousDetected = -1; //previously detected block to eliminate multiple detections of the same color
int detections[4]= {0,0,0,0}; //stores the amount of times each color has been detected

void setup() {
   //Declare output and input pins
  pinMode(trigPin, OUTPUT); // Sets the trigPin as an Output
  pinMode(echoPin, INPUT); // Sets the echoPin as an Input
  pinMode(speedPin, OUTPUT);
  pinMode(leftMotorPin, OUTPUT);
  pinMode(rightMotorPin, OUTPUT);
  pinMode(leftOverride, OUTPUT);
  pinMode(rightOverride, OUTPUT);

   //Setup for the gantry interrupt
  pinMode(gantryIRPIN, INPUT_PULLUP);
   //call the gantryInterrupt function when the digital output of the gantryIRPIN changes value
  attachInterrupt(digitalPinToInterrupt(gantryIRPIN), gantryInterrupt, CHANGE);
  
   //Setup for the gantry pulse detection
  gantry_detected = false;
  pulsecounter =0;
  maxPulse = 0; 

   //Set the motor states to max power but off
  analogWrite(speedPin, 0);
  digitalWrite(leftMotorPin, LOW);
  digitalWrite(rightMotorPin, LOW);
  digitalWrite(leftOverride, LOW);
  digitalWrite(rightOverride, LOW);
  
  stringComplete = false;
  objectDetected = false;
  forward = false;
    
  Serial.begin(9600); // initiate serial commubnication at 9600 baud rate
  Serial.print("+++"); //Enter xbee AT commenad mode, NB no carriage return here
  delay(1500);  // Guard time
  Serial.println("ATID 3311, CH C, CN"); // PAN ID 3311
  delay(1100);
  while(Serial.read() != -1) {}; // get rid of OK 
    inputString.reserve(200); //declare 200 bytes in arduino memory for this string

   //Send status to monitoring program
  Serial.println("Buggy: Setup Complete.");

  pixy.init();
}

void loop() {

   //Clear the pixy counters and check the ultrasonic sensor every (pingInterval) ms
  unsigned long currentTime = millis(); //Update the time variable with the current time
  if (currentTime - previousPingTime >= pingInterval){
    previousPingTime = currentTime; 
    handleObjectDetection();

    for (int i = 0; i < 4; i ++){
      detections[i] = 0; //The camera detections are reset every (pingInterval) ms
      //This way an object must be detected minimumDetections number of times within (pingInterval) ms before the buggy responds
    }
  }
  
   //If a new string has been received call the movecommand function
  if (stringComplete) {
     moveCommand(inputString.toInt());
    
     // clear the string:
    inputString = "";
    stringComplete = false;
  }
   
  readPulse(); //Handle gantry detection 
  detectSigns(); //Check for signs
}

void moveCommand(int command){
  
    switch (command){
       //Stop
      case 0:
        delay(20);
        analogWrite(speedPin, 0);
         //alert processing that the buggy has stopped
        Serial.println("~10"); 
        Serial.print("~8 ");
        Serial.println(0);
        forward = false;
        
        break;
  
      //Move Forward
      case 1:
        delay(20);
        analogWrite(speedPin, motorPower);
         //alert processing that buggy has started moving
        Serial.println("~9 ");
        Serial.print("~8 ");
        Serial.println(motorPower);
        forward = true;
      
        break;
        
       //Go to half speed
       case 2:
        if(forward){ //This prevents the buggy from starting when it detects a colour prior to receiving the start command
          motorPower = reducedSpeed;
          analogWrite(speedPin, motorPower);
          Serial.print("~8 ");
          Serial.println(motorPower);
        }
        break;
      
        //Resume full speed
        case 3:
          if(forward){
            motorPower = maxSpeed;
            analogWrite(speedPin, motorPower);
            Serial.print("~8 ");
            Serial.println(motorPower);
          }
          break;

        //Turn right
        case 4:
        if(turndirection){
          //turn right
          digitalWrite(leftOverride, HIGH);
          delay(1500); 
          digitalWrite(leftOverride, LOW);
          } 
          else {
          //turn left
          delay(200);
          digitalWrite(rightOverride, HIGH);
          delay(700);
          digitalWrite(rightOverride, LOW);
          }
          break;

          
        default:
          Serial.println("~20"); //alert the buggy that some unregistered command has been sent to the function
          break;
  }
  
}
//Moved from main loop to improve readability and reduce loop length.
//Calling this function will read the distance to the nearest object from the ultrasonic sensor and tell the buggy to react accordingly
void handleObjectDetection(){
    long distance = obstacleDistance();

    //The command to stop should only be called if the control program has told the buggy to move and an object hasnt already been detected
    //i.e. the buggy isnt already stationary
    if (!objectDetected && distance <= minimumDistance && forward){
      
      objectDetected = true; //The objectDetected boolean prevents the if statement from being repeatedly executed while the object is still present
      moveCommand(0);
      forward = true; //Calling move command 0 also sets forward to false which is unintended in this case
      Serial.print("~6 ");
      Serial.println(distance);

    }
    else if (objectDetected && distance > minimumDistance && forward){
        objectDetected = false;
        moveCommand(1);
    }
}

//This function returns the distance to the nearest object as measured by the ultrasonic sensor
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
  distance = round(duration*0.034/2); //return the distance based on the speed of sound
  return distance;

  
}

void gantryInterrupt(){
  //Interrupts have to be as short as possible to avoid slowing done the program. The flag updated in this function will allow the loop to handle the gantrydetection
  gantry_detected = true;
}

void serialEvent() {
  if (Serial.available()){
    inputString = Serial.readStringUntil('\n');
    stringComplete = true;
  }
}

void detectSigns(){ 
  uint16_t blocks = pixy.getBlocks();
  int realcolor; //variable used to combine the additional blue and green signatures into a single value in the detections array
    
  if (blocks > 0) {
    
    for (int i = 0; i < blocks; i++){
      if (pixy.blocks[i].y > 150){ //Only detections above y = 150 are considered so the buggy doesnt react to signs prematurely 
        
        realcolor = pixy.blocks[i].signature;
          if(realcolor > 3){
           realcolor = realcolor -3;
          }
        
        detections[realcolor - 1]++;
       }
        if (detections[realcolor -1] >= minimumDetections){
              //ensure that the same color isnt detected twice in a row
            if(previousDetected != pixy.blocks[i].signature && previousDetected != pixy.blocks[i].signature + 3 && previousDetected != pixy.blocks[i].signature - 3 ){
              Serial.print("~11");
              Serial.println(pixy.blocks[i].signature);
                //execute moveCommands based on the signatures
              if(pixy.blocks[i].signature == 1)
                moveCommand(2);
              else if (pixy.blocks[i].signature == 2)
                moveCommand(3);
              else if (pixy.blocks[i].signature == 3)
                moveCommand(4);
              else if (pixy.blocks[i].signature == 4)
                moveCommand(2);
              else if (pixy.blocks[i].signature == 5)
                moveCommand(3);
                
              previousDetected = pixy.blocks[i].signature;
            }
            else if(previousDetected == -1)
              previousDetected = pixy.blocks[i].signature;  
               
            break; //Only one detection will be considered

        }
            /*This assumes the colours are stored as follows:
             * 1. Half speed - blue
             * 2. Full speed - green
             * 3. Turn right - yellow
             * 4. Turn left
             */
      
      }        
    }
 }

void readPulse(){
  
  if(gantry_detected && pulsecounter < gantryCounter){
    //geting the duration of the pulse
    duration = pulseInLong(gantryIRPIN, LOW);
    
    if(duration > maxPulse){
      maxPulse = duration;
    } 
       
    gantry_detected = false;
    //adjust this delay to get faster timing
    //(make sure that the buggy waits long enough at the gantry to do this)
    delay(50);
    pulsecounter++;
    
  }
  else if(pulsecounter == gantryCounter){    
    
    int gantryNum = determineGantry();
    
    if(gantryNum == -1)
      Serial.println("~12"); //undetermined gantry
    else{
      Serial.print("~7 ");
      Serial.println(gantryNum);
    }
    
    pulsecounter = 0;
    maxPulse = 0; 
    
  }
}

int determineGantry(){
  //determine the gantry number based on the maximum duration of the pulse recorded.
  if(maxPulse < 1250 && maxPulse > 750){
    return 1;
  }else if(maxPulse < 2250 && maxPulse > 1750){
    return 2;
  }else if(maxPulse < 3250 && maxPulse > 2750){
    return 3;
  }else return -1;
  
}







