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

static char *major_classes[] = {
	"Miscellaneous", "Computer", "Phone", "LAN Access",
	"Audio/Video", "Peripheral", "Imaging", "Uncategorized"
};

static char *get_minor_device_name(int major, int minor);
static void cmd_scan();
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

static char *get_device_name(const bdaddr_t *local, const bdaddr_t *peer)
{
  return "Nintendo Controller";
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


static void cmd_scan()
{
  int dev_id = -1;
  inquiry_info *info = NULL;
  uint8_t lap[3] = { 0x33, 0x8b, 0x9e };
  int num_rsp, length, flags;
  uint8_t cls[3], features[8];
  char addr[18], name[249], *tmp;
  struct hci_version version;
  struct hci_dev_info di;
  struct hci_conn_info_req *cr;
  int refresh = 0, extcls = 1, extinf = 1, extoui = 0;
  int i, n, dd, cc, nc;

  length  = 8;	/* ~10 seconds */
  num_rsp = 0;
  flags   = 0;

  if (dev_id < 0) {
    dev_id = hci_get_route(NULL);
    if (dev_id < 0) {
      perror("Device is not available");
      exit(1);
    }
  }

  if (hci_devinfo(dev_id, &di) < 0) {
    perror("Can't get device info");
    exit(1);
  }

  printf("Scanning ...\n");
  num_rsp = hci_inquiry(dev_id, length, num_rsp, lap, &info, flags);
  if (num_rsp < 0) {
    perror("Inquiry failed");
    exit(1);
  }

  dd = hci_open_dev(dev_id);
  if (dd < 0) {
    perror("HCI device open failed");
    free(info);
    exit(1);
  }

  if (extcls || extinf || extoui)
    printf("\n");

  for (i = 0; i < num_rsp; i++) {
    uint16_t handle = 0;

    if (!refresh) {
      memset(name, 0, sizeof(name));
      tmp = get_device_name(&di.bdaddr, &(info+i)->bdaddr);
      if (tmp) {
        strncpy(name, tmp, 249);
        /* free(tmp); */
        nc = 1;
      } else
        nc = 0;
    } else
      nc = 0;

    if (!extcls && !extinf && !extoui) {
      ba2str(&(info+i)->bdaddr, addr);

      if (nc) {
        printf("\t%s\t%s\n", addr, name);
        continue;
      }

      if (hci_read_remote_name_with_clock_offset(dd,
                                                 &(info+i)->bdaddr,
                                                 (info+i)->pscan_rep_mode,
                                                 (info+i)->clock_offset | 0x8000,
                                                 sizeof(name), name, 100000) < 0)
        strcpy(name, "n/a");

      for (n = 0; n < 248 && name[n]; n++) {
        if ((unsigned char) name[i] < 32 || name[i] == 127)
          name[i] = '.';
      }

      name[248] = '\0';

      printf("\t%s\t%s\n", addr, name);
      continue;
    }

    ba2str(&(info+i)->bdaddr, addr);
    printf("BD Address:\t%s [mode %d, clkoffset 0x%4.4x]\n", addr,
           (info+i)->pscan_rep_mode, btohs((info+i)->clock_offset));

    attach_to_wiimote(addr);
    cc = 0;

    if (extinf) {
      cr = malloc(sizeof(*cr) + sizeof(struct hci_conn_info));
      if (cr) {
        bacpy(&cr->bdaddr, &(info+i)->bdaddr);
        cr->type = ACL_LINK;
        if (ioctl(dd, HCIGETCONNINFO, (unsigned long) cr) < 0) {
          handle = 0;
          cc = 1;
        } else {
          handle = htobs(cr->conn_info->handle);
          cc = 0;
        }
        free(cr);
      }

      if (cc) {
        if (hci_create_connection(dd, &(info+i)->bdaddr,
                                  htobs(di.pkt_type & ACL_PTYPE_MASK),
                                  (info+i)->clock_offset | 0x8000,
                                  0x01, &handle, 25000) < 0) {
          handle = 0;
          cc = 0;
        }
      }
    }

    if (handle > 0 || !nc) {
      if (hci_read_remote_name_with_clock_offset(dd,
                                                 &(info+i)->bdaddr,
                                                 (info+i)->pscan_rep_mode,
                                                 (info+i)->clock_offset | 0x8000,
                                                 sizeof(name), name, 100000) < 0) {
        if (!nc)
          strcpy(name, "n/a");
      } else {
        for (n = 0; n < 248 && name[n]; n++) {
          if ((unsigned char) name[i] < 32 || name[i] == 127)
            name[i] = '.';
        }

        name[248] = '\0';
        nc = 0;
      }
    }

    if (strlen(name) > 0)
      printf("Device name:\t%s%s\n", name, nc ? " [cached]" : "");

    if (extcls) {
      memcpy(cls, (info+i)->dev_class, 3);
      printf("Device class:\t");
      if ((cls[1] & 0x1f) > sizeof(major_classes) / sizeof(char *))
        printf("Invalid");
      else
        printf("%s, %s", major_classes[cls[1] & 0x1f],
               get_minor_device_name(cls[1] & 0x1f, cls[0] >> 2));
      printf(" (0x%2.2x%2.2x%2.2x)\n", cls[2], cls[1], cls[0]);
    }

    if (extinf && handle > 0) {
      if (hci_read_remote_version(dd, handle, &version, 20000) == 0) {
        char *ver = lmp_vertostr(version.lmp_ver);
        printf("Manufacturer:\t%s (%d)\n",
               bt_compidtostr(version.manufacturer),
               version.manufacturer);
        printf("LMP version:\t%s (0x%x) [subver 0x%x]\n",
               ver ? ver : "n/a",
               version.lmp_ver, version.lmp_subver);
        if (ver)
          bt_free(ver);
      }

      if (hci_read_remote_features(dd, handle, features, 20000) == 0) {
        char *tmp = lmp_featurestostr(features, "\t\t", 63);
        printf("LMP features:\t0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x"
               " 0x%2.2x 0x%2.2x 0x%2.2x 0x%2.2x\n",
               features[0], features[1],
               features[2], features[3],
               features[4], features[5],
               features[6], features[7]);
        printf("%s\n", tmp);
        bt_free(tmp);
      }

      if (cc) {
        usleep(10000);
        hci_disconnect(dd, handle, HCI_OE_USER_ENDED_CONNECTION, 10000);
      }
    }

    printf("\n");
  }

  bt_free(info);

  hci_close_dev(dd);
}


static char *get_minor_device_name(int major, int minor)
{
	switch (major) {
	case 0:	/* misc */
		return "";
	case 1:	/* computer */
		switch (minor) {
		case 0:
			return "Uncategorized";
		case 1:
			return "Desktop workstation";
		case 2:
			return "Server";
		case 3:
			return "Laptop";
		case 4:
			return "Handheld";
		case 5:
			return "Palm";
		case 6:
			return "Wearable";
		}
		break;
	case 2:	/* phone */
		switch (minor) {
		case 0:
			return "Uncategorized";
		case 1:
			return "Cellular";
		case 2:
			return "Cordless";
		case 3:
			return "Smart phone";
		case 4:
			return "Wired modem or voice gateway";
		case 5:
			return "Common ISDN Access";
		case 6:
			return "Sim Card Reader";
		}
		break;
	case 3:	/* lan access */
		if (minor == 0)
			return "Uncategorized";
		switch (minor / 8) {
		case 0:
			return "Fully available";
		case 1:
			return "1-17% utilized";
		case 2:
			return "17-33% utilized";
		case 3:
			return "33-50% utilized";
		case 4:
			return "50-67% utilized";
		case 5:
			return "67-83% utilized";
		case 6:
			return "83-99% utilized";
		case 7:
			return "No service available";
		}
		break;
	case 4:	/* audio/video */
		switch (minor) {
		case 0:
			return "Uncategorized";
		case 1:
			return "Device conforms to the Headset profile";
		case 2:
			return "Hands-free";
			/* 3 is reserved */
		case 4:
			return "Microphone";
		case 5:
			return "Loudspeaker";
		case 6:
			return "Headphones";
		case 7:
			return "Portable Audio";
		case 8:
			return "Car Audio";
		case 9:
			return "Set-top box";
		case 10:
			return "HiFi Audio Device";
		case 11:
			return "VCR";
		case 12:
			return "Video Camera";
		case 13:
			return "Camcorder";
		case 14:
			return "Video Monitor";
		case 15:
			return "Video Display and Loudspeaker";
		case 16:
			return "Video Conferencing";
			/* 17 is reserved */
		case 18:
			return "Gaming/Toy";
		}
		break;
	case 5:	/* peripheral */ {
		static char cls_str[48]; cls_str[0] = 0;

		switch (minor & 48) {
		case 16:
			strncpy(cls_str, "Keyboard", sizeof(cls_str));
			break;
		case 32:
			strncpy(cls_str, "Pointing device", sizeof(cls_str));
			break;
		case 48:
			strncpy(cls_str, "Combo keyboard/pointing device", sizeof(cls_str));
			break;
		}
		if ((minor & 15) && (strlen(cls_str) > 0))
			strcat(cls_str, "/");

		switch (minor & 15) {
		case 0:
			break;
		case 1:
			strncat(cls_str, "Joystick", sizeof(cls_str) - strlen(cls_str));
			break;
		case 2:
			strncat(cls_str, "Gamepad", sizeof(cls_str) - strlen(cls_str));
			break;
		case 3:
			strncat(cls_str, "Remote control", sizeof(cls_str) - strlen(cls_str));
			break;
		case 4:
			strncat(cls_str, "Sensing device", sizeof(cls_str) - strlen(cls_str));
			break;
		case 5:
			strncat(cls_str, "Digitizer tablet", sizeof(cls_str) - strlen(cls_str));
		break;
		case 6:
			strncat(cls_str, "Card reader", sizeof(cls_str) - strlen(cls_str));
			break;
		default:
			strncat(cls_str, "(reserved)", sizeof(cls_str) - strlen(cls_str));
			break;
		}
		if (strlen(cls_str) > 0)
			return cls_str;
	}
	case 6:	/* imaging */
		if (minor & 4)
			return "Display";
		if (minor & 8)
			return "Camera";
		if (minor & 16)
			return "Scanner";
		if (minor & 32)
			return "Printer";
		break;
	case 7: /* wearable */
		switch (minor) {
		case 1:
			return "Wrist Watch";
		case 2:
			return "Pager";
		case 3:
			return "Jacket";
		case 4:
			return "Helmet";
		case 5:
			return "Glasses";
		}
		break;
	case 8: /* toy */
		switch (minor) {
		case 1:
			return "Robot";
		case 2:
			return "Vehicle";
		case 3:
			return "Doll / Action Figure";
		case 4:
			return "Controller";
		case 5:
			return "Game";
		}
		break;
	case 63:	/* uncategorised */
		return "";
	}
	return "Unknown (reserved) minor device class";
}
