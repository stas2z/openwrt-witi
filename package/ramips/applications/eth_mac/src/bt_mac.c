#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <linux/autoconf.h>  //kernel config
#include <limits.h>

#define COMBO_CHIP
#define ETH_MAC


#define MTD_FACTORY 	"Factory"

#if defined(ETH_MAC)
#if defined (CONFIG_RALINK_RT6855A) || defined (CONFIG_RALINK_MT7621) || defined (CONFIG_ARCH_MT7623)
#define LAN_OFFSET    0xE000
#define WAN_OFFSET    0xE006
#else
#define LAN_OFFSET    0x28 
#define WAN_OFFSET    0x22
#endif
#endif

#if defined(COMBO_CHIP)
#if defined (CONFIG_ARCH_MT7623)
#define COMBO_BT_LEN		0x80
#define COMBO_BT_BDADDR_LEN	0x6
#define COMBO_BT_VOICE_LEN	0x2
#define COMBO_BT_CODEC_LEN	0x4
#define COMBO_BT_RADIO_LEN	0x6
#define COMBO_BT_SLEEP_LEN	0x7
#define COMBO_BT_OTHER_LEN	0x2
#define COMBO_BT_TX_PWR_LEN	0x3
#define COMBO_BT_COEX_LEN	0x6

#define COMBO_BT_OFFSET 0xE100
#define COMBO_BT_BDADDR_OFFSET	(0x0 + COMBO_BT_OFFSET)
#define COMBO_BT_VOICE_OFFSET	(0x6 + COMBO_BT_OFFSET)
#define COMBO_BT_CODEC_OFFSET	(0x8 + COMBO_BT_OFFSET)
#define COMBO_BT_RADIO_OFFSET	(0xC + COMBO_BT_OFFSET)
#define COMBO_BT_SLEEP_OFFSET	(0x12 + COMBO_BT_OFFSET)
#define COMBO_BT_OTHER_OFFSET	(0x19 + COMBO_BT_OFFSET)
#define COMBO_BT_TX_PWR_OFFSET	(0x1B + COMBO_BT_OFFSET)
#define COMBO_BT_COEX_OFFSET	(0x1E + COMBO_BT_OFFSET)

#define COMBO_WIFIRF_LEN 	512
#define COMBO_WIFI_OFFSET 0xE200
#endif /* CONFIG_ARCH_MT7623 */
#endif /* COMBO_CHIP */

#if defined(ETH_MAC)
#define MACADDR_LEN 	6
#define WIFIRF_LEN 	512
#endif

#define MEMGETINFO	_IOR('M', 1, struct mtd_info_user)
#define MEMERASE	_IOW('M', 2, struct erase_info_user)
#define MEMUNLOCK	_IOW('M', 6, struct erase_info_user)


static int get_offset(char *side)
{
	int offset = 0;
#if defined(ETH_MAC)	
	if (!strcmp(side, "wan"))
		offset = WAN_OFFSET;
	else if (!strcmp(side, "lan"))
		offset = LAN_OFFSET;
	else
#endif
#if defined(COMBO_CHIP)
#if defined(CONFIG_ARCH_MT7623)
	if (!strcmp(side, "bt"))
		offset = COMBO_BT_OFFSET;
	else if (!strcmp(side, "wifi"))
		offset = COMBO_WIFI_OFFSET;
	else if (!strcmp(side, "bdaddr"))
		offset = COMBO_BT_BDADDR_OFFSET;
	else if (!strcmp(side, "bdvoice"))
		offset = COMBO_BT_VOICE_OFFSET;
	else if (!strcmp(side, "bdcodec"))
		offset = COMBO_BT_CODEC_OFFSET;
	else if (!strcmp(side, "bdradio"))
		offset = COMBO_BT_RADIO_OFFSET;
	else if (!strcmp(side, "bdsleep"))
		offset = COMBO_BT_SLEEP_OFFSET;
	else if (!strcmp(side, "bdother"))
		offset = COMBO_BT_OTHER_OFFSET;
	else if (!strcmp(side, "bdtxpwr"))
		offset = COMBO_BT_TX_PWR_OFFSET;
	else if (!strcmp(side, "bdcoex"))
		offset = COMBO_BT_COEX_OFFSET;
	else 
#endif /* CONFIG_ARCH_MT7623 */
#endif /* COMBO_CHIP */
	{
		printf("Get offset \"%s\" failed.\n", side);
		offset = -1;
	}

	return offset;
}

static unsigned char get_str_byte_value(char *str)
{
	char buf[3] = {0};

	if((&str[1] == NULL) || (str[1] == '\n')){
		buf[0] = 0;
		buf[1] = str[0];
	}else{
		buf[0] = str[0];
		buf[1] = str[1];
	}

	return (unsigned char) strtol(buf, NULL, 16);
}

#if ! defined (CONFIG_MTK_EMMC_SUPPORT)
struct erase_info_user {
	unsigned int start;
	unsigned int length;
};

struct mtd_info_user {
	unsigned char type;
	unsigned int flags;
	unsigned int size;
	unsigned int erasesize;
	unsigned int oobblock;  
	unsigned int oobsize;  
	unsigned int ecctype;
	unsigned int eccsize;
};

int mtd_open(const char *mtd, int flags)
{
	FILE *fp;
	char dev[PATH_MAX];
	char part_name[256];
	int i;
	int ret;

	sprintf(part_name, "\"%s\"",mtd);
	if ((fp = fopen("/proc/mtd", "r"))) {
		while (fgets(dev, sizeof(dev), fp)) {
			if (sscanf(dev, "mtd%d:", &i) && strstr(dev, part_name)) {
				snprintf(dev, sizeof(dev), "/dev/mtd/%d", i);
				if ((ret=open(dev, flags))<0) {
					snprintf(dev, sizeof(dev), "/dev/mtd%d", i);
					ret=open(dev, flags);
				}
				fclose(fp);
				return ret;
			}
		}
		fclose(fp);
	}

	return open(mtd, flags);
}

int mtd_read(char *side)
{
	int fd = mtd_open(MTD_FACTORY, O_RDWR | O_SYNC);
	int i = 0;
	unsigned char mac_addr[MACADDR_LEN];

	if(fd < 0) {
		printf("Could not open mtd device: %s\n", MTD_FACTORY);
		return -1;
	}

	if (!strcmp(side, "wan"))
		lseek(fd, WAN_OFFSET, SEEK_SET);
	else
		lseek(fd, LAN_OFFSET, SEEK_SET);
	if(read(fd, mac_addr, MACADDR_LEN) != MACADDR_LEN){
		printf("read() failed\n");
		close(fd);
		return -1;
	}
	for (i = 0; i < MACADDR_LEN; i++)
	{
		printf("%02X", mac_addr[i]);
		if (i < MACADDR_LEN-1)
			printf(":");
		else
			printf("\n");
	}
	close(fd);

	return 0;
}

int mtd_write(char *side, char **value)
{
	int sz = 0;
	int i;
	struct mtd_info_user mtdInfo;
	struct erase_info_user mtdEraseInfo;
	int fd = mtd_open(MTD_FACTORY, O_RDWR | O_SYNC);
	unsigned char *buf, *ptr;

	if(fd < 0) {
		fprintf(stderr, "Could not open mtd device: %s\n", MTD_FACTORY);
		return -1;
	}
	if(ioctl(fd, MEMGETINFO, &mtdInfo)) {
		fprintf(stderr, "Could not get MTD device info from %s\n", MTD_FACTORY);
		close(fd);
		return -1;
	}
	mtdEraseInfo.length = sz = mtdInfo.erasesize;
	buf = (unsigned char *)malloc(sz);
	if(NULL == buf){
		printf("Allocate memory for sz failed.\n");
		close(fd);
		return -1;        
	}
	if(read(fd, buf, sz) != sz){
		fprintf(stderr, "read() %s failed\n", MTD_FACTORY);
		goto write_fail;
	}
	mtdEraseInfo.start = 0x0;
	for (mtdEraseInfo.start; mtdEraseInfo.start < mtdInfo.size; mtdEraseInfo.start += mtdInfo.erasesize) {
		ioctl(fd, MEMUNLOCK, &mtdEraseInfo);
		if(ioctl(fd, MEMERASE, &mtdEraseInfo)){
			fprintf(stderr, "Failed to erase block on %s at 0x%x\n", MTD_FACTORY, mtdEraseInfo.start);
			goto write_fail;
		}
	}		

	ptr = get_offset(side);
	if(ptr < 0){
		goto write_fail;
	}
	ptr += buf;
#if 0
	if (!strcmp(side, "wan"))
		ptr = buf + WAN_OFFSET;
	else if (!strcmp(side, "lan"))
		ptr = buf + LAN_OFFSET;
#endif

	for (i = 0; i < MACADDR_LEN; i++, ptr++)
		*ptr = strtoul(value[i], NULL, 16);
	lseek(fd, 0, SEEK_SET);
	if (write(fd, buf, sz) != sz) {
		fprintf(stderr, "write() %s failed\n", MTD_FACTORY);
		goto write_fail;
	}

	close(fd);
        free(buf);
	return 0;
write_fail:
	close(fd);
	free(buf);
	return -1;
}
#else
#define EMMC_PARTITION "/dev/mmcblk0" 
#define DUMCHAR_INFO "/proc/emmc_partition" 

static int get_start_address(long long *size, long long *start_addr)
{
	FILE *fp = fopen(DUMCHAR_INFO, "r");
	char p_name[20];
	char p_size[20];
	char p_start_addr[20];

	if (fp == NULL) {
		fprintf(stderr, "Can't open %s\n", DUMCHAR_INFO);
		return -1;
	}
	memset(p_name, 0, 20);
	while (EOF != fscanf(fp, "\n%s\t%s\t%s\t%*s\t%*s", 
			     p_name, p_size, p_start_addr)) {
		if (!strcmp(p_name, "factory"))
			break;
		memset(p_name, 0, 20);
		memset(p_size, 0, 20);
		memset(p_start_addr, 0, 20);
	}
	*size = strtoll(p_size, NULL, 16);
	*start_addr = strtoull(p_start_addr, NULL, 16);
	if (size == 0) {
		fprintf(stderr, "not found \"factory\" partition\n");
		fclose(fp);
		return -1;
	}
	fclose(fp);

	return 0;
}

int emmc_read_offset(char *side, const unsigned int offset, const unsigned int len)
{
	int fd, pos_emmc, pos_file, i, offset_t;
	char mac_addr[MACADDR_LEN];
	long long size, start_addr;
	char *ptr;

	fd = open(EMMC_PARTITION, O_RDWR | O_SYNC);
	if(fd < 0) {
		fprintf(stderr, "Could not open emmc device: %s\n", EMMC_PARTITION);
		exit(1);
		return -1;
	}
	if (get_start_address(&size, &start_addr) < 0) {
		close(fd);
		exit(1);
	}

	offset_t = get_offset(side);
	if(offset_t < 0){
		fprintf(stderr, "wrong partition name: %s\n", side);
		close(fd);
		exit(1);
	}

	offset_t += offset;
	if ((offset_t + len) >= size) {
		fprintf(stderr, "exceed than the maximum size: %llx\n", size);
		close(fd);
		exit(1);
	}
/*
	if (!strcmp(side, "wan"))
		lseek(fd, start_addr+WAN_OFFSET, SEEK_SET);
	else (!strcmp(side, "lan"))
		lseek(fd, start_addr+LAN_OFFSET, SEEK_SET);
*/
	lseek(fd, start_addr + offset_t, SEEK_SET);

	if(read(fd, mac_addr, len) != len){
		printf("read() failed\n");
		close(fd);
		return -1;
	}
	printf("read %s[%08X]=", side, offset_t);
	for (i = 0; i < len; i++)
	{
		printf("%02X", mac_addr[i]);
		if (i < len-1)
			printf(":");
		else
			printf("\n");
	}
	close(fd);

	return 0;
}

int emmc_read(char *side)
{
	int fd, pos_emmc, pos_file, i;
	char mac_addr[MACADDR_LEN];
	long long size, start_addr;

	fd = open(EMMC_PARTITION, O_RDWR | O_SYNC);
	if(fd < 0) {
		fprintf(stderr, "Could not open emmc device: %s\n", EMMC_PARTITION);
		exit(1);
		return -1;
	}
	if (get_start_address(&size, &start_addr) < 0) {
		close(fd);
		exit(1);
	}
	if (!strcmp(side, "wan"))
		lseek(fd, start_addr+WAN_OFFSET, SEEK_SET);
	else /*if (!strcmp(side, "lan"))*/
		lseek(fd, start_addr+LAN_OFFSET, SEEK_SET);

	if(read(fd, mac_addr, MACADDR_LEN) != MACADDR_LEN){
		printf("read() failed\n");
		close(fd);
		return -1;
	}
	for (i = 0; i < MACADDR_LEN; i++)
	{
		printf("%02X", mac_addr[i]);
		if (i < MACADDR_LEN-1)
			printf(":");
		else
			printf("\n");
	}
	close(fd);

	return 0;
}

int emmc_write_offset(char *side, const unsigned int offset, char *value)
{
	int fd, i, offset_t, len;
	//char buf[MACADDR_LEN];
	unsigned char *buf;
	long long size, start_addr;

	fd = open(EMMC_PARTITION, O_RDWR | O_SYNC);
	if(fd < 0) {
		fprintf(stderr, "Could not open emmc device: %s\n", EMMC_PARTITION);
		exit(1);
		return -1;
	}
	if (get_start_address(&size, &start_addr) < 0) {
		close(fd);
		exit(1);
	}
	if (WAN_OFFSET+MACADDR_LEN >= size) {
		fprintf(stderr, "exceed than the maximum size: %llx\n", size);
		close(fd);
		exit(1);
	}

	offset_t = get_offset(side);
	if(offset_t < 0){
		fprintf(stderr, "wrong partition name: %s\n", side);
		close(fd);
		exit(1);
	}

	offset_t += offset;
	len = strlen(value);
	len = (len / 2) + (len % 2);
	if ((offset_t + len) >= size) {
		fprintf(stderr, "exceed than the maximum size: %llx\n", size);
		close(fd);
		exit(1);
	}
/*
	if (!strcmp(side, "wan"))
		lseek(fd, start_addr+WAN_OFFSET, SEEK_SET);
	else
		lseek(fd, start_addr+LAN_OFFSET, SEEK_SET);
*/
	lseek(fd, start_addr + offset_t, SEEK_SET);

	buf = malloc(len * sizeof(char));
	for (i = 0; i < len; i++){
		buf[i] = get_str_byte_value(&value[i*2]);
		
	}
	if (write(fd, buf, MACADDR_LEN) != MACADDR_LEN) {
		fprintf(stderr, "write() %s failed\n", EMMC_PARTITION);
		close(fd);
		return -1;
	}
	printf("write %s[%08X]=%s(%02X%02X)\n", side, offset_t, value, buf[0], buf[1]);
	close(fd);
	return 0;
}

int emmc_write(char *side, char **value)
{
	int fd, i;
	char buf[MACADDR_LEN];
	long long size, start_addr;

	fd = open(EMMC_PARTITION, O_RDWR | O_SYNC);
	if(fd < 0) {
		fprintf(stderr, "Could not open emmc device: %s\n", EMMC_PARTITION);
		exit(1);
		return -1;
	}
	if (get_start_address(&size, &start_addr) < 0) {
		close(fd);
		exit(1);
	}
	if (WAN_OFFSET+MACADDR_LEN >= size) {
		fprintf(stderr, "exceed than the maximum size: %llx\n", size);
		close(fd);
		exit(1);
	}
	if (!strcmp(side, "wan"))
		lseek(fd, start_addr+WAN_OFFSET, SEEK_SET);
	else
		lseek(fd, start_addr+LAN_OFFSET, SEEK_SET);
	for (i = 0; i < MACADDR_LEN; i++)
		buf[i] = strtol(value[i], NULL, 16);
	if (write(fd, buf, MACADDR_LEN) != MACADDR_LEN) {
		fprintf(stderr, "write() %s failed\n", EMMC_PARTITION);
		close(fd);
		return -1;
	}
	close(fd);
	return 0;
}
#endif

void usage(char **str)
{
	printf("How to use:\n");
	printf("\tread:   %s r <lan|wan|bt> <offset> <len>\n", str[0]);
	printf("\twrite:  %s w <lan|wan|bt> <offset> <value> ...\n", str[0]);
	printf("\tex: %s r bt 0 6 => Read BD_ADDR from bt factory side, offset address 0\n", str[0]);
	printf("\tex: %s w bt 0 AABBCCDDEEFF => Write BD_ADDR from bt factory side, offset address 0\n", str[0]);
}

int main(int argc,char **argv)
{
	char op;

	if (argc < 3)
		goto CmdFail;

	op = *(argv[1]);
	switch (op) {
	case 'r':
#if ! defined (CONFIG_MTK_EMMC_SUPPORT)
		if (mtd_read(argv[2]) < 0)
#else
		if (emmc_read_offset(argv[2], strtol(argv[3], NULL, 10), strtol(argv[4], NULL, 10)) < 0)
#endif
			goto Fail;
		break;
	case 'w':
		if (argc < 4)
			goto CmdFail;
#if ! defined (CONFIG_MTK_EMMC_SUPPORT)
		if (mtd_write(argv[2], argv+3) < 0)
#else
		if (emmc_write_offset(argv[2], strtol(argv[3], NULL, 10), argv[4]) < 0)
#endif
			goto Fail;
		break;
	default:
		goto CmdFail;
	}

	return 0;
CmdFail:
	usage(argv);
Fail:
	return -1;
}

