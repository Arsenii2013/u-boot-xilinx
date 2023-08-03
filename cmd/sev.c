#include <common.h>
#include <command.h>

int do_sev(struct cmd_tbl *cmdtp, int flag, int argc,
           char *const argv[]){
    __asm__("sev");
    return CMD_RET_SUCCESS;
}

U_BOOT_CMD(sev, 1,  0,  do_sev,
       "just asm sev", "just asm sev"
);
