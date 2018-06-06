
// Extra libc helper functions

char *tzname[2];

void* _get_tzname() {
  return (void*)tzname;
}

int daylight;

int* _get_daylight() {
  return &daylight;
}

long timezone;

long* _get_timezone() {
  return &timezone;
}

char** environ;

char*** _get_environ() {
  return &environ;
}

// Call JS to build the default environment.

extern void __buildEnvironment(void*);

__attribute__((constructor (101))) // ensure a high priority for this constructor
// TODO: build and see if the bitcode is any different...
void __emscripten_environ_constructor(void) {
  __buildEnvironment((void*)&environ);
}

