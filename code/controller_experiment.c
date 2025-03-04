#include <errno.h>
#include <fcntl.h>
#include <libevdev-1.0/libevdev/libevdev.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {
  int x;
  int y;
  int rx;
  int ry;
  int z;
  int rz;
} state;

int maxs[6] = {0};
int mins[6] = {0};

const double zob = (double)(2 << 15);
const double zung = (double)(2 << 16);

void display_state(state *s) {
  printf("\033[2J\033[H");
  printf("[%03d%%] %.*s\n", s->z * 100 / 255, s->z * 20 / 255,
         "=================");
  printf("[%03d%%] %.*s\n", s->rz * 100 / 255, s->rz * 20 / 255,
         "=================");
  printf("╔═════════════════╗    ╔═════════════════╗\n"
         "║                 ║    ║                 ║\n"
         "║                 ║    ║                 ║\n"
         "║                 ║    ║                 ║\n"
         "║                 ║    ║                 ║\n"
         "║                 ║    ║                 ║\n"
         "║                 ║    ║                 ║\n"
         "║                 ║    ║                 ║\n"
         "║                 ║    ║                 ║\n"
         "║                 ║    ║                 ║\n"
         "╚═════════════════╝    ╚═════════════════╝\n");

  int x_pos = (s->x - mins[ABS_X]) * 8 / maxs[ABS_X];
  int y_pos = (s->y - mins[ABS_Y]) * 4 / maxs[ABS_Y];
  printf("\033[%d;%dH#", y_pos + 4, x_pos + 2);

  int rx_pos = (s->rx - mins[ABS_RX]) * 8 / maxs[ABS_RX];
  int ry_pos = (s->ry - mins[ABS_RY]) * 4 / maxs[ABS_RY];
  printf("\033[%d;%dH#", ry_pos + 4, rx_pos + 25);
  fflush(stdout);
}

void clean_up() { printf("\033[?25h\n"); }

int main() {
  struct libevdev *device = NULL;
  int fd;
  int rc;
  printf("\033[?25l\n");
  atexit(clean_up);

  fd = open("/dev/input/by-id/usb-8BitDo_8BitDo_Ultimate_2C_Wireless_Controller_C18618A740-event-joystick",
            O_RDONLY | O_NONBLOCK);
  rc = libevdev_new_from_fd(fd, &device);

  if (rc < 0) {
    fprintf(stderr, "Failed to init libevdev (%s)\n", strerror(-rc));
    exit(1);
  }

  const char events[] = {ABS_X, ABS_Y, ABS_RX, ABS_RY, ABS_Z, ABS_RZ};

  for (int i = 0; i < sizeof(events); i++) {
    maxs[i] = libevdev_get_abs_maximum(device, i);
    mins[i] = libevdev_get_abs_minimum(device, i);

    printf("%s -> %d : %d\n", libevdev_event_code_get_name(EV_ABS, i), maxs[i],
           mins[i]);
  }

  state s = {0};
  struct timespec last_print;
  struct timespec now;
  timespec_get(&last_print, TIME_UTC);

  do {
    struct input_event ev;
    rc = libevdev_next_event(device, LIBEVDEV_READ_FLAG_NORMAL, &ev);
    if (ev.type == EV_ABS) {
      switch (ev.code) {
      case ABS_X:
        s.x = ev.value;
        break;
      case ABS_Y:
        s.y = ev.value;
        break;
      case ABS_RX:
        s.rx = ev.value;
        break;
      case ABS_RY:
        s.ry = ev.value;
        break;
      case ABS_Z:
        s.z = ev.value;
        break;
      case ABS_RZ:
        s.rz = ev.value;
        break;
      }
    }

    timespec_get(&now, TIME_UTC);
    if (now.tv_sec > last_print.tv_sec ||
        now.tv_nsec - last_print.tv_nsec > 10000000) {
      display_state(&s);
      last_print = now;
    }
  } while (rc == 1 || rc == 0 || rc == -EAGAIN);
}
