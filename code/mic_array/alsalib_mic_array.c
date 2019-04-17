/*************************************************************************
        > File Name: mic_array.c
        > Author:
        > Mail:
        > Created Time: Mon May  4 14:22:33 2015
 ************************************************************************/
#define LOG_TAG "mic_array"
#define LOG_NDEBUG 0

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <signal.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/syscall.h>
#include <sys/resource.h>
#include <hardware/hardware.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <string.h>
#include <errno.h>
#include <cutils/log.h>
#include <utils/Log.h>
#include <log/log.h>
#include <cutils/atomic.h>
#include <cutils/properties.h>
#include <android/log.h>

#include <time.h>
#include <alsa/asoundlib.h>
#include <system/audio.h>
#include <hardware/audio.h>
#include <hardware/mic_array.h>


#define MODULE_NAME "mic_array@alsa-lib"
#define MODULE_AUTHOR "lujnan@rokid.com"

#ifndef LOG_TAG
#define LOG_TAG "ANDROID_MIC_ARRAY"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#endif


#define ROKID_MIC_ARRAY_DEVICE_DEFAULT_NAME "rokid_mics"
#if ! defined(MIC_CHANNEL)
  #define MIC_CHANNEL 4
#endif

#if ! defined(MIC_SAMPLE_RATE)
  #define MIC_SAMPLE_RATE 16000
#endif


#define MIC_PERIOD_SIZE 2304
#define MIC_PERIOD_COUNT 4
#define MIC_BUFFER_SIZE (MIC_PERIOD_SIZE * MIC_PERIOD_COUNT)
#define MIC_PCM_FORMAT SND_PCM_FORMAT_S16_LE
#define FRAME_COUNT (MIC_SAMPLE_RATE / 100 * MIC_CHANNEL * 2)
//#define MIC_PERIOD_SIZE 1024
//#define MIC_PERIOD_COUNT 4
//#define MIC_PCM_FORMAT SND_PCM_FORMAT_S16_LE

static struct mic_array_device_ex {
    struct mic_array_device_t mic_array;

    int pts;
    char* buffer;
};

static int mic_array_device_open(const struct hw_module_t* module,
    const char* name, struct hw_device_t** device);

static int mic_array_device_close(struct hw_device_t* device);

static int mic_array_device_start_stream(struct mic_array_device_t* dev);

static int mic_array_device_stop_stream(struct mic_array_device_t* dev);

static int mic_array_device_finish_stream(struct mic_array_device_t* dev);

static int mic_array_device_read_stream(
    struct mic_array_device_t* dev, char* buff, unsigned int frame_cnt);

static int mic_array_device_config_stream(
    struct mic_array_device_t* dev, int cmd, char* cmd_buff);

static int mic_array_device_get_stream_buff_size(
    struct mic_array_device_t* dev);

static int mic_array_device_resume_stream(struct mic_array_device_t* dev);

static struct hw_module_methods_t mic_array_module_methods = {
    .open = mic_array_device_open,
};

//static int debug_fd = 0;
//static int mic_timeout_count = 0;
//static pthread_t log_th;

struct mic_array_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .version_major = 1,
        .version_minor = 0,
        .id = MIC_ARRAY_HARDWARE_MODULE_ID,
        .name = MODULE_NAME,
        .author = MODULE_AUTHOR,
        .methods = &mic_array_module_methods,
    },
};

static int mic_array_device_find_card (const char *snd);

int find_snd(const char* snd)
{
    char* path = "/proc/asound/cards";
    FILE* fs;
    char buf[4096];
    char *b, *e;
    int card = -1;
    int len;

    if (!(fs = fopen(path, "r"))) {
        ALOGE("%s  :  %s", __FUNCTION__, strerror(errno));
        return errno;
    }

    len = fread(buf, 1, sizeof(buf) - 1, fs);
    buf[len - 1] = '\0';
    fclose(fs);

    b = buf;
    while (e = strchr(b, '\n')) {
        *e = '\0';
        if (strstr(b, snd)) {
            card = atoi(b);
            break;
        }
        b = e + 1;
    }
    ALOGI("find -> %d", card);
    return card;
}

#if 0
static void tinymix_set_value(struct mixer* mixer, const char* control,
    char** values, unsigned int num_values)
{
    struct mixer_ctl* ctl;
    enum mixer_ctl_type type;
    unsigned int num_ctl_values;
    unsigned int i;

    if (isdigit(control[0]))
        ctl = mixer_get_ctl(mixer, atoi(control));
    else
        ctl = mixer_get_ctl_by_name(mixer, control);

    if (!ctl) {
        ALOGE("Invalid mixer control\n");
        return;
    }

    type = mixer_ctl_get_type(ctl);
    num_ctl_values = mixer_ctl_get_num_values(ctl);

    if (isdigit(values[0][0])) {
        if (num_values == 1) {
            /* Set all values the same */
            int value = atoi(values[0]);

            for (i = 0; i < num_ctl_values; i++) {
                if (mixer_ctl_set_value(ctl, i, value)) {
                    ALOGE("Error: invalid value  %s\n", strerror(errno));
                    return;
                }
            }
        } else {
            /* Set multiple values */
            if (num_values > num_ctl_values) {
                ALOGE("Error: %d values given, but control only takes %d\n",
                    num_values, num_ctl_values);
                return;
            }
            for (i = 0; i < num_values; i++) {
                if (mixer_ctl_set_value(ctl, i, atoi(values[i]))) {
                    ALOGE("Error: invalid value for index %d\n", i);
                    return;
                }
            }
        }
    } else {
        if (type == MIXER_CTL_TYPE_ENUM) {
            if (num_values != 1) {
                ALOGE("Enclose strings in quotes and try again\n");
                return;
            }
            if (mixer_ctl_set_enum_by_string(ctl, values[0]))
                ALOGE("Error: invalid enum value\n");
        } else {
            ALOGE("Error: only enum types can be set with strings\n");
        }
    }
}
#endif

static int mic_array_device_find_card (const char *snd){
    if(snd == NULL)
        return -1;
    return find_snd(snd);
}

static int mic_array_device_open(const struct hw_module_t* module,
    const char* name, struct hw_device_t** device)
{
    int i = 0;
    struct mic_array_device_ex* dev_ex = NULL;
    struct mic_array_device_t* dev = NULL;

    ALOGE("open mic array start: %s\n", __func__);
    ALOGE("open mic array start: %s\n", __func__);

    dev_ex = (struct mic_array_device_ex*)malloc(
        sizeof(struct mic_array_device_ex));
    dev = (struct mic_array_device_t*)dev_ex;

    if (!dev_ex) {
        ALOGE("MIC_ARRAY: FAILED TO ALLOC SPACE");
        return -1;
    }

    memset(dev, 0, sizeof(struct mic_array_device_ex));
    dev->common.tag = HARDWARE_DEVICE_TAG;
    dev->common.version = 0;
    dev->common.module = (hw_module_t*)module;
    dev->common.close = mic_array_device_close;
    dev->start_stream = mic_array_device_start_stream;
    dev->stop_stream = mic_array_device_stop_stream;
    dev->finish_stream = mic_array_device_finish_stream;
    dev->resume_stream = mic_array_device_resume_stream;
    dev->read_stream = mic_array_device_read_stream;
    dev->config_stream = mic_array_device_config_stream;
    dev->get_stream_buff_size = mic_array_device_get_stream_buff_size;
    //dev->find_card = mic_array_device_find_card;

    dev->frame_cnt = FRAME_COUNT;
    dev->channels = MIC_CHANNEL;
    dev->sample_rate = MIC_SAMPLE_RATE;
    dev->bit = snd_pcm_format_physical_width(MIC_PCM_FORMAT);
    dev->pcm = NULL;
    *device = &(dev->common);
    return 0;
}

static void resetBuffer(struct mic_array_device_ex* dev) { dev->pts = 0; }

static int mic_array_device_close(struct hw_device_t* device)
{
    ALOGI("mic device close");

    struct mic_array_device_t* mic_array_device
        = (struct mic_array_device_t*)device;
    struct mic_array_device_ex* dev_ex
        = (struct mic_array_device_ex*)mic_array_device;

    mic_array_device_stop_stream(mic_array_device);
    if (dev_ex != NULL) {
      if (dev_ex->buffer) {
        free(dev_ex->buffer);
      }
      free(mic_array_device);
      mic_array_device = NULL;
      dev_ex = NULL;
    }
    return 0;
}

static int set_swparams(snd_pcm_t *handle, snd_pcm_sw_params_t *swparams)
{
        int err;
        /* get the current swparams */
        err = snd_pcm_sw_params_current(handle, swparams);
        if (err < 0) {
                printf("Unable to determine current swparams for playback: %s\n", snd_strerror(err));
                return err;
        }
        /* start the transfer when the buffer is almost full: */
        /* (buffer_size / avail_min) * avail_min */
        err = snd_pcm_sw_params_set_start_threshold(handle, swparams, (MIC_BUFFER_SIZE / MIC_PERIOD_SIZE) * MIC_PERIOD_SIZE);
        if (err < 0) {
                printf("Unable to set start threshold mode for playback: %s\n", snd_strerror(err));
                return err;
        }
        /* allow the transfer when at least period_size samples can be processed */
        /* or disable this mechanism when period event is enabled (aka interrupt like style processing) */
        err = snd_pcm_sw_params_set_avail_min(handle, swparams, MIC_PERIOD_SIZE);
        if (err < 0) {
                printf("Unable to set avail min for playback: %s\n", snd_strerror(err));
                return err;
        }
        /* write the parameters to the playback device */
        err = snd_pcm_sw_params(handle, swparams);
        if (err < 0) {
                printf("Unable to set sw params for playback: %s\n", snd_strerror(err));
                return err;
        }
        return 0;
}

static int set_hwparams(snd_pcm_t *pcm, snd_pcm_hw_params_t *hwparams)
{
	int err;
    	snd_pcm_uframes_t period_size = MIC_PERIOD_SIZE;
    	int periods = MIC_PERIOD_COUNT;
    	unsigned int channels = MIC_CHANNEL;
    	unsigned int rate = MIC_SAMPLE_RATE;

	snd_pcm_hw_params_any(pcm, hwparams);
	err = snd_pcm_hw_params_set_access(pcm, hwparams, SND_PCM_ACCESS_RW_INTERLEAVED);
        if (err < 0) {
                printf("set access error: %d,des=%s\n", err, snd_strerror(err));
                return err;
        }
	snd_pcm_hw_params_set_format(pcm, hwparams, MIC_PCM_FORMAT);
	snd_pcm_hw_params_set_rate_near(pcm, hwparams, &rate, 0);
	snd_pcm_hw_params_set_channels_near(pcm, hwparams, &channels);
	snd_pcm_hw_params_set_period_size(pcm, hwparams, period_size, 0);
	snd_pcm_hw_params_set_periods(pcm, hwparams, periods, 0);
	snd_pcm_hw_params_set_buffer_size(pcm, hwparams, MIC_BUFFER_SIZE);

	if ((err = snd_pcm_hw_params(pcm, hwparams)) < 0) {
		fprintf(stderr, "Unable to set ALSA parameters: %s\n",
					snd_strerror(err));
		return -1;
	}

        return 0;
}

/*
 *   Underrun and suspend recovery
 */

static int xrun_recovery(snd_pcm_t *handle, int err)
{
        if (err == -EPIPE) {    /* under-run */
                printf("%s ready to prepare pcm: %s\n",__func__, snd_strerror(err));
                err = snd_pcm_prepare(handle);
                if (err < 0)
                        printf("Can't recovery from underrun, prepare failed: %s\n", snd_strerror(err));
                return 0;
        } else if (err == -ESTRPIPE) {
                while ((err = snd_pcm_resume(handle)) == -EAGAIN)
                        sleep(1);       /* wait until the suspend flag is released */
                if (err < 0) {
                        err = snd_pcm_prepare(handle);
                        if (err < 0)
                                printf("Can't recovery from suspend, prepare failed: %s\n", snd_strerror(err));
                }
                return 0;
        }
        return err;
}

static int mic_array_device_start_stream(struct mic_array_device_t* dev)
{
    struct mic_array_device_ex* dev_ex = (struct mic_array_device_t*)dev;
    snd_pcm_t* pcm = NULL;
    snd_pcm_hw_params_t *hwparams;
    snd_pcm_hw_params_t *swparams;
    int err;

    ALOGE("ALOGE mic array start: %s\n", __func__);

		assert(dev->pcm == NULL);
    err = snd_pcm_open(&pcm, ROKID_MIC_ARRAY_DEVICE_DEFAULT_NAME, SND_PCM_STREAM_CAPTURE, 0);
    if (err < 0) {
    	ALOGE("open mic array error: %s\n", snd_strerror(err));
	return -1;
    }

    if (pcm == NULL) {
        ALOGE("pcm open failed");
        if (pcm != NULL) {
            snd_pcm_close(pcm);
            ALOGE("[stupidi_FSC]pcm open failed and pcm_close!");
            dev->pcm = NULL;
        }
        return -1;
    }

    dev->pcm = pcm;

	snd_pcm_hw_params_alloca(&hwparams);
	set_hwparams(pcm, hwparams);

	snd_pcm_sw_params_alloca(&swparams);
	set_swparams(pcm, swparams);

	snd_pcm_nonblock(pcm, 0);
	if ((err = snd_pcm_prepare(pcm)) < 0) {
		ALOGE("Unable to prepare ALSA: %s\n", snd_strerror(err));
		return -1;
	}

    //dev->frame_cnt = snd_pcm_frames_to_bytes(pcm, buffer_size);
    dev->frame_cnt = FRAME_COUNT;
    ALOGI("--------------alloc frame buffer size %d", dev->frame_cnt);
    dev_ex->buffer = (char*)malloc(dev->frame_cnt);
    return 0;
}

static int mic_array_device_stop_stream(struct mic_array_device_t* dev)
{
    struct mic_array_device_ex* dev_ex = (struct mic_array_device_t*)dev;
    if (dev->pcm != NULL) {
        snd_pcm_close((snd_pcm_t*)dev->pcm);
        dev->pcm = NULL;
        dev->frame_cnt = 0;
        free(dev_ex->buffer);
        dev_ex->buffer = NULL;
        dev_ex->pts = 0;
    }
    return 0;
}

static int mic_array_device_finish_stream(struct mic_array_device_t* dev)
{
    ALOGE("finish stream is no use");
    return -1;
}

static int read_frame(struct mic_array_device_t* dev, char* buffer)
{
    snd_pcm_t* pcm = (snd_pcm_t*) dev->pcm;
    int err = snd_pcm_readi(pcm, buffer, snd_pcm_bytes_to_frames(pcm, dev->frame_cnt));
    if (err < 0) {
    	if (xrun_recovery(pcm, err) < 0) {
        	printf("Write error: %s\n", snd_strerror(err));
        }
        return err; // let app know current data is not avalible.
    }

    return 0;
}

static int read_left_frame(
    struct mic_array_device_ex* dev, char* buff, int left)
{
    int ret = 0;
    if (dev->pts == 0) {
        if ((ret = read_frame(dev, dev->buffer)) != 0) {
            ALOGE("read_frame %s", snd_strerror(ret));
            resetBuffer(dev);
            return ret;
        }
        memcpy(buff, dev->buffer, left);
        memcpy(dev->buffer, dev->buffer + left, dev->mic_array.frame_cnt - left);
        dev->pts = dev->mic_array.frame_cnt - left;
    } else {
        if (dev->pts >= left) {
            memcpy(buff, dev->buffer, left);
            dev->pts -= left;
            if (dev->pts != 0) {
                memcpy(dev->buffer, dev->buffer + left, dev->pts);
            }
        } else {
            memcpy(buff, dev->buffer, dev->pts);
            left -= dev->pts;
            if ((ret = read_frame(dev, dev->buffer)) != 0) {
            	ALOGE("read_frame %s", snd_strerror(ret));
                resetBuffer(dev);
                return ret;
            }
            memcpy(buff + dev->pts, dev->buffer, left);
            memcpy(dev->buffer, dev->buffer + left,
                dev->mic_array.frame_cnt - left);
            dev->pts = dev->mic_array.frame_cnt - left;
        }
    }
    return 0;
}

static int mic_array_device_read_stream(
    struct mic_array_device_t* dev, char* buff, unsigned int frame_cnt)
{
    snd_pcm_t* pcm = (snd_pcm_t*)dev->pcm;
    struct mic_array_device_ex* dev_ex = (struct mic_array_device_ex*)dev;
    char* target = NULL;

    int ret = 0;
    int left = 0;
    int size = dev->frame_cnt;
    if (size <= 0) {
        ALOGE("frame cnt lt 0");
        return -1;
    }

    if (buff == NULL) {
        ALOGE("null buffer");
        return -1;
    }

    if (frame_cnt >= size) {
        int cnt = frame_cnt / size;
        int i;
        left = frame_cnt % size;

        if (dev_ex->pts > left) {
            --cnt;
        }
        if (dev_ex->pts > 0) {
            memcpy(buff, dev_ex->buffer, dev_ex->pts);
        }

        for (i = 0; i < cnt; i++) {
            ALOGE("read_frame in for,count=%d,i=%d\n", cnt, i);
            if ((ret = read_frame(dev, buff + dev_ex->pts + i * size)) != 0) {
                ALOGE("read_frame %s,i=%d\n", snd_strerror(ret), i);
                resetBuffer(dev_ex);
                return ret;
            }
        }
        if (frame_cnt - (dev_ex->pts + cnt * size) == 0) {
            dev_ex->pts = 0;
            return ret;
        }
        //		ALOGE("-------------------cnt : %d, left : %d, cache :
        //%d, frame_cnt : %d", cnt, left, dev_ex->pts, frame_cnt);
        target = buff + dev_ex->pts + cnt * size;
        left = frame_cnt - (dev_ex->pts + cnt * size);
        dev_ex->pts = 0;
    } else {
        target = buff;
        left = frame_cnt;
    }

    if ((ret = read_left_frame(dev_ex, target, left)) != 0) {
        ALOGE("read frame return %d, pcm read error", ret);
        resetBuffer(dev_ex);
        return ret;
    }

    return ret;
}

static int mic_array_device_config_stream(
    struct mic_array_device_t* dev, int cmd, char* cmd_buff)
{
    return -1;
}

static int mic_array_device_get_stream_buff_size(
    struct mic_array_device_t* dev)
{
    return dev->frame_cnt;
}

static int mic_array_device_resume_stream(struct mic_array_device_t* dev)
{
    ALOGI("not implmentation");
    return -1;
}
