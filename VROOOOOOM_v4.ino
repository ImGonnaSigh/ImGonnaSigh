  #include <Wire.h>
  #include <Adafruit_MotorShield.h>
  #include "utility/Adafruit_MS_PWMServoDriver.h"
  #include <Servo.h>
  Servo servo;

  const int servoPin = 5; //  Pin of servo motor
  int angle = 10;
  
  const int pingPin = 6; // Trigger Pin of Ultrasonic Sensor
  const int echoPin = 7; // Echo Pin of Ultrasonic Sensor
  long duration, cm;

  String var="";
  int SlowDist = 55; //distance where blockage detected is considered obstacle
  int ObstacleDist = 20; //distance where blockage detected is considered obstacle
  
  
  // Motor
  Adafruit_MotorShield AFMS = Adafruit_MotorShield(); 
  Adafruit_DCMotor *myMotor1 = AFMS.getMotor(1);
  Adafruit_DCMotor *myMotor2 = AFMS.getMotor(2);
  Adafruit_DCMotor *myMotor3 = AFMS.getMotor(3);
  Adafruit_DCMotor *myMotor4 = AFMS.getMotor(4);

  long cycleCount=0; //to indicate how many planning cycle has lapsed since started
  int deadendCount=0; //count the number of times staying in XXX, stay too long, reverse and U-turn
  
  int spd=50; // normal cruising speed

  // IR-6
  int LED = 13; // Use the onboard Uno LED
  int isObstaclePin[7]; //to hold detection by 6 IR sensor ; intentionally ignore [0]
  String ir=""; // to hold obstacle detection result for every 1 Detection Cycle 
  String ir1=""; // to hold obstacle detection result for 1 Detection Cycle from 0 to 200 deg
  String ir2=""; // to hold obstacle detection result for 1 Detection Cycle from 200 to 0 deg

  void setup() {
    // initialize digital pin LED_BUILTIN as an indicator on the board it is alive
    pinMode(LED_BUILTIN, OUTPUT);
    Serial.begin(9600);                // initialize serial baud rate; ensure serial monitor use same baud rate
    digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(2000);                       // wait for a second
    digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
    delay(1000);                       // wait for a second
    while (!Serial) {
      ; // wait for serial port to connect. Needed for native USB
    }
    Serial.println("serial connection completed");
  
    // Motor
    AFMS.begin();
    Serial.println("Motor Started");
    
    // IR pin
    for (int i=0; i<=6; i++){
      isObstaclePin[i]=i;
      pinMode(LED, OUTPUT);
      pinMode(isObstaclePin[i],INPUT);
      Serial.begin(9600);    
    }
  
    // ultrasonic servo
    servo.attach(servoPin);
    servo.write(angle);
  
    move_motor("F",50);
  }

  void loop() {
    //sense_function();
    scan();
    plan_motion();
    delay(100);
  }

  void scan() 
  { 
   // scan from 0 to 180 degrees
    ir="";ir1 = "";ir2 = "";
    //Serial.println("start: angle=" + String(angle));
    
    if (angle < 60){
      //Serial.println("less_than_60: angle=" + String(angle));
      for(angle = 10; angle <= 110; angle=angle+25)  
      {                                  
        servo.write(angle);
        if (angle>10) delay(150);    
        //ir1 = String(ir1) + detect();  
        ir = detect() + String(ir);             
        delay(30);                   
      } 
    }
    else if (angle > 60){
      //Serial.println("more_than_60: angle=" + String(angle));
      for(angle = 110; angle >= 10; angle=angle-25)    
      {                             
        servo.write(angle); 
        if (angle <110) delay(150);  
        //ir2 =  detect() + String(ir2);          
        ir = String(ir) + detect();
        delay(30);       
      } 
    }
    
    //servo.write(0); //return to position
    // now scan back from 200 to 0 degrees

    //delay(1000);  
    //Serial.println("back now @FF150 @FB150 @UC50 @SB5" + "@OD" + ObstacleDist + "@SD" + SlowDist); 
    //FF = fire forward aft 150ms motor move; B = fire backward;
    //SB = Sweep Back after 5ms delay; UC = Ultrasonice Clearance (from fire to receive)
    //OD = distance where detected = obstacle; SD = distance where detected causes car to slow down
    //

    //Serial.println("ir=" + ir);
    //Serial.println("ir1=" + ir1);
    //Serial.println("ir2=" + ir2);
    //delay(1000); 
  }

  bool bool_slow = false;
  
  String detect() {
     //LOW->HIGH->LOW [PWM pulse]
     pinMode(pingPin, OUTPUT);
     digitalWrite(pingPin, LOW);
     delayMicroseconds(2);
     
     digitalWrite(pingPin, HIGH);
     delayMicroseconds(10);
     
     digitalWrite(pingPin, LOW);
     
     pinMode(echoPin, INPUT);
     duration = pulseIn(echoPin, HIGH);
     cm = duration/29/2;

     /* 
     Serial.print ("angle =");
     Serial.print (angle);
     Serial.print (" deg -> distance =");
     Serial.print(cm);
     Serial.println("cm");
     */
     
     if (cm<=SlowDist){bool_slow=true;}
     else bool_slow=false;

     if (cm<=ObstacleDist){return ("x");}
     else return ("o");
  }
  
  void plan_motion(){
    //check which IR sensor has detected obstacle and decide 
    //motion: forward L/R or Reverse L/R or stop
    cycleCount = cycleCount + 1;
    
    //F1,F2,F3 at Digital Pin 2,3,4
    var = "";
    //var = ir.substring(2,3) + ir.substring(3,4) + ir.substring(4,5); //using IR
    var = ir; //using Ultrasonic
    //Serial.println (String(cycleCount) + "> var=" + var);



    if (var=="ooooo"){move_motor("F",100); Serial.println(var + " F80");}
    else if ((var=="oooox")||(var=="oooxo")||(var=="oooxx")||(var=="ooxxo")||(var=="ooxxx")||(var=="ooxox")||(var=="oxxxx")){move_motor("L",85); Serial.println(var + " L65");}  
    else if ((var=="xoooo")||(var=="oxooo")||(var=="xxooo")||(var=="oxxoo")||(var=="xxxoo")||(var=="xoxoo")||(var=="xxxxo")){move_motor("R",85); Serial.println(var + " R65");}  
    else if ((var=="xxxxx")||(var=="oxoox")||(var=="oxxox")||(var=="oxoxx")||(var=="xooxo")||(var=="xoxxo")||(var=="xxoxo")||(var=="oxxxo")||(var=="ooxoo")) 
    { 
      move_motor("F",50); //slow down
      deadendCount = deadendCount + 1;
      if (deadendCount>5){
        move_motor("S",0);
        move_motor("B",50);
        delay (1000);
        deadendCount = 0;
      }
      move_motor("R",50); 
      delay (30);
    }
    
    /* //resolution of 3
    if (var=="ooo"){move_motor("F",100); Serial.println(var + " F100");}
    else if (var=="oox"){move_motor("L",105); Serial.println(var + " L105");}  
    else if (var=="oxx"){move_motor("L",105); Serial.println(var + " L105");}   
    else if (var=="oxo"){move_motor("R",105); Serial.println(var + " R105");}   
    else if (var=="xoo"){move_motor("R",105); Serial.println(var + " R105");}
    else if (var=="xxo"){move_motor("R",105); Serial.println(var + " R105");}
    else if (var=="xox"){move_motor("R",105); Serial.println(var + " R105");} 
    else if (var=="xxx"){
      deadendCount = deadendCount + 1;
      if (deadendCount>30){
        move_motor("B",100);
        delay (2000);
        deadendCount = 0;
      }
      move_motor("R",100); 
      delay (150);
    }
    */
    
  }

  void move_motor(String dir,int speedy){
    //dir: B=Backward, F=Forward,S=Stop 
    //dir: R=Turn-R, L=Turn-L, C=Crawl
   
    myMotor1->setSpeed(speedy);
    myMotor2->setSpeed(speedy);
    myMotor3->setSpeed(speedy);
    myMotor4->setSpeed(speedy);
    
    if (dir.equalsIgnoreCase("B")){
      myMotor1->run(BACKWARD);
      myMotor2->run(BACKWARD);
      myMotor3->run(BACKWARD);
      myMotor4->run(BACKWARD);
    }
    else if (dir.equalsIgnoreCase("F")){
      if (bool_slow == true) speedy = 30;
      
      myMotor1->setSpeed(speedy);
      myMotor2->setSpeed(speedy);
      myMotor3->setSpeed(speedy);
      myMotor4->setSpeed(speedy); 
           
      myMotor1->run(FORWARD);
      myMotor2->run(FORWARD);
      myMotor3->run(FORWARD);
      myMotor4->run(FORWARD);
    }
    else if (dir.equalsIgnoreCase("R")){
      //rotate right: M2,4 Backward
      myMotor1->run(FORWARD);
      myMotor2->run(BACKWARD);
      myMotor3->run(FORWARD);
      myMotor4->run(BACKWARD);
    }
    else if (dir.equalsIgnoreCase("L")){
      //rotate left: M1,3 Backward
      myMotor1->run(BACKWARD);
      myMotor2->run(FORWARD);
      myMotor3->run(BACKWARD);
      myMotor4->run(FORWARD);
    } 
    else if (dir.equalsIgnoreCase("S")){
      myMotor1->run(RELEASE);
      myMotor2->run(RELEASE);
      myMotor3->run(RELEASE);
      myMotor4->run(RELEASE);
    }

  }
  

 /*NOTE 2 AND 4 IS RIGHT WHEEL
   * 1 AND 3 IS LEFT WHEEL
   * */
   



 
  
