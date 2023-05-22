#include <ECE3.h>

// pins constants
const int LEFT_NSLP_PIN = 31;
const int LEFT_DIR_PIN  = 29;
const int LEFT_PWM_PIN  = 40;

const int RIGHT_NSLP_PIN = 11;
const int RIGHT_DIR_PIN  = 30;
const int RIGHT_PWM_PIN  = 39;

// PID constants
const float KP = 20;
const float KD = 40;
const float KI = 0;
const float BASE_SPEED = 100;
const float TURN_SPEED = 50;

// PID variables
float previousLinePosition = 0;
float integralLinePosition = 0;

uint16_t sensorValues[8];
uint16_t offsetValues[8];
char isInitialized = 0;
float leftSpd = 0;
float rightSpd = 0;

// initialize/calibrate the white values
void calibrateWhite() {
  for (int i = 0; i < 20; i++) {
    ECE3_read_IR(sensorValues);
    delay(50);
  }
  for (int i = 0; i < 8; i++) {
    offsetValues[i] = sensorValues[i];
  }
}

// note: returns 0 if no line detected
// left (+1)     0       right (-1)
float getLinePosition(uint16_t inputSensorValues[]) {
  float sumOfSensors = 0;
  float sumOfSensorsWeighted = 0;
  for (unsigned char i = 0; i < 8; i++)
  {
    float offsettedSensorValue = inputSensorValues[i] - offsetValues[i];
    float normalizedSensorValue = offsettedSensorValue / (float)(2500 - offsetValues[i]);
    sumOfSensors += normalizedSensorValue;
    sumOfSensorsWeighted += normalizedSensorValue * (float)(i + 1);
    
    //Serial.print(normalizedSensorValue);
    //Serial.print('\t'); // tab to format the raw data into columns in the Serial monitor
  }
  if (sumOfSensors > SUM_OF_SENSORS_MIN) {
    
    // sumOfSensorsWeighted/sumOfSensors is centred at 4.5 and goes from 1.5 - 7.5
    float linePosition = ((sumOfSensorsWeighted/sumOfSensors) - 4.5) / 3;
    return linePosition;
  } else {
    return -99;
  }
}

// initialize pin values
void initPins() {

  // Pin Settings
  pinMode(left_nslp_pin,OUTPUT);
  pinMode(left_dir_pin,OUTPUT);
  pinMode(left_pwm_pin,OUTPUT);
  pinMode(right_nslp_pin,OUTPUT);
  pinMode(right_dir_pin,OUTPUT);
  pinMode(right_pwm_pin,OUTPUT);

  // Setting Initial Values
  digitalWrite(left_dir_pin,LOW);
  digitalWrite(left_nslp_pin,HIGH);
  digitalWrite(right_dir_pin,LOW);
  digitalWrite(right_nslp_pin,HIGH);

}

void setMotorSpeedLeft(int speed) {
  analogWrite(left_pwm_pin, speed);
}

void setMotorSpeedRight(int speed) {
  analogWrite(right_pwm_pin, speed);
}

void setup()
{
  ECE3_Init();
  Serial.begin(9600); // set the data rate in bits per second for serial data transmission

  isInitialized = 0;
  Serial.println("initializing");
  // MAKE SURE VEHICLE IS ON A WHITE SURFACE WHEN INIT
  initPins(); // in
  setMotorSpeedLeft(0);
  setMotorSpeedRight(0);
  calibrateWhite(); // initialize the white values
  delay(2000);
  
  Serial.println("init completed");
  isInitialized = 1;
}

void loop()
{
  if (!isInitialized) return;
  
  // read raw sensor values
  ECE3_read_IR(sensorValues);

  float linePosition = getLinePosition(sensorValues);
  if (linePosition == -99) linePosition = previousLinePosition;
  //Serial.println(linePosition);

  float derivative = linePosition - previousLinePosition;
  float integral = integralLinePosition;
  float proportional = linePosition;

  // implement PID controller
  float output = kP * proportional + kI * integral + kD * derivative;
  
  setMotorSpeedLeft(SPEED - output - TURN_COEFF*abs(linePosition));
  setMotorSpeedRight(SPEED + output - TURN_COEFF*abs(linePosition));

  // update PID values
  previousLinePosition = linePosition;
  integralLinePosition += linePosition;
  delay(10);
}
