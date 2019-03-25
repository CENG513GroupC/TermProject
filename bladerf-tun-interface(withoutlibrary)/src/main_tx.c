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


/* The RX and TX modules are configured independently for these parameters */
    unsigned int samples_len = SAMPLE_SET_SIZE;
    int16_t *tx_samples;
    struct bladerf *devtx;
    int tun_rx_fd;
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
  /* if the operation was successful, write back the name of the
   * interface to the variable "dev", so the caller can know
   * it. Note that the caller MUST reserve space in *dev (see calling
   * code below) */
  strcpy(dev, ifr.ifr_name);

  /* this is the special file descriptor that the caller will use to talk
   * with the virtual interface */
  return fd;
}

int main(int argc, char *argv[])
{
    int status;
    struct module_config config_rx;
    struct module_config config_tx;
    struct bladerf_devinfo dev_info;
    unsigned char header[8];        // data header
    unsigned char payload[PAYLOAD_LENGTH]={0};      // data payload
    float complex y[BUFFER_SIZE];          // frame samples
    unsigned int  buf_len = PAYLOAD_LENGTH;
    float complex buf[buf_len];
    unsigned int i;
    unsigned int symbol_len;
    int frame_complete = 0;
    int lastpos = 0;
    int cnt=0;
    flexframegenprops_s ffp;
    int nread;

    char name [10]={0};
    name[0]= 't';
    name[1]= 'u';
    name[2]= 'n';
    name[3]= '0';


  


    bladerf_init_devinfo(&dev_info);

    if (argc >= 2) {
        strncpy(dev_info.serial, argv[1], sizeof(dev_info.serial) - 1);
    }
    status = bladerf_open_with_devinfo(&devtx, &dev_info);

    if (status != 0) {
        fprintf(stderr, "Unable to open device: %s\n", bladerf_strerror(status));
        return 1;
    }
    fprintf(stdout, "Device Serial: %s\tbladerf_open_with_devinfo: %s\n", dev_info.serial, bladerf_strerror(status));

//    hostedx115-latest.rbf
    bladerf_load_fpga(devtx, "hostedx115-latest.rbf");

    /* Set up RX module parameters */
    config_rx.module     = BLADERF_MODULE_RX;
    config_tx.module     = BLADERF_MODULE_TX;
    config_tx.frequency  = config_rx.frequency  = FREQUENCY_USED;
    config_tx.bandwidth  = config_rx.bandwidth  = BANDWIDTH_USED;
    config_tx.samplerate = config_rx.samplerate = SAMPLING_RATE_USED;
    config_tx.rx_lna     = config_rx.rx_lna     = BLADERF_LNA_GAIN_MID;
    config_tx.vga1       = config_rx.vga1       = 10;
    config_tx.vga2       = config_rx.vga2       = 0;

    tun_rx_fd = tun_alloc(name, IFF_TUN | IFF_NO_PI); 
    

   
    

    status = configure_module(devtx, &config_tx);
    if (status != 0) {
        fprintf(stderr, "Failed to configure RX module. Exiting.\n");
        return status;
    }
    fprintf(stdout, "configure_module: %s\n", bladerf_strerror(status));


    /* Initialize synch interface on RX and TX modules */
    status = init_sync_tx(devtx);
    if (status != 0) {
    	fprintf(stderr, "init_sync_tx. Exiting.\n");
    	return status;
    }
    fprintf(stdout, "init_sync_tx: %s\n", bladerf_strerror(status));


    status = calibrate(devtx);
    if (status != 0) {
        fprintf(stderr, "Failed to calibrate RX module. Exiting.\n");
        return status;
    }
    fprintf(stdout, "calibrate: %s\n", bladerf_strerror(status));


    flexframegenprops_init_default(&ffp);
	//ffp.check = false;
    ffp.fec0 = LIQUID_FEC_NONE;
    ffp.fec1 = LIQUID_FEC_NONE;
    ffp.mod_scheme = LIQUID_MODEM_QAM4;

    flexframegen fg = flexframegen_create(&ffp);
    flexframegen_print(fg);

// INIT HEADER
    for (i=0; i<8; i++)
        header[i] = i;

    while(status == 0 )
    {
    		cnt ++;

			memset(payload, 0x00, PAYLOAD_LENGTH);
			//sprintf((char*)payload,"abcdef (%d)",cnt);
			//memset(&payload[13], 0x00, PAYLOAD_LENGTH-13);
            nread=read(tun_rx_fd,payload,buf_len);
            if (nread<=0)
            {
                perror("Reading from interface");
                close(tun_rx_fd);
                exit(1);
            }

		for(int a_ = 0; a_ < 10 ; a_++)
		{

			flexframegen_assemble(fg, header, payload, PAYLOAD_LENGTH);
			symbol_len = flexframegen_getframelen(fg);

			frame_complete = 0;
    		lastpos = 0;
    		while (!frame_complete) {
            	frame_complete = flexframegen_write_samples(fg, buf, buf_len);
            	memcpy(&y[lastpos], buf, buf_len*sizeof(float complex));
            	lastpos = lastpos + buf_len;
        	}
        	printf("number of samples %u %u\n", symbol_len, lastpos);
        	samples_len=symbol_len;
        	tx_samples = convert_comlexfloat_to_sc16q11( y, symbol_len );
        	if (tx_samples == NULL) {
        		fprintf(stdout, "malloc error: %s\n", bladerf_strerror(status));
        		return BLADERF_ERR_MEM;
        	}

        	status =  sync_tx(devtx, tx_samples, samples_len);
			if (status != 0) {
				fprintf(stderr, "Failed to sync_tx(). Exiting. %s\n", bladerf_strerror(status));
				goto out;
			}
		}
			
			fprintf(stdout, "Packet %d transmitted\n", cnt);
			//usleep(10000);
    }


out:
    bladerf_close(devtx);
    flexframegen_destroy(fg);
    fprintf(stderr, "TX STATUS: %u,  %s\n", status, bladerf_strerror(status));
    return status;
}
