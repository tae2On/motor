/* PID 제어 */

#include "wiringPi.h"                   // analogRead(), pinMode(), delay() 함수 등 사용 
#include <softPwm.h>
#include <iostream>                    // C++ 입출력 라이브러리
#include <thread>
#include <chrono>
#include <ctime>
#include <string>
#include <unistd.h>
#include <algorithm>
#include <math.h>

#define M_PI 3.14159265358979323846
using namespace std;

/* 핀 번호가 아니라 wiringPi 번호 ! */
/* gpio readall -> GPIO핀 / wiringPi핀 번호 확인법 */
/* ex) 핀 번호 8번, GPIO 14번, wiringPi 15번 */

// DC 모터 왼쪽 (엔코더 O)                                                      
#define pwmPinB 15              // 모터드라이버 ENA - GPIO핀 번호: 14 
#define BIN1 16                // IN1 - GPIO핀 번호: 15
#define BIN2 1                 // IN2 - GPIO핀 번호 : 18
#define encPinA 8               // 보라색 (A) - GPIO핀 번호 : 2
#define encPinB 9               // 파랑색 (B) - GPIO핀 번호 : 3

/* PID 제어 */
const float proportion = 360. / (84 * 10);       // 한 바퀴에 약 1350펄스 (정확하지 않음 - 계산값)

/* PID 상수 */
float kp = 0.; 
float kd = 0.;         
float ki = 0.;

float encoderPosRight;             // 엔코더 값 - 오른쪽

float motorDegB = 0;                   // 모터 각도B
float target_deg = 360.;

float errorB = 0; 
float error_prev_B = 0.;
float error_prev_prev_B = 0;

double controlB = 0.;
double tolerance = 0.1;

double de_B;
double di_B = 0;
double dt = 0;

double delta_vB;
double time_prev = 0;
double dt_sleep = 0.01;


std::time_t start_time = std::time(nullptr);

// 인터럽트 
void doEncoderB() {
  encoderPosRight  += (digitalRead(encPinA) == digitalRead(encPinB)) ? 1 : -1;
}
void doEncoderB() {
  encoderPosRight  += (digitalRead(encPinA) == digitalRead(encPinB)) ? -1 : 1;
}

int main(){
    wiringPiSetup();

    pinMode(encPinA, INPUT);
    pullUpDnControl(encPinA, PUD_UP);
    pinMode(encPinB, INPUT);
    pullUpDnControl(encPinB, PUD_UP);
    pinMode(pwmPinB, OUTPUT); 
    pinMode(BIN1, OUTPUT);
    pinMode(BIN2, OUTPUT);

    softPwmCreate(pwmPinB, 0, 255);
    softPwmWrite(pwmPinB, 0);

    digitalWrite(pwmPinB, LOW);
    digitalWrite(BIN1, LOW);
    digitalWrite(BIN2, LOW);

    wiringPiISR(encPinB, INT_EDGE_BOTH, &doEncoderB);

    // PID 상수 입력
    printf("kp: ");
    scanf("%f", &kp);
    printf("ki: ");
    scanf("%f", &ki);
    printf("kd: ");
    scanf("%f", &kd);

    while(true) {
        motorDegB = encoderPosRight * proportion;
        errorB = target_deg - motorDegB;
        de_B = errorB -error_prev_B;
        di_B += errorB * dt;
        dt = time(nullptr) - time_prev;
        
        delta_vB = kp*de_B + ki*errorB + kd*(errorB - 2*error_prev_B + error_prev_prev_B);
        controlB += delta_vB;
        error_prev_B = errorB;
        error_prev_prev_B = error_prev_B;

        digitalWrite(BIN1, controlB >= 0);
        digitalWrite(BIN2, controlB <= 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        digitalWrite(std::min(std::abs(controlB), 100.));

        printf("time = %6.3f, enc = %d, deg = %5.1f, err = %5.1f, ctrl = %7.1f\n", 
               time.time()-start_time, encoderPosRight, motorDegB, errorB, controlB);

        if (std::abs(errorB) <= tolerance) {
            digitalWrite(BIN1, controlB >= 0);
            digitalWrite(BIN2, controlB <= 0);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            softPwmWrite(pwmPinB, 0);
            break;
        }

        time_prev = time.time();
        std::this_thread::sleep_for(std::chrono::milliseconds(dt_sleep * 1000));
    }
    
    return 0;
}