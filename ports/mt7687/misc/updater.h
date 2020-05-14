#ifndef __MISC_UPDATER_H__
#define __MISC_UPDATER_H__

extern void updater_pre_init (void);
extern bool updater_check_path (void *path);
extern bool updater_start (void);
extern bool updater_write (uint8_t *buf, uint32_t len);
extern void updater_finnish (void);
extern bool updater_verify (uint8_t *rbuff, uint8_t *hasbuff);

#endif
