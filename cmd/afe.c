#include <common.h>
#include <command.h>
#include <dm.h>
#include <asm/gpio.h>   
#include <time.h>

#define EMIO_PIN      54
#define AFE_CONF_PIN  EMIO_PIN + 4
#define nCONFIG_PIN   AFE_CONF_PIN
#define nSTATUS_PIN   AFE_CONF_PIN + 1
#define CONF_DONE_PIN AFE_CONF_PIN + 2
#define INIT_DONE_PIN AFE_CONF_PIN + 3
#define DCLK_PIN      AFE_CONF_PIN + 4
#define DATA_PIN      AFE_CONF_PIN + 5

#define MSEL0_PIN AFE_CONF_PIN + 6
#define MSEL1_PIN AFE_CONF_PIN + 7

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

int start_config(void){

	int ret;
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
    
    gpio_direction_output(MSEL0_PIN, 0);
    gpio_direction_output(MSEL1_PIN, 0);
    gpio_direction_output(DCLK_PIN, 0);
    gpio_direction_output(DATA_PIN, 0);

    gpio_direction_output(nCONFIG_PIN, 0);
    gpio_direction_input(nSTATUS_PIN);  
    gpio_direction_input(CONF_DONE_PIN);  
    gpio_direction_input(INIT_DONE_PIN);  
    delay_us(1);
    gpio_direction_output(nCONFIG_PIN, 1);
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
    gpio_direction_output(DATA_PIN, bit);
    gpio_direction_output(DCLK_PIN, 1);
    gpio_direction_output(DCLK_PIN, 0);
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
    return 0;
}

int do_afe(struct cmd_tbl *cmdtp, int flag, int argc,
           char *const argv[]){

    unsigned long addr;
    unsigned long size;
    char *endp;
    int ret;
    unsigned long uret;

    if (argc != 3)
        return CMD_RET_USAGE;
    
    addr = hextoul(argv[1], &endp);
    if (*argv[1] == 0 || *endp != 0)
        return CMD_RET_USAGE;
    
    size = hextoul(argv[2], &endp);
    if (*argv[2] == 0 || *endp != 0)
        return CMD_RET_USAGE;

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

U_BOOT_CMD(afe, 3,  0,  do_afe,
       "Load AFE board firmware",
       "addr len - load AFE bitstream throught Passive Serial interface\n"
);
 
