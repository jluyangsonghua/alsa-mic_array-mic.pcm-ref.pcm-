
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/syscall.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <pthread.h>

#include <hardware/hardware.h>
#include <hardware/mic_array.h>

struct mic_array_device_t* device = NULL;
#if !(defined(BOARD_ROKID_MIC_USE_NON_HAL_BUILD) || defined(ALTER_MIC_ARRAY))
struct mic_array_module_t* module = NULL;
static inline int mic_array_device_open (const hw_module_t *module, struct mic_array_device_t **device) {
	return module->methods->open (module, MIC_ARRAY_HARDWARE_MODULE_ID, (struct hw_device_t **) device);
}
#endif


char* debugPcmFile = "/data/debug.pcm";

int main(int argc, char* argv[])
{
	int ret;
	int mic_array_frame_size;
	char *Buff;
	FILE *fp = NULL;
	int count = 1000;
	int count_max = count;

	printf("Rokid-MicArray/File 20190412 open=%s\n", debugPcmFile);

#if 1
	if (hw_get_module (MIC_ARRAY_HARDWARE_MODULE_ID, (const struct hw_module_t **)&module) == 0) {
		//open mic array
		if (0 != mic_array_device_open(&module->common, &device)) {
			return -1;
		}
	} else {
		printf("get module failed\n");
		return -1;
	}

#else
	if (0 != mic_array_device_open(&device)) {
		return -1;
	}
#endif

	fp = fopen(debugPcmFile, "wb");
	device->start_stream(device);
	mic_array_frame_size = device->get_stream_buff_size(device);
	printf("mic_array_frame_size(bytes)=%d\n", mic_array_frame_size);
	Buff = (char *)malloc(mic_array_frame_size);

	printf("start stream\n");
	while(count > 0){
		printf("mic_array count=%d,i=%d\n", count_max, count);
		ret = device->read_stream(device, Buff, mic_array_frame_size);
		if(ret != 0) {
			printf("read stream failed ret=%d\n", ret);
			sleep(10);
			exit(1);
		} 
		fwrite (Buff, sizeof(char), mic_array_frame_size, fp);
		count--;
	}
	fclose(fp);
	device->stop_stream(device);
	device->common.close(device);
	return 0;
}
