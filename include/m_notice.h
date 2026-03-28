#ifndef M_NOTICE_H
#define M_NOTICE_H

#include "types.h"
#include "m_mail.h"
#include "lb_rtc.h"

#ifdef __cplusplus
extern "C" {
#endif

#define mNtc_BOARD_POST_COUNT 15

/* sizeof(mNtc_board_post) == 0xC8 */
typedef struct notice_board_post_s {
  /* 0x00 */ u8 message[MAIL_BODY_LEN]; /* post contents */
  /* 0xC0 */ lbRTC_time_c post_time; /* date-time of post */
} mNtc_board_post_c;

extern void mNtc_SetInitData();
extern int mNtc_notice_write_num();
extern void mNtc_notice_write(mNtc_board_post_c* new_post);
extern void mNtc_auto_nwrite_time_ct();
extern void mNtc_set_auto_nwrite_data();

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

extern int mNtc_get_message_of_the_week();

class BBSList {
public:
    int mCount;
    int mNumSet;
    int* mItems;
    int* mIndices;

    virtual ~BBSList();

    int GetIdx(int i) const;
    int GetItem(int i) const;
};

template <typename T>
class RngList : public BBSList {
public:
    virtual ~RngList();

    void Reset();
    void AddIdx(int idx);
    void ClearIdx(int idx);
    BOOL IsSet(int idx) const;
    BOOL IsItemSet(int item) const;
    int GetN(int* out, int n);
    T Get();
    T GetAndCrossOff();
};

template <typename T>
class InverseRngList : public RngList<T> {
public:
    virtual ~InverseRngList();

    void Reset();
    void AddIdx(int idx);
    void ClearIdx(int idx);
    BOOL IsSet(int idx) const;
    int GetN(int* out, int n);
    T Get();
    T GetAndCrossOff();
};

template <typename T>
class BackedInverseRngList : public InverseRngList<T> {
public:
    u32* mBackingData;

    BackedInverseRngList(u32* data, int count);
    virtual ~BackedInverseRngList();
};

#endif /* __cplusplus */

#endif
