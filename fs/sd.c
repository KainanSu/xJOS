#include "fs/fs.h"
#include <inc/arm.h>
#include <fs/sd_card.h>
#include <fs/imx_usdhc.h>
#include <inc/lib.h>
#include <inc/mmu.h>

sdcard_t sdcard = {
    .host = (imx_usdhc_t *)(FS_SD_BASE),
    .rca = 0x45670000,
    .host_init = usdhc_init,
    .send_cmd = usdhc_send_command,
    .get_resp = usdhc_get_response,
    .read_block = usdhc_read_block,
    .write_block = usdhc_write_block
};

int sd_init(){
    return sdcard_init(&sdcard);
}


int sd_read(uint32_t secno, void *dst, size_t nsecs)
{
    for(int i = 0; i < nsecs; i++){
        void *_dst = (void*)((uint32_t)dst + i * SECTSIZE);
        if(sdcard_read_block(&sdcard, user_get_pa(_dst), (secno + i) * 512) != 0)
            panic("sdcard_read_block err");
    }
    return 0;
}

int sd_write(uint32_t secno, const void *src, size_t nsecs)
{
    log_trace("sd_write: secno=%d src=%p nsecs=%d\n", secno, src, nsecs);
    for(int i = 0; i < nsecs; i++){
        void *_src = (void*)((uint32_t)src + i * SECTSIZE);
        if(sdcard_write_block(&sdcard, user_get_pa(_src), (secno + i) * 512) != 0)
            panic("sdcard_write_block err");
    }
    return 0;
}
