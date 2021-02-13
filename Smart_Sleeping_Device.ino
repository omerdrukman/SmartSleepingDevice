
/*
  Smart Sleeping Device
 A smart sleeping device which can help you to lucid dream, set up smart alarm clocks
 for middle of the night nba games and snoring monitor.
 
  The circuit:
  None

  Created By:
  Omer Drukman #315473223
*/ 

#define BLYNK_PRINT SerialUSB
//#define BLYNK_DEBUG SerialUSB

#include <Adafruit_CircuitPlayground.h>
#include <Proximity.h>
#include <FiniteStateMachine.h>
#include <ESP8266_Lib.h>
#include <BlynkSimpleShieldEsp8266.h>

#define ESP_SSID "" // Your network name here
#define ESP_PASS "" // Your network password here

// Blynk
char auth[] = "TOKEN";

char ssid[] = "WIFI_NAME";
char pass[] = "WIFI_PASSWORD";

#define EspSerial Serial1
#define ESP8266_BAUD 9600

ESP8266 wifi(&EspSerial);

// Snoring params
int soundValue;
int soundCheckTime = 500;
int soundThreshold = 70;
int soundPreviousMillis = millis();


// Lucid Dreams $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
void lucidInitVars () {
  waiting = true;
}

State deactiveLucidState = State(NULL);
State activeLucidState = State(lucidInitVars, onActiveLucidUpdate, NULL);
FSM lucidDreamsMachine = FSM(deactiveLucidState);

// Moving params
float baseX, baseY, baseZ;
bool slideSwitch;
unsigned long previousMillis = 0;
float X,Y,Z;
int MOVE_THRESHOLD = 15;
long OnTime = 50;
long OffTime = 750;
bool waiting = true;

// Proximity params
Proximity prox;
int last_dist = prox.lastDist();
int difference = 0;
int last_difference = 0;
long last_prox = 0;
int count_shifts = 0;

// Moving params
bool is_moving = false;
bool is_eye_moving = false;
double storedVector;


void onActiveLucidUpdate() {
  // Moving $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
  unsigned long currentMillis = millis();
  
  if ((waiting) && (currentMillis - previousMillis >= OnTime)) {
    waiting = false;
    X = CircuitPlayground.motionX();
    Y = CircuitPlayground.motionY();
    Z = CircuitPlayground.motionZ();
    double newVector = X*X;
    newVector += Y*Y;
    newVector += Z*Z;
    newVector = sqrt(newVector);
    
    // are we moving 
    if (abs(10*newVector - 10*storedVector) > MOVE_THRESHOLD) {
      is_moving = true;
      OnTime = 500;
    } else {
      is_moving = false;
      OnTime = 50;
    }
    
    previousMillis = currentMillis;
  }
  else if ((!waiting) && (currentMillis - previousMillis >= OnTime)) {
    waiting = true;

    X = CircuitPlayground.motionX();
    Y = CircuitPlayground.motionY();
    Z = CircuitPlayground.motionZ();
   
     // Get the magnitude (length) of the 3 axis vector
    // http://en.wikipedia.org/wiki/Euclidean_vector#Length
    storedVector = X*X;
    storedVector += Y*Y;
    storedVector += Z*Z;
    storedVector = sqrt(storedVector);
  
    previousMillis = currentMillis;
  }
  // Moving $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

  // Proxing $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
  difference += abs(prox.lastDist() - last_dist);
  if (millis() - last_prox > 3000) {
    if (last_difference == 0) {
      last_difference = difference;
    } else if (difference > last_difference + 30) {
      is_eye_moving = true;
      count_shifts = 0;
    } else {
      count_shifts++;
      if (count_shifts > 7) {
        is_eye_moving = false;
      } 
    }

    last_prox = millis();
    last_difference = difference;
    difference = 0;
    last_dist = prox.lastDist();
  }
  // Proxing $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$


  // Check lucid dream state
  if (!is_moving && is_eye_moving) {
    CircuitPlayground.setPixelColor(0, 0xFF0000);
  } else {
    CircuitPlayground.clearPixels();
  }
}
// Lucid Dreams $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$

// NBA Games $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
int game_diff_threshold = 0;
int last_game_check = millis();
int gameId = 0;
int alarm_minutes = 0;
int setGameMillis = 0;
int CHECK_INTERVAL = 1000 * 60 * 20; // 20 minutes
int nbaValue = -1;

void onNBASaveGameWaitInit() {
  Serial.println("In game wait");
  nbaValue = -1;
}


void onNBASaveGameIdInit() {
  Serial.println("In game id");
  nbaValue = -1;
}

void onNBASaveGameTimeInit() {
  Serial.println("In game time");
  nbaValue = -1;
}


void onNBASaveGameQueringInit() {
  Serial.println("In game quering");
  nbaValue = -1;
}

void onNBASaveGameAlarmInit() {
  Serial.println("In game alarm");
  nbaValue = -1;
}

State nbaWaitForGame = State(onNBASaveGameWaitInit, onNBASaveGameWaitUpdate, NULL);
State nbaGetGameId = State(onNBASaveGameIdInit, onNBASaveGameIdUpdate, NULL);
State nbaGetGameTime= State(onNBASaveGameTimeInit, onNBASaveGameTimeUpdate, NULL);
State nbaQueringGame = State(onNBASaveGameQueringInit, onNBAGameQuering, NULL);
State nbaAlarm = State(onNBASaveGameAlarmInit, onNBAAlarm, NULL);
FSM nbaGamesMachine = FSM(nbaWaitForGame);


void onNBASaveGameWaitUpdate() {
  if (nbaValue >= 0) {
    Blynk.virtualWrite(V11, "What is the game id?");
    nbaGamesMachine.transitionTo(nbaGetGameId);
  }
}


void onNBASaveGameIdUpdate() {
  if (nbaValue >= 0) {
    Blynk.virtualWrite(V11, "What is the game time?");
    gameId = nbaValue;
    nbaGamesMachine.transitionTo(nbaGetGameTime);
  }
}



void onNBASaveGameTimeUpdate() {
  if (nbaValue >= 0) {
    Blynk.virtualWrite(V11, "Game set.");
    last_game_check = millis() + last_game_check / 60; // In minutes
    nbaGamesMachine.transitionTo(nbaQueringGame);
  }
}


void onNBAGameQuering() {
  if (millis() - last_game_check >= CHECK_INTERVAL) {
    String message = "gameId:" + gameId;
    Blynk.virtualWrite(V11, message);
    
    last_game_check = millis();
  }
  
  if (nbaValue >= 0) {
      if (nbaValue <= game_diff_threshold) {
        nbaGamesMachine.transitionTo(nbaAlarm);
      } else if (nbaValue > 100) {
        Blynk.virtualWrite(V11, "Game Canceled.");
        nbaGamesMachine.transitionTo(nbaWaitForGame);
      }
  }
}


void onNBAAlarm() {
  // Active the alarm
  CircuitPlayground.playTone(523 , 8000);
  nbaGamesMachine.transitionTo(nbaWaitForGame);
}

// NBA Games $$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
  
void setup() {
  // put your setup code here, to run once:
  while (!Serial);
  Serial.begin(19200);
  CircuitPlayground.begin();
  prox.begin(71);

  SerialUSB.begin(9600);
  EspSerial.begin(ESP8266_BAUD);
  delay(10);
 

  Blynk.begin(auth, wifi, ssid, pass);
  
  baseX = CircuitPlayground.motionX();
  baseY = CircuitPlayground.motionY();
  baseZ = CircuitPlayground.motionZ();
}

void loop() {
  Blynk.run();

  lucidDreamsMachine.update();
  nbaGamesMachine.update();

  // Get slide switch param
  slideSwitch = CircuitPlayground.slideSwitch();

   // Monitor snoring
  if (slideSwitch && (millis() - soundPreviousMillis > soundCheckTime)) {
    Serial.println("Snore on");
    soundValue = CircuitPlayground.mic.soundPressureLevel(10);
  
    if (soundValue > soundThreshold) {
      String message = "Snore:" + soundValue;
      Blynk.virtualWrite(V11, message);
    }
  }   
}

// Receive messages
BLYNK_WRITE(V10) {
  // Get message param
  int pinValue = param.asInt();
  
  nbaValue = pinValue;
}

// Receive game difference threshold from slider
BLYNK_WRITE(V3) {
  // Get x param of the slider
  int pinValue = param.asInt(); 
  game_diff_threshold = pinValue;
}
