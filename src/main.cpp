#include <Arduino.h>
#include <Wire.h>


#define MAX_SAMPLES 1500
#define PRESSURE_SENSOR_ADD 0x18
#define PRESSURE_MIN 0
#define PRESSURE_MAX 300
#define OUTPUT_MIN 419430
#define OUTPUT_MAX 3774873


/************************************************************************
 * Reads the pressure
 * Return values:
 * -1 = Device is busy
 * -2 = No power
 * -3 = Memory is bad
 * -4 = Saturated
 * -5 = No bytes available to read
 * Anything else is the 24 bit pressure value
 ************************************************************************/ 
int32_t readPressure(){ //Int is only 16 bits in Arduino with AtMega chips
  uint8_t status;
  uint8_t pressureByte0;
  uint16_t pressureByte1;
  uint32_t pressureByte2;

  //begin transmission
  Wire.beginTransmission(PRESSURE_SENSOR_ADD);
  //write 3 bytes
  char cmd [3];
  cmd[0] = 0xaa;
  cmd[1] = 0;
  cmd[2] = 0;
  Wire.write(cmd);
  //end transmission
  Wire.endTransmission();
  delay(5);

  Wire.requestFrom(PRESSURE_SENSOR_ADD, 4, true);
  
  if(Wire.available()){
    status = Wire.read();
    
    if(status & (1 << 5)){ //device is busy
      return -1;
    }
     if(!(status & (1 << 6))){ //no power
      return -2;
    }
     if(status & (1 << 2)){ //memory is bad
      return -3;
    }
     if(status & (1 << 0)){ //saturated
      return -4;
    }

    //Pressure is available to read, read the 24 bits, one byte at a time in order
    pressureByte2 = Wire.read();
    pressureByte1 = Wire.read();
    pressureByte0 = Wire.read();

    Wire.endTransmission();

    //In the return: Necessary cast because otherwise left shift by 16 will overflow the bounds of the 
    //temp int that holds shifted number, because int is only 16 bits in Arduino with AtMega chips

    //Concatenate the 3 bytes together and return
    return pressureByte0 + (pressureByte1 << 8) + (pressureByte2 << 16);;
  }

  return -5; //No bytes available to read
}


/*************************************************************************
 * Converts the measured pressure to real pressure by using the transfer function
 * Return values:
 * -1 = Device is busy
 * -2 = No power
 * -3 = Memory is bad
 * -4 = Saturated
 * -5 = No bytes available to read
 * Anything else is a float representing the calculated pressure value
**************************************************************************/
float getRealPressure(int32_t rawPressure){
  if(rawPressure < 0){
    return rawPressure;
  }

  //Use transfer function from data sheet
  return (rawPressure - OUTPUT_MIN) * (PRESSURE_MAX - PRESSURE_MIN) / (OUTPUT_MAX - OUTPUT_MIN) + PRESSURE_MIN;
}


void setup() {
  Serial.begin(9600);
  Wire.begin();
}


void loop() {
  //Display welcome message
  Serial.println("Welcome to Aron's Blood Pressure Monitor(Embedded Systems Project Spring 2021)!");
  Serial.println("To start, please apply the cuff and pump the pressure up to about 170mmHg.");

  float myCalculatedPressure = getRealPressure(readPressure());

  if(myCalculatedPressure < 0){
    //ERROR - something went wrong
    switch ((int)myCalculatedPressure)//cast to int so the switch is valid
    {
    case -1:
      Serial.println("Device is busy, please try again soon.");
      break;
    case -2:
      Serial.println("No power, please check wires or power source.");
      break;
    case -3:
      Serial.println("Memory error has occurred");
      break;
    case -4:
      Serial.println("Math saturation has occurred");
      break;
    case -5:
      Serial.println("Unavailable");
    default:
      break;
    }
  }else{
    delay(50);

    float secondPressure = getRealPressure(readPressure());

    //Wait until there's a meaningful difference(i.e. more than 5mmHg) in pressure from initial reading
    while((secondPressure - myCalculatedPressure) < 5){
      secondPressure = getRealPressure(readPressure());
      delay(750);
    }

    //Pump up tp 170
    while (myCalculatedPressure < 170){
      Serial.print("You're at ");
      Serial.println((int)myCalculatedPressure + "mmHg. Keep pumping!");
      delay(1000);
    }
    
    Serial.println("Required pressure reached. Stop pumping!");
    Serial.println("Use the release valve to allow the cuff to deflate at a slow and steady rate(~4mmHg/s).");

    secondPressure = myCalculatedPressure;
    
    delay(1000);
    
    myCalculatedPressure = getRealPressure(readPressure());

    float avgSlope = secondPressure - myCalculatedPressure;
    
    uint8_t numWarnings = 0;
    int32_t diastolic = 0;
    int32_t systolic = 0;
    uint8_t numOscillations = 0;
    uint16_t pressureCount = 0;

    //Only care about 170mmHg and below
    float pressures [MAX_SAMPLES];// = {169.92, 169.65, 169.42, 169.09, 168.84, 168.50, 168.28, 168.01, 167.73, 167.49, 167.17, 166.94, 166.70, 166.43, 166.15, 165.87, 165.60, 165.40, 165.09, 164.87, 164.70, 164.74, 164.70, 164.57, 164.32, 164.02, 163.58, 163.18, 162.85, 162.54, 162.20, 161.93, 161.63, 161.41, 161.15, 160.87, 160.61, 160.40, 160.16, 159.95, 159.76, 159.56, 159.26, 159.01, 158.74, 158.45, 158.23, 157.92, 157.67, 157.39, 157.16, 156.92, 156.65, 156.40, 156.17, 155.91, 155.69, 155.40, 155.21, 154.97, 154.72, 154.49, 154.26, 154.02, 153.77, 153.48, 153.31, 153.01, 152.78, 152.60, 152.52, 152.62, 152.69, 152.59, 152.44, 152.09, 151.66, 151.17, 150.81, 150.50, 150.17, 149.93, 149.69, 149.44, 149.18, 148.96, 148.72, 148.50, 148.36, 148.20, 147.99, 147.82, 147.55, 147.29, 147.07, 146.84, 146.53, 146.28, 146.07, 145.80, 145.62, 145.36, 145.13, 144.87, 144.67, 144.51, 144.22, 143.96, 143.75, 143.57, 143.37, 143.13, 142.97, 142.69, 142.53, 142.27, 142.03, 141.83, 141.58, 141.40, 141.17, 140.94, 140.75, 140.80, 140.96, 141.08, 141.09, 140.94, 140.74, 140.37, 139.98, 139.53, 139.07, 138.76, 138.50, 138.25, 138.00, 137.82, 137.58, 137.45, 137.16, 137.09, 136.97, 136.79, 136.57, 136.41, 136.23, 135.92, 135.71, 135.50, 135.25, 135.03, 134.78, 134.54, 134.36, 134.14, 133.94, 133.74, 133.50, 133.32, 133.12, 132.91, 132.76, 132.56, 132.36, 132.15, 131.94, 131.82, 131.56, 131.37, 131.18, 130.96, 130.69, 130.55, 130.38, 130.16, 130.03, 130.15, 130.38, 130.53, 130.54, 130.44, 130.20, 129.83, 129.42, 129.04, 128.59, 128.25, 127.92, 127.67, 127.42, 127.22, 126.98, 126.83, 126.70, 126.58, 126.47, 126.32, 126.17, 126.03, 125.83, 125.59, 125.35, 125.10, 124.88, 124.62, 124.43, 124.17, 123.96, 123.79, 123.61, 123.47, 123.23, 123.09, 122.91, 122.75, 122.57, 122.40, 122.23, 122.07, 121.85, 121.67, 121.43, 121.31, 121.11, 120.90, 120.71, 120.61, 120.79, 121.08, 121.30, 121.43, 121.36, 121.15, 120.81, 120.51, 120.18, 119.74, 119.38, 119.01, 118.70, 118.42, 118.17, 117.93, 117.74, 117.65, 117.48, 117.39, 117.26, 117.15, 117.01, 116.90, 116.70, 116.49, 116.30, 116.09, 115.84, 115.59, 115.39, 115.17, 115.02, 114.78, 114.64, 114.44, 114.34, 114.15, 113.97, 113.86, 113.69, 113.51, 113.40, 113.20, 113.08, 112.91, 112.74, 112.59, 112.42, 112.23, 112.11, 112.17, 112.51, 112.81, 113.07, 113.08, 112.95, 112.66, 112.38, 112.11, 111.82, 111.51, 111.22, 110.95, 110.66, 110.41, 110.13, 109.81, 109.58, 109.48, 109.34, 109.28, 109.17, 109.08, 108.95, 108.81, 108.57, 108.41, 108.26, 108.01, 107.82, 107.62, 107.39, 107.20, 107.03, 106.84, 106.68, 106.52, 106.33, 106.21, 106.02, 105.95, 105.77, 105.66, 105.58, 105.32, 105.20, 105.06, 104.95, 104.79, 104.59, 104.46, 104.38, 104.53, 104.82, 105.19, 105.48, 105.42, 105.31, 105.09, 104.86, 104.60, 104.40, 104.12, 103.91, 103.69, 103.46, 103.21, 102.96, 102.70, 102.46, 102.29, 102.16, 102.08, 102.01, 101.93, 101.82, 101.60, 101.45, 101.28, 101.07, 100.89, 100.66, 100.49, 100.26, 100.08, 99.89, 99.66, 99.49, 99.33, 99.17, 99.00, 98.85, 98.73, 98.62, 98.43, 98.34, 98.28, 98.10, 97.98, 97.81, 97.77, 97.54, 97.44, 97.33, 97.42, 97.74, 98.14, 98.48, 98.46, 98.34, 98.14, 97.94, 97.74, 97.54, 97.35, 97.12, 96.96, 96.73, 96.55, 96.30, 96.09, 95.89, 95.81, 95.76, 95.67, 95.60, 95.45, 95.37, 95.23, 95.04, 94.88, 94.71, 94.59, 94.38, 94.22, 94.04, 93.83, 93.66, 93.47, 93.27, 93.12, 92.98, 92.81, 92.65, 92.48, 92.39, 92.26, 92.18, 92.03, 91.88, 91.75, 91.56, 91.47, 91.39, 91.46, 91.77, 92.20, 92.35, 92.37, 92.25, 92.05, 91.90, 91.67, 91.47, 91.35, 91.12, 90.96, 90.76, 90.57, 90.38, 90.24, 90.12, 90.04, 89.94, 89.87, 89.83, 89.70, 89.54, 89.38, 89.25, 89.13, 88.94, 88.83, 88.63, 88.50, 88.32, 88.17, 87.99, 87.83, 87.69, 87.57, 87.44, 87.30, 87.15, 86.97, 86.91, 86.75, 86.62, 86.46, 86.32, 86.26, 86.27, 86.52, 86.87, 87.01, 86.97, 86.93, 86.72, 86.59, 86.49, 86.35, 86.23, 86.13, 85.92, 85.83, 85.69, 85.48, 85.30, 85.21, 85.11, 85.02, 84.92, 84.81, 84.72, 84.61, 84.54, 84.37, 84.26, 84.14, 83.99, 83.83, 83.70, 83.57, 83.40, 83.26, 83.13, 83.01, 82.85, 82.74, 82.58, 82.50, 82.31, 82.23, 82.11, 82.00, 81.90, 81.72, 81.62, 81.48, 81.41, 81.46, 81.75, 81.96, 81.91, 81.95, 81.85, 81.74, 81.66, 81.51, 81.37, 81.32, 81.19, 81.06, 80.95, 80.83, 80.66, 80.52, 80.44, 80.36, 80.28, 80.22, 80.08, 79.95, 79.90, 79.77, 79.74, 79.60, 79.44, 79.28, 79.18, 79.10, 78.96, 78.88, 78.72, 78.64, 78.54, 78.34, 78.23, 78.12, 77.98, 77.90, 77.79, 77.71, 77.58, 77.51, 77.39, 77.24, 77.18, 77.03, 76.93, 76.82, 76.72, 76.83, 77.08, 77.19, 77.15, 77.08, 77.01, 76.92, 76.86, 76.74, 76.71, 76.58, 76.49, 76.35, 76.34, 76.19, 76.06, 75.96, 75.87, 75.85, 75.76, 75.64, 75.58, 75.53, 75.38, 75.29, 75.21, 75.16, 75.04, 74.95, 74.82, 74.75, 74.64, 74.54, 74.44, 74.33, 74.23, 74.08, 74.03, 73.94, 73.82, 73.72, 73.62, 73.50, 73.47, 73.41, 73.27, 73.19, 73.08, 73.03, 72.93, 72.85, 72.69, 72.63, 72.63, 72.81, 72.98, 72.93, 72.91, 72.86, 72.79, 72.73, 72.63, 72.60, 72.52, 72.43, 72.35, 72.30, 72.20, 72.09, 71.97, 71.91, 71.90, 71.82, 71.81, 71.75, 71.65, 71.61, 71.52, 71.49, 71.39, 71.32, 71.27, 71.17, 71.08, 70.99, 70.94, 70.87, 70.78, 70.67, 70.61, 70.45, 70.35, 70.31, 70.19, 70.05, 70.01, 69.95, 69.81, 69.73, 69.62, 69.52, 69.45, 69.37, 69.23, 69.16, 69.10, 69.09, 69.30, 69.32, 69.30, 69.26, 69.21, 69.15, 69.09, 68.98, 68.94, 68.83, 68.80, 68.72, 68.67, 68.50, 68.36, 68.24, 68.19, 68.13, 68.08, 67.96, 67.90, 67.84, 67.72, 67.62, 67.56, 67.46, 67.37, 67.23, 67.13, 67.09, 66.97, 66.85, 66.75, 66.64, 66.54, 66.45, 66.34, 66.23, 66.13, 66.02, 65.97, 65.84, 65.76, 65.69, 65.58, 65.51, 65.35, 65.27, 65.22, 65.22, 65.30, 65.42, 65.38, 65.36, 65.33, 65.25, 65.26, 65.13, 65.11, 65.10, 64.99, 64.94, 64.85, 64.76, 64.70, 64.56, 64.45, 64.38, 64.31, 64.27, 64.18, 64.11, 63.99, 63.94, 63.82, 63.77, 63.64, 63.59, 63.51, 63.42, 63.32, 63.25, 63.10, 62.99, 62.91, 62.79, 62.72, 62.61, 62.50, 62.42, 62.36, 62.31, 62.10, 62.06, 62.02, 61.94, 61.88, 61.78, 61.64, 61.57, 61.49, 61.52, 61.62, 61.67, 61.73, 61.68, 61.67, 61.65, 61.56, 61.55, 61.44, 61.40, 61.33, 61.27, 61.24, 61.13, 61.02, 60.88, 60.83, 60.74, 60.69, 60.67, 60.57, 60.52, 60.42, 60.37, 60.30, 60.22, 60.08, 60.02, 59.93, 59.89, 59.76, 59.67, 59.59, 59.51, 59.40, 59.28, 59.20, 59.22, 58.98, 58.93, 58.85, 58.73, 58.70, 58.62, 58.58, 58.41, 58.40, 58.29, 58.20, 58.14, 58.01, 57.98, 57.93, 58.12, 58.19, 58.19, 58.15, 58.12, 58.11, 58.11, 58.05, 57.95, 57.93, 57.90, 57.83, 57.73, 57.71, 57.61, 57.50, 57.42, 57.34, 57.26, 57.24, 57.13, 57.14, 57.05, 56.92, 56.90, 56.84, 56.78, 56.68, 56.61, 56.52, 56.45, 56.39, 56.36, 56.21, 56.14, 56.06, 56.02, 55.90, 55.77, 55.73, 55.67, 55.66, 55.49, 55.43, 55.36, 55.27, 55.28, 55.13, 55.12, 55.05, 54.99, 54.93, 54.80, 54.79, 54.81, 54.94, 55.01, 55.01, 55.01, 54.99, 54.94, 54.90, 54.83, 54.79, 54.76, 54.65, 54.58, 54.54, 54.48, 54.38, 54.32, 54.28, 54.20, 54.15, 54.10, 54.15, 54.04, 53.97, 53.92, 53.89, 53.83, 53.78, 53.67, 53.66, 53.56, 53.53, 53.45, 53.38, 53.27, 53.21, 53.13, 53.03, 52.97, 52.87, 52.83, 52.72, 52.74, 52.60, 52.52, 52.47, 52.39, 52.31, 52.24, 52.18, 52.07, 52.10, 52.22, 52.28, 52.25, 52.23, 52.17, 52.21, 52.18, 52.06, 51.94, 51.88, 51.88, 51.76, 51.69, 51.61, 51.52, 51.46, 51.33, 51.34, 51.29, 51.23, 51.14, 51.13, 51.11, 51.03, 50.95, 50.87, 50.77, 50.71, 50.62, 50.57, 50.53, 50.46, 50.35, 50.29, 50.20, 50.13, 50.03, 49.96, 49.90, 49.78, 49.75, 49.65, 49.55, 49.50, 49.42, 49.39, 49.37, 49.43, 49.55, 49.57, 49.53, 49.52, 49.54, 49.51, 49.49, 49.37, 49.31, 49.26, 49.23, 49.11, 49.08, 49.00, 48.93, 48.83, 48.76, 48.70, 48.67, 48.63, 48.61, 48.56, 48.46, 48.43, 48.40, 48.35, 48.26, 48.20, 48.06, 48.04, 48.00, 47.92, 47.81, 47.75, 47.72, 47.64, 47.55, 47.45, 47.42, 47.34, 47.28, 47.16, 47.11, 47.05, 46.99, 47.06, 47.08, 47.17, 47.17, 47.22, 47.26, 47.20, 47.12, 47.07, 47.10, 47.03, 46.99, 46.92, 46.85, 46.75, 46.69, 46.64, 46.64, 46.51, 46.44, 46.41, 46.40, 46.33, 46.28, 46.27, 46.25, 46.14, 46.10, 46.05, 45.95, 45.87, 45.82, 45.76, 45.65, 45.60, 45.61, 45.46, 45.41, 45.32, 45.27, 45.24, 45.21, 45.09, 45.05, 44.99, 44.90, 44.84, 44.78, 44.85, 44.92, 45.05, 45.01, 44.98, 45.03, 44.97, 44.94, 44.89, 44.84, 44.77, 44.75, 44.71, 44.67, 44.56, 44.49, 44.45, 44.43, 44.35, 44.32, 44.33, 44.28, 44.19, 44.18, 44.07, 44.11, 44.04, 44.02, 43.94, 43.87, 43.79, 43.75, 43.72, 43.64, 43.56, 43.53, 43.43, 43.42, 43.36, 43.34, 43.25, 43.32, 43.20, 43.07, 43.06, 43.00, 42.98, 43.01, 43.06, 43.15, 43.14, 43.16, 43.16, 43.13, 43.09, 43.03, 43.00, 43.01, 42.93, 42.89, 42.84, 42.75, 42.68, 42.56, 42.56, 42.54, 42.52, 42.51, 42.48, 42.44, 42.40, 42.38, 42.36, 42.29, 42.28, 42.23, 42.18, 42.09, 42.09, 42.03, 42.00, 41.91, 41.86, 41.86, 41.83, 41.70, 41.76, 41.60, 41.65, 41.59, 41.53, 41.45, 41.39, 41.47, 41.53, 41.56, 41.55, 41.51, 41.50, 41.48, 41.41, 41.36, 41.29, 41.28, 41.18, 41.21, 41.07, 41.01, 40.94, 40.91, 40.87, 40.80, 40.82, 40.74, 40.67, 40.68, 40.62, 40.55, 40.52, 40.40, 40.36, 40.33, 40.25, 40.22, 40.17, 40.11, 40.02, 39.95, 39.91, 39.85, 39.79, 39.72, 39.67, 39.64, 39.57, 39.58, 39.62, 39.73, 39.65, 39.71, 39.64, 39.66, 39.60, 39.54, 39.52, 39.52, 39.43, 39.42, 39.35, 39.34, 39.23, 39.16, 39.15, 39.16, 39.09, 39.02, 39.01, 38.96, 38.91, 38.92, 38.84, 38.79, 38.74, 38.69, 38.65, 38.56, 38.51, 38.50, 38.46, 38.42, 38.32, 38.28, 38.24, 38.17, 38.12, 38.07, 38.01, 37.94, 37.88, 37.84, 37.86, 37.96, 37.97, 38.03, 38.02, 37.97, 37.97, 37.98, 37.90, 37.84, 37.80, 37.74, 37.75, 37.69, 37.61, 37.55, 37.50, 37.44, 37.48, 37.39, 37.36, 37.40, 37.31, 37.25, 37.18, 37.19, 37.10, 37.13, 37.03, 37.01, 36.94, 36.86, 36.81, 36.78, 36.75, 36.69, 36.64, 36.71, 36.56, 36.50, 36.47, 36.43, 36.36, 36.36, 36.29, 36.24, 36.24, 36.18, 36.12, 36.09, 36.05, 36.00

    //Only measure til we reach 40
    while(myCalculatedPressure > 40){

      //check slope every 50 measurements(1 second) and print warning if too high above or too low below 4
      if(pressureCount % 50 == 0){
        if(avgSlope > 6){
          Serial.println("Pressure being released too fast!");
          numWarnings++;
        }else if (avgSlope < 2){
          Serial.println("Pressure being released too slow!");
          numWarnings++;
        }else{
          Serial.println("Keep it steady at this rate.");
        }
      }
      
      //bounds check, if too many samples taken already, stop storing samples
      //we will calculate BP with first 3000 samples taken
      if(pressureCount < MAX_SAMPLES){
        pressures[pressureCount] = myCalculatedPressure;
        pressureCount++;
      
        //update second pressure and slope every 50 measurements
        if(pressureCount % 50 == 0){ 
          avgSlope = secondPressure - myCalculatedPressure;
          secondPressure = myCalculatedPressure;
        }
        
      }

      //take next measurement
      delay(15);
      myCalculatedPressure = getRealPressure(readPressure());
    }

    //pressureCount = 3000 - 1678; sample pressure data size

    if(numWarnings > 5){ //If too many warnings, we won't be able to calculate accurate values
      Serial.println("Pressure was released too fast or slow. Start over.");
    }else{
      float averages [MAX_SAMPLES]; //Our rolling average array
      float tempSum = 0;

      for(int i = 0; i < 5; i++){
        tempSum += pressures[i];
        averages[i] = pressures[i];
      }

      //get rolling average of 5 using "sliding window" algorithm
      for(uint16_t i = 5; i < pressureCount; i++){
        tempSum -= pressures[i - 5];
        tempSum += pressures[i];
        averages[i] = tempSum/5;
      }

      float differences [MAX_SAMPLES]; //Our oscillation array

      //Set the max oscillation to the first difference between measurement and rolling average
      float maxOscillation = 0.7 + pressures[5] - averages[5];

      //Find max oscillation
      for(uint16_t i = 5; i < pressureCount; i++){
        //Adding 0.7 to get the minimum of the elements in differences to be about 0, otherwise the 
        //percentage math gets messed up(for SBP and DBP) because of the negative differences
        differences[i] = 0.7 + pressures[i] - averages[i];
        if(differences[i] > maxOscillation){
          maxOscillation = differences[i];
        }
      }

      //Find DBP and SBP
      bool foundSystolic = false;
      for(uint16_t i = 5; i < pressureCount; i++){
        if((differences[i] > (0.78 * maxOscillation)) && (differences[i] < (0.83 * maxOscillation))){
          diastolic = pressures[i];
        }else if((differences[i] > (0.48 * maxOscillation)) && (differences[i] < (0.53 * maxOscillation)) && !foundSystolic){
          systolic = pressures[i];
          foundSystolic = true;
        }
      }

      //Find average heart rate by finding the indices of the first and last oscillations and the amount of oscillations between
      float oscillationMin = 1;
      bool foundFirst = false;
      int firstIndex = 0;
      int lastIndex = 0;
      for(uint16_t i = 5; i < pressureCount; i++){
        if(differences[i] > oscillationMin){
          if(!foundFirst){
            firstIndex = i;
            foundFirst = true;
          }
          lastIndex = i;
          numOscillations++;
        }
      }

      numOscillations /= 2.6;//TODO We have to correct for measuring the same oscillation multiple times
      float timeDiff = lastIndex - firstIndex;
      float heartRate = numOscillations/timeDiff; //this is average beats per 20ms(because we sampled at 50Hz)
      heartRate *= 50; //beats per second
      heartRate *= 60; //beats per minute


      Serial.print("Your blood pressure is ");
      Serial.print(systolic);
      Serial.print("/");
      Serial.println(diastolic);
      Serial.print("Your heart rate is ");
      Serial.print((int)heartRate);
      Serial.println(" BPM");
    }
  }
}
