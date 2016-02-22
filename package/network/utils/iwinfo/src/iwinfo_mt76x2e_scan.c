/*
 * iwinfo - Wireless Information Library - Linux Wireless Extension Backend
 *
 *   Copyright (C) 2009-2010 Jo-Philipp Wich <xm@subsignal.org>
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


static int mt76x2e_ioctl(const char *ifname, int cmd, struct iwreq *wrq)
{
    strncpy(wrq->ifr_name, ifname, IFNAMSIZ - 1);
    return iwinfo_ioctl(cmd, wrq);
}

static inline double mt76x2e_freq2float(const struct iw_freq *in)
{
    int		i;
    double	res = (double) in->m;
    for(i = 0; i < in->e; i++) res *= 10;
    return res;
}

static inline int mt76x2e_extract_event(struct stream_descr *stream, struct iw_event *iwe, int wev)
{
    const struct iw_ioctl_description *descr = NULL;
    int event_type = 0;
    unsigned int event_len = 1;
    char *pointer;
    unsigned cmd_index;		/* *MUST* be unsigned */

    /* Check for end of stream */
    if((stream->current + IW_EV_LCP_PK_LEN) > stream->end)
        return 0;

    /* Extract the event header (to get the event id).
     * Note : the event may be unaligned, therefore copy... */
    memcpy((char *) iwe, stream->current, IW_EV_LCP_PK_LEN);

    /* Check invalid events */
    if(iwe->len <= IW_EV_LCP_PK_LEN)
        return -1;

    /* Get the type and length of that event */
    if(iwe->cmd <= SIOCIWLAST)
    {
        cmd_index = iwe->cmd - SIOCIWFIRST;
        if(cmd_index < standard_ioctl_num)
            descr = &(standard_ioctl_descr[cmd_index]);
    }
    else
    {
        cmd_index = iwe->cmd - IWEVFIRST;
        if(cmd_index < standard_event_num)
            descr = &(standard_event_descr[cmd_index]);
    }

    if(descr != NULL)
        event_type = descr->header_type;

    /* Unknown events -> event_type=0 => IW_EV_LCP_PK_LEN */
    event_len = event_type_size[event_type];

    /* Fixup for earlier version of WE */
    if((wev <= 18) && (event_type == IW_HEADER_TYPE_POINT))
        event_len += IW_EV_POINT_OFF;

    /* Check if we know about this event */
    if(event_len <= IW_EV_LCP_PK_LEN)
    {
        /* Skip to next event */
        stream->current += iwe->len;
        return 2;
    }

    event_len -= IW_EV_LCP_PK_LEN;

    /* Set pointer on data */
    if(stream->value != NULL)
        pointer = stream->value;			/* Next value in event */
    else
        pointer = stream->current + IW_EV_LCP_PK_LEN;	/* First value in event */

    /* Copy the rest of the event (at least, fixed part) */
    if((pointer + event_len) > stream->end)
    {
        /* Go to next event */
        stream->current += iwe->len;
        return -2;
    }

    /* Fixup for WE-19 and later : pointer no longer in the stream */
    /* Beware of alignement. Dest has local alignement, not packed */
    if( (wev > 18) && (event_type == IW_HEADER_TYPE_POINT) )
        memcpy((char *) iwe + IW_EV_LCP_LEN + IW_EV_POINT_OFF, pointer, event_len);
    else
        memcpy((char *) iwe + IW_EV_LCP_LEN, pointer, event_len);

    /* Skip event in the stream */
    pointer += event_len;

    /* Special processing for iw_point events */
    if(event_type == IW_HEADER_TYPE_POINT)
    {
        /* Check the length of the payload */
        unsigned int extra_len = iwe->len - (event_len + IW_EV_LCP_PK_LEN);
        if(extra_len > 0)
        {
            /* Set pointer on variable part (warning : non aligned) */
            iwe->u.data.pointer = pointer;

            /* Check that we have a descriptor for the command */
            if(descr == NULL)
                /* Can't check payload -> unsafe... */
                iwe->u.data.pointer = NULL;	/* Discard paylod */
            else
            {
                /* Those checks are actually pretty hard to trigger,
                * because of the checks done in the kernel... */

                unsigned int	token_len = iwe->u.data.length * descr->token_size;

                /* Ugly fixup for alignement issues.
                * If the kernel is 64 bits and userspace 32 bits,
                * we have an extra 4+4 bytes.
                * Fixing that in the kernel would break 64 bits userspace. */
                if((token_len != extra_len) && (extra_len >= 4))
                {
                    uint16_t alt_dlen = *((uint16_t *) pointer);
                    unsigned int alt_token_len = alt_dlen * descr->token_size;
                    if((alt_token_len + 8) == extra_len)
                    {
                        /* Ok, let's redo everything */
                        pointer -= event_len;
                        pointer += 4;
                        /* Dest has local alignement, not packed */
                        memcpy((char *) iwe + IW_EV_LCP_LEN + IW_EV_POINT_OFF, pointer, event_len);
                        pointer += event_len + 4;
                        iwe->u.data.pointer = pointer;
                        token_len = alt_token_len;
                    }
                }

                /* Discard bogus events which advertise more tokens than
                * what they carry... */
                if(token_len > extra_len)
                    iwe->u.data.pointer = NULL;	/* Discard paylod */

                /* Check that the advertised token size is not going to
                * produce buffer overflow to our caller... */
                if((iwe->u.data.length > descr->max_tokens)
                        && !(descr->flags & IW_DESCR_FLAG_NOMAX))
                    iwe->u.data.pointer = NULL;	/* Discard paylod */

                /* Same for underflows... */
                if(iwe->u.data.length < descr->min_tokens)
                    iwe->u.data.pointer = NULL;	/* Discard paylod */
            }
        }
        else
            /* No data */
            iwe->u.data.pointer = NULL;

        /* Go to next event */
        stream->current += iwe->len;
    }
    else
    {
        /* Ugly fixup for alignement issues.
        * If the kernel is 64 bits and userspace 32 bits,
        * we have an extra 4 bytes.
        * Fixing that in the kernel would break 64 bits userspace. */
        if((stream->value == NULL)
                && ((((iwe->len - IW_EV_LCP_PK_LEN) % event_len) == 4)
                    || ((iwe->len == 12) && ((event_type == IW_HEADER_TYPE_UINT) ||
                                             (event_type == IW_HEADER_TYPE_QUAL))) ))
        {
            pointer -= event_len;
            pointer += 4;
            /* Beware of alignement. Dest has local alignement, not packed */
            memcpy((char *) iwe + IW_EV_LCP_LEN, pointer, event_len);
            pointer += event_len;
        }

        /* Is there more value in the event ? */
        if((pointer + event_len) <= (stream->current + iwe->len))
            /* Go to next value */
            stream->value = pointer;
        else
        {
            /* Go to next event */
            stream->value = NULL;
            stream->current += iwe->len;
        }
    }

    return 1;
}

static inline void mt76x2e_fill_wpa(unsigned char *iebuf, int ielen, struct iwinfo_scanlist_entry *e)
{
    static unsigned char ms_oui[3] = { 0x00, 0x50, 0xf2 };

    while (ielen >= 2 && ielen >= iebuf[1])
    {
        switch (iebuf[0])
        {
        case 48: /* RSN */
            iwinfo_parse_rsn(&e->crypto, iebuf + 2, iebuf[1],
                             IWINFO_CIPHER_CCMP, IWINFO_KMGMT_8021x);
            break;

        case 221: /* Vendor */
            if (iebuf[1] >= 4 && !memcmp(iebuf + 2, ms_oui, 3) && iebuf[5] == 1)
                iwinfo_parse_rsn(&e->crypto, iebuf + 6, iebuf[1] - 4,
                                 IWINFO_CIPHER_TKIP, IWINFO_KMGMT_PSK);
            break;
        }

        ielen -= iebuf[1] + 2;
        iebuf += iebuf[1] + 2;
    }
}


static inline void mt76x2e_fill_entry(struct stream_descr *stream, struct iw_event *event,
                                      struct iw_range *iw_range, int has_range, struct iwinfo_scanlist_entry *e)
{
    int i;
    double freq;

    /* Now, let's decode the event */
    switch(event->cmd)
    {
    case SIOCGIWAP:
        memcpy(e->mac, &event->u.ap_addr.sa_data, 6);
        break;

    case SIOCGIWFREQ:
        if( event->u.freq.m >= 1000 )
        {
            freq = mt76x2e_freq2float(&(event->u.freq));

            for(i = 0; i < iw_range->num_frequency; i++)
            {
                if( mt76x2e_freq2float(&iw_range->freq[i]) == freq )
                {
                    e->channel = iw_range->freq[i].i;
                    break;
                }
            }
        }
        else
        {
            e->channel = event->u.freq.m;
        }

        break;

    case SIOCGIWMODE:
        switch(event->u.mode)
        {
        case 1:
            e->mode = IWINFO_OPMODE_ADHOC;
            break;

        case 2:
        case 3:
            e->mode = IWINFO_OPMODE_MASTER;
            break;

        default:
            e->mode = IWINFO_OPMODE_UNKNOWN;
            break;
        }

        break;

    case SIOCGIWESSID:
        if( event->u.essid.pointer && event->u.essid.length && event->u.essid.flags )
            memcpy(e->ssid, event->u.essid.pointer, event->u.essid.length);

        break;

    case SIOCGIWENCODE:
        e->crypto.enabled = !(event->u.data.flags & IW_ENCODE_DISABLED);
        break;

    case IWEVQUAL:
        e->signal = event->u.qual.level;
        e->quality = event->u.qual.qual;
        e->quality_max = iw_range->max_qual.qual;
        break;
#if 0
    case SIOCGIWRATE:
        if(state->val_index == 0)
        {
            lua_pushstring(L, "bitrates");
            lua_newtable(L);
        }
        //iw_print_bitrate(buffer, sizeof(buffer), event->u.bitrate.value);
        snprintf(buffer, sizeof(buffer), "%d", event->u.bitrate.value);
        lua_pushinteger(L, state->val_index + 1);
        lua_pushstring(L, buffer);
        lua_settable(L, -3);

        /* Check for termination */
        if(stream->value == NULL)
        {
            lua_settable(L, -3);
            state->val_index = 0;
        } else
            state->val_index++;
        break;
#endif
    case IWEVGENIE:
        mt76x2e_fill_wpa(event->u.data.pointer, event->u.data.length, e);
        break;
    }
}

#define MYLOG(mes) \
	{   char buf[8096];  \
	sprintf(buf, "echo -e \"%s:%d:\n%s\n\">>/tmp/iwinfo", __FUNCTION__, __LINE__, mes);	 \
	system(buf);	\
	}

int mt76x2e_get_scanlist(const char *ifname, char *buf, int *len)
{
    int retval = 0, i = 0, apCount = 0;
    char data[8192];
    char ssid_str[256];
    char header[128];
    struct iwreq wrq;
    int entrylen=0;
    int line_len;
    SSA *ssap;

    struct iwinfo_scanlist_entry tempscanlist;

    memset(data, 0x00, 32);
    strcpy(data, "SiteSurvey=1");
    wrq.u.data.length = strlen(data)+1;
    wrq.u.data.pointer = data;
    wrq.u.data.flags = 0;

    if (mt76x2e_ioctl(ifname, RTPRIV_IOCTL_SET, &wrq) < 0)
        return -1;

    sleep(5);

    memset(data, 0, sizeof(data));
    wrq.u.data.length = sizeof(data);
    wrq.u.data.pointer = data;
    wrq.u.data.flags = 0;

    if (mt76x2e_ioctl(ifname, RTPRIV_IOCTL_GSITESURVEY, &wrq) < 0)
        return -1;

    line_len = sizeof(struct _SITE_SURVEY);

    if (wrq.u.data.length > 0)
    {
        char *sp, *op;
        /*		MYLOG(wrq.u.data.pointer+1); */
        op = sp = wrq.u.data.pointer+line_len+1;
        int llen = strlen(op);
        ssap=(SSA *)(op);
        while (*sp && ((llen - (sp-op)) >= 0))
        {
            ssap->SiteSurvey[i].channel[3] = '\0';
            ssap->SiteSurvey[i].ssid[32] = '\0';
            ssap->SiteSurvey[i].bssid[19] = '\0';
            ssap->SiteSurvey[i].encryption[22] = '\0';
            ssap->SiteSurvey[i].signal[8] = '\0';
            ssap->SiteSurvey[i].wmode[7] = '\0';
            ssap->SiteSurvey[i].extch[6] = '\0';
            ssap->SiteSurvey[i].nt[2] = '\0';
            sp+=line_len+1;
            apCount=++i;
        }
        if (apCount)
        {
            char *enc, *enc1, auth[25];
            int flag;
            for (i = 0; i < apCount; i++)
            {
                memset(&tempscanlist, 0, sizeof(struct iwinfo_scanlist_entry));
                tempscanlist.mode=IWINFO_OPMODE_MASTER;
                tempscanlist.channel=atoi(ssap->SiteSurvey[i].channel);
                int ssidlen=31;
                while(ssap->SiteSurvey[i].ssid[ssidlen]==' ')ssidlen--;
                strlcpy(tempscanlist.ssid,ssap->SiteSurvey[i].ssid,ssidlen+2);
                sscanf(ssap->SiteSurvey[i].bssid, "%2hhx:%2hhx:%2hhx:%2hhx:%2hhx:%2hhx",
                       &tempscanlist.mac[0],
                       &tempscanlist.mac[1],
                       &tempscanlist.mac[2],
                       &tempscanlist.mac[3],
                       &tempscanlist.mac[4],
                       &tempscanlist.mac[5]);
                tempscanlist.quality_max=100;
                tempscanlist.quality=atoi(ssap->SiteSurvey[i].signal);

                strcpy(auth, ssap->SiteSurvey[i].encryption);
                char *enc = strchrnul(auth,' ');
                enc[0] = '\0';
                MYLOG(auth);
                if (strstr(auth,"NONE")) {
                    tempscanlist.crypto.auth_algs |= IWINFO_AUTH_OPEN;
                    tempscanlist.crypto.pair_ciphers |= IWINFO_CIPHER_NONE;
                    tempscanlist.crypto.auth_suites |= IWINFO_KMGMT_NONE;
                } else {
                    char *enc = strchr(auth,'/');
                    if (!enc)
                        enc1 = auth;
                    else
                    {
                        enc[0] = '\0';
                        enc1 = enc+1;
                    }
		    /*
		    MYLOG(auth);
                    MYLOG(enc1);
		    */
                    if (strcmp(enc1,"WEP")==0) {
                        tempscanlist.crypto.pair_ciphers |= IWINFO_CIPHER_WEP40;
                        tempscanlist.crypto.auth_suites |= IWINFO_KMGMT_NONE;
                        tempscanlist.crypto.auth_algs |= IWINFO_AUTH_OPEN;
                    } else if (strcmp(enc1,"TKIP")==0) {
                        tempscanlist.crypto.pair_ciphers |= IWINFO_CIPHER_TKIP;
                        tempscanlist.crypto.auth_suites |= IWINFO_KMGMT_PSK;
                        tempscanlist.crypto.auth_algs |= IWINFO_AUTH_SHARED;
                    } else if (strcmp(enc1,"AES")==0) {
                        tempscanlist.crypto.pair_ciphers |= IWINFO_CIPHER_CCMP;
                        tempscanlist.crypto.auth_suites |= IWINFO_KMGMT_PSK;
                        tempscanlist.crypto.auth_algs |= IWINFO_AUTH_SHARED;
                    } else if (strcmp(enc1,"TKIPAES")==0) {
                        tempscanlist.crypto.pair_ciphers |= IWINFO_CIPHER_TKIP;
                        tempscanlist.crypto.pair_ciphers |= IWINFO_CIPHER_CCMP;
                        tempscanlist.crypto.auth_suites |= IWINFO_KMGMT_PSK;
                        tempscanlist.crypto.auth_algs |= IWINFO_AUTH_SHARED;
                    }

                    /* EAP / wpa version */
                    if (strcmp(auth,"WPA")==0) {
                        tempscanlist.crypto.auth_suites |= IWINFO_KMGMT_8021x;
                        tempscanlist.crypto.wpa_version = 1;
                    }
                    if (strcmp(auth,"WPA2")==0) {
                        tempscanlist.crypto.auth_suites |= IWINFO_KMGMT_8021x;
                        tempscanlist.crypto.wpa_version = 2;
                    }
                    if (strcmp(auth,"WPA1WPA2")==0) {
                        tempscanlist.crypto.auth_suites |= IWINFO_KMGMT_8021x;
                        tempscanlist.crypto.wpa_version = 3;
                    }
                    if (strcmp(auth, "WPANONE")==0)
                        tempscanlist.crypto.auth_suites |= IWINFO_KMGMT_NONE;

                    /* wpa-psk / version */
                    if (strstr(auth,"WPAPSK"))
                        tempscanlist.crypto.wpa_version = 1;
                    if (strstr(auth,"WPA2PSK"))
                        tempscanlist.crypto.wpa_version = 2;
                    if (strstr(auth,"WPA1PSKWPA2PSK"))
                        tempscanlist.crypto.wpa_version = 3;
                }
                tempscanlist.crypto.group_ciphers = tempscanlist.crypto.pair_ciphers;
                tempscanlist.crypto.enabled = (tempscanlist.crypto.wpa_version && tempscanlist.crypto.auth_suites) ? 1 : 0;

                memcpy(&buf[entrylen], &tempscanlist, sizeof(struct iwinfo_scanlist_entry));
                entrylen += sizeof(struct iwinfo_scanlist_entry);
            }
        }
    }
    *len = entrylen;
    return 0;
}

