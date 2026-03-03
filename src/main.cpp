#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <esp_sleep.h>
#include "cat.h"
#include "heartQueue.h"

#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64

#define PIN_LEFT       1
#define PIN_RIGHT      2
#define PIN_OLED_SDA   3
#define PIN_OLED_SCL   4
#define PIN_OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define OLED_I2C_ADDR  0x3C

Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, PIN_OLED_RESET);

#define IDLE_INTERVAL_MS 200
#define PREP_WAIT_MS     1600
#define TAP_INTERVAL_MS  50

#define HEART_IMG_LIFE   hollowHeartBoomLargerAllArray_LEN - 1
#define HEART_IMG_WIDTH  hollowHeartBoomLargerAllArray_W
#define HEART_IMG_HEIGHT hollowHeartBoomLargerAllArray_H
#define HEART_IMG_ARRAY  hollowHeartBoomLargerAllArray

#define HEART_IMG_R_X_DEFAULT 90
#define HEART_IMG_R_Y_DEFAULT 32
#define HEART_IMG_L_X_DEFAULT 28
#define HEART_IMG_L_Y_DEFAULT 32
#define HEART_IMG_X_RANDOM    25
#define HEART_IMG_Y_RANDOM    24

#define INACTIVITY_SLEEP_MS (60UL * 1000UL)

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

enum {
    LEFT = 0,
    RIGHT = 1,
};

HeartQueue heartQueue;

uint8_t catTap = TAP_NONE;
uint8_t catImgState = IMG_NONE;

unsigned long tapCount = 0;
unsigned long lastActivityMillis = 0;

void readKey(void);
void catLogic(void);
void imgProcess(void);
enum catState idleState(bool init);
enum catState prepState(bool init);
enum catState tapState(bool init);
void appendHeart(int leftRight);
void goToDeepSleep(void);

void setup()
{
    Serial.begin(115200);

    pinMode(PIN_RIGHT, INPUT_PULLUP);
    pinMode(PIN_LEFT, INPUT_PULLUP);

    Wire.setPins(PIN_OLED_SDA, PIN_OLED_SCL);

    if (!display.begin(OLED_I2C_ADDR, false)) {
        Serial.println(F("SH1106 allocation failed"));
        for (;;) {

        } // Don't proceed, loop forever
    }

    display.clearDisplay();
    Serial.println(F("OLED init success"));

    lastActivityMillis = millis();
}

void loop()
{
    readKey();
    heartQueue.process();
    catLogic();
    imgProcess();

    if (millis() - lastActivityMillis >= INACTIVITY_SLEEP_MS) {
        goToDeepSleep();
    }
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

    display.setTextColor(SH110X_WHITE);
    display.setCursor(10, 0);
    display.println(tapCount);
    display.display();
}

void catLogic()
{
    static enum catState lastState = IDLE;
    static bool stateInit = true;
    enum catState nextState;

    switch (lastState) {
    case IDLE:
        nextState = idleState(stateInit);
        break;
    case PREP:
        nextState = prepState(stateInit);
        break;
    case TAP:
        nextState = tapState(stateInit);
        break;
    default:
        nextState = IDLE;
    }
    stateInit = false;

    if (lastState != nextState) {
        lastState = nextState;
        stateInit = true;
    }
}

enum catState idleState(bool init)
{
    static unsigned long lastMillis = 0;

    if (init) {
        catImgState = IMG_IDLE_START;
        lastMillis = millis();
    }

    if (millis() - lastMillis >= IDLE_INTERVAL_MS) {
        lastMillis = millis();

        catImgState++;
        if (catImgState > IMG_IDLE_END) {
            catImgState = IMG_IDLE_START;
        }
    }

    if (catTap != TAP_NONE) {
        return TAP;
    }

    return IDLE;
}

enum catState prepState(bool init)
{
    static unsigned long lastMillis = 0;

    if (init) {
        catImgState = IMG_PREPARE;
        lastMillis = millis();
    }

    if (millis() - lastMillis >= PREP_WAIT_MS) {
        lastMillis = millis();
        return IDLE;
    }

    if (catTap != TAP_NONE) {
        return TAP;
    }

    return PREP;
}

enum catState tapState(bool init)
{
    static unsigned long lastMillis = 0;

    if (init) {
        if (catTap == TAP_NONE) {
            return PREP;
        }
        catImgState = IMG_TAP + (catTap - 1) * 2;
        lastMillis = millis();
    }

    if (millis() - lastMillis >= TAP_INTERVAL_MS) {
        lastMillis = millis();

        switch (catImgState) {
        case IMG_TAP_R:
            appendHeart(RIGHT);
            tapCount++;
            catImgState = IMG_TAP_RD;
            break;
        case IMG_TAP_RD:
            switch (catTap) {
            case TAP_NONE:
                return PREP;
            case TAP_LEFT:
                catImgState = IMG_TAP_BLD;
                break;
            case TAP_BOTH:
                catImgState = IMG_TAP_BRD;
                break;
            }
            break;
        case IMG_TAP_L:
            appendHeart(LEFT);
            tapCount++;
            catImgState = IMG_TAP_LD;
            break;
        case IMG_TAP_LD:
            switch (catTap) {
            case TAP_NONE:
                return PREP;
            case TAP_RIGHT:
                catImgState = IMG_TAP_BRD;
                break;
            case TAP_BOTH:
                catImgState = IMG_TAP_BLD;
                break;
            }
            break;
        case IMG_TAP_B:
            appendHeart(LEFT);
            appendHeart(RIGHT);
            tapCount++;
            catImgState = IMG_TAP_BD;
            break;
        case IMG_TAP_BD:
            switch (catTap) {
            case TAP_NONE:
                return PREP;
            case TAP_LEFT:
                catImgState = IMG_TAP_LD;
                break;
            case TAP_RIGHT:
                catImgState = IMG_TAP_RD;
                break;
            }
            break;
        case IMG_TAP_BLD:
            appendHeart(RIGHT);
            catImgState = IMG_TAP_BD;
            break;
        case IMG_TAP_BRD:
            appendHeart(LEFT);
            catImgState = IMG_TAP_BD;
            break;
        }
    }

    return TAP;
}

void appendHeart(int leftRight)
{
    if (leftRight == LEFT) { // left
        heartQueue.appendHeart(HEART_IMG_LIFE, HEART_IMG_L_X_DEFAULT - random(HEART_IMG_X_RANDOM),
            SCREEN_HEIGHT / 2 + HEART_IMG_L_Y_DEFAULT - HEART_IMG_HEIGHT - random(HEART_IMG_Y_RANDOM));
    }
    else { // right
        heartQueue.appendHeart(HEART_IMG_LIFE, HEART_IMG_R_X_DEFAULT + random(HEART_IMG_X_RANDOM),
            SCREEN_HEIGHT / 2 + HEART_IMG_R_Y_DEFAULT - HEART_IMG_HEIGHT - random(HEART_IMG_Y_RANDOM));
    }
}

void readKey(void)
{
    catTap = !digitalRead(PIN_RIGHT) + !digitalRead(PIN_LEFT) * 2;

    if (catTap != TAP_NONE) {
        lastActivityMillis = millis();
    }
}

void goToDeepSleep(void)
{
    display.clearDisplay();
    display.display();

    esp_deep_sleep_enable_gpio_wakeup(1ULL << PIN_RIGHT | 1ULL << PIN_LEFT, ESP_GPIO_WAKEUP_GPIO_LOW);
    Serial.flush();
    esp_deep_sleep_start();
}
