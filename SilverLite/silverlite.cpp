#include "silverlite.h"
#include "drv_osd.h"
#include "console.h"
#include "config.h"
#include <string.h>

//------------------------------------------------------------------------------
extern "C" {
    extern int packetpersecond;
    extern int hw_crc_errors, b_crc_errors;
    extern int timingfail_errors, skipchannel_errors;
    extern int pkt_hits;
    extern int onground;
    extern char aux[];
    extern float vbattfilt;
    extern bool lowbatt;

    extern float pidkp[];
    extern float pidki[];
    extern float pidkd[];
    extern float apidkp[];
    extern float apidkd[];

    extern uint32_t gettime();
    extern const char *tprintf(const char* fmt, ...);
}

//------------------------------------------------------------------------------
static void update_osd();

//------------------------------------------------------------------------------
static float _custom_pidkp[] = ACRO_P;
static float _custom_pidki[] = ACRO_I;
static float _custom_pidkd[] = ACRO_D;

//------------------------------------------------------------------------------
void silverlite_init(void)
{
    console_init();
    osd_init();
    update_osd();

    // Update acro PID terms
    for (int i=0; i<3; i++)
    {
        pidkp[i] = _custom_pidkp[i];
        pidki[i] = _custom_pidki[i];
        pidkd[i] = _custom_pidkd[i];
    }

    // Update angle mode PID terms
    apidkp[0] = ANGLE_P1;
    apidkd[0] = ANGLE_D1;
}

//------------------------------------------------------------------------------
static uint32_t osd_ticks = 0;
static uint32_t flyTimeTicks = 0;
static const char *flyModes[] = { "ACRO ", "LEVEL" };
static uint32_t osd_time;
static uint32_t _max_loop_time;
static void update_osd()
{
    int row;
    const char *flyMode;
#ifdef LEVELMODE
    if ( aux[ LEVELMODE ] ) 
    {
        flyMode = flyModes[1];
    }
    else
#endif      
    {
        flyMode = flyModes[0];
    }

    // Fly mode on upper left
    row = 1;
    osd_print(row, 1, flyMode);

    // RSSI (packets per second), on upper right
    const char *s = tprintf("\x01%3d", packetpersecond);
    osd_print(row, 30 - strlen(s) - 1, s);
    s = tprintf("%3d", _max_loop_time);
    osd_print(row+1, 24, s);
    s = tprintf("%3d", osd_time);
    osd_print(row+2, 24, s);


    // Battery voltage on lower left
    uint8_t maxRows = osd_get_max_rows();
    row = maxRows - 1;
    const int vbatt = vbattfilt * 100 + 0.5f;
    int volts = vbatt/100;
    s = tprintf("%d.%02dV", volts, vbatt%100);
    osd_print(row, 1, s);

    // Flight time on lower right
    uint32_t flyTimeSec =  (flyTimeTicks / (1000000 / LOOPTIME));
    uint32_t minutes = flyTimeSec / 60;
    s = tprintf("\x9C%d:%02d", minutes, flyTimeSec%60);
    osd_print(row, 30 - strlen(s) - 1, s);

    s = "LOW BATTERY";
    const int sLen = 11;    // strlen(s);
    if (lowbatt)
    {
        osd_print(maxRows/4*3, (30 - sLen) / 2, s);
    }
    else
    {
        osd_erase(maxRows/4*3, (30 - sLen) / 2, sLen);
    }
    osd_refresh();
}

//------------------------------------------------------------------------------
bool silverlite_update()
{
    // Make sure USB VCP is updated or risk losing connection
    console_poll();

    if (!onground)
    {
        flyTimeTicks++;
    }

    // Compute total elapsed milliseconds
    uint32_t ms =  osd_ticks++ / (1000 / LOOPTIME);

    // Every 10th of a second (100ms) we'll update the osd
    if (0 == (ms % 100))
    {
        uint32_t now = gettime();
        update_osd();
        osd_time = gettime() - now;

        // return false to signal to main loop to not count this frame
        // when determining max loop time. OSD update/refresh spikes
        // will always occur and are being measured above with osd_time
        // and therefore shouldn't be considered
        return false;
    }
    return true;
}

//------------------------------------------------------------------------------
// Make sure USB VCP is updated or risk losing connection
void silverlite_poll(void)
{
    console_poll();
}

//------------------------------------------------------------------------------
bool silverlite_postupdate(uint32_t max_used_loop_time)
{
    _max_loop_time = max_used_loop_time;
    static uint32_t secondTimer;
    if ((gettime() - secondTimer) >= 1000000)
    {
        uint32_t now = gettime();

#if 0
        osd_clear(1);
        int row = 1;
        osd_print(row++, 1, tprintf("LOOPTIME: %d", max_used_loop_time));
        osd_print(row++, 1, tprintf("PPS: %3d, HIT: %d", packetpersecond, pkt_hits));
        osd_print(row++, 1, tprintf("HCRC: %2d, BCRC: %d", hw_crc_errors, b_crc_errors));
        osd_print(row++, 1, tprintf("TIMEFAIL: %2d SKIPFAIL: %d", timingfail_errors, skipchannel_errors));
        osd_time = gettime() - now;
        osd_print(row++, 1, tprintf("OSD: %2d", osd_time));
//        osd_print(row++, 1, "\x9B \x9C \x70 \x71");
        // x9C == "fly/mn"
        // x01 == "signal bars"
        osd_print(row++, 1, "\x9B \x9C \x70 \x71 \x01 \x06");
#endif

#if 1			
        // Sending this packet takes approx 500us
        console_openPacket();
        console_appendPacket16(max_used_loop_time);
        console_appendPacket16(packetpersecond);
        console_appendPacket16(pkt_hits);
        console_appendPacket16(hw_crc_errors);
        console_appendPacket16(b_crc_errors);
        console_appendPacket16(osd_time);
        console_closePacket(0x01);
#endif
        
        max_used_loop_time = 0;
        secondTimer=  gettime();
        osd_time = secondTimer - now;
        return true;
    }
    
    return false;
}

