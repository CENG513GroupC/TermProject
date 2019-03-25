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

#include <sys/types.h>

#include <stdio.h>

//Tuntap library...
#include "tuntap.h"


/* The RX and TX modules are configured independently for these parameters */
    unsigned int samples_len = SAMPLE_SET_SIZE;
    int16_t *tx_samples;
    struct bladerf *devtx;
    int tun_rx_fd;

//For tun0 device
struct device *dev;


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




	//Initialization of "tun0" device
	//It sets a random name but later we will change it to "tun0"
	//It is a tun device so it will transfer the IP packets without containing ethernet header...
	dev = tuntap_init();
	if (tuntap_start(dev, TUNTAP_MODE_TUNNEL, TUNTAP_ID_ANY) == -1) {
		return 1;
	}
	
	//We are correcting the device name, dev-> "tun0"
	tuntap_set_ifname(dev, "tun0");
	
	//Ip allocation for "tun0", 
	if (tuntap_set_ip(dev, "10.0.0.1", 24) == -1) {
		return 1;
	}
	
	//We are setting tun0 device running mode...
	//They can be seen via "ifconfig" command...
	if (tuntap_up(dev) == -1) {
		return 1;
	}
	
	
    //tun_rx_fd = tun_alloc(name, IFF_TUN | IFF_NO_PI); -- old one...
    

   
    

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
            
            
            
            //nread=read(tun_rx_fd,payload,buf_len); -- old one
            //Read the "tun0" device...	
			nread = tuntap_read(dev, payload, buf_len);
            
            
            
            if (nread<=0)
            {
                perror("Reading from interface");
               	tuntap_destroy(dev);

                
                //close(tun_rx_fd); -- old one
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
    
          tuntap_destroy(dev);

    return status;
}
