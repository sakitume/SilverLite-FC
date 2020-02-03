#ifndef __SILVERLITE_H__
#define __SILVERLITE_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void silverlite_init(void);
extern void silverlite_update(void);
extern bool silverlite_postupdate(uint32_t max_used_loop_time);
extern void silverlite_poll(void);

#ifdef __cplusplus
}
#endif

#endif 