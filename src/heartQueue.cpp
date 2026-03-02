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
    for (size_t i = 0; i < len(); i++) {
        Heart *h = getHeart(i);
        if (h->life == 0) {
            dequeue();
        }
        else {
            h->life--;
        }
    }
}

Heart *HeartQueue::getHeart(size_t i) {
    if (i < len()) {
        return &hearts[(head + i) & (HEART_BUF_LEN - 1)];
    }
    return nullptr;
}
