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
 * Code creates two tun devices "tun0" and "tun1"
 * sets ip address 10.0.0.1/24 to "tun0", 10.0.0.2/24 to "tun1"
 * set all creates devices run mode. When chechek with "ifconfig" command, the created tun devices could be seen...
 *  
 * constantly listens on this tun devices file descriptors with "select"
 * if a readable data occurs on "tun0" reads it and sends it to "tun1" device
 * if a readable data occurs on "tun1" reads it and sends it to "tun0" device...
 * 
 * 
 * */
#include <sys/types.h>

#include <stdio.h>

#include "tuntap.h"


//The size of the buffer for transmitting data, 1500 is the MTU size...
#define SIZE 1500
//The buffer to store transfer data...
unsigned char buff [SIZE] = {0};

//To keep how many bytes has been read or send...
int count;


int
main(void) {
	
	//Required device definitions, dev for "tun0", dev2 for "tun1"
	struct device *dev, *dev2;

	//Initialization of "tun0" device
	//It sets a random name but later we will change it to "tun0"
	//It is a tun device so it will transfer the IP packets without containing ethernet header...
	dev = tuntap_init();
	if (tuntap_start(dev, TUNTAP_MODE_TUNNEL, TUNTAP_ID_ANY) == -1) {
		return 1;
	}
	
	
	//Initialization of "tun1" device
	//It sets a random name but later we will change it to "tun1"
	//It is a tun device so it will transfer the IP packets without containing ethernet header...

	dev2 = tuntap_init();
	if (tuntap_start(dev2, TUNTAP_MODE_TUNNEL, TUNTAP_ID_ANY) == -1) {
		return 1;
	}


	//We are correcting the device names, dev-> "tun0", dev2->"tun1"
	tuntap_set_ifname(dev, "tun0");
	tuntap_set_ifname(dev2, "tun1");

	//Ip allocation for "tun0", 
	if (tuntap_set_ip(dev, "10.0.0.1", 24) == -1) {
		return 1;
	}
	
	//Ip allocation for "tun1"
	if (tuntap_set_ip(dev2, "10.0.0.2", 24) == -1) {
		return 1;
	}

	
	//We are setting tun devices running mode...
	//They can be seen via "ifconfig" command...
	if (tuntap_up(dev) == -1) {
		return 1;
	}
	
	if (tuntap_up(dev2) == -1) {
		return 1;
	}
	

//We are getting file descriptors of the tun devices for select function....
int file = tuntap_get_fd(dev);
int file2 = tuntap_get_fd(dev2);
	
	
	
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
	count = tuntap_read(dev, buff, SIZE);
		if(count < 0)
		{
			perror("Reading from tun0 interface");
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
		count = tuntap_write(dev2, buff, count);

		printf("%d bytes written to Tun 1 interface\n", count);
	
	//Display the carried data on IP packet....
	printf("len : %d - Buff : %s\n",count, &buff[28]);		
					
		}
		
		//If the data is on "tun1"
		else if (i == file2) 
		{
			
			
			printf("Tun 1 data received ....\n\n\n");
		//Erase the previous garbage data...
		//Read from "tun1" device
		memset(buff, 0, SIZE);
		count = tuntap_read(dev2, buff, SIZE);
		if(count < 0)
		{
			perror("Reading from interface");
      close(file);
      close(file2);
      exit(1);

	}
	
	
	//Display the read IP packet...
	
	
		printf("Okay...");

		printf("Source IP : %d:%d:%d:%d\n", (int)buff[12], (int)buff[13], (int)buff[14], (int)buff[15]);

		printf("Dest IP : %d:%d:%d:%d\n", (int)buff[16], (int)buff[17], (int)buff[18], (int)buff[19]);
		printf("Source port : %d\n", (int)((buff[20] << 8)+buff[21]));
		printf("Dest port : %d\n", (int)((buff[22] << 8)+buff[23]));


		count = tuntap_write(dev, buff, count);
		printf("%d bytes written to Tun 0\n", count);
	printf("len : %d - Buff : %s\n",count, &buff[28]);		
					
		}
		
		
				
	}//if FD_SET
	

	
	
}//for i





}//while(1)
	
	
	


	tuntap_destroy(dev);
	return 0;
}
