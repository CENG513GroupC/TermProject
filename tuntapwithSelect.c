
/*
 * 25/03/2019
 * 
 * 
 * Group C 
 * Barış Can Çam - Burak - Onur - Saim
 * 
 * 
 * A simple tun implementaion
 * This program will interact with the socket programs
 * Tun devices are sufficient for us to talk with sockets, we are not using any tap device because they will also inclue the ethernet header...
 * We are going to transfer only IP packets....
 * 
 * 
 * Code retrieves file descriptor of the created tun devices' "tun0" and "tun1", all the tun devices must be created and IP adresses must be allocated
 * and set to up mode before running this code...
 * 
 *   
 * constantly listens on this tun devices file descriptors with "select"
 * if a readable data occurs on "tun0" reads it and sends it to "tun1" device
 * if a readable data occurs on "tun1" reads it and sends it to "tun0" device...
 * 
 * 
 * */








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
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <errno.h>
#include <stdarg.h>



//The function that open a created tun device
//The tun devices are created via command line parameters...
//This code piece was taken from an example about the tun/tap interface...
int tun_alloc(char *dev, int flags) {

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


int main()
{
	
//tun0 tun device name	
char name [10]={0};
name[0]= 't';
name[1]= 'u';
name[2]= 'n';
name[3]= '0';

//tun1 tun device name
char name1 [10]={0};
name1[0]= 't';
name1[1]= 'u';
name1[2]= 'n';
name1[3]= '1';

//We are retrieving the tun device's file descriptors...
int file = tun_alloc(name, IFF_TUN | IFF_NO_PI);
int file2 = tun_alloc(name1, IFF_TUN | IFF_NO_PI);



//The size of the buffer for transmitting data, 1500 is the MTU size...

#define SIZE 1500
//The buffer to store transfer data...

unsigned char buff [SIZE] = {0};
//To keep how many bytes has been read or send...

int count = 0;



//for keeping the max numbered file number for select...	

int max_file_id = 0;


//We want select to only listen for incoming data...
fd_set read_file_desc;
//Select function will modify the descriptor passed, to keep the original unchanged we will assing the original to this var while using select function...

fd_set temp_desc; 
//Empty the read list...

FD_ZERO(&read_file_desc);
FD_ZERO(&temp_desc);

//Set the file descriptors of the devices for select function
//and keep the maximum file descriptor ID for select function
FD_SET(file, &read_file_desc);
if(file > max_file_id) max_file_id = file;
FD_SET(file2, &read_file_desc);
if(file2 > max_file_id) max_file_id = file2;



while(1)
{
	//Select will modify the var passed to it, so to keep the original unchanged we are assigning it to temp variable...	

	temp_desc = read_file_desc;
if (select(max_file_id+1, &temp_desc, NULL, NULL, NULL) == -1) {
            perror("select error, closing program");
            close(file);
            close(file2);
            exit(1);
  }
  
  
  //When a readable occurs on any device's file descriptor...  

//Walk among the file descriptors... 
for(int i = 0 ; i <= max_file_id+1; i++)
{
		//If a readable data occurs...

	if (FD_ISSET(i, &temp_desc)) 
	{
		//If the data is on the "tun0" device's file descriptor...

		if (i == file) 
		{
			printf("Tun 0 data received ....\n\n\n");

		//Empty the previous garbage data...

		memset(buff, 0, SIZE);
		//Read the "tun0" device...	

		count = read(file, buff, SIZE);
		if(count < 0)
		{
			perror("Reading from interface");
      close(file);
      close(file2);
      exit(1);

	}
	
		//Display the IP packet information to user...

		printf("Okay...");

		printf("Source IP : %d:%d:%d:%d\n", (int)buff[12], (int)buff[13], (int)buff[14], (int)buff[15]);

		printf("Dest IP : %d:%d:%d:%d\n", (int)buff[16], (int)buff[17], (int)buff[18], (int)buff[19]);
		printf("Source port : %d\n", (int)((buff[20] << 8)+buff[21]));
		printf("Dest port : %d\n", (int)((buff[22] << 8)+buff[23]));

		//Send the read from "tun0" to "tun1"

		count = write(file2, buff, count);
		printf("%d bytes written to Tun 1 interface\n", count);
	
	//Display the carried data on IP packet....

	printf("len : %d - Buff : %s\n",count, &buff[28]);		
					
		}
		//If the data is on "tun1"

		else if (i == file2) 
		{
			printf("Tun 1 data received ....\n\n\n");
		memset(buff, 0, SIZE);
		count = read(file2, buff, SIZE);
		if(count < 0)
		{
			perror("Reading from interface");
      close(file);
      close(file2);
      exit(1);

	}
		printf("Okay...");

		printf("Source IP : %d:%d:%d:%d\n", (int)buff[12], (int)buff[13], (int)buff[14], (int)buff[15]);

		printf("Dest IP : %d:%d:%d:%d\n", (int)buff[16], (int)buff[17], (int)buff[18], (int)buff[19]);
		printf("Source port : %d\n", (int)((buff[20] << 8)+buff[21]));
		printf("Dest port : %d\n", (int)((buff[22] << 8)+buff[23]));


		count = write(file, buff, count);
		printf("%d bytes written to Tun 0\n", count);
	printf("len : %d - Buff : %s\n",count, &buff[28]);		
					
		}
		
		
				
	}//if FD_SET
	

	
	
}//for i





}//while(1)

return 0;


}//main
