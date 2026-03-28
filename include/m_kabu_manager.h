#ifndef M_KABU_MANAGER_H
#define M_KABU_MANAGER_H

#include "types.h"
#include "lb_rtc.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
  Kabu_TRADE_MARKET_TYPE_START,

  Kabu_TRADE_MARKET_TYPE_A = Kabu_TRADE_MARKET_TYPE_START, /* spike trend */
  Kabu_TRADE_MARKET_TYPE_B, /* random trend */
  Kabu_TRADE_MARKET_TYPE_C, /* falling trend */

  Kabu_TRADE_MARKET_TYPE_NUM
};

#define Kabu_PRICE_MIN 10
#define Kabu_PRICE_MAX 2000

/* sizeof(Kabu_price_c) == 0x18 */
typedef struct kabu_price_s {
  /* 0x00 */ u16 daily_price[lbRTC_WEEKDAYS_MAX];
  /* 0x0E */ u16 trade_market;
  /* 0x10 */ lbRTC_time_c update_time;
} Kabu_price_c;

extern void Kabu_decide_price_schedule();
extern u16 Kabu_get_price();
extern void Kabu_manager();

#if VERSION >= VER_DELUXE
/* Deluxe new half-day Stalk Market (AM/PM prices, 5 trend types) */
#define Kabu_NOON_HOUR 12

/* sizeof(Kabu_price_new_c) == 0x24 */
typedef struct kabu_price_new_s {
  /* 0x00 */ u16 daily_price[lbRTC_WEEKDAYS_MAX][2]; /* [day][0=AM, 1=PM] */
  /* 0x1C */ u8 trade_market;
  /* 0x1D */ u8 pad;
  /* 0x1E */ lbRTC_ymd_c update_date;
  /* 0x22 */ u8 pad2[2];
} Kabu_price_new_c;

extern void Kabu_decide_price_schedule_new();
extern u16 Kabu_get_price_new();
extern void Kabu_manager_new();
#endif

#ifdef __cplusplus
}
#endif

#endif
