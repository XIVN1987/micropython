#ifndef __MISC_TELNET_H__
#define __MISC_TELNET_H__

/******************************************************************************
 DECLARE EXPORTED FUNCTIONS
 ******************************************************************************/
extern void telnet_init (void);
extern void telnet_run (void);
extern void telnet_tx_strn (const char *str, int len);
extern bool telnet_rx_any (void);
extern int  telnet_rx_char (void);
extern void telnet_enable (void);
extern void telnet_disable (void);
extern void telnet_reset (void);


#endif
