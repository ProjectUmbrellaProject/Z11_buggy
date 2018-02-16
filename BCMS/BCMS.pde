import meter.*;
import controlP5.*;
import processing.serial.*;

Serial port;
Meter motorOutput;
int counter = 0;
boolean callBack, controlToggle;
ControlP5 cp5;

Toggle motorState;

void setup(){
  size(1280, 720);
  background(255);
  cp5 = new ControlP5(this);
  controlToggle = false;
  
  motorOutput = new Meter(this, int(pixelWidth*0.66), int(pixelHeight*0.64), true);
  
  PImage[] imgs = {loadImage("go2.png"),loadImage("stop2.png")};
  cp5.addToggle("controlToggle")
  .setValue(false)
  .setPosition(500, 600)
  .setImages(imgs)
  .setSize(200,100);
  
  image(loadImage("stopped.png"), 0, 520);

  if (Serial.list().length > 0){
    
    String portName = Serial.list()[0];
    port = new Serial(this, portName, 9600);
    port.write("+++");
    delay(1100);
    port.write("ATID 3311, CH C, CN");
    delay(1100);
    port.bufferUntil( 10 );
  }
  else
    print("No Xbee detected");
  
  
  
  callBack = false;
  
}


void draw(){
  motorOutput.updateMeter(0);
 
}

void serialEvent(Serial p){
   String receivedString = p.readString();
   print(receivedString);
   counter++; //During the initial startup the serial event callback will be called 9 times. The counter allows these calls to be ignored
   
   if (counter > 9){
    
     if ((receivedString.trim()).equals("obstacle"))
       cp5.getController("controlToggle").setValue(0);
     
     callBack = true;
     
   }
   
   p.clear();
   
 
}

public void controlEvent(ControlEvent theEvent){
  
    switch (theEvent.getController().getName()){
      case "controlToggle":
 
        
        if (controlToggle)
          port.write("1" + "\n");
          
        else
          port.write("0" + "\n");
          
         controlToggle = !controlToggle;
      
        break;

    }
  
}

void keyPressed(){
  switch (key){
    case 32:
      float temp;
      if (cp5.getController("controlToggle").getValue() == 0){
        temp = 1;
      }
      else{
        temp = 0;
      }
      cp5.getController("controlToggle").setValue(temp);
  
    break;
  }
  
}