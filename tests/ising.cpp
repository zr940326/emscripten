#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL/SDL.h>

#include <emscripten.h>

#include <vector>

const int SIZE = 300;
double BETA = 0.5; // interesting values: 1, 0.1, 0.01
const int ITERS_PER_FRAME = (SIZE * SIZE) / 30;

struct State {
  int size;
  std::vector<int> spins;

  State() {
    spins.resize(SIZE * SIZE);
  }

  int& spin(int i, int j) {
    return spins[i + (j * SIZE)];
  }

  bool inRange(int i, int j) {
    return i >= 0 && i < SIZE && j >= 0 && j < SIZE;
  }

  void randomize() {
    for (int i = 0; i < SIZE; i++) {
      for (int j = 0; j < SIZE; j++) {
        spin(i, j) = emscripten_random() < 0.5 ? -1 : +1;
      }
    }
  }

  int calculateEnergy(int i0 = 0, int i1 = SIZE, int j0 = 0, int j1 = SIZE) {
    int energy = 0;
    for (int i = i0; i < i1; i++) {
      for (int j = j0; j < j1; j++) {
        if (inRange(i - 1, j - 1)) energy += spin(i, j) * spin(i - 1, j - 1);
        if (inRange(i    , j - 1)) energy += spin(i, j) * spin(i    , j - 1);
        if (inRange(i + 1, j - 1)) energy += spin(i, j) * spin(i + 1, j - 1);
        if (inRange(i - 1, j    )) energy += spin(i, j) * spin(i - 1, j    );
        if (inRange(i + 1, j    )) energy += spin(i, j) * spin(i + 1, j    );
        if (inRange(i - 1, j + 1)) energy += spin(i, j) * spin(i - 1, j + 1);
        if (inRange(i    , j + 1)) energy += spin(i, j) * spin(i    , j + 1);
        if (inRange(i + 1, j + 1)) energy += spin(i, j) * spin(i + 1, j + 1);
      }
    }
    return -energy;
  }

  void doMetropolisHastings() {
    // pick a position
    int i = emscripten_random() * SIZE;
    int j = emscripten_random() * SIZE;
    // check how flipping it alters the energy
    int oldEnergy = calculateEnergy(i - 1, i + 1, j - 1, j + 1);
    spin(i, j) *= -1;
    int newEnergy = calculateEnergy(i - 1, i + 1, j - 1, j + 1);
    int delta = newEnergy - oldEnergy;
    // if we reduced the energy, then don't undo this
    if (delta <= 0) return;
    // otherwise, check if we should stay or not
    if (emscripten_random() < exp(-BETA * delta)) return;
    // undo
    spin(i, j) *= -1;
  }
};

SDL_Surface *screen;
State state;

void loop() {
  SDL_LockSurface(screen);
  int32_t* data = (int32_t*)screen->pixels;
  for (int i = 0; i < SIZE; i++) {
    for (int j = 0; j < SIZE; j++) {
      int x = state.spin(i, j);
      data[i + (j * SIZE)] = x | (x << 8) | (x << 16);
    }
  }
  SDL_UnlockSurface(screen);
  for (int i = 0; i < ITERS_PER_FRAME; i++) {
    state.doMetropolisHastings();
  }
}

extern "C" EMSCRIPTEN_KEEPALIVE void setBeta(double beta) {
  BETA = beta;
}

int main(int argc, char **argv) {
  state.randomize();
  SDL_Init(SDL_INIT_VIDEO);
  screen = SDL_SetVideoMode(SIZE, SIZE, 32, SDL_SWSURFACE);
  emscripten_set_main_loop(loop, 0, 0);
  // create a slider
  EM_ASM({
    function scale(value) {
      return Math.log(1 + ((Math.exp(1) - 1) * 0.25 * value));
    }
    Module._setBeta(scale(0.5));

    var slider = document.createElement('input');
    slider.type = 'range';
    slider.min = 1;
    slider.max = 100;
    slider.onchange = function(event) {
      Module._setBeta(scale(event.target.value / 100));
    };
    document.body.appendChild(slider);
  });
}

