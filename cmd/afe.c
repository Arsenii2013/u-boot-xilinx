#include <common.h>
#include <command.h>
#include <dm.h>
#include <asm/gpio.h>   
#include <time.h>

#define EMIO_PIN      54
#define AFE_CONF_PIN  EMIO_PIN + 3
#define nCONFIG_PIN   AFE_CONF_PIN
#define nSTATUS_PIN   AFE_CONF_PIN + 1
#define CONF_DONE_PIN AFE_CONF_PIN + 2
#define INIT_DONE_PIN AFE_CONF_PIN + 3
#define DCLK_PIN      AFE_CONF_PIN + 4
#define DATA_PIN      AFE_CONF_PIN + 5
#define MSEL0_PIN     AFE_CONF_PIN + 6
#define MSEL1_PIN     AFE_CONF_PIN + 7
#define PRSNT_PIN     AFE_CONF_PIN + 8
#define PWR_GD_PIN    AFE_CONF_PIN + 9
#define PWR_ENA_PIN   AFE_CONF_PIN + 10

void delay_us(unsigned long us){
    unsigned long start = get_timer(0);
    while(get_timer(start) * 1000000 / CONFIG_SYS_HZ < us);
}

int gpio_wait_value_us(unsigned int gpio, int value, unsigned long us){
    unsigned long start = get_timer(0);
    while(get_timer(start) * 1000000 / CONFIG_SYS_HZ < us){
        int ret = gpio_get_value(gpio);
        if(ret == value)
            return ret;
    }
    return -1;
}

int setup_pins(void){

	int ret;
	ret = gpio_request(PRSNT_PIN, "cmd_gpio");
	if (ret && ret != -EBUSY) {
		printf("ERROR: requesting pin PRSNT_PIN failed\n");
		return -1;
	}
	ret = gpio_request(PWR_GD_PIN, "cmd_gpio");
	if (ret && ret != -EBUSY) {
		printf("ERROR: requesting pin PWR_GD_PIN failed\n");
		return -1;
	}
	ret = gpio_request(PWR_ENA_PIN, "cmd_gpio");
	if (ret && ret != -EBUSY) {
		printf("ERROR: requesting pin PWR_ENA_PIN failed\n");
		return -1;
	}
	ret = gpio_request(MSEL0_PIN, "cmd_gpio");
	if (ret && ret != -EBUSY) {
		printf("ERROR: requesting pin MSEL0_PIN failed\n");
		return -1;
	}
	ret = gpio_request(MSEL1_PIN, "cmd_gpio");
	if (ret && ret != -EBUSY) {
		printf("ERROR: requesting pin MSEL1_PIN failed\n");
		return -1;
	}
	ret = gpio_request(nCONFIG_PIN, "cmd_gpio");
	if (ret && ret != -EBUSY) {
		printf("ERROR: requesting pin nCONFIG_PIN failed\n");
		return -1;
	}
	ret = gpio_request(nSTATUS_PIN, "cmd_gpio");
	if (ret && ret != -EBUSY) {
		printf("ERROR: requesting pin nSTATUS_PIN failed\n");
		return -1;
	}
	ret = gpio_request(CONF_DONE_PIN, "cmd_gpio");
	if (ret && ret != -EBUSY) {
		printf("ERROR: requesting pin CONF_DONE_PIN failed\n");
		return -1;
	}
	ret = gpio_request(INIT_DONE_PIN, "cmd_gpio");
	if (ret && ret != -EBUSY) {
		printf("ERROR: requesting pin INIT_DONE_PIN failed\n");
		return -1;
	}
	ret = gpio_request(DCLK_PIN, "cmd_gpio");
	if (ret && ret != -EBUSY) {
		printf("ERROR: requesting pin DCLK_PIN failed\n");
		return -1;
	}
	ret = gpio_request(DATA_PIN, "cmd_gpio");
	if (ret && ret != -EBUSY) {
		printf("ERROR: requesting pin DATA_PIN failed\n");
		return -1;
	}
    gpio_direction_input(MSEL0_PIN);
    gpio_direction_input(MSEL1_PIN);
    
    gpio_direction_output(DCLK_PIN, 0);
    gpio_direction_output(DATA_PIN, 0);

    gpio_direction_input(PRSNT_PIN);  
    gpio_direction_input(PWR_GD_PIN);  
    gpio_direction_output(PWR_ENA_PIN, 0);

    gpio_direction_output(nCONFIG_PIN, 1);
    gpio_direction_input(nSTATUS_PIN);  
    gpio_direction_input(CONF_DONE_PIN);  
    gpio_direction_input(INIT_DONE_PIN);  
    return 0;
}

int afe_prsnt(void){
    int prsntVal = gpio_get_value(PRSNT_PIN);
    printf("INFO: AFE present %d\n", prsntVal);
    return CMD_RET_SUCCESS;
}

enum AFEPwrCmd {
    OFF = 0,
    ON = 1,
    GET = 2
};
int afe_pwr(enum AFEPwrCmd cmd){
    if(cmd == GET){
        int pgVal = gpio_get_value(PWR_GD_PIN);
        printf("INFO: AFE power good %d\n", pgVal);
        return CMD_RET_SUCCESS;
    } else if(cmd == ON) {
        printf("INFO: AFE power enable 1\n");
        gpio_set_value(PWR_ENA_PIN, 1);
        return CMD_RET_SUCCESS;
    } else if(cmd == OFF) {
        printf("INFO: AFE power enable 0\n");
        gpio_set_value(PWR_ENA_PIN, 0);
        return CMD_RET_SUCCESS;
    }
    return CMD_RET_FAILURE;
}

int start_config(void){
    gpio_direction_output(MSEL0_PIN, 0);
    gpio_direction_output(MSEL1_PIN, 0);
    delay_us(10000);

    gpio_set_value(nCONFIG_PIN, 0);
    delay_us(1);
    gpio_set_value(nCONFIG_PIN, 1);
    if(gpio_get_value(nSTATUS_PIN) != 0)
        return -1;
    if(gpio_wait_value_us(nSTATUS_PIN, 1, 230) == -1)
        return -1;
    if(gpio_get_value(CONF_DONE_PIN) != 0)
        return -1;
    delay_us(2);
    return 0;
}

void send_bit(uint32_t bit){
    gpio_set_value(DATA_PIN, bit);
    gpio_set_value(DCLK_PIN, 1);
    gpio_set_value(DCLK_PIN, 0);
}

void send_word(uint32_t word){
    for(int bit_n = 0; bit_n < 32; bit_n++){
        send_bit(word & (1 << bit_n));
    }
}

unsigned long send_data(unsigned long addr, unsigned long len){
    if(gpio_get_value(CONF_DONE_PIN) == 1)
        return -1;

    uint32_t * cur_word = (uint32_t*)addr;
    for(unsigned long send_cnt = 0; send_cnt < len / 4; send_cnt++){
        if(gpio_get_value(CONF_DONE_PIN) == 1){
            return send_cnt*4;
        }
        send_word(*cur_word);
        cur_word++;
    }
    return 0;
} 

int end_config(void){
    if(gpio_get_value(CONF_DONE_PIN) != 1)
        return -1;
    if(gpio_wait_value_us(INIT_DONE_PIN, 1, 650) == -1)
        return -1;
    delay_us(10000);
    gpio_direction_input(MSEL0_PIN);
    gpio_direction_input(MSEL1_PIN);
    return 0;
}

int afe_load(unsigned long addr, unsigned long size){
    int ret;
    unsigned long uret;
    ret = start_config();
    if(ret == -1){
	    printf("ERROR: start\n");
        return CMD_RET_FAILURE;
    }
    uret = send_data(addr, size);
    if(uret == -1){
	    printf("ERROR: sending data\n");
        return CMD_RET_FAILURE;
    } else if(uret){
        printf("INFO: actual bitstream size %lx\n", uret);
    }
    ret = end_config();
    if(ret == -1){
	    printf("ERROR: end\n");
        return CMD_RET_FAILURE;
    }
    printf("INFO: OK\n");
    return CMD_RET_SUCCESS;
}

int do_afe(struct cmd_tbl *cmdtp, int flag, int argc,
           char *const argv[]){

    if (argc < 2)
        return CMD_RET_USAGE;

    if(setup_pins() == -1)
        return CMD_RET_FAILURE;

    if(strncmp(argv[1], "prsnt", 5) == 0){
        return afe_prsnt();
    } else if(strncmp(argv[1], "pwr", 1) == 0){
        enum AFEPwrCmd cmd;
        if(strncmp(argv[2], "on", 2) == 0){
            cmd = ON;
        } else if(strncmp(argv[2], "off", 2) == 0){
            cmd = OFF;
        } else if(argc == 2 || strncmp(argv[2], "get", 1) == 0){
            cmd = GET;
        } else {
            return CMD_RET_USAGE;
        }
        return afe_pwr(cmd);
    } else if(strncmp(argv[1], "load", 1) == 0){
        unsigned long addr;
        unsigned long size;
        char *endp;

        addr = hextoul(argv[2], &endp);
        if (*argv[2] == 0 || *endp != 0)
            return CMD_RET_USAGE;
        
        size = hextoul(argv[3], &endp);
        if (*argv[3] == 0 || *endp != 0)
            return CMD_RET_USAGE;
        return afe_load(addr, size);
    }

    

    return CMD_RET_USAGE;
}

U_BOOT_CMD(afe, 4,  0,  do_afe,
       "AFE board operations:",
       "[operation type] <arguments>\n"
       "\tprsnt             - get AFE_PRSNT signal\n"
       "\tpwr  [get|on|off] - get/set/clear AFE_PWR_GD/AFE_PWR_ENA signals\n"
       "\tload <addr> <len> - load AFE bitstream\n"
);
 
