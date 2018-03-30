import meter.*;
import controlP5.*;
import processing.serial.*;

Serial port; //Object to facilitate sending and receiving information via the Xbee
Meter motorOutput; //A voltmeter to display the current voltage applied to the motors
short currentMotorValue; //Variable to track the current motor speed (8 bit value)
int previousTime; //Variable used to flash UI elements on and off
int currentDetection; //Variable used to track the most recent location of the buggy
boolean controlToggle; //Boolean to track the state of the buggy start/stop toggle
boolean moving; //Boolean to track whether or not the buggy is currently moving
boolean obstacleDetected; //Boolean to track if the buggy has detected an object with the ultrasonic sensor
ControlP5 cp5; //Object required for handling control p5 GUI elements

void setup(){
  //Specifying the window dimensions, refresh rate and background colour
  frameRate(60);
  size(1280, 720);
  background(255);
  
  //Initialising the global variables
  cp5 = new ControlP5(this);
  controlToggle = false;
  moving = false;
  obstacleDetected = false;
  currentMotorValue = 0;
  currentDetection = 0;
  motorOutput = new Meter(this, 855, 460, true);
  
  
  PImage[] imgs = {loadImage("Assets/go.png"),loadImage("Assets/stop.png")}; //Loading the images used for the start/stop toggle
  cp5.addToggle("controlToggle") //Adding the start stop toggle, specifying its initial state, position and dimensions
  .setValue(false)
  .setPosition(500, 600)
  .setImages(imgs)
  .setSize(200,100);
  

  //A basic check to ensure that there is at least 1 COM device connected (which is most likely to be the Xbee)
  if (Serial.list().length > 0){
    //Initialising the port variable and running the AT commands to establish the Xbee connection
    String portName = Serial.list()[0];
    port = new Serial(this, portName, 9600);
    port.write("+++");
    delay(1100);
    port.write("ATID 3311, CH C, CN");
    delay(1100);
    port.bufferUntil( 10 );
    
  }
  else
    print("Warning: No Xbee detected"); //If there are no COM devices detected then the Xbee cannot be connected
  
  previousTime = millis(); //Storing the current time
  
}


void draw(){
  image(loadImage("Assets/map.png"), 0, 0); //Loading the image of the map and rendering it at coordinates (0, 0)
  
  //The if statements below cause the most recent location of the buggy on the map to flash every half second
  if (millis() - previousTime < 500){
    highLightLocation(); //This function draws a red rectangle around the most recent location
  }
  else if (millis() - previousTime > 500 && millis() - previousTime < 1000){
    image(loadImage("Assets/map.png"), 0, 0); //The easiest way to locally clear the screen is to render the map over the original image

  }
  else if (millis() - previousTime > 1000)
    previousTime = millis();

  
  if (obstacleDetected) //If an obstacle is detected inform the user on screen
      image(loadImage("Assets/obstacleDetected.png"), 0, 520);
  else if (moving) //If the buggy is moving illustrate this on screen
      image(loadImage("Assets/forward.png"), 0, 520);
  else if (!moving) //If the buggy is stopped illustrate this on screen
      image(loadImage("Assets/stopped.png"), 0, 520);
  
    motorOutput.updateMeter(currentMotorValue); //Update the value of the meter with the current motor value (even if it's unchanged)

}

//Function for handling serial communication
void serialEvent(Serial p){
   String receivedString = p.readString();
   
   //If the string starts with ~ it contains useful information
   if (receivedString.charAt(0) == '~'){
       //Pass the information to other functions to be analysed and eventually inform the user
       printCommandInformation(receivedString); 
       commandInterpreter(receivedString);          
   }
   else
     print(receivedString); //The string was not a command, print it for possible debugging

   p.clear(); //Clear the buffer for the next string
  
}

//Function to extract information from newly received commands and update respective global variables
void commandInterpreter(String command){
  
  //Make sure the command is information bearing before trying to extract information (command list specifies that commands beginning with ~ are information bearing)
  if (command.charAt(0) == '~'){
    //Commands are 2 digit numbers so only the 2nd and 3rd characters in the string should be considered
    switch (command.substring(1,3).trim()){
      //6: Obstacle detected
      case "6":
        obstacleDetected = true;
        moving = false;
        
      break;
      
      //7: Gantry XX detected
      case "7":
        currentDetection = Integer.valueOf((command.substring(3)).trim());     
        
      break;
      
      //8: Motor power set to XX
      case "8":
        currentMotorValue = Short.valueOf((command.substring(3)).trim());
        
      break;
          
      //9: Move command confirmation
      case "9":
        obstacleDetected = false;
        moving = true;
        
      break;
      
      //10: Stop command confirmation
      case "10":
        obstacleDetected = false;
        moving = false; 
        
       break;
       
      //11: Detected colour ID XX
      case "11":
        int colourId = Integer.valueOf((command.substring(3)).trim());
        
        //Colours 1 and 4 correspond to blue
        if (colourId == 1 || colourId == 4){
          
          if (currentDetection == 3)
            currentDetection = 5;
          else if (currentDetection == 6)
            currentDetection = 7;
            
        //Colours 2 and 5 correspond to green    
        } else if (colourId == 2 || colourId == 5){
          
            if (currentDetection == 5)
              currentDetection = 6;
            else if (currentDetection == 2)
              currentDetection = 6;
            else if (currentDetection == 7)
              currentDetection = 8;
              
        //Colour 3 corresponds to yellow    
        } else if (colourId == 3)
            currentDetection = 4;
            
      break;
      
      //20: Unknown command
      case "20":
        print("Unrecognised command recieved by buggy");
      break;
           
  }

    

  }
}

public void controlEvent(ControlEvent theEvent){
  //This if statement is require to prevent an error occuring on launch
  //For some reason the cp5 event handler is called before the UI elements have been fully initialised
  if (millis() > 2000){
    
      if ((theEvent.getController().getName()).equals("controlToggle")){
        
        if (controlToggle)
          port.write("1 \n"); //Send the start command
        else
          port.write("0 \n"); //Send the stop command
        
         controlToggle = !controlToggle;

    }
  }
  
}

//Event handler for key presses
void keyPressed(){
  switch (key){
    //The state of the start/stop toggle can also be toggled by pressing the space bar (ASCII 32)
    case 32:
      float temp; //The setValue function accepts a float argument to change the state of the UI element
      if (cp5.getController("controlToggle").getValue() == 0) //If the current state is 0 change it to 1, if it's 1 change it to 0
        temp = 1;    
      else
        temp = 0;
      //Note: the command to start/stop the buggy does not need to sent here because toggling the toggle will result in controlEvent being called
      cp5.getController("controlToggle").setValue(temp);
  
    break;
  }
  
}

//This function draws a red rectangle around the most recent location of the buggy as indicated by the global currentDetection varaible.
void highLightLocation(){
  
      switch (currentDetection){
      case 1:
        stroke(255, 0, 0);
        fill(255, 0);
        rect(200, 7, 120, 65, 10);
        break;
      
      case 2:
        stroke(255, 0, 0);
        fill(255, 0);
        rect(696, 7, 100, 65, 10);
        
        break;
        
      case 3:
        stroke(255, 0, 0);
        fill(255, 0);
        rect(618, 132, 70, 70, 10);
        
        break;
        
      case 4:
        stroke(255, 0, 0);
        fill(255, 0);
        rect(520, 20, 125, 45, 10);
        
        break;
        
      case 5:
        stroke(255, 0, 0);
        fill(255, 0);
        rect(715, 248, 90, 73, 10);
        
        break;
        
      case 6:
        stroke(255, 0, 0);
        fill(255, 0);
        rect(575, 452, 135, 40, 10);
        
        break;
        
      case 7:
        stroke(255, 0, 0);
        fill(255, 0);
        rect(228, 452, 125, 40, 10);
        
        break;
        
      case 8:
        stroke(255, 0, 0);
        fill(255, 0);
        rect(8, 355, 90, 72, 10);
        
        break;
     
      
    }
}

//Function to convert the 'encoded' communication between the buggy and monitoring program into understandable messages printed in the console
//This functionality could be integrated in the commandInterpreter function, however, writing this as a seperate function allows its use to be toggled on and off by
//commenting out line 93. This can be useful when debugging new features.
void printCommandInformation(String command){
  switch (command.substring(1, 3).trim()){
    case "6":
      println("Obstacle detected at " + command.substring(3).trim() + " cm");
      
    break;
    
    case "7":
      println("Detected gantry #: " + (command.substring(3)).trim());
    break;
    
    case "8":
      println("Motor power: " + (command.substring(3)).trim());
    break;
    
    case "9":
      println("Start command received");
    break;
    
    case "10":
      println("Stop command received");
    break;
    
    case "11":
    
      print("Detected colour: ");
      int colourId = Integer.valueOf((command.substring(3)).trim());
        if (colourId == 1 || colourId == 4)
          println("Blue");
        else if (colourId == 2 || colourId == 5)
          println("Green");
        else if (colourId == 3)
          println("Yellow");

      break;
      
    case "12":
      println("Did not recognise gantry number");
      break;

  }
  
}