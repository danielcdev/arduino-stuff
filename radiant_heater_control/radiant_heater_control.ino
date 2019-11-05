#include <Servo.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

#define THERMISTOR_READ_PIN A0
#define THERMISTOR_POWER_PIN 2
#define SUPPIED_VOLTAGE 5.0
#define RESISTOR_OHMS_K 9.2
#define TARGET_TEMPERATURE 20
#define TARGET_VARIANCE 0.3
#define SERVO_PIN 3

Servo knobTurner;
int pos = 0;
const float lowerTarget = TARGET_TEMPERATURE - TARGET_VARIANCE;

ISR (WDT_vect) {
  wdt_disable();
}

void configureWatchdog() {
  cli();
  MCUSR = 0;
  WDTCSR = bit (WDCE) | bit (WDE);
  WDTCSR = bit (WDIE) | bit (WDP3) | bit (WDP0);
  sei();
}

void sleep(unsigned int cycles) {
  configureWatchdog();
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  power_adc_disable();
  
  for (unsigned int i = 0; i < cycles; i++) {
    sleep_mode();
    sleep_disable();
  }

  power_all_enable();
}

int readThermistor() {
  int analogReading = 0;
  
  digitalWrite(THERMISTOR_POWER_PIN, HIGH);
  analogReading = analogRead(THERMISTOR_READ_PIN);
  digitalWrite(THERMISTOR_POWER_PIN, LOW);

  return analogReading;
}

float readTemperature() {
  float inputVoltage = ((readThermistor() * 5) / 1023.0);
  float thermistor_resistance = ((SUPPIED_VOLTAGE * ( RESISTOR_OHMS_K / inputVoltage)) - 10);
  thermistor_resistance = log(thermistor_resistance * 1000);
  
  float temperature = (1 / (0.001129148 + (0.000234125 * thermistor_resistance) + (0.0000000876741 * thermistor_resistance * thermistor_resistance * thermistor_resistance)));
  temperature = temperature - 273.15;

  return temperature;
}

void setServoQuiet(int value) {
  for (; pos > value; pos--) {
    knobTurner.write(pos);
    delay(100);
  }

  for (; pos < value; pos++) {
    knobTurner.write(pos);
    delay(100);
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(THERMISTOR_POWER_PIN, OUTPUT);
  
  knobTurner.attach(SERVO_PIN);
  knobTurner.write(pos);
  
  delay(500);
}

void loop() {
  float currentTemperature = readTemperature();

  if (currentTemperature >= TARGET_TEMPERATURE && pos > 0)
    setServoQuiet(0);
  else if (currentTemperature < lowerTarget && pos < 180)
    setServoQuiet(180);
    
  sleep(4);
}
