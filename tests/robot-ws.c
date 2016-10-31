/*
  Robot test program
*/
#include <stdarg.h>
#include "simpletools.h"
#include "abdrive.h"
#include "ping.h"
#include "cmd.h"

// uncomment this if the wifi module is on pins other than 31/30
//#define SEPARATE_WIFI_PINS

#ifdef SEPARATE_WIFI_PINS
#define WIFI_RX     9
#define WIFI_TX     8
#else
#define WIFI_RX     31
#define WIFI_TX     30
#endif

#define PING_PIN    10

#define DEBUG

int wheelLeft;
int wheelRight;

void init_robot(void);
int process_robot_command(int whichWay);            
void set_robot_speed(int left, int right);

int main(void)
{    
    int pingHandle = 0;
    int lastPingDistance = -1;
    int handle;
    
    cmd_init(WIFI_RX, WIFI_TX, 31, 30);
    
    init_robot();

    request("SET:cmd-pause-time,5");
    waitFor(CMD_START "=S,0\r");

    request("LISTEN:WS,/ws/robot");
    waitFor(CMD_START "=S,^d\r", &handle);
    
    for (;;) {
        char url[128], arg[128];
        int type, handle, listener, count, pingDistance;
        char result;
        
        waitcnt(CNT + CLKFREQ/4);

        request("POLL");
        waitFor(CMD_START "=^c,^i,^i\r", &type, &handle, &listener);
        if (type != 'N')
            dprint(debug, "Got %c: handle %d, listener %d\n", type, handle, listener);
        
        switch (type) {
        case 'W':
            request("PATH:%d", handle);
            waitFor(CMD_START "=S,^s\r", url, sizeof(url));
            dprint(debug, "%d: path '%s'\n", handle, url);
            pingHandle = handle;
            break;
        case 'D':
            request("RECV:%d,%d", handle, sizeof(arg));
            waitFor(CMD_START "=S,^i\r", &count);
            collectPayload(arg, sizeof(arg), count);
            dprint(debug, "%d: PAYLOAD %d\n", pingHandle, count);
            if (process_robot_command(arg[0]) != 0)
                dprint(debug, "Unknown robot command: '%c'\n", arg[0]);
            break;
        case 'S':
            dprint(debug, "Send completed\n");
            break;
        case 'N':
            break;
        default:
            dprint(debug, "unknown response: 0x%02x\n", type);
            break;
        }

        if (pingHandle > 0) {
            if ((pingDistance = ping_cm(PING_PIN)) != lastPingDistance) {
                char buf[128];
                dprint(debug, "New PING))) distance: %d\n", pingDistance);
                lastPingDistance = pingDistance;
                sprintf(buf, "%d", pingDistance);
                request("SEND:%d,%d", pingHandle, strlen(buf));
                requestPayload(buf, strlen(buf));
                waitFor(CMD_START "=^c,^i\r", &result, &count);
                dprint(debug, " got %c %d\n", result, count);
            }
        }
    }
    
    return 0;
}

void init_robot(void)
{
  wheelLeft = wheelRight = 0;
  high(26);
  set_robot_speed(wheelLeft, wheelRight);
}

int process_robot_command(int whichWay)            
{ 
  toggle(26);
      
  switch (whichWay) {
  
  case 'F': // forward
    #ifdef DEBUG
      dprint(debug, "Forward\n");
    #endif
    if (wheelLeft > wheelRight)
      wheelRight = wheelLeft;
    else if (wheelLeft < wheelRight) 
      wheelLeft = wheelRight;
    else {           
      wheelLeft = wheelLeft + 32;
      wheelRight = wheelRight + 32;
    }      
    break;    
    
  case 'R': // right
    #ifdef DEBUG
      dprint(debug, "Right\n");
    #endif
    wheelLeft = wheelLeft + 16;
    wheelRight = wheelRight - 16;
    break;
    
  case 'L': // left
    #ifdef DEBUG
      dprint(debug, "Left\n");
    #endif
    wheelLeft = wheelLeft - 16;
    wheelRight = wheelRight + 16;
    break;
    
  case 'B': // reverse
    #ifdef DEBUG
      dprint(debug, "Reverse\n");
    #endif
    if(wheelLeft < wheelRight)
      wheelRight = wheelLeft;
    else if (wheelLeft > wheelRight) 
      wheelLeft = wheelRight;
    else {           
      wheelLeft = wheelLeft - 32;
      wheelRight = wheelRight - 32;
    }
    break;  
        
  case 'S': // stop
    #ifdef DEBUG
      dprint(debug, "Stop\n");
    #endif
    wheelLeft = 0;
    wheelRight = 0;
    break;
    
  default:  // unknown request
    return -1;
  }    
  
  if (wheelLeft > 128) wheelLeft = 128;
  if (wheelLeft < -128) wheelLeft = -128;
  if (wheelRight > 128) wheelRight = 128;
  if (wheelRight < -128) wheelRight = -128;
  
  set_robot_speed(wheelLeft, wheelRight);
    
  return 0;
}

void set_robot_speed(int left, int right)
{  
  #ifdef DEBUG
    dprint(debug, "L %d, R %d\n", wheelLeft, wheelRight);
  #endif
  
  wheelLeft = left;
  wheelRight = right;
  drive_speed(wheelLeft, wheelRight);
}
