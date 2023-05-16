#include "wiringPi.h"
#include <softPwm.h>
#include <iostream>

#define pwmPinA 25             // 모터드라이d버 ENA - GPIO핀 번호: 12
#define AIN1 22            // IN1 - GPIO핀 번호: 16
#define AIN2 21            // IN2 - GPIO핀 번호 : 25 
#define encPinA 29           // 보라색 (A) - GPIO핀 번호 : 21
#define encPinB 0           // 파랑색 (B) - GPIO핀 번호 : 24

#define pwmPinB 25           // 모터 드라이버 ENB - GPIO핀 번호 : 26    
#define BIN3 22           // IN3 - GPIO핀 번호 : 6
#define BIN4 21           // IN4 - GPIO핀 번호 : 5
#define encPinC 3            // 보라색 (C) - GPIO핀 번호 : 22
#define encPinD 6            // 파랑색 (D) - GPIO핀 번호 : 24

volatile int pulse_countA = 0;
volatile int pulse_countB = 0;

void pulse_callbackA() {
    if(pulse_countA < 11){
        pulse_countA++;
    }
    //std::cout << "pulse" << std::endl;
}

void pulse_callbackB() {
    if(pulse_countB < 11){
        pulse_countB++;
    }
    std::cout << "pulse" << std::endl;
      
}

int main() {
    wiringPiSetup();

    if (wiringPiSetup() == -1) {
        std::cerr << "WiringPi 초기화 실패" << std::endl;
        return 1;
    }

    pinMode(encPinA, INPUT);
    pullUpDnControl(encPinA, PUD_UP);
    pinMode(encPinB, INPUT);
    pullUpDnControl(encPinB, PUD_UP);

    pinMode(pwmPinA, OUTPUT); 
    pinMode(AIN1, OUTPUT);
    pinMode(AIN2, OUTPUT);

    softPwmCreate(pwmPinA, 0, 100);
    softPwmWrite(pwmPinA, 20);

    digitalWrite(pwmPinA, LOW);
    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, HIGH);
    
    wiringPiISR(encPinA, INT_EDGE_RISING, &pulse_callbackA);
    wiringPiISR(encPinB, INT_EDGE_RISING, &pulse_callbackB);

    while (1) {
        delay(100);

        if (pulse_countA == 10){

            softPwmWrite(pwmPinA, 0);   
            pulse_countA = 0;    
          //  std::cout << "Pulse CountA: " << pulse_countA << std::endl; 
        }
    }

    return 0;
}