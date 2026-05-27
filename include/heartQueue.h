#include <Arduino.h>

#define HEART_BUF_LEN 16 // Must be a power of 2

typedef struct {
  size_t life;
  size_t x;
  size_t y;
} Heart;

class HeartQueue {
private:
  size_t head;
  size_t tail;
  Heart hearts[HEART_BUF_LEN];

  void enqueue(Heart h);
  void dequeue();
public:
  size_t len();
  void appendHeart(size_t life, size_t x, size_t y);
  void process();
  Heart *getHeart(size_t i);
};
