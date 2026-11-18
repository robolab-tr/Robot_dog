#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// Inicializace PCA9685 na výchozí I2C adrese 0x40
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

// Nastavení frekvence PWM pro servomotory (standard je 50 Hz)
#define SERVO_FREQ 50 

/**
 * Otočí zadané servo na požadovaný úhel.
 * 
 * @param servoNum   Číslo kanálu na PCA9685 (0 až 15)
 * @param uhel       Cílový úhel ve stupních
 * @param minStupne  Minimální úhel serva (např. 0)
 * @param maxStupne  Maximální úhel serva (např. 180)
 */
void otocServo(uint8_t servoNum, int uhel, int minStupne, int maxStupne) {
  // 1. Omezení rozsahu úhlu, aby servo nejelo mimo své fyzické hranice
  uhel = constrain(uhel, minStupne, maxStupne);

  // 2. Přepočet stupňů na délku impulsu v mikrosekundách (µs)
  // Standardní serva mají 0° při ~1000 µs a 180° při ~2000 µs (případně 500-2500 µs dle typu)
  int minUs = 500;
  int maxUs = 2500;
  
  int us = map(uhel, minStupne, maxStupne, minUs, maxUs);

  // 3. Přepočet mikrosekund na hodnotu "ticks" pro PCA9685 (12bit = 4096 kroků na periodu 20ms)
  // Perioda při 50Hz = 20 000 µs. 1 tick = 20000 / 4096 = ~4.88 µs
  uint16_t pulseLen = (uint32_t)us * 4096 / (1000000 / SERVO_FREQ);

  // 4. Odeslání signálu na dané servo
  pwm.setPWM(servoNum, 0, pulseLen);
}

void setup() {
  Serial.begin(9600);
  pwm.begin();
  pwm.setPWMFreq(SERVO_FREQ);
  delay(10);
}

void loop() {
  // Příklad 1: Otočí servo na kanálu 0 na 90 stupňů (rozsah 0 až 180°)
  // kanál - otočení_ve_stupních - rozsah_min - rozsah_max
  otocServo(0,15,0,180); //1
  delay(1000);
  otocServo(1,30,0,180); //2
  delay(1000);
  otocServo(2,45,0,180); //3
  delay(1000);
  otocServo(3,60,0,180); //4
  delay(1000);
  otocServo(4,75,0,180); //5
  delay(1000);
  otocServo(5,90,0,180); //6
  delay(1000);
  otocServo(6,105,0,180); //7
  delay(1000);
  otocServo(7,120,0,180); //8
  delay(1000);
  otocServo(8,135,0,180); //9
  delay(1000);
  otocServo(9,150,0,180); //10
  delay(1000);
  otocServo(10,165,0,180); //11
  delay(1000);
  otocServo(11,180,0,180); //12
  delay(1000);

}
