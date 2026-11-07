#pragma once

#include <Arduino.h>

#define HEART_BUF_LEN 16 // Must be a power of 2

typedef struct {
  uint8_t life;
  int16_t x;
  int16_t y;
} Heart;

class HeartQueue {
private:
  size_t head = 0;
  size_t tail = 0;
  Heart hearts[HEART_BUF_LEN];

  void enqueue(Heart h);
  void dequeue();
public:
  size_t len();
  void appendHeart(uint8_t life, int16_t x, int16_t y);
  void process();
  Heart *getHeart(size_t i);
};
