/* ====================================================
 *  DRV8833 Library
 *    Author: luisf18 (github)
 *    Ver.: 0.0.2
 *    last_update: 23/07/2026
 * ====================================================
 */

#ifndef DRV8833_H
#define DRV8833_H

#include <Arduino.h>

typedef struct {
  int16_t vl=0, vr=0;
  unsigned long dt=0;
} move_t;

class DRV8833 {
  private:
    uint8_t  pins[4];
    uint32_t PWM_HZ  = 25000;
    uint8_t  PWM_RES = 10;
    uint16_t PWM_MAX = 1023;

    // Sound
    uint16_t SOUND_VOL = 15;

    // Movement
    bool     _is_moving    = false;
    uint32_t _move_timeout = 0;
    size_t   _move_count   = 0;
    move_t   _next_move;
    move_t  *_move_list; //!!! onde isso se inicializa? onde se destrói?

    int16_t _vels[2] = {0, 0};

  public:
    // ===========================================================
    // SETUP
    // ===========================================================

    DRV8833(uint8_t in1, uint8_t in2, uint8_t in3, uint8_t in4) {
      pins[0] = in1; pins[1] = in2; pins[2] = in3; pins[3] = in4;
    }

    void begin() { begin(PWM_HZ, PWM_RES); }

    void begin(uint32_t f_Hz, uint8_t res) {
      pinMode(pins[0], OUTPUT);
      pinMode(pins[1], OUTPUT);
      pinMode(pins[2], OUTPUT);
      pinMode(pins[3], OUTPUT);

      Serial.println("[x] DRV8833 Begin");
      Serial.printf(" | - Pins: [ %d, %d, %d, %d ]\n", pins[0], pins[1], pins[2], pins[3]);
      Serial.printf(" | - PWM: [ Range: 0 to %d ] [ %d bits / %d Hz ]\n",
                    PWM_MAX, PWM_RES, PWM_HZ);

      init_pwm(f_Hz, res); //! checar retorno
      stop();
    }

    uint8_t init_pwm(uint32_t HZ, uint8_t RES) {
      #ifdef ESP32
        if ((RES > 0) && (RES <= 12)) PWM_RES = RES;
        PWM_MAX = (1 << PWM_RES) - 1;
        bool ok = ledcAttach(pins[0], HZ, PWM_RES);
        PWM_HZ = ok ? HZ : PWM_HZ;

        bool ok0 = ledcAttach(pins[0], PWM_HZ, PWM_RES);
        bool ok1 = ledcAttach(pins[1], PWM_HZ, PWM_RES);
        bool ok2 = ledcAttach(pins[2], PWM_HZ, PWM_RES);
        bool ok3 = ledcAttach(pins[3], PWM_HZ, PWM_RES);

        Serial.printf(" | - ledcAttach: [pino %d: %s] [pino %d: %s] [pino %d: %s] [pino %d: %s]\n",
                      pins[0], ok0 ? "OK" : "FALHOU",
                      pins[1], ok1 ? "OK" : "FALHOU",
                      pins[2], ok2 ? "OK" : "FALHOU",
                      pins[3], ok3 ? "OK" : "FALHOU");
        //! retornar erro e mover print pro begin
      #else
        if (RES >= 4 && RES <= 16) PWM_RES = RES;
        PWM_MAX = (1 << PWM_RES) - 1;
        analogWriteResolution(PWM_RES);

        if (HZ >= 100 && HZ <= 60000) PWM_HZ = HZ;
        analogWriteFreq(PWM_HZ);
      #endif
      //! lidar melhor com valores ruins

      Serial.printf("PWM: [ Range: 0 to %d ] [ %d bits / %d Hz ]\n",
                    PWM_MAX, PWM_RES, PWM_HZ);

      return (true | (PWM_HZ == HZ) << 1 | (PWM_RES == RES) << 2);
    }

    int16_t read(uint8_t n) { return (n > 1 ? 0 : _vels[n]); }
    int16_t range()         { return PWM_MAX; }

    // ===========================================================
    //  ALTO NÍVEL — ACIONAMENTO DIFERENCIAL
    // ===========================================================

    // Controle diferencial clássico (linear/angular)
    // entrada em unidades PWM, ou em µs de rádio (com radio_range=true)
    void diff_drive(int16_t linear, int16_t angular, bool invert, bool radio_range) {
      if (radio_range) {
        linear  = map(linear,  1000, 2000, -PWM_MAX, PWM_MAX);
        angular = map(angular, 1000, 2000, -PWM_MAX, PWM_MAX);
      }
      int16_t vel_l = constrain(linear + angular, -PWM_MAX, PWM_MAX);
      int16_t vel_r = constrain(linear - angular, -PWM_MAX, PWM_MAX);

      if (invert) move(-vel_r, -vel_l);
      else        move( vel_l,  vel_r);
    }

    // ===========================================================
    //  MOVIMENTOS COM TIMER (não bloqueantes)
    //  ATENÇÃO: chame update() no loop() para que funcionem!
    // ===========================================================

    void clear_moving() { _is_moving = false; _move_count = 0; }

    // Move indefinidamente
    void move(move_t m) {
      move(m.vl, m.vr);
      _move_timeout = millis() + m.dt;
      _is_moving = true;
    }

    // Executa uma lista de movimentos em sequência
    void move(move_t *m, size_t len) {
      move(*m);
      _move_list  = m + 1;
      _move_count = len - 1;
    }

    // Move por 'duration' ms e para em seguida
    void move_for(int16_t vl, int16_t vr, uint32_t duration) {
      move(vl, vr);
      _is_moving = true;
      _move_count = 0;
      _move_timeout = millis() + duration;
    }

    // Move por 'duration' ms e depois executa um segundo movimento
    void move_for_then(int16_t vl,      int16_t vr,      uint32_t duration,
                       int16_t vl_next, int16_t vr_next, uint32_t duration_next) {
      _is_moving = true;
      _move_timeout = millis() + duration;
      move(vl, vr);

      //!!! isso vaza uma referência a um ponteiro de pilha
      _move_count = 1;
      _next_move.vl = vl;
      _next_move.vr = vr;
      _next_move.dt = duration_next;
      _move_list    = &_next_move;
    }

    // Deve ser chamado no loop() para processar os timers
    int update() {
      if (!_is_moving) return -1;

      if (millis() >= _move_timeout) {
        _is_moving = false;
        if (_move_count) {
          _is_moving = true;
          _move_count--;
          _move_timeout += _move_list->dt;

          if (abs(_move_list->vl) <= PWM_MAX) write(_move_list->vl, 0); else stop(0);
          if (abs(_move_list->vr) <= PWM_MAX) write(_move_list->vr, 1); else stop(1);
          _move_list++;
        }
      }

      return (_is_moving + _move_count);
    }

    // ===========================================================
    //  BAIXO NÍVEL — MOVIMENTOS DIRETOS E ESCRITA PWM
    // ===========================================================

    void move   (int16_t vl, int16_t vr) { write   (vl, 0); write   (vr, 1); }
    void moveRaw(int16_t vl, int16_t vr) { writeRaw(vl, 0); writeRaw(vr, 1); }

    // Freio ativo (curto-circuita os terminais — para mais rápido)
    void stop() { stop(0); stop(1); }

    void stop(uint8_t motor) {
      if (motor > 1) return;
      #ifdef ESP32
        ledcWrite(pins[0 + 2*motor], PWM_MAX);
        ledcWrite(pins[1 + 2*motor], PWM_MAX);
      #else
        analogWrite(pins[0 + 2*motor], PWM_MAX);
        analogWrite(pins[1 + 2*motor], PWM_MAX);
      #endif
      _vels[motor] = 0;
    }

    // Corte de energia (motor desacelera por inércia)
    void off()              { off(0); off(1); }
    void off(uint8_t motor) { write(motor, 0); }

    // Escrita com sinal de velocidade salvo em _vels[]
    void write(int16_t vel, uint8_t motor) {
      if (motor > 1) return;
      writeRaw(vel, motor);
      _vels[motor] = vel;
    }

    // Escrita raw (sem salvar em _vels[])
    void writeRaw(int16_t vel, uint8_t motor) {
      if (motor > 1) return;
      const int m = 2*motor;
      #ifdef ESP32
        if (vel >= 0) { ledcWrite(pins[0+m],  vel); ledcWrite(pins[1+m], 0); }
        else          { ledcWrite(pins[1+m], -vel); ledcWrite(pins[0+m], 0); }
      #else
        if (vel >= 0) { analogWrite(pins[0+m],  vel); analogWrite(pins[1+m], 0); }
        else          { analogWrite(pins[1+m], -vel); analogWrite(pins[0+m], 0); }
      #endif
    }

    // ===========================================================
    //  SOM — uso dos motores como buzzer
    // ===========================================================

    void     sound_vol(uint8_t vol) { if (vol <= 25) SOUND_VOL = vol; }
    uint16_t sound_duty()           { return SOUND_VOL * (PWM_MAX / 100); }

    // Emite n bipes em ambos os motores
    void bip(uint8_t n, uint16_t dt, uint32_t tone) { //! unificar com bip(motor)
                                                      //! bip->beep/bipe
      sound_tone(tone);
      uint16_t duty = sound_duty();
      for (int i = 0; i < n; i++) {
        moveRaw(duty, duty); delay(dt);
        moveRaw(0,    0);    delay(dt);
      }
      sound_stop();
    }

    // Emite n bipes em um motor específico
    void bip(uint8_t n, uint16_t dt, uint32_t tone, uint8_t motor) {
      if (motor > 1) return;
      sound_tone(tone, motor);
      uint16_t duty = sound_duty();
      for (int i = 0; i < n; i++) {
        writeRaw(duty, motor); delay(dt);
        writeRaw(0,    motor); delay(dt);
      }
      sound_stop(motor);
    }

    void sound_tone(uint32_t tone) { sound_tone(tone, 0); sound_tone(tone, 1); }

    void sound_tone(uint32_t tone, uint8_t motor) {
      if (motor > 1) return;
      #ifdef ESP32
        ledcAttach(pins[0 + 2*motor], tone, PWM_RES);
        ledcAttach(pins[1 + 2*motor], tone, PWM_RES);
      #else
        analogWriteFreq(tone);
      #endif
      writeRaw(sound_duty(), motor);
    }

    void sound_stop() { sound_stop(0); sound_stop(1); }

    void sound_stop(uint8_t motor) {
      if (motor > 1) return;
      #ifdef ESP32
        ledcAttach(pins[0 + 2*motor], PWM_HZ, PWM_RES);
        ledcAttach(pins[1 + 2*motor], PWM_HZ, PWM_RES);
      #else
        analogWriteFreq(PWM_HZ);
      #endif
      write(_vels[motor], motor);
    }
};

#endif // DRV8833_H
