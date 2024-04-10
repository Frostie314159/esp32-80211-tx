#include "freedom_tx.h"

extern wifi_osi_funcs_t* g_osi_funcs_p;
extern void* g_wifi_global_lock;
extern char* g_wifi_nvs;
void* ic_ebuf_alloc(const void*, uint32_t, uint32_t);
void* ic_get_default_sched();
char ic_get_80211_tx_rate(char);
void* ieee80211_post_hmac_tx(void*);

esp_err_t esp_wifi_80211_tx_openmac(wifi_interface_t ifx, const void *buffer, int len, bool en_sys_seq) {
    esp_err_t ret = 0;
    g_osi_funcs_p->_mutex_lock(g_wifi_global_lock);
    void* ebuf = ic_ebuf_alloc(buffer, 0x1, len);
    if (ebuf == NULL) {
        ret = 0x101;
        goto exit;
    }
    *(uint16_t *)(ebuf + 0x14) = 0x18;
    *(uint16_t *)(ebuf + 0x16) = len - 0x18;
    **(uint32_t **)(ebuf + 0x2c) |= 0x4000;

    void* unknown = *(void **)(ebuf + 0x2c);
    *(void **)(unknown + 0x1c) = ic_get_default_sched();

    char tx_rate = ic_get_80211_tx_rate(ifx & 0xff);
    *(char *)(unknown + 0xc) = tx_rate;
    if (ifx == 0) {
        *(uint32_t *)(unknown + 0x10) = ((*(uint32_t *)(unknown + 0x10) ) & 0xfffbffff) | ((uint32_t)(g_wifi_nvs[0x9d] == 2) << 0x12);
    } else {
        *(uint32_t *)(unknown + 0x10) = ((*(uint32_t *)(unknown + 0x10) ) & 0xfffbffff) | ((uint32_t)(g_wifi_nvs[0x3fb] == 2) << 0x12);
    }  

    if (en_sys_seq != 0) {
        **(uint32_t **)(ebuf + 0x2c) = (**(uint32_t **)(ebuf + 0x2c)) | 0x1;
    }

    *(uint *)(unknown + 0x10) = (*(uint *)(unknown + 0x10) & 0xfff7ffff) | ((ifx & 0x1) << 0x13);

    ieee80211_post_hmac_tx(ebuf);

    exit:
    g_osi_funcs_p->_mutex_unlock(g_wifi_global_lock);
    return ret;
}
