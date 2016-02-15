/*
 * iwinfo - Wireless Information Library - MT76X2E Backend
 *
 *   Copyright (C) 2009 Jo-Philipp Wich <xm@subsignal.org>
 *
 * The iwinfo library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * The iwinfo library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with the iwinfo library. If not, see http://www.gnu.org/licenses/.
 *
 * Parts of this code are derived from the Linux wireless tools, iwlib.c,
 * iwlist.c and iwconfig.c in particular.
 */

#include "iwinfo.h"
#include "iwinfo_mt76x2e.h"

/* Some usefull constants */
#define KILO	1e3
#define MEGA	1e6
#define GIGA	1e9

char data[4096];
int iw_ignore_version_sp = 0;

typedef struct _CH_FREQ_MAP_{
    unsigned short channel;
    unsigned short freqKHz;
} CH_FREQ_MAP;

#define FREQ_REF 40
#define FREQ_REF_CYCLES 89
#define DOUBLER 1
CH_FREQ_MAP CH_HZ_ID_MAP[]=
	{
	    {1, 2412},
	    {2, 2417},
	    {3, 2422},
	    {4, 2427},
	    {5, 2432},
	    {6, 2437},
	    {7, 2442},
	    {8, 2447},
	    {9, 2452},
	    {10, 2457},
	    {11, 2462},
	    {12, 2467},
	    {13, 2472},
	    {14, 2484},

	    /*  UNII */
	    {36, 5180},
	    {40, 5200},
	    {44, 5220},
	    {48, 5240},
	    {52, 5260},
	    {56, 5280},
	    {60, 5300},
	    {64, 5320},
	    {149, 5745},
	    {153, 5765},
	    {157, 5785},
	    {161, 5805},
	    {165, 5825},
	    {167, 5835},
	    {169, 5845},
	    {171, 5855},
	    {173, 5865},
			
	    /* HiperLAN2 */
	    {100, 5500},
	    {104, 5520},
	    {108, 5540},
	    {112, 5560},
	    {116, 5580},
	    {120, 5600},
	    {124, 5620},
	    {128, 5640},
	    {132, 5660},
	    {136, 5680},
	    {140, 5700},
			
	    /* Japan MMAC */
	    {34, 5170},
	    {38, 5190},
	    {42, 5210},
	    {46, 5230},
		    
	    /*  Japan */
	    {183, 4915},
	    {184, 4920},
	    {185, 4925},
	    {187, 4935},
	    {188, 4940},
	    {189, 4945},
	    {192, 4960},
	    {196, 4980},
	    {200, 5000},
	    {204, 5020},
	    {208, 5040},	/* Japan, means J08 */
	    {212, 5060},	/* Japan, means J12 */   
	    {216, 5080},	/* Japan, means J16 */
};

#define MYLOG(mes) \
	{   char buf[256];  \
	sprintf(buf, "logger -t iwinfo %s:%d: %s", __FUNCTION__, __LINE__, mes);	 \
	system(buf);	\
}



/*------------------------------------------------------------------*/
/*
 * Get the range information out of the driver
 */
int
ralink_get_range_info(iwrange *	range, char* buffer, int length)
{
  union iw_range_raw *	range_raw;

  /* Point to the buffer */
  range_raw = (union iw_range_raw *) buffer;

  /* For new versions, we can check the version directly, for old versions
   * we use magic. 300 bytes is a also magic number, don't touch... */
  if (length < 300)
    {
      /* That's v10 or earlier. Ouch ! Let's make a guess...*/
      range_raw->range.we_version_compiled = 9;
    }

//  fprintf(stderr, "Version: %d", range_raw->range.we_version_compiled);

  /* Check how it needs to be processed */
  if (range_raw->range.we_version_compiled > 15)
    {
      /* This is our native format, that's easy... */
      /* Copy stuff at the right place, ignore extra */
      memcpy((char *) range, buffer, sizeof(iwrange));
    }
  else
    {
      /* Zero unknown fields */
      bzero((char *) range, sizeof(struct iw_range));

      /* Initial part unmoved */
      memcpy((char *) range,
	     buffer,
	     iwr15_off(num_channels));
      /* Frequencies pushed futher down towards the end */
      memcpy((char *) range + iwr_off(num_channels),
	     buffer + iwr15_off(num_channels),
	     iwr15_off(sensitivity) - iwr15_off(num_channels));
      /* This one moved up */
      memcpy((char *) range + iwr_off(sensitivity),
	     buffer + iwr15_off(sensitivity),
	     iwr15_off(num_bitrates) - iwr15_off(sensitivity));
      /* This one goes after avg_qual */
      memcpy((char *) range + iwr_off(num_bitrates),
	     buffer + iwr15_off(num_bitrates),
	     iwr15_off(min_rts) - iwr15_off(num_bitrates));
      /* Number of bitrates has changed, put it after */
      memcpy((char *) range + iwr_off(min_rts),
	     buffer + iwr15_off(min_rts),
	     iwr15_off(txpower_capa) - iwr15_off(min_rts));
      /* Added encoding_login_index, put it after */
      memcpy((char *) range + iwr_off(txpower_capa),
	     buffer + iwr15_off(txpower_capa),
	     iwr15_off(txpower) - iwr15_off(txpower_capa));
      /* Hum... That's an unexpected glitch. Bummer. */
      memcpy((char *) range + iwr_off(txpower),
	     buffer + iwr15_off(txpower),
	     iwr15_off(avg_qual) - iwr15_off(txpower));
      /* Avg qual moved up next to max_qual */
      memcpy((char *) range + iwr_off(avg_qual),
	     buffer + iwr15_off(avg_qual),
	     sizeof(struct iw_quality));
    }

  /* We are now checking much less than we used to do, because we can
   * accomodate more WE version. But, there are still cases where things
   * will break... */
  if (!iw_ignore_version_sp)
    {
      /* We don't like very old version (unfortunately kernel 2.2.X) */
      if (range->we_version_compiled <= 10)
	{
	  fprintf(stderr, "Warning: Driver for device %s has been compiled with an ancient version\n", "raxx");
	  fprintf(stderr, "of Wireless Extension, while this program support version 11 and later.\n");
	  fprintf(stderr, "Some things may be broken...\n\n");
	}

      /* We don't like future versions of WE, because we can't cope with
       * the unknown */
      if (range->we_version_compiled > 22)
	{
	  fprintf(stderr, "Warning: Driver for device %s has been compiled with version %d\n", "raxx", range->we_version_compiled);
	  fprintf(stderr, "of Wireless Extension, while this program supports up to version %d.\n", 22);
	  fprintf(stderr, "Some things may be broken...\n\n");
	}

      /* Driver version verification */
      if ((range->we_version_compiled > 10) &&
	 (range->we_version_compiled < range->we_version_source))
	{
	  fprintf(stderr, "Warning: Driver for device %s recommend version %d of Wireless Extension,\n", "raxx", range->we_version_source);
	  fprintf(stderr, "but has been compiled with version %d, therefore some driver features\n", range->we_version_compiled);
	  fprintf(stderr, "may not be available...\n\n");
	}
      /* Note : we are only trying to catch compile difference, not source.
       * If the driver source has not been updated to the latest, it doesn't
       * matter because the new fields are set to zero */
    }

  /* Don't complain twice.
   * In theory, the test apply to each individual driver, but usually
   * all drivers are compiled from the same kernel. */
  iw_ignore_version_sp = 1;

  return (0);
}

int GetBW(int BW)
{
	switch(BW)
	{
	case BW_10:
		return 1;
	case BW_20:
		return 2;
	case BW_40:
		return 4;
	case BW_80:
		return 8;
	default:
		return 0;
	}
}

char* GetPhyMode(int Mode)
{
	switch(Mode)
	{
	case MODE_CCK:
		return "CCK";
	case MODE_OFDM:
		return "OFDM";
	case MODE_HTMIX:
		return "HTMIX";
	case MODE_HTGREENFIELD:
		return "HT_GF";
	case MODE_VHT:
		return "VHT";
	default:
		return "N/A";
	}
}

static int
getMCS(MACHTTRANSMIT_SETTING HTSetting)
{
	int mcs_1ss = (int)HTSetting.field.MCS;

	if (HTSetting.field.MODE >= MODE_VHT) {
		if (mcs_1ss > 9)
			mcs_1ss %= 16;
	}

	return mcs_1ss;
}

static const int
MCSMappingRateTable[] =
{
	 2,  4,   11,  22,								// CCK

	12,  18,  24,  36,  48,  72,  96, 108,						// OFDM

	13,  26,  39,  52,  78, 104, 117, 130, 26,  52,  78, 104, 156, 208, 234, 260,	// 11n: 20MHz, 800ns GI, MCS: 0 ~ 15
	39,  78, 117, 156, 234, 312, 351, 390,						// 11n: 20MHz, 800ns GI, MCS: 16 ~ 23
	27,  54,  81, 108, 162, 216, 243, 270, 54, 108, 162, 216, 324, 432, 486, 540,	// 11n: 40MHz, 800ns GI, MCS: 0 ~ 15
	81, 162, 243, 324, 486, 648, 729, 810,						// 11n: 40MHz, 800ns GI, MCS: 16 ~ 23
	14,  29,  43,  57,  87, 115, 130, 144, 29, 59,   87, 115, 173, 230, 260, 288,	// 11n: 20MHz, 400ns GI, MCS: 0 ~ 15
	43,  87, 130, 173, 260, 317, 390, 433,						// 11n: 20MHz, 400ns GI, MCS: 16 ~ 23
	30,  60,  90, 120, 180, 240, 270, 300, 60, 120, 180, 240, 360, 480, 540, 600,	// 11n: 40MHz, 400ns GI, MCS: 0 ~ 15
	90, 180, 270, 360, 540, 720, 810, 900,

	13,  26,  39,  52,  78, 104, 117, 130, 156,					// 11ac: 20Mhz, 800ns GI, MCS: 0~8
	27,  54,  81, 108, 162, 216, 243, 270, 324, 360,				// 11ac: 40Mhz, 800ns GI, MCS: 0~9
	59, 117, 176, 234, 351, 468, 527, 585, 702, 780,				// 11ac: 80Mhz, 800ns GI, MCS: 0~9
	14,  29,  43,  57,  87, 115, 130, 144, 173,					// 11ac: 20Mhz, 400ns GI, MCS: 0~8
	30,  60,  90, 120, 180, 240, 270, 300, 360, 400,				// 11ac: 40Mhz, 400ns GI, MCS: 0~9
	65, 130, 195, 260, 390, 520, 585, 650, 780, 867					// 11ac: 80Mhz, 400ns GI, MCS: 0~9
};

static int
getRate(MACHTTRANSMIT_SETTING HTSetting)
{
	int rate_count = sizeof(MCSMappingRateTable)/sizeof(int);
	int rate_index = 0;
	int num_ss_vht = 1;

	if (HTSetting.field.MODE >= MODE_VHT) {
		int mcs_1ss = (int)HTSetting.field.MCS;
		
		if (mcs_1ss > 9) {
			num_ss_vht = (mcs_1ss / 16) + 1;
			mcs_1ss %= 16;
		}
		if (HTSetting.field.BW == BW_20)
			rate_index = 108 + ((unsigned char)HTSetting.field.ShortGI * 29) + mcs_1ss;
		else if (HTSetting.field.BW == BW_40)
			rate_index = 117 + ((unsigned char)HTSetting.field.ShortGI * 29) + mcs_1ss;
		else if (HTSetting.field.BW == BW_80)
			rate_index = 127 + ((unsigned char)HTSetting.field.ShortGI * 29) + mcs_1ss;
	}
	else if (HTSetting.field.MODE >= MODE_HTMIX)
		rate_index = 12 + ((unsigned char)HTSetting.field.BW * 24) + ((unsigned char)HTSetting.field.ShortGI * 48) + ((unsigned char)HTSetting.field.MCS);
	else if (HTSetting.field.MODE == MODE_OFDM)
		rate_index = (unsigned char)(HTSetting.field.MCS) + 4;
	else if (HTSetting.field.MODE == MODE_CCK)
		rate_index = (unsigned char)(HTSetting.field.MCS);

	if (rate_index < 0)
		rate_index = 0;

	if (rate_index >= rate_count)
		rate_index = rate_count-1;

	return (MCSMappingRateTable[rate_index] * num_ss_vht * 5)/10;
}

static double mt76x2e_freq2float(const struct iw_freq *in)
{
	int		i;
	double	res = (double) in->m;
	for(i = 0; i < in->e; i++) res *= 10;
	return res;
}

int
mt76x2e_freq_to_channel(double			freq,
		   const struct iw_range *	range)
{
  double	ref_freq;
  int		k;

  /* Check if it's a frequency or not already a channel */
  if(freq < KILO)
    return(-1);

  /* We compare the frequencies as double to ignore differences
   * in encoding. Slower, but safer... */
  for(k = 0; k < range->num_frequency; k++)
    {
      ref_freq = mt76x2e_freq2float(&(range->freq[k]));
      if(freq == ref_freq)
	return(range->freq[k].i);
    }
  /* Not found */
  return(-2);
}

int
mt76x2e_channel_to_freq(int				channel,
		   double *			pfreq,
		   const struct iw_range *	range)
{
  int		has_freq = 0;
  int		k;

  /* Check if the driver support only channels or if it has frequencies */
  for(k = 0; k < range->num_frequency; k++)
    {
      if((range->freq[k].e != 0) || (range->freq[k].m > (int) KILO))
	has_freq = 1;
    }
  if(!has_freq)
    return(-1);

  /* Find the correct frequency in the list */
  for(k = 0; k < range->num_frequency; k++)
    {
      if(range->freq[k].i == channel)
	{
	  *pfreq = mt76x2e_freq2float(&(range->freq[k]));
	  return(channel);
	}
    }
  /* Not found */
  return(-2);
}

static inline int mt76x2e_freq2mhz(const struct iw_freq *in)
{
	int i;

	if( in->e == 6 )
	{
		return in->m;
	}
	else
	{
		return (int)(mt76x2e_freq2float(in) / 1000000);
	}
}

int CH_HZ_ID_MAP_NUM = (sizeof(CH_HZ_ID_MAP)/sizeof(CH_FREQ_MAP));

void mt76x2e_ch2freq(
    unsigned char Ch,
    double *pFreq)
{
    int chIdx;
    for (chIdx = 0; chIdx < CH_HZ_ID_MAP_NUM; chIdx++)
    {
	if ((Ch) == CH_HZ_ID_MAP[chIdx].channel)
	{
	    (*pFreq) = CH_HZ_ID_MAP[chIdx].freqKHz * 1000;
	    break;
	}
    }
    if (chIdx == CH_HZ_ID_MAP_NUM)
	(*pFreq) = 2412000;
}

static inline int mt76x2e_ioctl(const char *ifname, int cmd, struct iwreq *wrq)
{
	if( !strncmp(ifname, "mon.", 4) )
		strncpy(wrq->ifr_name, &ifname[4], IFNAMSIZ);
	else
		strncpy(wrq->ifr_name, ifname, IFNAMSIZ);

	return iwinfo_ioctl(cmd, wrq);
}


static int mt76x2e_probe(const char *ifname)
{
	struct iwreq wrq;

	if(mt76x2e_ioctl(ifname, SIOCGIWNAME, &wrq) >= 0)
		return 1;

	return 0;
}

static void mt76x2e_close(void)
{
	/* Nop */
}

static int mt76x2e_get_mode(const char *ifname, int *buf)
{
	*buf = strncmp(ifname,"ra",2)?IWINFO_OPMODE_CLIENT:IWINFO_OPMODE_MASTER;
	return 0;
}

static int mt76x2e_get_ssid(const char *ifname, char *buf)
{
	struct iwreq wrq;

	wrq.u.essid.pointer = (caddr_t) buf;
	wrq.u.essid.length  = IW_ESSID_MAX_SIZE + 1;
	wrq.u.essid.flags   = 0;

	if(mt76x2e_ioctl(ifname, SIOCGIWESSID, &wrq) >= 0)
		return 0;

	return -1;
}

static int mt76x2e_get_bssid(const char *ifname, char *buf)
{
	struct iwreq wrq;

	char cmd[256];
	FILE *fp = NULL;

	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "ifconfig %s | grep UP", ifname);
	fp = popen(cmd, "r");
	fscanf(fp, "%s\n", buf);
	pclose(fp);

	if(strlen(buf)>=2 && mt76x2e_ioctl(ifname, SIOCGIWAP, &wrq) >= 0)
	{
		sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X",
			(uint8_t)wrq.u.ap_addr.sa_data[0], (uint8_t)wrq.u.ap_addr.sa_data[1],
			(uint8_t)wrq.u.ap_addr.sa_data[2], (uint8_t)wrq.u.ap_addr.sa_data[3],
			(uint8_t)wrq.u.ap_addr.sa_data[4], (uint8_t)wrq.u.ap_addr.sa_data[5]);

		return 0;
	}
	else
		sprintf(buf, "00:00:00:00:00:00");

	return -1;
}

static int mt76x2e_get_bitrate(const char *ifname, int *buf)
{
	struct iwreq wrq;

	if(mt76x2e_ioctl(ifname, SIOCGIWRATE, &wrq) >= 0)
	{
		*buf = (wrq.u.bitrate.value / 1000);
		return 0;
	}

	return -1;
}

static int mt76x2e_get_channel(const char *ifname, int *buf)
{
	struct iwreq wrq;
	struct iw_range range;
	double freq;
	int i;

	if(mt76x2e_ioctl(ifname, SIOCGIWFREQ, &wrq) >= 0)
	{
		freq = mt76x2e_freq2float(&wrq.u.freq);
		*buf = freq;
		return 0;
	}

	return -1;
}

static int mt76x2e_get_frequency(const char *ifname, int *buf)
{
	double freq;
	int i, channel;

	i = mt76x2e_get_channel(ifname, &channel);
	mt76x2e_ch2freq(channel, &freq);
	*buf = (int)freq/1000;

	return 0;
}

static int mt76x2e_get_txpower(const char *ifname, int *buf)
{
	struct iwreq wrq;

	wrq.u.txpower.flags = 0;

	if(mt76x2e_ioctl(ifname, SIOCGIWTXPOW, &wrq) >= 0)
	{
		if(wrq.u.txpower.flags & IW_TXPOW_MWATT)
			*buf = iwinfo_mw2dbm(wrq.u.txpower.value);
		else
			*buf = wrq.u.txpower.value;

		return 0;
	}

	return -1;
}

static int mt76x2e_get_signal(const char *ifname, int *buf)
{
	struct iwreq wrq;
	struct iw_statistics stats;

	wrq.u.data.pointer = (caddr_t) &stats;
	wrq.u.data.length  = sizeof(struct iw_statistics);
	wrq.u.data.flags   = 1;

	if(mt76x2e_ioctl(ifname, SIOCGIWSTATS, &wrq) >= 0)
	{
		*buf = (stats.qual.updated & IW_QUAL_DBM)
			? (stats.qual.level - 0x100) : stats.qual.level;

		return 0;
	}

	return -1;
}

static int mt76x2e_get_noise(const char *ifname, int *buf)
{
	struct iwreq wrq;
	struct iw_statistics stats;

	wrq.u.data.pointer = (caddr_t) &stats;
	wrq.u.data.length  = sizeof(struct iw_statistics);
	wrq.u.data.flags   = 1;

	if(mt76x2e_ioctl(ifname, SIOCGIWSTATS, &wrq) >= 0)
	{
		*buf = (stats.qual.updated & IW_QUAL_DBM)
			? (stats.qual.noise - 0x100) : stats.qual.noise;

		return 0;
	}

	return -1;
}

static int mt76x2e_get_quality(const char *ifname, int *buf)
{
	struct iwreq wrq;
	struct iw_statistics stats;

	wrq.u.data.pointer = (caddr_t) &stats;
	wrq.u.data.length  = sizeof(struct iw_statistics);
	wrq.u.data.flags   = 1;

	if(mt76x2e_ioctl(ifname, SIOCGIWSTATS, &wrq) >= 0)
	{
		*buf = stats.qual.qual;
		return 0;
	}

	return -1;
}

static int mt76x2e_get_quality_max(const char *ifname, int *buf)
{
	struct iwreq wrq;
	struct iw_range range;

	wrq.u.data.pointer = (caddr_t) &range;
	wrq.u.data.length  = sizeof(struct iw_range);
	wrq.u.data.flags   = 0;

	if(mt76x2e_ioctl(ifname, SIOCGIWRANGE, &wrq) >= 0)
	{
		*buf = range.max_qual.qual;
		return 0;
	}

	return -1;
}

#define RTPRIV_IOCTL_GET_MAC_TABLE_STRUCT	(SIOCIWFIRSTPRIV + 0x1F)

static int mt76x2e_get_assoclist(const char *ifname, char *buf, int *len)
{
	struct	iwreq wrq;
	int ret, i, rssi;
	RT_802_11_MAC_TABLE *mp;
	char mac_table_data[4096];
	struct iwinfo_assoclist_entry entry;
	MACHTTRANSMIT_SETTING rx;

	bzero(mac_table_data, sizeof(mac_table_data));
	wrq.u.data.pointer = mac_table_data;
	wrq.u.data.length = sizeof(mac_table_data);
	wrq.u.data.flags = 0;

	if (mt76x2e_ioctl(ifname, RTPRIV_IOCTL_GET_MAC_TABLE_STRUCT, &wrq) >= 0) {

		mp = (RT_802_11_MAC_TABLE *) wrq.u.data.pointer;
		for(i=0;i < mp->Num; i++) {
			rssi = -127;
			rssi = (int)mp->Entry[i].AvgRssi0;
			if ((int)mp->Entry[i].AvgRssi1 > rssi && mp->Entry[i].AvgRssi1 != 0)
				rssi = (int)mp->Entry[i].AvgRssi1;
			if ((int)mp->Entry[i].AvgRssi2 > rssi && mp->Entry[i].AvgRssi2 != 0)
				rssi = (int)mp->Entry[i].AvgRssi2;
			rx.word = mp->Entry[i].LastRxRate;
			memset(&entry, 0, sizeof(entry));
			memcpy(entry.mac, mp->Entry[i].Addr, MAC_ADDR_LENGTH);
			entry.signal = rssi;
			entry.tx_rate.rate = getRate(mp->Entry[i].TxRate)*1000;
			entry.tx_rate.mcs = getMCS(mp->Entry[i].TxRate);
			entry.tx_rate.is_40mhz = GetBW(mp->Entry[i].TxRate.field.BW) > 2 ? 1 : 0;
			entry.tx_rate.is_short_gi = mp->Entry[i].TxRate.field.ShortGI ? 1 : 0;
			entry.rx_rate.rate = getRate(rx)*1000;
			entry.rx_rate.mcs = getMCS(rx);
			entry.rx_rate.is_40mhz = GetBW(rx.field.BW) > 2 ? 1 : 0;;
			entry.rx_rate.is_short_gi = rx.field.ShortGI ? 1 : 0;
			
			memcpy(&buf[i*sizeof(entry)], &entry, sizeof(entry));
		}
		*len = (i)*sizeof(entry);
	}
	return 0;
}

static int mt76x2e_get_txpwrlist(const char *ifname, char *buf, int *len)
{
	struct iwreq wrq;
	struct iw_range range;
	struct iwinfo_txpwrlist_entry entry;
	int i;

	wrq.u.data.pointer = (caddr_t) &range;
	wrq.u.data.length  = sizeof(struct iw_range);
	wrq.u.data.flags   = 0;

	if( (mt76x2e_ioctl(ifname, SIOCGIWRANGE, &wrq) >= 0) &&
	    (range.num_txpower > 0) && (range.num_txpower <= IW_MAX_TXPOWER) &&
	    !(range.txpower_capa & IW_TXPOW_RELATIVE)
	) {
		for( i = 0; i < range.num_txpower; i++ )
		{
			if( range.txpower_capa & IW_TXPOW_MWATT )
			{
				entry.dbm = iwinfo_mw2dbm(range.txpower[i]);
				entry.mw  = range.txpower[i];
			}

			/* Madwifi does neither set mW not dBm caps, also iwlist assumes
			 * dBm if mW is not set, so don't check here... */
			else /* if( range.txpower_capa & IW_TXPOW_DBM ) */
			{
				entry.dbm = range.txpower[i];
				entry.mw  = iwinfo_dbm2mw(range.txpower[i]);
			}

			memcpy(&buf[i*sizeof(entry)], &entry, sizeof(entry));
		}

		*len = i * sizeof(entry);
		return 0;
	}

	return -1;
}

static int mt76x2e_get_freqlist(const char *ifname, char *buf, int *len)
{
	struct iwreq wrq;
	struct iw_range range;
	struct iwinfo_freqlist_entry entry;
	char buffer[sizeof(iwrange) * 2];
	int i, bl;

	wrq.u.data.pointer = (caddr_t) buffer;
	wrq.u.data.length = sizeof(buffer);
	wrq.u.data.flags = 0;

	if(mt76x2e_ioctl(ifname, SIOCGIWRANGE, &wrq) >= 0)
	{
		if (ralink_get_range_info(&range, buffer, wrq.u.data.length) < 0)
			return -1;

		bl = 0;
		char mes[128];
		sprintf(mes,"num_freq: %d", range.num_frequency);
		for(i = 0; i < range.num_frequency; i++)
		{
			entry.mhz        = mt76x2e_freq2mhz(&range.freq[i]);
			entry.channel    = range.freq[i].i;
			entry.restricted = 0;

			memcpy(&buf[bl], &entry, sizeof(struct iwinfo_freqlist_entry));
			bl += sizeof(struct iwinfo_freqlist_entry);
		}

		*len = bl;
		return 0;
	}
	return -1;
}

static int mt76x2e_get_country(const char *ifname, char *buf)
{
	sprintf(buf, "00");
	return 0;
}

static int mt76x2e_get_countrylist(const char *ifname, char *buf, int *len)
{
	/* Stub */
	return -1;
}

static int mt76x2e_get_hwmodelist(const char *ifname, int *buf)
{
	char chans[IWINFO_BUFSIZE] = { 0 };
	struct iwinfo_freqlist_entry *e = NULL;
	int len = 0;

	*buf = 0;

	if( !mt76x2e_get_freqlist(ifname, chans, &len) )
	{
		for( e = (struct iwinfo_freqlist_entry *)chans; e->channel; e++ )
		{
			if( e->channel <= 14 )
			{
				*buf |= IWINFO_80211_B;
				*buf |= IWINFO_80211_G;
			}
			else
			{
				*buf |= IWINFO_80211_A;
			}
		}

		return 0;
	}

	return -1;
}

static int mt76x2e_get_encryption(const char *ifname, char *buf)
{
	/* No reliable crypto info in wext */
	return -1;
}

static int mt76x2e_get_phyname(const char *ifname, char *buf)
{
	/* No suitable api in wext */
	strcpy(buf, ifname);
	return 0;
}

static int mt76x2e_get_mbssid_support(const char *ifname, int *buf)
{
	/* No multi bssid support atm */
	*buf = 1;
	return -1;
}

static char * mt76x2e_sysfs_ifname_file(const char *ifname, const char *path)
{
	FILE *f;
	static char buf[128];
	char *rv = NULL;

	snprintf(buf, sizeof(buf), "/sys/class/net/%s/%s", ifname, path);

	if ((f = fopen(buf, "r")) != NULL)
	{
		memset(buf, 0, sizeof(buf));

		if (fread(buf, 1, sizeof(buf), f))
			rv = buf;

		fclose(f);
	}

	return rv;
}

static int mt76x2e_get_hardware_id(const char *ifname, char *buf)
{
	char *data;
	struct iwinfo_hardware_id *id = (struct iwinfo_hardware_id *)buf;

	memset(id, 0, sizeof(struct iwinfo_hardware_id));

	data = mt76x2e_sysfs_ifname_file(ifname, "device/vendor");
	if (data)
		id->vendor_id = strtoul(data, NULL, 16);

	data = mt76x2e_sysfs_ifname_file(ifname, "device/device");
	if (data)
		id->device_id = strtoul(data, NULL, 16);

	data = mt76x2e_sysfs_ifname_file(ifname, "device/subsystem_device");
	if (data)
		id->subsystem_device_id = strtoul(data, NULL, 16);

	data = mt76x2e_sysfs_ifname_file(ifname, "device/subsystem_vendor");
	if (data)
		id->subsystem_vendor_id = strtoul(data, NULL, 16);

	return (id->vendor_id > 0 && id->device_id > 0) ? 0 : -1;
}

static int mt76x2e_get_hardware_name(const char *ifname, char *buf)
{
	sprintf(buf, "Mediatek MT76X2E");
	return 0;
}

static int mt76x2e_get_txpower_offset(const char *ifname, int *buf)
{
	/* Stub */
	*buf = 0;
	return -1;
}

static int mt76x2e_get_frequency_offset(const char *ifname, int *buf)
{
	/* Stub */
	*buf = 0;
	return -1;
}

const struct iwinfo_ops mt76x2e_ops = {
	.name             = "mt76x2e",
	.probe            = mt76x2e_probe,
	.channel          = mt76x2e_get_channel,
	.frequency        = mt76x2e_get_frequency,
	.frequency_offset = mt76x2e_get_frequency_offset,
	.txpower          = mt76x2e_get_txpower,
	.txpower_offset   = mt76x2e_get_txpower_offset,
	.bitrate          = mt76x2e_get_bitrate,
	.signal           = mt76x2e_get_signal,
	.noise            = mt76x2e_get_noise,
	.quality          = mt76x2e_get_quality,
	.quality_max      = mt76x2e_get_quality_max,
	.mbssid_support   = mt76x2e_get_mbssid_support,
	.hwmodelist       = mt76x2e_get_hwmodelist,
	.mode             = mt76x2e_get_mode,
	.ssid             = mt76x2e_get_ssid,
	.bssid            = mt76x2e_get_bssid,
	.country          = mt76x2e_get_country,
	.hardware_id      = mt76x2e_get_hardware_id,
	.hardware_name    = mt76x2e_get_hardware_name,
	.encryption       = mt76x2e_get_encryption,
	.phyname          = mt76x2e_get_phyname,
	.assoclist        = mt76x2e_get_assoclist,
	.txpwrlist        = mt76x2e_get_txpwrlist,
	.scanlist         = mt76x2e_get_scanlist,
	.freqlist         = mt76x2e_get_freqlist,
	.countrylist      = mt76x2e_get_countrylist,
	.close            = mt76x2e_close
};

