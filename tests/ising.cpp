#include <stdio.h>
#include <stdlib.h>
#include <SDL/SDL.h>
#include <emscripten.h>
#include <vector>

const int SIZE = 400;

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

  int calculateEnergy() {
    int energy = 0;
    for (int i = 0; i < SIZE; i++) {
      for (int j = 0; j < SIZE; j++) {
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
};

SDL_Surface *screen;

void draw(State& state) {
  SDL_LockSurface(screen);

  int32_t* data = (int32_t*)screen->pixels;
  for (int i = 0; i < SIZE; i++) {
    for (int j = 0; j < SIZE; j++) {
      unsigned char x = state.spin(i, j);
      data[i + (j * SIZE)] = SDL_MapRGBA(screen->format, x, x, x, 255);
    }
  }

  SDL_UnlockSurface(screen);
}

int main(int argc, char **argv) {
  SDL_Init(SDL_INIT_VIDEO);
  screen = SDL_SetVideoMode(SIZE, SIZE, 32, SDL_SWSURFACE);

  State state;
  state.randomize();
  draw(state);
  
  SDL_Quit();

  return 0;
}

