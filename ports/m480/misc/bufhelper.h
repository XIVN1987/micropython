#ifndef __MISC_BUFHELPER_H__
#define __MISC_BUFHELPER_H__


void pyb_buf_get_for_send(mp_obj_t o, mp_buffer_info_t *bufinfo, byte *tmp_data);
mp_obj_t pyb_buf_get_for_recv(mp_obj_t o, vstr_t *vstr);


#endif
