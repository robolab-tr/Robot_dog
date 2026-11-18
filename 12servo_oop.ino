#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>

// Nastavení frekvence PWM pro servomotory (standard je 50 Hz)
#define SERVO_FREQ 50

// Inicializace PCA9685 na výchozí I2C adrese 0x40
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

/**
 * Třída reprezentující jedno servo připojené k PCA9685.
 * Každé servo má vlastní: kanál, úhel, minimální a maximální úhel.
 */
class ServoMotor {
  public:
    /**
     * @param driver     Reference na PCA9685 driver
     * @param channel    Číslo kanálu na PCA9685 (0 až 15)
     * @param angle      Počáteční / cílový úhel ve stupních
     * @param minAngle   Minimální úhel serva (např. 0)
     * @param maxAngle   Maximální úhel serva (např. 180)
     */
    ServoMotor(Adafruit_PWMServoDriver &driver, uint8_t channel,
               int angle = 0, int minAngle = 0, int maxAngle = 180)
      : _driver(driver), _channel(channel),
        _minAngle(minAngle), _maxAngle(maxAngle),
        _angle(constrain(angle, minAngle, maxAngle)) {}

    // ---- Settery (všechny 4 hodnoty) ----

    void setChannel(uint8_t channel) { _channel = channel; }

    // Nastaví úhel a hned servo otočí
    void setAngle(int angle) {
      _angle = constrain(angle, _minAngle, _maxAngle);
      apply();
    }

    void setMinAngle(int minAngle) {
      _minAngle = minAngle;
      _angle = constrain(_angle, _minAngle, _maxAngle);
    }

    void setMaxAngle(int maxAngle) {
      _maxAngle = maxAngle;
      _angle = constrain(_angle, _minAngle, _maxAngle);
    }

    // Nastaví oba limity najednou
    void setRange(int minAngle, int maxAngle) {
      _minAngle = minAngle;
      _maxAngle = maxAngle;
      _angle = constrain(_angle, _minAngle, _maxAngle);
    }

    // ---- Gettery ----

    uint8_t getChannel() const { return _channel; }
    int getAngle() const { return _angle; }
    int getMinAngle() const { return _minAngle; }
    int getMaxAngle() const { return _maxAngle; }

    // Odešle aktuální úhel na servo
    void apply() {
      // Přepočet stupňů na délku impulsu v µs (500-2500 µs dle typu serva)
      const int minUs = 500;
      const int maxUs = 2500;
      int us = map(_angle, _minAngle, _maxAngle, minUs, maxUs);

      // Přepočet µs na "ticks" (12bit = 4096 kroků na periodu 20 ms)
      uint16_t ticks = (uint32_t)us * 4096 / (1000000 / SERVO_FREQ);

      _driver.setPWM(_channel, 0, ticks);
    }

  private:
    Adafruit_PWMServoDriver &_driver;
    uint8_t _channel;
    int _minAngle, _maxAngle;
    int _angle;
};

// Pole 12 serv: (driver, kanál, úhel, min, max) - stejné pořadí jako dřív
const uint8_t SERVO_COUNT = 12;
ServoMotor servos[SERVO_COUNT] = {
  ServoMotor(pwm, 0,  15,  0, 180),
  ServoMotor(pwm, 1,  30,  0, 180),
  ServoMotor(pwm, 2,  45,  0, 180),
  ServoMotor(pwm, 3,  60,  0, 180),
  ServoMotor(pwm, 4,  75,  0, 180),
  ServoMotor(pwm, 5,  90,  0, 180),
  ServoMotor(pwm, 6,  105, 0, 180),
  ServoMotor(pwm, 7,  120, 0, 180),
  ServoMotor(pwm, 8,  135, 0, 180),
  ServoMotor(pwm, 9,  150, 0, 180),
  ServoMotor(pwm, 10, 165, 0, 180),
  ServoMotor(pwm, 11, 180, 0, 180)
};

void setup() {
  Serial.begin(9600);
  pwm.begin();
  pwm.setPWMFreq(SERVO_FREQ);
  delay(10);
}

void loop() {
  // Každé servo se otočí na svůj úhel uložený v objektu
  for (uint8_t i = 0; i < SERVO_COUNT; i++) {
    servos[i].apply();
    delay(1000);
  }

  // Příklad změny hodnot za běhu:
  // servos[0].setChannel(12);
  // servos[0].setRange(20, 160);
  // servos[0].setAngle(90);
}
