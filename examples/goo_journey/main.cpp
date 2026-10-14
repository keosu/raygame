#include "Game.h"
#include "Smoke.h"
int main(int argc, char** argv) {
    return Campaign::Run<Goo::Game>(argc, argv);
}
