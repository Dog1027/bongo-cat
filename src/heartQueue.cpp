#include "heartQueue.h"

size_t HeartQueue::len() {
    return (tail - head) & (HEART_BUF_LEN - 1);
}

void HeartQueue::enqueue(Heart h) {
    if (len() < HEART_BUF_LEN - 1) {
        hearts[tail] = h;
        tail = (tail + 1) & (HEART_BUF_LEN - 1);
    }
}

void HeartQueue::dequeue() {
    if (len() > 0) {
        head = (head + 1) & (HEART_BUF_LEN - 1);
    }
}

void HeartQueue::appendHeart(size_t life, size_t x, size_t y) {
    Heart h;
    h.life = life;
    h.x = x;
    h.y = y;
    enqueue(h);
}

void HeartQueue::process() {
    // Hearts all start with the same life and are queued in order, so expired
    // ones are always at the head. Drop them first, then age the rest; removing
    // inside the aging loop would shift indices and skip a heart.
    while (len() > 0 && getHeart(0)->life == 0) {
        dequeue();
    }

    for (size_t i = 0; i < len(); i++) {
        getHeart(i)->life--;
    }
}

Heart *HeartQueue::getHeart(size_t i) {
    if (i < len()) {
        return &hearts[(head + i) & (HEART_BUF_LEN - 1)];
    }
    return nullptr;
}
