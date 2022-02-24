#include <Wire.h>
#include <Adafruit_MLX90614.h>
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

#include <Servo.h>
Servo SV;

#include "WT2003S_Player.h"
#include <SoftwareSerial.h>
SoftwareSerial SSerial(2, 3);  // RX-D3, TX-D2
#define COMSerial SSerial
#define ShowSerial Serial
WT2003S<SoftwareSerial> Mp3Player;

uint32_t sd_songs = 0;
STROAGE workdisk = SD;
struct Play_history {
    uint8_t disk;
    uint16_t index;
    char name[8];
}*SPISong, *SDSong;

void readSongName(struct Play_history* ph, uint32_t num, STROAGE disk) {
    Mp3Player.volume(0);
    switch (disk) {
        case SD:
            Mp3Player.playSDRootSong(0x0001);
            break;
    }
    for (int i = 0; i < num ; i++) {
        ph[i].disk = disk;
        ph[i].index = Mp3Player.getTracks();
        Mp3Player.getSongName(ph[i].name);
        Mp3Player.next();
    }
    Mp3Player.pause_or_play();
    Mp3Player.volume(40);  // 볼륨조절
}

void getAllSong() {
    uint8_t diskstatus = Mp3Player.getDiskStatus();
    if (diskstatus && 0x02) { // SDcard안의 파일을 재생할것
        sd_songs = Mp3Player.getSDMp3FileNumber();
        if (sd_songs > 0) {
            SDSong = (struct Play_history*)malloc((sd_songs) * sizeof(struct Play_history));
            readSongName(SDSong, sd_songs, SD);
        }
    }
}

int sw=4;
// int GPIO_24=5;  // 4번핀, DSP로 통신보내는 핀
int state=0;
int a=0;
int stateA=0;
int stateB=0;

int IR=11;  // 적외선센서, 냄비 인식 후 냄비가 인식되면 모터 멈춤
int UV=12;  // 적외선센서, 화구에 있는 냄비 인식
int onoff=0;  // ON, OFF 알림음 한번만 울리게 만든 변수

int trigPin_L=6;  // 초음파센서
int echoPin_L=7;  //
int trigPin_R=8;  //
int echoPin_R=9;  //
float duration_L, duration_R;  //초음파센서 거리측정, 계산을 위한 변수
float distance_L, distance_R;  //

unsigned long previousMillis=0;  // 이전시간
const long delayTime=5000;  // 5초(5000) 대기시간 -대기시간마다 스피커에서 화상경고알림 출력

void setup() {
  SV.attach(10);  // 10번 핀에 서보모터의 신호선을 연결
  ShowSerial.begin(9600);
  COMSerial.begin(9600);
  Mp3Player.init(COMSerial);
  mlx.begin();
  pinMode(sw, INPUT);
  pinMode(trigPin_L, OUTPUT);
  pinMode(echoPin_L, INPUT);
  pinMode(trigPin_R, OUTPUT);
  pinMode(echoPin_R, INPUT);
  //pinMode(GPIO_24, OUTPUT);  // DSP GPIO24 INPUT 
  pinMode(state, OUTPUT);
  getAllSong();
}

void loop() {
  int stateA=analogRead(A0);
  Serial.print("A0=");
  Serial.print(stateA);
  int stateB=analogRead(A1);
  Serial.print("   A1=");
  Serial.print(stateB);
  Serial.print("///  ");
  int a=digitalRead(IR);
  Serial.print("IR= ");
  Serial.print(a);
  Serial.print("///  ");
  
  /*
  int state=digitalRead(GPIO_24);
  Serial.print("GPIO24=");
  Serial.print(state);
  Serial.print("///  ");
  */
  
  Serial.print("Ambient= ");
  Serial.print(mlx.readAmbientTempC());
  Serial.print("*C");
  Serial.print("   Object= ");
  Serial.print(mlx.readObjectTempC());
  Serial.print("*C");
  Serial.print("///  ");
  
  digitalWrite(trigPin_L, HIGH);
  delay(10);
  digitalWrite(trigPin_L, LOW);
  duration_L=pulseIn(echoPin_L, HIGH);  // echoPin 이 HIGH를 유지한 시간을 저장 한다.
  distance_L=((340*duration_L)/10000)/2;
  
  digitalWrite(trigPin_R, HIGH);
  delay(10);
  digitalWrite(trigPin_R, LOW);
  duration_R=pulseIn(echoPin_R, HIGH);  // echoPin 이 HIGH를 유지한 시간을 저장 한다.
  distance_R=((340*duration_R)/10000)/2;
  
  Serial.print("distance_L= ");
  Serial.print(distance_L);
  Serial.print("cm");
  
  Serial.print("   distance_R= ");
  Serial.print(distance_R);
  Serial.println("cm");

  char cmd=ShowSerial.read();
  unsigned long currentMillis=millis();  // 현재시간
  // 현재온도= 27*C로 되어있음, 실제코딩해야할 온도= 40*C
  if((mlx.readObjectTempC()>40)&&((distance_L<50)||(distance_R<50))&&(currentMillis-previousMillis>=delayTime)&&(digitalRead(state)==HIGH))  //&&(digitalRead(GPIO_24)==HIGH)  // DSP안써서 뺌
  {
    previousMillis=currentMillis;
    cmd='3';  // 3번음악-WARNING-을 플레이 000011.mp3, 3.mp3
    Mp3Player.playSDRootSong(cmd - '0');  // 숫자누르는대로 음악을 재생할것
  }
  if((digitalRead(sw)==HIGH)&&(onoff==0))
  {
    Serial.println("sw_ON");
    cmd='1';  // 1번음악-ON-을 플레이 000001.mp3, 1.mp3
    Mp3Player.playSDRootSong(cmd - '0');  // 숫자누르는대로 음악을 재생할것
    //digitalWrite(GPIO_24, HIGH);
    digitalWrite(state, HIGH);
    onoff=1;
    analogWrite(A0, 0);  // 초기화
    analogWrite(A1, 0);
  }
  if((digitalRead(sw)==LOW)&&(onoff==1))
  {
    Serial.println("sw_OFF");
    cmd='2';  // 2번음악-OFF-을 플레이 000010.mp3, 2.mp3
    Mp3Player.playSDRootSong(cmd - '0');  // 숫자누르는대로 음악을 재생할것
    //digitalWrite(GPIO_24, LOW);
    digitalWrite(state, LOW);
    onoff=0;
  }
  if((digitalRead(sw)==LOW)&&(digitalRead(UV)==HIGH))
  {
    analogWrite(A0, 0);  // 냄비상승
    analogWrite(A1, 255);
  }
  
  if((digitalRead(UV)==LOW)&&(onoff==1)&&(digitalRead(state)==HIGH))  // 냄비가 들어오면  //&&(digitalRead(GPIO_24)==HIGH)  DSP안써서 뺌
  {
    if(digitalRead(IR)==HIGH)
    {
      analogWrite(A0, 255);  // 냄비하강
      analogWrite(A1, 0);
    }
    if(digitalRead(IR)==LOW)
    {
      analogWrite(A0, 0);  // 냄비하강정지
      analogWrite(A1, 0);
    }
    Serial.println("pot_IN");
    SV.write(90);  // 기본위치 복귀
  }
  if((digitalRead(UV)==HIGH)&&(onoff==1)&&(digitalRead(state==HIGH)))  // 냄비가 제거되면 //&&(digitalRead(GPIO_24)==HIGH)  DSP안써서 뺌
  {
    if(digitalRead(IR)==HIGH)
    {
      analogWrite(A0, 0);  // 냄비상승
      analogWrite(A1, 255);
    }
    //digitalWrite(GPIO_24, LOW);
    Serial.println("pot_OUT");
    cmd='4';  // 4번음악-REMOVE-을 플레이 000100.mp3, 4.mp3
    Mp3Player.playSDRootSong(cmd - '0');  // 숫자누르는대로 음악을 재생할것
    SV.write(00);  // 90도 회전-스위치 내려줌
    delay(2000);
    SV.write(90);  // 기본위치 복귀
    delay(500);
    cmd='2';  // 2번음악-OFF-을 플레이 000010.mp3, 2.mp3
    Mp3Player.playSDRootSong(cmd - '0');  // 숫자누르는대로 음악을 재생할것
    onoff=0;
  }
}
