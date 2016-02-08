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

#define MTD_FACTORY 	"Factory"

#if defined (CONFIG_RALINK_RT6855A) || defined (CONFIG_RALINK_MT7621) || defined (CONFIG_ARCH_MT7623)
#define LAN_OFFSET    0xE000
#define WAN_OFFSET    0xE006
#define W2G_OFFSET    0x0004
#define W5G_OFFSET    0x8004

#else
#define LAN_OFFSET    0x28 
#define WAN_OFFSET    0x22
#endif

#define MACADDR_LEN 	6
#define WIFIRF_LEN 	512

#define MEMGETINFO	_IOR('M', 1, struct mtd_info_user)
#define MEMERASE	_IOW('M', 2, struct erase_info_user)
#define MEMUNLOCK	_IOW('M', 6, struct erase_info_user)



#define NKEYS (sizeof(lookuptable)/sizeof(t_symstruct))

int keyfromstring(char *key)
{
    int i;
    typedef struct { char *key; int val; } t_symstruct;

    static t_symstruct lookuptable[] = {
		{ "wan", WAN_OFFSET },
#if defined (CONFIG_RALINK_RT6855A) || defined (CONFIG_RALINK_MT7621) || defined (CONFIG_ARCH_MT7623)
		{ "2g", W2G_OFFSET },
		{ "5g", W5G_OFFSET },
#endif
		{ "lan", LAN_OFFSET },
        };

#define NKEYS (sizeof(lookuptable)/sizeof(t_symstruct))
    
    printf ("Keys: %d, Reading: %s\n", NKEYS, key);
    for (i=0; i < NKEYS; i++) {
//	printf ("Iter %d, Key %s?", i, lookuptable[i].key);
        if (strcmp(lookuptable[i].key, key) == 0) {
//	    printf("Yes! Offset: %0X \n",lookuptable[i].val);
            return lookuptable[i].val;
//	    } else {
//	    printf ("No :(\n");
	    }
    }
    printf("Invalid parameter, using zero offset.\n");
    return 0;
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

		lseek(fd, keyfromstring(side), SEEK_SET);
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

		ptr = buf + keyfromstring(side);

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
		lseek(fd, start_addr+keyfromstring(side), SEEK_SET);
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
		lseek(fd, start_addr+keyfromstring(side), SEEK_SET);
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
#if defined (CONFIG_RALINK_RT6855A) || defined (CONFIG_RALINK_MT7621) || defined (CONFIG_ARCH_MT7623)
	printf("\tread:   %s r <lan|wan|2g|5g>\n", str[0]);
	printf("\twrite:  %s w <lan|wan|2g|5g> <MACADDR[0]> <MACADDR[1]> ...\n", str[0]);
#else
	printf("\tread:   %s r <lan|wan>\n", str[0]);
	printf("\twrite:  %s w <lan|wan> <MACADDR[0]> <MACADDR[1]> ...\n", str[0]);
#endif
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
		if (emmc_read(argv[2]) < 0)
#endif
			goto Fail;
		break;
	case 'w':
		if (argc < 4)
			goto CmdFail;
#if ! defined (CONFIG_MTK_EMMC_SUPPORT)
		if (mtd_write(argv[2], argv+3) < 0)
#else
		if (emmc_write(argv[2], argv+3) < 0)
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

