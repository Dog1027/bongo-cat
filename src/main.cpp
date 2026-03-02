#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "cat.h"
#include "heartQueue.h"

#define SCREEN_WIDTH 128 // OLED 寬度像素
#define SCREEN_HEIGHT 64 // OLED 高度像素

// 設定OLED
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define FRAME_INTERVAL 10
#define COUNT_DOWN_TIME 10
#define IDLE_DELAY_RATIO 3
#define HEART_IMG_LIFE hollowHeartBoomLargerAllArray_LEN - 1
#define HEART_IMG_WIDTH hollowHeartBoomLargerAllArray_W
#define HEART_IMG_HEIGHT hollowHeartBoomLargerAllArray_H
#define HEART_IMG_ARRAY hollowHeartBoomLargerAllArray

#define HEART_IMG_R_X_DEFAULT 90
#define HEART_IMG_R_Y_DEFAULT 32
#define HEART_IMG_L_X_DEFAULT 28
#define HEART_IMG_L_Y_DEFAULT 32
#define HEART_IMG_X_RANDOM 25
#define HEART_IMG_Y_RANDOM 24

#define PIN_RIGHT 2
#define PIN_LEFT 1

#define OLED_I2C_SDA 3
#define OLED_I2C_SCL 4
#define OLED_I2C_ADDR 0x3C

enum catState {
    IDLE,
    PREP,
    TAP,
};

enum catImgState {
    IMG_NONE,
    IMG_IDLE_START,
    IMG_IDLE_1 = IMG_IDLE_START,
    IMG_IDLE_2,
    IMG_IDLE_3,
    IMG_IDLE_4,
    IMG_IDLE_5,
    IMG_IDLE_END = IMG_IDLE_5,
    IMG_PREPARE,
    IMG_TAP,
    IMG_TAP_R = IMG_TAP,
    IMG_TAP_RD,
    IMG_TAP_L,
    IMG_TAP_LD,
    IMG_TAP_B,
    IMG_TAP_BD,
    IMG_TAP_BRD,
    IMG_TAP_BLD,
};

enum catTap {
    TAP_NONE,
    TAP_RIGHT,
    TAP_LEFT,
    TAP_BOTH,
};

HeartQueue heartQueue;

enum catState catState = IDLE;
uint8_t catTapRecord = TAP_NONE;
uint8_t catTap = TAP_NONE;
uint8_t catImgState = IMG_NONE;
bool stateInit = false;
unsigned long tapCount = 0;

void readKey(void);
void catLogic(void);
void imgProcess(void);
void idleState(void);
void prepState(void);
void tapState(void);
void appendHeart(bool rightOrLeft);

void setup()
{
    Serial.begin(115200);

    pinMode(PIN_RIGHT, INPUT_PULLUP);
    pinMode(PIN_LEFT, INPUT_PULLUP);

    Wire.setPins(OLED_I2C_SDA, OLED_I2C_SCL);

    if (!display.begin(OLED_I2C_ADDR, false)) {
        Serial.println(F("SH1106 allocation failed"));
        for (;;) {

        } // Don't proceed, loop forever
    }

    display.clearDisplay();
    Serial.println(F("OLED init success"));
}

void loop()
{
    // static unsigned long timePrevious = 0;
    // unsigned long time = millis();
    // if ((time - timePrevious) > FRAME_INTERVAL) {
    //   timePrevious = time;
    readKey();
    heartQueue.process();
    catLogic();
    imgProcess();
    // }
}

void imgProcess()
{
    display.clearDisplay();

    for (size_t i = 0; i < heartQueue.len(); i++) {
        Heart *h = heartQueue.getHeart(i);
        display.drawBitmap(h->x, h->y, HEART_IMG_ARRAY[HEART_IMG_LIFE - h->life], HEART_IMG_WIDTH, HEART_IMG_HEIGHT, 1);
    }

    if (catImgState != IMG_NONE) {
        display.drawBitmap(0, 32, catAllArray[catImgState - 1], 128, 32, 1);
    }
    else {
        display.println(F("Error: No cat image"));
    }

    // display.setTextSize(2); // Draw 2X-scale text
    display.setTextColor(SH110X_WHITE);
    display.setCursor(10, 0);
    display.println(tapCount);
    display.display();
}

void catLogic()
{
    if (catTap)
        catTapRecord = catTap;
    switch (catState) {
    case IDLE:
        idleState();
        break;
    case PREP:
        prepState();
        break;
    case TAP:
        tapState();
        break;
    }
}

void idleState()
{
    static uint8_t delay = 0;
    if (stateInit) {
        stateInit = false;
        catImgState = IMG_NONE;
        delay = 0;
    }

    // img state process
    if (delay % IDLE_DELAY_RATIO == 0)
        catImgState += 1;
    if (catImgState > IMG_IDLE_END)
        catImgState = IMG_IDLE_START;

    delay++;

    if (catTapRecord != TAP_NONE) {
        catState = PREP;
        stateInit = true;
    }
}

void prepState()
{
    static uint8_t prepCountDown = COUNT_DOWN_TIME;
    if (stateInit) {
        stateInit = false;
    }

    // img state process
    catImgState = IMG_PREPARE;

    prepCountDown--;
    if (prepCountDown == 0) {
        catState = IDLE;
        stateInit = true;
    }

    if (catTapRecord != TAP_NONE) {
        catState = TAP;
        stateInit = true;
        prepCountDown = COUNT_DOWN_TIME;
    }
}

void tapState()
{
    static uint8_t catImgStatePre = IMG_NONE;
    if (stateInit) {
        stateInit = false;
        catImgStatePre = IMG_NONE;
    }

    // img state process
    switch (catImgStatePre) {
    case IMG_NONE:
        catImgState = IMG_TAP + (catTapRecord - 1) * 2;
        break;
    case IMG_TAP_L:
        tapCount++;
        appendHeart(false);
        catImgState = IMG_TAP_LD;
    case IMG_TAP_LD:
        if (catTapRecord == TAP_BOTH)
            catImgState = IMG_TAP_BLD;
        else if (catTapRecord == TAP_RIGHT)
            catImgState = IMG_TAP_BRD;
        catTapRecord = catTap;
        break;
    case IMG_TAP_R:
        tapCount++;
        appendHeart(true);
        catImgState = IMG_TAP_RD;
    case IMG_TAP_RD:
        if (catTapRecord == TAP_BOTH)
            catImgState = IMG_TAP_BRD;
        else if (catTapRecord == TAP_LEFT)
            catImgState = IMG_TAP_BLD;
        catTapRecord = catTap;
        break;
    case IMG_TAP_B:
        tapCount++;
        appendHeart(true);
        appendHeart(false);
        catImgState = IMG_TAP_BD;
    case IMG_TAP_BD:
    IMG_TAP_BD:
        if (catTapRecord == TAP_LEFT)
            catImgState = IMG_TAP_LD;
        else if (catTapRecord == TAP_RIGHT)
            catImgState = IMG_TAP_RD;
        catTapRecord = catTap;
        break;
    case IMG_TAP_BLD:
        tapCount++;
        appendHeart(true);
        catImgState = IMG_TAP_BD;
        goto IMG_TAP_BD;
    case IMG_TAP_BRD:
        tapCount++;
        appendHeart(false);
        catImgState = IMG_TAP_BD;
        goto IMG_TAP_BD;
    }
    catImgStatePre = catImgState;

    if (catTapRecord == TAP_NONE) {
        catState = PREP;
        // stateInit = true;
    }
}

void appendHeart(bool rightOrLeft)
{
    if (rightOrLeft) // RIGHT
        heartQueue.appendHeart(HEART_IMG_LIFE, HEART_IMG_R_X_DEFAULT + random(HEART_IMG_X_RANDOM), SCREEN_HEIGHT / 2 + HEART_IMG_R_Y_DEFAULT - HEART_IMG_HEIGHT - random(HEART_IMG_Y_RANDOM));
    else // LEFT
        heartQueue.appendHeart(HEART_IMG_LIFE, HEART_IMG_L_X_DEFAULT - random(HEART_IMG_X_RANDOM), SCREEN_HEIGHT / 2 + HEART_IMG_L_Y_DEFAULT - HEART_IMG_HEIGHT - random(HEART_IMG_Y_RANDOM));
}

void readKey(void)
{
    catTap = !digitalRead(PIN_RIGHT) + !digitalRead(PIN_LEFT) * 2;
}
