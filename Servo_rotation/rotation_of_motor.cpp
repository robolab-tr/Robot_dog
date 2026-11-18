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
  otocServo(0, 90, 0, 180);
  delay(1000);
  otocServo(0,0,0,180);
  delay(1000);

}
