#ifndef __DRV_OSD_H__
#define __DRV_OSD_H__

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

void osd_init();
void osd_hide(void);
void osd_show(void);
void osd_print(int row, int col, const char *s);
void osd_erase(int row, int col, int count);
void osd_clear();
void osd_refresh(void);
int  osd_get_max_rows();

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif  // __DRV_OSD_H__
