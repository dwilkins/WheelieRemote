#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <math.h>
#include <ncurses.h>
#include <poll.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include "xwiimote.h"
#include <dbus/dbus.h>


#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/hci_lib.h>
/* #include "oui.h" */

static char *tiny_scan();


enum window_mode {
	MODE_ERROR,
	MODE_NORMAL,
	MODE_EXTENDED,
};

static bool rumble_on = false;

#define MAX_ACCEL_SAMPLES 100

int create_name(char *buf, size_t size, const char *path, const char *address, const char *name)
{
	return snprintf(buf, size, "%s/%s/%s", path, address, name);
}

static char ** enumerate()
{
  struct xwii_monitor *mon;
  static char *wiimote_paths[10] = {NULL};
  char *ent;
  int num = 0;

  mon = xwii_monitor_new(false, false);
  if (!mon) {
    printf("Cannot create monitor\n");
    return NULL;
  }

  while ((ent = xwii_monitor_poll(mon))) {
    printf("  Found device #%d: %s\n", num, ent);
    wiimote_paths[num++] = ent;
//    free(ent);
  }

  xwii_monitor_unref(mon);
  return wiimote_paths;
}

static void rumble_toggle(struct xwii_iface *iface)
{
  int ret;

  rumble_on = !rumble_on;
  ret = xwii_iface_rumble(iface, rumble_on);
  if (ret) {
    fprintf(stderr,"Error: Cannot toggle rumble motor: %d", ret);
    rumble_on = !rumble_on;
  }
}


void handle_accel(struct xwii_iface *iface,int x,int y,int z) {
  static int value_count = 0;
  static int top_value = 0;
  static int x_sum[MAX_ACCEL_SAMPLES], y_sum[MAX_ACCEL_SAMPLES], z_sum[MAX_ACCEL_SAMPLES];
  int i = 0;
  long int x_avg = 0, y_avg = 0, z_avg = 0;

  if(top_value == MAX_ACCEL_SAMPLES) {
    top_value = 0;
  }
  x_sum[top_value] = x;
  y_sum[top_value] = y;
  z_sum[top_value++] = z;

  if(value_count < MAX_ACCEL_SAMPLES) {
    value_count++;
  }

  for(i=0;i<value_count;i++) {
    x_avg += x_sum[i];
    y_avg += y_sum[i];
    z_avg += z_sum[i];
  }

  x_avg = (x_avg / value_count);
  y_avg = (y_avg / value_count);
  z_avg = (z_avg / value_count);

   printf("XWII_EVENT_ACCEL           x=% 4.4d y=% 4.4d z=% 4.4d  - Average x=%ld y=%ld z=%ld   %d                                             \r", x,y,z,x_avg,y_avg,z_avg, value_count);
  if( value_count == MAX_ACCEL_SAMPLES) {
    if((y_avg < -90 && !rumble_on) || (y_avg > -90 && rumble_on)) {
      rumble_toggle(iface);
      value_count = 0;
      top_value = 0;
    }
  }
}





static int run_iface(struct xwii_iface *iface)
{
  struct xwii_event event;
  int ret = 0;
  struct pollfd fds[2];

  memset(fds, 0, sizeof(fds));
  fds[0].fd = 0;
  fds[0].events = POLLIN;
  fds[1].fd = xwii_iface_get_fd(iface);
  fds[1].events = POLLIN;

  ret = xwii_iface_watch(iface, true);
  if (ret)
    fprintf(stderr,"Error: Cannot initialize hotplug watch descriptor");

  while (true) {
    ret = poll(fds, 2, -1);
    if (ret < 0) {
      if (errno != EINTR) {
        ret = -errno;
        fprintf(stderr,"Error: Cannot poll fds: %d", ret);
        break;
      }
    }

    ret = xwii_iface_dispatch(iface, &event, sizeof(event));
    if (ret) {
      if (ret != -EAGAIN) {
        fprintf(stderr,"Error: Read failed with err:%d",
                    ret);
        break;
      }
    } else {
      switch (event.type) {
      case XWII_EVENT_WATCH:
        /*        handle_watch(); */
        printf("XWII_EVENT_WATCH\n");
        break;
      case XWII_EVENT_KEY:
        printf("XWII_EVENT_KEY\n");
        /*          key_show(&event); */
        break;
      case XWII_EVENT_ACCEL:
        /* accel_show_ext(&event); */
        handle_accel(iface,event.v.abs[0].x,event.v.abs[0].y,event.v.abs[0].z);
          /* accel_show(&event); */
        break;
      case XWII_EVENT_IR:
        /* if (mode == MODE_EXTENDED) */
          /* ir_show_ext(&event); */
        printf("XWII_EVENT_IR\n");
        /* if (mode != MODE_ERROR) */
          /* ir_show(&event); */
        break;
      case XWII_EVENT_MOTION_PLUS:
        printf("XWII_EVENT_MOTION_PLUS\n");
        /* if (mode != MODE_ERROR) */
        /*   mp_show(&event); */
        break;
      case XWII_EVENT_NUNCHUK_KEY:
        printf("XWII_EVENT_NUNCHUK_KEY\n");
        break;
      case XWII_EVENT_NUNCHUK_MOVE:
        printf("XWII_EVENT_NUNCHUK_MOVE\n");
        /* if (mode == MODE_EXTENDED) */
        /*   nunchuk_show_ext(&event); */
        break;
      case XWII_EVENT_CLASSIC_CONTROLLER_KEY:
        printf("XWII_EVENT_CLASSIC_CONTROLLER_KEY\n");
      case XWII_EVENT_CLASSIC_CONTROLLER_MOVE:
        printf("XWII_EVENT_CLASSIC_CONTROLLER_MOVE\n");
        /* if (mode == MODE_EXTENDED) */
        /*   classic_show_ext(&event); */
        break;
      case XWII_EVENT_BALANCE_BOARD:
        printf("XWII_EVENT_BALANCE_BOARD\n");
        /* if (mode == MODE_EXTENDED) */
        /*   bboard_show_ext(&event); */
        break;
      case XWII_EVENT_PRO_CONTROLLER_KEY:
        printf("XWII_EVENT_PRO_CONTROLLER_KEY\n");
        break;
      case XWII_EVENT_PRO_CONTROLLER_MOVE:
        printf("XWII_EVENT_PRO_CONTROLLER_MOVE\n");
        break;
        /* if (mode == MODE_EXTENDED) */
        /*   pro_show_ext(&event); */
        break;
      }
    }
  }

  return ret;
}

char *get_default_adapter() {
  DBusConnection *conn;
  DBusMessage *msg, *reply;
  const char *name;
  static char default_adapter[100];
  conn = dbus_bus_get(DBUS_BUS_SYSTEM, NULL);
  msg = dbus_message_new_method_call("org.bluez",
                                     "/",
                                     "org.bluez.Manager", "DefaultAdapter");
  reply = dbus_connection_send_with_reply_and_block(conn, msg, -1, NULL);
  dbus_message_get_args(reply, NULL,
                        DBUS_TYPE_OBJECT_PATH, &name,
                        DBUS_TYPE_INVALID);
  strcpy(default_adapter,name);
  dbus_message_unref(msg);
  dbus_message_unref(reply);
  printf("Default adapter is %s\n",default_adapter);
  return default_adapter;
}

int attach_to_wiimote(char *address) {
  DBusConnection *conn;
  DBusMessage *msg;
  dbus_uint32_t serial;
  dbus_bool_t success;
  char *default_adapter = get_default_adapter();
  char bt_device[1000];
  int device_len = 0;
  int i;
  strcpy(bt_device,default_adapter);
  strcat(bt_device,"/dev_");
  device_len = strlen(bt_device);
  for(i=0;i < strlen(address);i++) {
    char ch = address[i];
    ch = (ch == ':' ? '_' : ch);
    bt_device[device_len + i] = ch;
    bt_device[device_len + i + 1] = 0;
  }

  printf("Bluetooth Device address is %s\n",bt_device);
  conn = dbus_bus_get(DBUS_BUS_SYSTEM, NULL);
  msg = dbus_message_new_method_call("org.bluez",
                                     bt_device,
                                     "org.bluez.Input", "Connect");
  success = dbus_connection_send(conn, msg, &serial);
  printf("return from dbus_connection_send was %d\n",success);
//  dbus_connection_flush(conn);
  printf("Sleeping a bit\n");
  for(int x=0;i<10;x++) {
    dbus_connection_read_write_dispatch(conn,1000);
  }


  /* dbus_message_get_args(reply, NULL, DBUS_TYPE_INVALID, NULL); */
  dbus_message_unref(msg);
  /* dbus_message_unref(reply); */
  return !success;
}



int main(int argc, char *argv[]) {
  char ** device_paths;
  int ret;
  struct xwii_iface *iface;


  attach_to_wiimote(tiny_scan());
  device_paths = enumerate();

  ret = xwii_iface_new(&iface, device_paths[0]);
  if(ret) {
    fprintf(stderr,"Error: Cannot create interface: %d\n", ret);
  }
  if(!ret) {
    ret = xwii_iface_open(iface, xwii_iface_available(iface));
    if (ret) {
      fprintf(stderr,"Error: Cannot open interface: %d\n", ret);
    }
    ret = xwii_iface_open(iface, XWII_IFACE_NUNCHUK);
    if (ret) {
      fprintf(stderr,"Error: Cannot open nunchuk interface: %d\n", ret);
    }

  }
  run_iface(iface);
}


static char * tiny_scan() {

    inquiry_info *ii = NULL;
    int max_rsp, num_rsp;
    int dev_id, sock, len, flags;
    int i;
    static char addr[19] = { 0 };
    char name[248] = { 0 };

    dev_id = hci_get_route(NULL);
    sock = hci_open_dev( dev_id );
    if (dev_id < 0 || sock < 0) {
        perror("opening socket");
        exit(1);
    }

    len  = 8;
    max_rsp = 255;
    flags = IREQ_CACHE_FLUSH;
    ii = (inquiry_info*)malloc(max_rsp * sizeof(inquiry_info));

    num_rsp = hci_inquiry(dev_id, len, max_rsp, NULL, &ii, flags);
    if( num_rsp < 0 ) perror("hci_inquiry");

    for (i = 0; i < num_rsp; i++) {
        ba2str(&(ii+i)->bdaddr, addr);
        memset(name, 0, sizeof(name));
        if (hci_read_remote_name(sock, &(ii+i)->bdaddr, sizeof(name),
            name, 0) < 0)
        strcpy(name, "[unknown]");
        printf("%s  %s\n", addr, name);
    }

    free( ii );
    close( sock );
    return addr;
}



