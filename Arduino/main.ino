#include <Servo.h>
#include <SparkFun_MS5803_I2C.h>
#include <TimerOne.h>
#define WAIT_RX while(Serial1.available()==0);
//#define _debug_
#ifdef _debug_
#define debugPrint(x) Serial.print(x);
#define debugPrintln(x) Serial.println(x);
#else
#define debugPrint(x) 
#define debugPrintln(x) 
#endif
#define MAX_PWM 0
#define MIN_PWM 1

#define LED 26

MS5803 sensor(ADDRESS_LOW);
float temperature_c=0;//Water temperature from ms5803
double pressure_abs=0;//Water pressure from ms5803
byte angles[9];//This array stored when receive data from mpu6050 
long int roll=0,yaw=0,pitch=0,temperature=0;//These four varible are getting from mpu6050
unsigned long int inturruptPeriod=20;//This is the period of inturruption,mainly for motor control.
double horizonPressure=1040;//This global varible set the standard horizon pressure.

#define readMs5803WaterTemperature()  temperature_c=sensor.getTemperature(CELSIUS,ADC_512);
#define readMs5803Pressure()          pressure_abs=sensor.getPressure(ADC_4096);
Servo motor1;
Servo motor2;
Servo motor3;
Servo motor4;

double errorK=0,errorK_1=0;
double output=0;
double target=1042.1;
double kp=10,ki=0.05,kd=5;
double errorSum=0;//This must reset at setup function and I dont know why.
double maxOutput=1400;
double minOutput=1000;
int motor1_max_pwm=2000,motor1_min_pwm=1000;
int motor2_max_pwm=2000,motor2_min_pwm=1000;
int motor3_max_pwm=2000,motor3_min_pwm=1000;
int motor4_max_pwm=2000,motor4_min_pwm=1000;
int motor1Speed=0;
int motor2Speed=0;
int motor3Speed=0;
int motor4Speed=0;

bool PIDStart=false;
bool openMs5803=true;

void MotorPIDControl()
{
  if(PIDStart)
  {
    errorK=target-pressure_abs;
    double tmp=kp*errorK+ki*errorSum+kd*(errorK-errorK_1);
    output=1000+tmp; 
    if(output<minOutput)output=minOutput;
    if(output>maxOutput)output=maxOutput;
    motor1Speed=output;
    motor2Speed=output;
    errorSum=errorSum+errorK;
    errorK_1=errorK;
    motor1.writeMicroseconds(output);
    motor2.writeMicroseconds(output);
    debugPrint(output);
    debugPrint(" ");
    debugPrint(errorK);
    debugPrint(" ");
    debugPrint(errorSum);
    debugPrint(" ");
    debugPrintln(pressure_abs);
  }
}

void MotorAttach()
{
  motor1.attach(4,motor1_min_pwm,motor1_max_pwm);
  motor2.attach(2,motor2_min_pwm,motor2_max_pwm);
  motor3.attach(3,motor3_min_pwm,motor3_max_pwm);
  motor4.attach(5,motor4_min_pwm,motor4_max_pwm);
}

void MotorAttach2()
{
  motor1.attach(4,motor1_min_pwm,motor1_max_pwm);
  motor2.attach(2,motor2_min_pwm,motor2_max_pwm);
  motor3.attach(3,motor3_min_pwm,motor3_max_pwm);
  motor4.attach(5,motor4_min_pwm,motor4_max_pwm);
  motor1.writeMicroseconds(1000);
  motor2.writeMicroseconds(1000);
  motor3.writeMicroseconds(1000);
  motor4.writeMicroseconds(1000);
}

void MotorInit()
{
  motor1.writeMicroseconds(motor1_max_pwm);
  motor2.writeMicroseconds(motor2_max_pwm);
  motor3.writeMicroseconds(motor3_max_pwm);
  motor4.writeMicroseconds(motor4_max_pwm);
  delay(2000);
  motor1.writeMicroseconds(motor1_min_pwm);
  motor2.writeMicroseconds(motor2_min_pwm);
  motor3.writeMicroseconds(motor3_min_pwm);
  motor4.writeMicroseconds(motor4_min_pwm);
  delay(2000);
}

void turnOnLed()
{
  digitalWrite(LED,0);
}

void turnOffLed()
{
  digitalWrite(LED,1);
}

void readMpu6050()
{
  while(Serial1.available())Serial1.read();
  delay(2);

    while(Serial1.available()!=0)
{
  byte data=Serial1.read();
  WAIT_RX;
  if(data==0x55)
  {
  WAIT_RX;
  data=Serial1.read();
      if(data==0x53)//Angle bag
      {
           for(int i=0;i<9;i++)
           {
             WAIT_RX;
             angles[i]=Serial1.read();
           }
           roll=angles[1]<<8|angles[0];
           pitch=angles[3]<<8|angles[2];
           yaw=angles[5]<<8|angles[4];
           temperature=angles[7]<<8|angles[6];
           roll=roll*180/32768;
           pitch=pitch*180/32768;
           yaw=yaw*180/32768;
           temperature=(temperature/340+36); 
           break;
       } 
    } 
} 
}

float readVolt()
{
  float data=analogRead(A0);
  data=data/43.9734;
  return data;
}

void setHorizonPressure(float input)
{
  if(input==0)
  horizonPressure=pressure_abs;
  else
  horizonPressure=input;
}

void SpeedSet(int x,int speedNum)
{
   speedNum=speedNum+1000;
   if(x==1||x==2)
   {
    if(speedNum>motor1_max_pwm)speedNum=motor1_max_pwm;
    if(speedNum<motor1_min_pwm)speedNum=motor1_min_pwm;
    if(x==1)motor1.writeMicroseconds(speedNum);
    else if(x==2)motor2.writeMicroseconds(speedNum);
   }
   else if(x==3||x==4)
   {
    if(speedNum>motor3_max_pwm)speedNum=motor3_max_pwm;
    if(speedNum<motor3_min_pwm)speedNum=motor3_min_pwm;
    if(x==3)motor3.writeMicroseconds(speedNum);
    else if(x==4)motor4.writeMicroseconds(speedNum);
   }
}

void setRangePwmMotor(int motorIndex,int input,int mode)
{
  if(mode==MAX_PWM)
  {
    debugPrint("Set motor ");
    debugPrint(motorIndex);
    debugPrint(" max pwm to ");
    debugPrintln(input);
    switch(motorIndex)
    {
      case 1:
      motor1_max_pwm=input;
      break;

      case 2:
      motor2_max_pwm=input;
      break;

      case 3:
      motor3_max_pwm=input;
      break;

      case 4:
      motor4_max_pwm=input;
      break;    
    }
  }
  else if(mode==MIN_PWM)
  {
    debugPrint("Set motor ");
    debugPrint(motorIndex);
    debugPrint(" min pwm to ");
    debugPrintln(input);
    
      switch(motorIndex)
    {
      case 1:
      motor1_min_pwm=input;
      break;

      case 2:
      motor2_min_pwm=input;
      break;

      case 3:
      motor3_min_pwm=input;
      break;

      case 4:
      motor4_min_pwm=input;
      break;    
    }
  }
}

void startPID()
{
    PIDStart=true;  
}

void stopPID()
{
    PIDStart=false;
    errorSum=0;
    output=1000;
    motor1.writeMicroseconds(output);
    motor2.writeMicroseconds(output);
}

void stopAllMotors()
{
  stopPID();
  motor1.writeMicroseconds(motor1_min_pwm);
  motor2.writeMicroseconds(motor1_min_pwm);
  motor3.writeMicroseconds(motor3_min_pwm);
  motor4.writeMicroseconds(motor3_min_pwm);
}

void Action(String commandHead,String commandTail)
{
    if(commandHead.length()==0&&commandTail.length()==0)return ;
    if(commandHead.length()==1)//command head is just one character,this shorten the time of command resolving
    {
      char commandType=commandHead[0];
      switch(commandType)
      {
          case 'a'://set horizon pressure
            if(commandTail.length()!=0)
              setHorizonPressure(commandTail.toFloat());
            else
              setHorizonPressure(0);
            //Serial.println("{ok}");
          break;

          case 'A'://read horizon pressure
            Serial.print("{");
            Serial.print(horizonPressure);
            Serial.println("}");
          break;

          case 'b':
            MotorAttach();
          break;

          case 'c':
            MotorAttach2();
          break;
          
          case 'B'://set max pwm of motor1
            MotorInit();
            //Serial.println("{ok}");
          break;

          case 'd'://read mpu6050 and temperature
            readMpu6050();
            Serial.print("{");
            Serial.print(roll);Serial.print(",");
            Serial.print(yaw);Serial.print(",");
            Serial.print(pitch);Serial.print(",");
            Serial.print(temperature);Serial.println("}");
          break;
  
          case 'e': //readMs5803Pressure();
            Serial.print("{");
            Serial.print(pressure_abs);
            Serial.println("}");
          break;     
  
          case 'f'://read water temperature
            readMs5803WaterTemperature();
            Serial.print("{");
            Serial.print(temperature_c);
            Serial.println("}");
          break;

          case 'g'://read volt
            Serial.print("{");
            Serial.print(readVolt());
            Serial.println("}");
          break;

          case 'h'://set pid target
            target=commandTail.toFloat();
           break;

          case 'i'://control led
            if(commandTail[0]=='1')
              turnOnLed();
            else
              turnOffLed();
          break;

          case 'j':
            if(commandTail[0]=='1')
              openMs5803=true;
            else
              openMs5803=false;
          break;

          case 's'://start pid control 
            startPID();
          break;

          case 't'://stop pid control
            stopPID();
          break; 

          case 'p'://stop all motor
            stopAllMotors();
          break;
      }
    }
    else//This command type length is more than one character
    {
      if(commandHead=="AT")
      {
        Serial.println("{ok}");
      }
      else if(commandHead[0]=='m')
      {
        uint8_t motorNum=commandHead[1]-'0';
        SpeedSet(motorNum,commandTail.toInt());
        //Serial.println("{ok}");
      }
      else if(commandHead[0]=='B')
      {
        uint8_t motorIndex=commandHead[1]-'0';//'1' or '2' or '3' or '4'
        uint8_t mode=commandHead[2]-'0';//'0' or '1' ,compare to MAX_PWM and MIN_PWM
        setRangePwmMotor(motorIndex,commandTail.toInt(),mode);              
        //Serial.println("{ok}");
      }
      else if(commandHead[0]=="p")
      {
        char pidParamter=commandHead[1];
        if(pidParamter=='p'){kp=commandTail.toFloat();}
        else if(pidParamter=='i'){ki=commandTail.toFloat();}
        else if(pidParamter=='d'){kd=commandTail.toFloat();}
      }
    }
}

bool Listen()//Return serial buffer state
{
    return Serial.available();
}

String Receive()//Receive a string begins with '{' and ends with '}' 
{
    String tmp="";
    int overflowCount=50;
    int overflowLength=100;
    bool beginStoreFlag=false;
    while(Serial.available()!=0)
    {
        char data=Serial.read();
        delayMicroseconds(300);
        if(data=='{')
        {
            beginStoreFlag=true;
        }
        if(beginStoreFlag)
        {
            tmp+=data;
            if(overflowLength--==0)
            {
                break;
            }
        }
        if(data=='}')break;
        if(overflowCount--==0)break;
    }
    return tmp;
}

bool CheckString(String input)//Check if input string is formated and do crc calculation
{
    if(input.length()<3)return false;
    if(input[0]!='{'||input[input.length()-1]!='}')return false;
        #ifdef xorJudge
        uint8_t judgeFlag=0;
        for(int i=1;i<input.length()-2)
            judgeFlag^=input[i];
        if(judgeFlag!=input[input.length()-2])return false;
        #endif
    return true;
}

void ParseString(String input)//Parse command
{
    bool checkResult=CheckString(input);
    if(!checkResult) return;
    debugPrintln(input);
    char foreheadJudge=0;
    String commandHead="";
    String commandTail="";
    bool nextJudge=false;
    for(int i=1;i<input.length();i++)
    {
        if(input[i]==':')
        {
            foreheadJudge=1;
            continue;
        }else if(input[i]==','||input[i]=='}')
        {
            foreheadJudge=0;
            nextJudge=true;
        }
        if(nextJudge==true)
        {
            nextJudge=false;
            Action(commandHead,commandTail);
            commandHead="";
            commandTail="";
            continue;
        }
        if(foreheadJudge==0)
        {
            commandHead+=input[i];
        }else
        {
            commandTail+=input[i];
        }
    }
}

void taskDistribute()
{
    MotorPIDControl();
}

void setup()
{
    Serial.begin(115200);
    Serial1.begin(115200);
    Serial3.begin(115200);
    pinMode(LED,OUTPUT);
    turnOffLed();
    
    sensor.reset();
    sensor.begin();
    inturruptPeriod=30;
    //Timer1.initialize(1000*inturruptPeriod);//This is inturrupt happens every inturrptPeriod ms.
    //Timer1.attachInterrupt(taskDistribute);
    errorSum=0;//PID control paramater
    PIDStart=false;
    openMs5803=true;
}

void loop()
{
    bool listenState=Listen();
    if(listenState)
    {
        listenState=false;
        String recvData=Receive();
        if(recvData.length()!=0)
        {
            ParseString(recvData);
        }
    }
    if(openMs5803)
      readMs5803Pressure();//Every loop do a pressure update,this takes about 25ms.
    else
      delay(25);
    MotorPIDControl();
}