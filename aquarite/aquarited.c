#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>

#include "mongoose.h"
#include "aq_serial.h"
#include "config.h"
#include "ar_config.h"

#include "utils.h"
#include "ar_net_services.h"

//#define SLOG_MAX 80
//#define PACKET_MAX 10000
#define CACHE_FILE "/tmp/aquarite.cache"

#define TIMEOUT_TRIES 10

#define AR_ID 0x50

struct aqconfig _config;
struct arconfig _ar_prms;

bool _keepRunning = true;
//int _percentsalt_ = 50;

void main_loop();

void intHandler(int dummy) {
  _keepRunning = false;
  logMessage(LOG_NOTICE, "Stopping!");
}

/*
void printHex(char *pk, int length) {
  int i = 0;
  for (i = 0; i < length; i++) {
    printf("0x%02hhx|", pk[i]);
  }
}

void printPacket(unsigned char ID, unsigned char *packet_buffer, int packet_length) {
  // if (packet_buffer[PKT_DEST] != 0x00)
  // printf("\n");

  printf("    Received  |");
  printf(" HEX: ");
  printHex((char *)packet_buffer, packet_length);

  if (packet_buffer[PKT_CMD] == CMD_MSG || packet_buffer[PKT_CMD] == CMD_MSG_LONG) {
    printf("  Message : ");
    // fwrite(packet_buffer + 4, 1, AQ_MSGLEN+1, stdout);
    fwrite(packet_buffer + 4, 1, packet_length - 7, stdout);
  }

  printf("\n");
}
*/

void debugPacketPrint(unsigned char ID, unsigned char *packet_buffer, int packet_length)
{
  char buff[1000];
  int i=0;
  int cnt = 0;

  //cnt = sprintf(buff, "%4.4s 0x%02hhx of type %8.8s", (packet_buffer[PKT_DEST]==0x00?"From":"To"), ID, get_packet_type(packet_buffer, packet_length));
  cnt += sprintf(buff+cnt, "Received %8.8s | HEX: ", get_packet_type(packet_buffer, packet_length));
  //printHex(packet_buffer, packet_length);
  for (i=0;i<packet_length;i++)
    cnt += sprintf(buff+cnt, "0x%02hhx|",packet_buffer[i]);
  
  if (packet_buffer[PKT_CMD] == CMD_MSG) {
    cnt += sprintf(buff+cnt, "  Message : ");
    //fwrite(packet_buffer + 4, 1, packet_length - 4, stdout);
    strncpy(buff+cnt, (char*)packet_buffer+PKT_DATA+1, AQ_MSGLEN);
    cnt += AQ_MSGLEN;
  }
  
  cnt += sprintf(buff+cnt,"\n");

  logMessage(LOG_DEBUG_SERIAL, "%s", buff);
}

void debugStatusPrint() {
  if (_ar_prms.status == 0x00) {
    logMessage(LOG_DEBUG_SERIAL, "*** Received status - OK ***\n");
  } else if (_ar_prms.status == 0x01) {
    logMessage(LOG_DEBUG_SERIAL, "*** Received status - NO FLOW ***\n");
  } else if (_ar_prms.status == 0x02) {
    logMessage(LOG_DEBUG_SERIAL, "*** Received status - LOW SALT ***\n");
  } else if (_ar_prms.status == 0x04) {
    logMessage(LOG_DEBUG_SERIAL, "*** Received status - VERY LOW SALT ***\n");
  } else if (_ar_prms.status == 0x08) {
    logMessage(LOG_DEBUG_SERIAL, "*** Received status - HIGH CURRENT ***\n");
  } else if (_ar_prms.status == 0x09) {
    logMessage(LOG_DEBUG_SERIAL, "*** Received status - TURNING OFF ***\n");
  } else if (_ar_prms.status == 0x10) {
    logMessage(LOG_DEBUG_SERIAL, "*** Received status - CLEAN CELL ***\n");
  } else if (_ar_prms.status == 0x20) {
    logMessage(LOG_DEBUG_SERIAL, "*** Received status - LOW VOLTAGE ***\n");
  } else if (_ar_prms.status == 0x40) {
    logMessage(LOG_DEBUG_SERIAL, "*** Received status - WATER TEMP LOW ***\n");
  } else if (_ar_prms.status == 0x80) {
    logMessage(LOG_DEBUG_SERIAL, "*** Received status - CHECK PCB ***\n");
  }
}

int main(int argc, char *argv[]) {
  //int rs_fd;
  //int packet_length;
  //unsigned char packet_buffer[AQ_MAXPKTLEN];
  //unsigned char lastID;
  //int received_packets = 0;
  //bool ar_connected = false;
  //int no_reply = 0;
  //struct mg_mgr mgr;
  // int sent = 0;
  int i;
  //int
  //struct config arconf;
  //struct aqconfig config;
  //struct arconfig ar_prms;

  char *cfgFile = "aquarited.conf";

  if (getuid() != 0) {
    fprintf(stderr, "ERROR %s Can only be run as root\n", argv[0]);
    return EXIT_FAILURE;
  }

  _ar_prms.PPM = TEMP_UNKNOWN;
  _ar_prms.Percent = 50;
  _ar_prms.generating = false;
  _ar_prms.cache_file = CACHE_FILE;

  // Initialize the daemon's parameters.
  init_parameters(&_config);

  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-d") == 0) {
      _config.deamonize = false;
    } else if (strcmp(argv[i], "-c") == 0) {
      cfgFile = argv[++i];
    }
  }

  readCfg(&_config, NULL, cfgFile);
  setLoggingPrms(_config.log_level, _config.deamonize, _config.log_file);

  read_cache(&_ar_prms);
  

  if (_config.deamonize == true) {
    char pidfile[256];
    // sprintf(pidfile, "%s/%s.pid",PIDLOCATION, basename(argv[0]));
    sprintf(pidfile, "%s/%s.pid", "/run", basename(argv[0]));
    daemonise(pidfile, main_loop);
  } else {
    main_loop();
  }

  exit(EXIT_SUCCESS);
}

void main_loop() {
  int rs_fd;
  int packet_length;
  unsigned char packet_buffer[AQ_MAXPKTLEN];
  int no_reply = 0;
  struct mg_mgr mgr;
  bool sendUpdate = false;
  bool broadcast = true;
  int cnt=0;

  logMessage(LOG_DEBUG, "Starting aquarited!\n");

  if (!start_net_services(&mgr, &_config, &_ar_prms)) {
    logMessage(LOG_ERR, "Can not start mqtt on port\n");
    exit(EXIT_FAILURE);
  }

  rs_fd = init_serial_port(_config.serial_port);

  if (rs_fd < 0) {
    logMessage(LOG_ERR, "Can not open serial port '%s'\n",_config.serial_port);
    exit(EXIT_FAILURE);
  }

  signal(SIGINT, intHandler);
  signal(SIGTERM, intHandler);

  mg_mgr_poll(&mgr, 500);

  send_command(rs_fd, AR_ID, CMD_PROBE, 0x62, NUL);
  logMessage(LOG_DEBUG_SERIAL,"Send Probe\n");
  
  while (_keepRunning == true) {
//printf("AR_CONNECTED = %d\n", _ar_prms.ar_connected);
    if (rs_fd < 0) {
      logMessage(LOG_DEBUG, "ERROR, serial port disconnect\n");
      _keepRunning = false;
    }
    
    if (broadcast == false)
      broadcast = broadcast_aquaritestate(mgr.active_connections);

    packet_length = get_packet(rs_fd, packet_buffer);

    if (packet_length == -1) {
      // Unrecoverable read error. Force an attempt to reconnect.
      logMessage(LOG_DEBUG, "ERROR, on serial port\n");
      _keepRunning = false;
    } else if (packet_length == 0) {
      // Nothing read
      logMessage(LOG_DEBUG_SERIAL,"Nothing read\n");
      //if (ar_connected == false && no_reply >= PING_POLL) {
        send_command(rs_fd, AR_ID, CMD_PROBE, 0x62, NUL);
        //logMessage(LOG_DEBUG_SERIAL,"Send Probe\n");
      //  no_reply = 0;
      //} else {
        no_reply++;
        if (no_reply >= TIMEOUT_TRIES) {
          logMessage(LOG_DEBUG_SERIAL,"*** %d BLANK READS *****\n",no_reply);
          if (_ar_prms.ar_connected == true || _ar_prms.generating == true) {
             _ar_prms.ar_connected = false;
             _ar_prms.generating = false;
             broadcast = broadcast_aquaritestate(mgr.active_connections);
          }/*
          if (_ar_prms.generating == true) {
            _ar_prms.generating = false;
            broadcast = broadcast_aquaritestate(mgr.active_connections);
          }*/
          //ar_prms.generating = false;
          no_reply = 0;
        }
      //}
      // sleep(1);
    } else if (packet_length > 0) {
      no_reply = 0;
      if (packet_buffer[PKT_DEST] == 0x00) {
        _ar_prms.ar_connected = true;
        if (getLogLevel() >= LOG_DEBUG_SERIAL)
          debugPacketPrint(AR_ID, packet_buffer, packet_length);

        switch (packet_buffer[PKT_CMD]) {
        case CMD_ACK:
          if (_ar_prms.generating == false) {
            // Chlorinator Translator =   GetID | HEX: 0x10|0x02|0x50|0x14|0x00|0x76|0x10|0x03|
            // AquaLinkRD To 0x50 of type GetID | HEX: 0x10|0x02|0x50|0x14|0x01|0x77|0x10|0x03|
            send_command(rs_fd, AR_ID, CMD_GETID, 0x01, 0x77); // Returns AquaPure or AquaRite
            //send_command(rs_fd, AR_ID, CMD_GETID, 0x00, 0x76); // Returns BOOTS
            //logMessage(LOG_DEBUG_SERIAL,"Send Get Status/ID\n");
          } else {
            send_command(rs_fd, AR_ID, CMD_PERCENT, (unsigned char)_ar_prms.Percent, NUL);
            //logMessage(LOG_DEBUG_SERIAL,"Send set percent salt to %d\n",_ar_prms.Percent);
          }
          break;
        case CMD_PPM:
          logMessage(LOG_DEBUG_SERIAL,"Received PPM %d\n", (packet_buffer[4] * 100));
          sendUpdate = false;

          if (_ar_prms.status != packet_buffer[5]) {
            _ar_prms.status = packet_buffer[5];
            sendUpdate = true;
          }

          if (_ar_prms.status == 0x00 || _ar_prms.status == 0x04 || _ar_prms.status == 0x20) {
            if (_ar_prms.PPM != packet_buffer[4] * 100) {
              _ar_prms.PPM = packet_buffer[4] * 100;
              //broadcast_aquaritestate(mgr.active_connections);
              sendUpdate = true;
              write_cache(&_ar_prms);
            }
            if (_ar_prms.generating == false) {
              _ar_prms.generating = true;
              //broadcast_aquaritestate(mgr.active_connections);
              sendUpdate = true;
            }
          } else {
            if (_ar_prms.generating == true) {
              _ar_prms.generating = false;
              //broadcast_aquaritestate(mgr.active_connections);
              sendUpdate = true;
            }
          }

          if (sendUpdate)
            broadcast = broadcast_aquaritestate(mgr.active_connections);

          if (getLogLevel() >= LOG_DEBUG_SERIAL && _ar_prms.status != 0x00)
            debugStatusPrint();

        case CMD_MSG: // Want to fall through
          send_command(rs_fd, AR_ID, CMD_PERCENT, (unsigned char)_ar_prms.Percent, NUL);
          break;
        // case 0x16:
        // break;
        default:
          printf("do nothing, didn't understand return\n");
          break;
        }
      }

      //printf("Packets received %d\n", received_packets++);
    }
    //sleep(1);
    check_net_services(&mgr);
    mg_mgr_poll(&mgr, 500);
    
    if (cnt >= 600) {
      cnt = 0;
      if (_ar_prms.ar_connected == true)
        broadcast = false;  
    }
    cnt ++;
  }

  return;
}
