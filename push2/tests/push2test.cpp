#include "push2lib.h"

#include <unistd.h>


int main(int argc, char** argv) {

    Push2API::Push2 push2;
    push2.init();

    // 'test for Push 1 compatibility mode'
    int row = 2;
    push2.clearDisplay();
    push2.drawText(row, 10, "...01234567891234567......");
    push2.clearRow(row);
    push2.p1_drawCell(row, 0, "01234567891234567");
    push2.p1_drawCell(row, 1, "01234567891234567");
    push2.p1_drawCell(row, 2, "01234567891234567");
    push2.p1_drawCell(row, 3, "01234567891234567");

    for (int i = 0; i < 60; i++) {
        push2.render();
        usleep(1000000);
    }

    push2.deinit();
}
