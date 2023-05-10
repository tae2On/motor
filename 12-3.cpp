/* 라이다 센서 연동 */
/* 두 모터의 속도 차이를 이용한 전/후/좌/우 회전 */ 
// 회전각도 구하기, 이동거리 구하기 -> 제어 확인
// 자이로 센서 이용하여 좌우각도 회전하기 

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
#define pwmPinA 21              // 모터드라이버 ENA - GPIO핀 번호: 5
#define AIN1 16                 // IN1 - GPIO핀 번호: 15
#define AIN2 28                 // IN2 - GPIO핀 번호 : 20
#define encPinA 22               // 보라색 (A) - GPIO핀 번호 : 6
#define encPinB 23               // 파랑색 (B) - GPIO핀 번호 : 13

/* PID 제어 */
const float proportion = 360. / (30 * 52);       // 한 바퀴에 약 1350펄스 (정확하지 않음 - 계산값)

/* PID 상수 */
float kp_A = 0.85; 
float kd_A = 0;         
float ki_A = 0;

float encoderPosLeft = 0;              // 엔코더 값 - 왼쪽

float motorDegA = 0;                   // 모터 각도A
float motor_distance_A = 0.;           // 왼쪽 모터 이동거리 

float derrorA = 0.;
float errorA = 0;
float error_prev_A = 0.;
float error_prev_prev_A = 0;

double controlA = 0.;
 
double wheel; 
double target_deg;                      // 목표 각도 

// 모터 이동거리 구할 때 필요
double target_distance = 0.;            // 목표 거리     

// 모터의 회전각도 구할 때 필요 
double target_turn_deg;
double de_A;
double di_A = 0;
double dt = 0;

double delta_vA;
double time_prev = 0;

int lidar_way;
int x;

std::time_t start_time = std::time(nullptr);

// 인터럽트 
void doEncoderA() {
  encoderPosLeft  += (digitalRead(encPinA) == digitalRead(encPinB)) ? -1 : 1;
}
void doEncoderB() {
  encoderPosLeft  += (digitalRead(encPinA) == digitalRead(encPinB)) ? 1 : -1;
}


class MotorControl{
public:
    void call(int x);
    int getInput();
};


void Calculation() {
    wheel = 2*M_PI*11.5;
    target_deg = (360*target_distance / wheel) ;      // 목표 각도
        
    //DC모터 왼쪽
    motorDegA = encoderPosLeft * proportion;
    errorA = target_deg - motorDegA;
    de_A = errorA -error_prev_A;
    di_A += errorA * dt;
    dt = time(nullptr) - time_prev;
        
    delta_vA = kp_A*de_A + ki_A*errorA + kd_A*(errorA - 2*error_prev_A + error_prev_prev_A);
    controlA += delta_vA;
    error_prev_A = errorA;
    error_prev_prev_A = error_prev_A;
    
    motor_distance_A = motorDegA * wheel / 360;           // 모터 움직인 거리
    derrorA = abs(target_distance - motor_distance_A);    // 거리 오차값
    }

// 원하는 방향 입력
int MotorControl::getInput() {
    int x;
    cout << "정지 : 0 / 직진 : 1 / 후진 : 2 / 오른쪽 : 3 / 왼쪽 : 4" << endl;
    cout << "원하는 방향을 입력하시오 : ";
    cin >> x;
    
    return x; 
}


void MotorControl::call(int x){
    // 정지
    if (x == 0){
        // 방향 설정 
        digitalWrite(AIN1, LOW);
        digitalWrite(AIN2, LOW);
        delay(10);
        // 속도 설정 
        softPwmWrite(pwmPinA, 0);

        // 이동거리 출력 
        cout << "차체 중앙 기준 이동거리 = " << motor_distance_A << endl;
        cout << "deg = " << motorDegA << endl;
        // x(방향)의 값이 0(정지)이 아닐 경우 x(방향)을 다시 입력 받음 
        if(x != 0){
            x = getInput();
        }
    }
    
    // 전진
    else if (x == 1) {
        // 방향 설정 
        digitalWrite(AIN1, HIGH);
        digitalWrite(AIN2, LOW);
        delay(10);
        // 속도 설정 
        softPwmWrite(pwmPinA, min(abs(controlA), 100.));    
        
        
        // x(방향)의 값이 1(전진)이 아닐 경우 x(방향)을 다시 입력 받음 
        if(x != 1){
            x = getInput();
        }
        else {
            // Calculation();
            delay(1000);
            // 이동거리 출력 
            cout << "왼쪽 모터 이동거리  = " << motor_distance_A << endl;
            cout << "왼쪽 모터 오차값 = " << derrorA << endl;

            cout << "ctrlA = " << controlA << ", degA = " << motorDegA << ", errA = " << errorA << ", disA = " << motor_distance_A << ", derrA = " << derrorA << endl;
            cout << "encA = " << encoderPosLeft<< endl;
        }
    }
}

int main(){
    wiringPiSetup();

    MotorControl control;

    pinMode(encPinA, INPUT);
    pullUpDnControl(encPinA, PUD_UP);
    pinMode(encPinB, INPUT);
    pullUpDnControl(encPinB, PUD_UP);
    pinMode(pwmPinA, OUTPUT); 
    pinMode(AIN1, OUTPUT);
    pinMode(AIN2, OUTPUT);
   
    softPwmCreate(pwmPinA, 0, 100);
    softPwmWrite(pwmPinA, 0);

    digitalWrite(pwmPinA, LOW);
    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, LOW);

    wiringPiISR(encPinA, INT_EDGE_BOTH, &doEncoderA);
    wiringPiISR(encPinB, INT_EDGE_BOTH, &doEncoderB);

    cout << "ctrlA = " << controlA << ", degA = " << motorDegA << ", errA = " << errorA << ", disA = " << motor_distance_A << ", derrA = " << derrorA << endl;
    cout << "encA = " << encoderPosLeft<< endl;

    while(true) {
        int lidar_way = control.getInput();
        control.call(lidar_way);
        delay(1000);   
    }
    return 0;

}