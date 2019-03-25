#include "config.h"
#include "utils.h"

#include <libbladeRF.h>
#include "transceive.h"


#include<netinet/in.h>
#include<errno.h>
#include<netdb.h>
#include<netinet/if_ether.h>  //For ETH_P_ALL
#include <netinet/ip.h>
#include<netinet/ip_icmp.h>
#include<netinet/udp.h>   //Provides declarations for udp header
#include<netinet/tcp.h>   //Provides declarations for tcp header
#include<netinet/ip.h> 
#include<netinet/if_ether.h>  //For ETH_P_ALL
#include<net/ethernet.h>  //For ether_header
#include<sys/ioctl.h>
#include<sys/types.h>
#include <math.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <linux/if_tun.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <errno.h>
#include <stdarg.h>


void DisplayPacket(unsigned char*, int);
int tun_tx_fd;

int tun_alloc(char *dev, int flags) 
{

  struct ifreq ifr;
  int fd, err;
  char *clonedev = "/dev/net/tun";

  /* Arguments taken by the function:
   *
   * char *dev: the name of an interface (or '\0'). MUST have enough
   *   space to hold the interface name if '\0' is passed
   * int flags: interface flags (eg, IFF_TUN etc.)
   */

   /* open the clone device */
   if( (fd = open(clonedev, O_RDWR)) < 0 ) {
     return fd;
   }
  printf("dev net tune has been opened\n");

   /* preparation of the struct ifr, of type "struct ifreq" */
   memset(&ifr, 0, sizeof(ifr));

   ifr.ifr_flags = flags;   /* IFF_TUN or IFF_TAP, plus maybe IFF_NO_PI */

   if (*dev) {
     /* if a device name was specified, put it in the structure; otherwise,
      * the kernel will try to allocate the "next" device of the
      * specified type */
     strncpy(ifr.ifr_name, dev, IFNAMSIZ);
   }

   /* try to create the device */
   if( (err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0 ) {
     close(fd);
     return err;
   }
  printf("TUNSETIFF\n");
  /* if the operation was successful, write back the name of the
   * interface to the variable "dev", so the caller can know
   * it. Note that the caller MUST reserve space in *dev (see calling
   * code below) */
  strcpy(dev, ifr.ifr_name);

  /* this is the special file descriptor that the caller will use to talk
   * with the virtual interface */
  return fd;
}


/* The RX and TX modules are configured independently for these parameters */
void print_bytes(const void *object, size_t size)
{
  const unsigned char * const bytes = object;
  size_t i;

  printf("[ ");
  for(i = 0; i < size; i++)
  {
    printf("%02x ", bytes[i]);
  }
  printf("]");
}



// user-defined static callback function
static int mycallback(unsigned char *  _header,
                      int              _header_valid,
                      unsigned char *  _payload,
                      unsigned int     _payload_len,
                      int              _payload_valid,
                      framesyncstats_s _stats,
                      void *           _userdata)
{
  int nwrite = 0;

  if (_header_valid)
  {
    DisplayPacket((unsigned char *) _payload, _payload_len);

    //printf("%s\n",(char *) _userdata );
    nwrite = write(tun_tx_fd, _payload, _payload_len);
    if (nwrite < 0) 
    {

      perror("Writing to interface");
      close(tun_tx_fd);
      exit(1);
    } 
  }
  else
  {
    printf("  header (%s)\n", _header_valid ? "valid" : "INVALID");
    printf("  payload (%s)\n", _payload_valid ? "valid" : "INVALID");
  }
  return 0;
/*
//    printf("***** callback invoked!\n");
//    printf("  header (%s)\n",  _header_valid  ? "valid" : "INVALID");
//    printf("  payload (%s)\n", _payload_valid ? "valid" : "INVALID");
//    printf("  payload length (%d)\n", _payload_len);
//
//    // type-cast, de-reference, and increment frame counter
   unsigned int * counter = (unsigned int *) _userdata;
    (*counter)++;
//    framesyncstats_print(&_stats);

	if ( _header_valid  )
	{
//		printf("Packet %u contains (%s) with RSSI %5.5f\n", *counter, _payload, _stats.rssi);

	unsigned int n = 6;
	unsigned char payload[PAYLOAD_LENGTH]={0};
    snprintf((char * )payload, 7, "Packet");
    unsigned int num_bit_errors = count_bit_errors_array(payload, _payload, n);
    printf("[%u]: (%s):  %3u / %3u\tRSSI=(%5.5f)\n", *counter, _payload, num_bit_errors, n*8, _stats.rssi);
	}
    return 0;
*/
}


int process_samples(int16_t * samples, unsigned int sample_length) {
	int status=0;
	float complex * y = convert_sc16q11_to_comlexfloat(samples, sample_length);
	if ( y != NULL )
	{
		for (int i=0; i<=sample_length; i=i+32)
			flexframesync_execute(fs, &y[i], 32);
		free(y);
	}
	else
	{
		status = BLADERF_ERR_MEM;
	}
    return status;
}

/* Usage:
 *   libbladeRF_example_boilerplate [serial #]
 *
 * If a serial number is supplied, the program will attempt to open the
 * device with the provided serial number.
 *
 * Otherwise, the first available device will be used.
 */

int main(int argc, char *argv[])
{

    char name1 [10]={0};
    name1[0]= 't';
    name1[1]= 'u';
    name1[2]= 'n';
    name1[3]= '1';

   


    int status;
    struct module_config config_rx;
    struct module_config config_tx;
    struct bladerf *devrx = NULL;
    struct bladerf_devinfo dev_info;
    unsigned int frame_counter = 0;

    bladerf_init_devinfo(&dev_info);

    if (argc >= 2) {
        strncpy(dev_info.serial, argv[1], sizeof(dev_info.serial) - 1);
    }
    status = bladerf_open_with_devinfo(&devrx, &dev_info);
    if (status != 0) {
        fprintf(stderr, "Unable to open device: %s\n", bladerf_strerror(status));
        return 1;
    }
    fprintf(stdout, "bladerf_open_with_devinfo: %s\n", bladerf_strerror(status));


    //    hostedx115-latest.rbf
    status = bladerf_load_fpga(devrx,  "hostedx115-latest.rbf");
    if (status != 0) {
        fprintf(stderr, "Unable to bladerf_load_fpga  device: %s\n", bladerf_strerror(status));
        return status;
    }
    fprintf(stdout, "bladerf_load_fpga: %s\n", bladerf_strerror(status));


    /* Set up RX module parameters */
    config_rx.module     = BLADERF_MODULE_RX;
    config_tx.module     = BLADERF_MODULE_TX;
    config_tx.frequency  = config_rx.frequency  = FREQUENCY_USED;
    config_tx.bandwidth  = config_rx.bandwidth  = BANDWIDTH_USED;
    config_tx.samplerate = config_rx.samplerate = SAMPLING_RATE_USED;
    config_tx.rx_lna     = config_rx.rx_lna     = BLADERF_LNA_GAIN_MID;
    config_tx.vga1       = config_rx.vga1       = 30;
    config_tx.vga2       = config_rx.vga2       = 3;



    tun_tx_fd = tun_alloc(name1, IFF_TUN | IFF_NO_PI);



     status = configure_module(devrx, &config_tx);
      if (status != 0) {
          fprintf(stderr, "Failed to configure RX module. Exiting.\n");
          goto out;
      }
      fprintf(stdout, "configure_module: %s\n", bladerf_strerror(status));
      status = configure_module(devrx, &config_rx);
       if (status != 0) {
           fprintf(stderr, "Failed to configure RX module. Exiting.\n");
           goto out;
       }
       fprintf(stdout, "configure_module: %s\n", bladerf_strerror(status));

    /* Initialize synch interface on RX and TX modules */
    status = init_sync_tx(devrx);
    if (status != 0) {
        goto out;
    }

    /* Initialize synch interface on RX and TX modules */
    status = init_sync_rx(devrx);
    if (status != 0) {
        goto out;
    }


    status = calibrate(devrx);
    if (status != 0) {
        fprintf(stderr, "Failed to calibrate RX module. Exiting.\n");
        goto out;
    }
    fprintf(stdout, "calibrate: %s\n", bladerf_strerror(status));


    fs = flexframesync_create(mycallback, (void*)&frame_counter);
    if ( fs==NULL)
    {
        fprintf(stderr, "Failed to framesync64_create. Exiting.\n");
        goto out;
    }
    flexframesync_print(fs);

    status =  sync_rx(devrx, &process_samples);
    if (status != 0) {
            fprintf(stderr, "Failed to sync_rx(). Exiting. %s\n", bladerf_strerror(status));
            goto out;
    }

out:
    bladerf_close(devrx);
    fprintf(stderr, "RX STATUS: %u,  %s\n", status, bladerf_strerror(status));
    return status;
}

void DisplayPacket(unsigned char* buff, int size) {

  printf("Okay...");

  printf("Source IP : %d:%d:%d:%d\n", (int)buff[12], (int)buff[13], (int)buff[14], (int)buff[15]);

  printf("Dest IP : %d:%d:%d:%d\n", (int)buff[16], (int)buff[17], (int)buff[18], (int)buff[19]);
  printf("Source port : %d\n", (int)((buff[20] << 8)+buff[21]));
  printf("Dest port : %d\n", (int)((buff[22] << 8)+buff[23]));

  printf("payload : %s \n\n",&buff[28]);
}