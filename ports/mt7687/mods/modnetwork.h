#ifndef __MODS_MODNETWORK_H__
#define __MODS_MODNETWORK_H__

/******************************************************************************
 DEFINE CONSTANTS
 ******************************************************************************/
#define MOD_NETWORK_IPV4ADDR_BUF_SIZE             (4)

/******************************************************************************
 DEFINE TYPES
 ******************************************************************************/
typedef struct _mod_network_nic_type_t {
    mp_obj_type_t base;
} mod_network_nic_type_t;

typedef struct _mod_network_socket_base_t {
    union {
        struct {
            // this order is important so that fileno gets > 0 once
            // the socket descriptor is assigned after being created.
            uint8_t domain;
            int8_t fileno;
            uint8_t type;
            uint8_t proto;
        } u_param;
        int16_t sd;
    };
    uint32_t timeout_ms; // 0 for no timeout
    bool cert_req;
} mod_network_socket_base_t;

typedef struct _mod_network_socket_obj_t {
    mp_obj_base_t base;
    mod_network_socket_base_t sock_base;
} mod_network_socket_obj_t;

/******************************************************************************
 EXPORTED DATA
 ******************************************************************************/
extern const mod_network_nic_type_t mod_network_nic_type_wlan;

/******************************************************************************
 DECLARE FUNCTIONS
 ******************************************************************************/
void mod_network_init0(void);

#endif
