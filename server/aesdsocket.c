#include <sys/types.h>
#include <sys/socket.h>
#include <sys/socket.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

bool caught_sigint = false;
bool caught_sigterm = false;

static void signal_handler (int signal_number)
{
	if ( signal_number == SIGINT )
	{
		caught_sigint = true;
	}
	else if( signal_number == SIGTERM )
	{
		caught_sigterm = true;
	}
}

int main(int argc, char *argv[])
{
   int daemon_mode = 0;

   if (argc == 2 && strcmp(argv[1], "-d") == 0)
   {
       daemon_mode = 1;
   }

   if (daemon_mode)
   {
       pid_t pid = fork();

       if (pid < 0)
       {
           perror("fork");
           return -1;
       }

       if (pid > 0)
       {
           /* Parent exits */
           return 0;
       }

       /* Child continues */
       if (setsid() < 0)
       {
           perror("setsid");
           return -1;
       }

       chdir("/");
   }

  struct sigaction new_action;
	
  int sockfd, newsocket, len, i, valread;
  struct sockaddr_in tcp_server_addr;
  struct sockaddr_in tcp_client_addr;
  struct sockaddr_storage server_storage;
  socklen_t addr_size, client_addr_size;
  int opt=1;

  sockfd = socket(PF_INET, SOCK_STREAM, 0); 
  if(sockfd == -1) 
  { 
	  printf("socket error !\n"); 
	  return -1; 
  }

  if (setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt)) < 0)
  {
	perror("setsockopt");
	close(sockfd);
	return -1;
  }

  tcp_server_addr.sin_family = AF_INET;
  tcp_server_addr.sin_port = htons(9000); // server port
  tcp_server_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // local-host

  memset(tcp_server_addr.sin_zero, '\0', sizeof tcp_server_addr.sin_zero);

  bind(sockfd, (struct sockaddr *) &tcp_server_addr, sizeof(tcp_server_addr)); 
  addr_size = sizeof server_storage;

  if (listen(sockfd, 10) < 0)
  {
      perror("In listen");
      return -1;
  }

  int addrlen = sizeof(tcp_server_addr);

  FILE *file = fopen("/var/tmp/aesdsocketdata", "w");

  if (file)
  {
      fclose(file);
  }

  while(caught_sigint == false && caught_sigterm == false)
  {
    if ((newsocket = accept(sockfd, (struct sockaddr *)&tcp_server_addr, (socklen_t*)&addrlen))<0)
    {
        perror("In accept");
        return -1;
    }
    
    file = fopen("/var/tmp/aesdsocketdata", "a+");
    
    size_t n;
    size_t total_size = 0;

    char *buffer = NULL;
    char temp[1024];

    printf("stratin receiving\n");

    while ((valread = read(newsocket, temp, sizeof(temp))) > 0)
    {
            char *new_buf = realloc(buffer, total_size + valread);
            if (!new_buf)
            {
                free(buffer);
                perror("realloc");
                return -1;
            }
	    

            buffer = new_buf;

            memcpy(buffer + total_size, temp, valread);	
	    
	    total_size = total_size + valread;


            if (memchr(buffer, '\n', total_size))
   	    {
              break;
   	    }

    }

    fwrite(buffer, 1, total_size, file);
    fflush(file);
    
    fseek(file, 0, SEEK_SET);
    
    while (( n = fread(buffer, 1, sizeof(buffer), file)) > 0) 
    {
    	write(newsocket, buffer, n);
    }

    free(buffer);    
    fclose(file);
    close(newsocket);
  }

  close(sockfd);

  return 0;
}
