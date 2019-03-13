/*
Name: Chanel Young
UID: 604914166
EMAIL: chanelyoung99@gmail.com
*/
/*
Using poll: https://linux.die.net/man/3/poll
printing the string correctly: https://stackoverflow.com/questions/8345581/c-printf-a-float-value

*/

#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <poll.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>
#include <fcntl.h>
#include <stdlib.h>
#include <ctype.h>
#include <mraa.h>
#include <aio.h>

int log_flag = 0; 
int logfd; 
char unit = 'F'; //fahrenheit by default
int period = 1; 
int stopped = 0; 

double readingToTemp(int reading, char unit){
	//copied these lines from Alexander Tiard's slides
	double r = 1023.0/reading - 1.0; 
	r = 100000*r; 
	int B = 4275; 
	double temp = 1.0/(log(r/100000.0)/B + 1/298.15) - 273.15;
	if(unit == 'F')
		temp = temp*1.8 + 32; 
	return temp; 
}
void run_command(const char* command){
	if(strcmp(command, "START") == 0){
		stopped = 0; 
		if(log_flag){
			dprintf(logfd, "START\n"); 
		}
	}
	else if(strcmp(command, "STOP") == 0){
		stopped = 1; 
		if(log_flag){
			dprintf(logfd, "STOP\n");
		}
	}
	else if(strcmp(command, "SCALE=F") == 0){
		unit = 'C'; 
		if(log_flag){
			dprintf(logfd, "SCALE=C\n"); 
		}
	}
	else if(strcmp(command, "SCALE=C") == 0){
		unit = 'C'; 
		if(log_flag){
			dprintf(logfd, "SCALE=C\n"); 
		}
	}
	else if(strcmp(command, "OFF") == 0){
		fprintf(stdout, "OFF\n"); 
		if(log_flag){
			dprintf(logfd, "OFF\n"); 
		}
		time_t raw_time;
	    struct tm* cur_time;
	    time(&raw_time);
	    cur_time = localtime(&raw_time);
	    fprintf(stdout, "%.2d:%.2d:%.2d SHUTDOWN\n", cur_time->tm_hour, cur_time->tm_min, cur_time->tm_sec);
	    if(log_flag) {
	        dprintf(logfd, "%.2d:%.2d:%.2d SHUTDOWN\n", cur_time->tm_hour, cur_time->tm_min, cur_time->tm_sec);
	    }
	    exit(0);
	}
	else if(strncmp(command, "PERIOD=", 7) == 0){
		period = (int)atoi(command + 7);
		if(log_flag && !stopped)
			dprintf(logfd, "%s\n", command);
	}
	else if(strncmp(command, "LOG", 3) == 0){
		if(log_flag){
			dprintf(logfd, "%s\n", command); 
		}
	}
	else{
		fprintf(stderr, "Error, unrecognized command %s", command);
		exit(1); 
	}
		

}
int main(int argc, char* argv[]){
	static struct option long_options[] =
    {
      {"SCALE", required_argument, 0, 's'},
      {"PERIOD", required_argument, 0, 'p'},
      {"LOG", required_argument, 0, 'l'},
      {0, 0, 0, 0}
    };
    int arg; 
    while((arg = getopt_long(argc, argv, "", long_options, NULL)) != -1){
	    switch (arg)
	      {
	      case 's':
	        if(optarg[0] == 'F' || optarg[0] == 'C'){
	        	unit = optarg[0];
	        }
	        else{
	        	fprintf(stderr, "Error, unit must be F or C\n");
	        	exit(1); 
	        }
	        break;
	      case 'p':
	        period = atoi(optarg);
	        break;
	      case 'l':
	        log_flag = 1;
	        logfd = creat(optarg, 0666); 
	        if(logfd < 0){
	        	fprintf(stderr, "Error opening file %s\n", optarg);
	        	exit(1); 
	        }
	        break;
	      case '?':
	        fprintf(stderr, "Invalid argument %s", optarg);
	        exit(1);
	      }
  	}
  	//button setup
  	mraa_gpio_context button; 
  	button = mraa_gpio_init(62);
  	if(button == NULL){
  		fprintf(stderr, "Error initializing button\n");
  	}
  	mraa_gpio_dir(button, MRAA_GPIO_IN);
  	//sensor setup
  	mraa_aio_context sensor; 
  	sensor = mraa_aio_init(1); 
  	if(sensor == NULL){
  		fprintf(stderr, "Error initializing sensor\n");
  	}
  	//polling setup
  	struct pollfd fds[1];

  	fds[0].fd = STDIN_FILENO; 
  	fds[0].events = POLLIN | POLLHUP | POLLERR;

  	int reading; 
  	struct timeval clk; 
  	time_t next = 0;
  	struct tm* current_time;
  	char input[100]; 
  	while(1){
  		reading =  mraa_aio_read(sensor); 
  		double temp = readingToTemp(reading, unit); 
		gettimeofday(&clk, 0); 
		if(!stopped && clk.tv_sec >= next){
			current_time = localtime(&clk.tv_sec); 
			fprintf(stdout, "%02d:%02d:%02d %.1f\n", current_time->tm_hour, current_time->tm_min, current_time->tm_sec, temp);
			if(log_flag)
				dprintf(logfd,  "%02d:%02d:%02d %.1f\n", current_time->tm_hour, current_time->tm_min, current_time->tm_sec, temp);
			next = clk.tv_sec + period; 
		}

		int error = poll(fds, 1, 0); 
		if(error == -1){
			fprintf(stderr, "Error getting poll value\n"); 
			exit(1); 
		}

		if(fds[0].revents & POLLIN){
			int bytes_read = read(STDIN_FILENO, input, 100); 
			if(bytes_read == -1){
				fprintf(stderr, "Error reading from stdin\n"); 
				exit(1); 
			}
			int i = 0; 
			int cur_index = 0; 
			char cur_cmd[128]; 
			for(; i <  bytes_read && cur_index < 100; i++){
				if(input[i] == '\n'){ //end of current command
					run_command((char*)&cur_cmd);
					cur_index = 0; 
					memset(cur_cmd, 0, 100); 
				}
				else{
					cur_cmd[cur_index] = input[i]; 
					cur_index++; 
				}
			}
		}
  	}
  	mraa_aio_close(sensor); 
  	mraa_gpio_close(button); \
	close(logfd);
  	exit(0);
}
