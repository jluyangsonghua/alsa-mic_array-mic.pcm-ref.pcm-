/*************************************************************************
	> File Name: mic_array.h
	> Author: 
	> Mail: 
	> Created Time: Mon May  4 14:19:38 2015
 ************************************************************************/

#ifndef ANDROID_MIC_ARRAY_H
#define ANDROID_MIC_ARRAY_H

#include <stdint.h>
#include <hardware/hardware.h>

//#include <alsa/asoundlib.h>
typedef void rk_pcm_t;
//typedef struct pcm rk_pcm_t;

__BEGIN_DECLS

#define MIC_ARRAY_HARDWARE_MODULE_ID "mic_array"

//maybe u should not define it
#if (defined(ALTER_MIC_ARRAY) || defined(BOARD_ROKID_MIC_USE_NON_HAL_BUILD))
struct mic_array_device_t {
#else
struct mic_array_module_t {
    struct hw_module_t common;
};
struct mic_array_device_t {
    struct hw_device_t common;
#endif
    unsigned int channels;
    unsigned int sample_rate;
    unsigned int bit;
    unsigned int frame_cnt;
    rk_pcm_t *pcm;
/*
    unsigned int period_size;
    unsigned int period_count;
*/

    int (*get_stream_buff_size) (struct mic_array_device_t *dev);
    int (*start_stream) (struct mic_array_device_t *dev);
    int (*stop_stream) (struct mic_array_device_t *dev);
    int (*finish_stream) (struct mic_array_device_t * dev);
    int (*resume_stream) (struct mic_array_device_t *dev);
    int (*read_stream) (struct mic_array_device_t *dev, char *buff, unsigned int frame_cnt);
    int (*config_stream) (struct mic_array_device_t *dev, int cmd, char *cmd_buff);
//    int (*find_card) (const char *snd);
};

#if (defined(ALTER_MIC_ARRAY) || defined(BOARD_ROKID_MIC_USE_NON_HAL_BUILD))
extern int mic_array_device_open(struct mic_array_device_t **);
extern int mic_array_device_close(struct mic_array_device_t *);
#endif

__END_DECLS
#endif
