#include <sys/types.h>
#include <sys/socket.h>

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <syslog.h>

#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

volatile sig_atomic_t caught_sigint = 0;
volatile sig_atomic_t caught_sigterm = 0;

static void signal_handler (int signal_number)
{
	if ( signal_number == SIGINT )
	{
		caught_sigint = 1;
	}
	else if( signal_number == SIGTERM )
	{
		caught_sigterm = 1;
	}
}

int main(int argc, char *argv[])
{

  int daemon_mode = 0;

   openlog("aesdsocket", LOG_PID, LOG_USER);

   if (argc == 2 && strcmp(argv[1], "-d") == 0)
   {
       daemon_mode = 1;
   }
	
  int sockfd, newsocket, valread;
  struct sockaddr_in tcp_server_addr;
  struct sockaddr_in tcp_client_addr;

  int opt=1;

  sockfd = socket(PF_INET, SOCK_STREAM, 0); 
  if(sockfd == -1) 
  { 
	  perror("socket error !"); 
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
  tcp_server_addr.sin_addr.s_addr = inet_addr("0.0.0.0"); 

  memset(tcp_server_addr.sin_zero, '\0', sizeof tcp_server_addr.sin_zero);

  if (bind(sockfd, (struct sockaddr *)&tcp_server_addr, sizeof(tcp_server_addr)) < 0)
  {
    perror("bind");
    close(sockfd);
    return -1;
  }


  if (listen(sockfd, 10) < 0)
  {
      perror("In listen");
      return -1;
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

  int fd = open("/dev/null", O_RDWR);
  
  if (fd == -1)
  {
      return -1;
  }
  
  dup2(fd, STDIN_FILENO);
  dup2(fd, STDOUT_FILENO);
  dup2(fd, STDERR_FILENO);
  
  if (fd > STDERR_FILENO)
  {
      close(fd);
  }

     chdir("/");
  }

  socklen_t addrlen = sizeof(tcp_client_addr);

  FILE *file = fopen("/var/tmp/aesdsocketdata", "w");

  if (file)
  {
      fclose(file);
  }
 
  struct sigaction sa;

  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = signal_handler;
  
  sigemptyset(&sa.sa_mask);
  
  if (sigaction(SIGINT, &sa, NULL) < 0)
  {
      perror("SIGINT received!");
      return -1;
  }
  
  if (sigaction(SIGTERM, &sa, NULL) < 0)
  {
      perror("SIGTERM received!");
      return -1;
  }

  while(caught_sigint == 0 && caught_sigterm == 0)
  {
    addrlen = sizeof(tcp_client_addr);

    if ((newsocket = accept(sockfd, (struct sockaddr *)&tcp_client_addr, (socklen_t*)&addrlen))<0)
    {
	if (errno == EINTR)
	  break;

        perror("In accept");
        return -1;
    }

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(tcp_client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);

    syslog(LOG_INFO, "Accepted connection from %s", client_ip);
    
    file = fopen("/var/tmp/aesdsocketdata", "a+");
    if (!file)
    {
      close(newsocket);
      continue;
    }

    size_t n;
    size_t total_size = 0;

    char *buffer = NULL;
    char temp[1024];

    while ((valread = read(newsocket, temp, sizeof(temp))) > 0)
    {
            char *new_buf = realloc(buffer, total_size + valread);

	    if (!new_buf)
            {
                syslog(LOG_ERR, "realloc failed");
                goto cleanup;
            }
	    
              buffer = new_buf;

              memcpy(buffer + total_size, temp, valread);	
	      
	      total_size = total_size + valread;
	      if (memchr(buffer, '\n', total_size))
              {
                break;
              }

    }
char *newline;

while ((newline = memchr(buffer, '\n', total_size)) != NULL)
{
    size_t packet_size =
        (newline - buffer) + 1;

    fwrite(buffer,
           1,
           packet_size,
           file);

    fflush(file);

    total_size -= packet_size;

    if (total_size > 0)
    {
        memmove(buffer,
                buffer + packet_size,
                total_size);
    }
}
    
    fseek(file, 0, SEEK_SET);
    
   char sendbuf[1024];

   while ((n = fread(sendbuf, 1, sizeof(sendbuf), file)) > 0)
   {
       size_t total_sent = 0;
   
       while (total_sent < n)
       {
           ssize_t rc = send(
                   newsocket,
                   sendbuf + total_sent,
                   n - total_sent,
                   MSG_NOSIGNAL);
   
           if (rc <= 0)
           {
               break;
           }
   
           total_sent += rc;
       }
   } 

   cleanup:
    free(buffer);    
    fclose(file);
    close(newsocket);

   syslog(LOG_INFO, "Closed connection from %s", client_ip);
  }
  syslog(LOG_INFO, "Caught signal, exiting");

  close(sockfd);
  closelog();

  unlink("/var/tmp/aesdsocketdata");
  return 0;
}
