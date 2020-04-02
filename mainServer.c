/* 
 * tiny.c - a minimal HTTP server that serves static and
 *          dynamic content with the GET method. Neither 
 *          robust, secure, nor modular. Use for instructional
 *          purposes only.
 *          
 * 		Semo Capstone Experience Firewall
 * 		Group Members: Eshter Ahmed, Zihang Zhang, Jihao Deng, Katherine Summerfield	
 *          
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/types.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUFSIZE 1024
#define MAXERRS 16
int exit_thread = 1;
extern char **environ; /* the environment */
typedef struct data
{
	/* variables for connection management */
	int parentfd;          /* parent socket */
	int childfd;           /* child socket */
	int portno;            /* port to listen on */
	int clientlen;         /* byte size of client's address */
	struct hostent *hostp; /* client host info */
	char *hostaddrp;       /* dotted decimal host addr string */
	int optval;            /* flag value for setsockopt */
	struct sockaddr_in serveraddr; /* server's addr */
	struct sockaddr_in clientaddr; /* client addr */

	/* variables for connection I/O */
	FILE *stream;          /* stream version of childfd */
	char buf[BUFSIZE];     /* message buffer */
	char method[BUFSIZE];  /* request method */
	char uri[BUFSIZE];     /* request uri */
	char version[BUFSIZE]; /* request method */
	char filename[BUFSIZE];/* path derived from uri */
	char filetype[BUFSIZE];/* path derived from uri */
	char cgiargs[BUFSIZE]; /* cgi argument list */
	char *p;               /* temporary pointer */
	int is_static;         /* static request? */
	struct stat sbuf;      /* file status */
	int fd;                /* static content filedes */
	int pid;               /* process id from fork */
	int wait_status;       /* status from wait */
}data_struct;

struct args {
    struct sockaddr_in clientaddr;
    int childfd;
    int requestno;
};
void *thread_1(void *);
/*
 * error - wrapper for perror used for bad syscalls
 */
void error(char *msg) {
  perror(msg);
  exit(1);
}
#define MAX 10000

struct node {
	char addr[30];
	int port_num;
};

/* 
 * create 3 lists because we are going to use Hash Table.
 * Hash Table key is 3.
*/

//blacklist
struct node b1_list[MAX];	
struct node b2_list[MAX];	
struct node b3_list[MAX];	
//whitelist
struct node w1_list[MAX];	
struct node w2_list[MAX];	
struct node w3_list[MAX];	

//count of nodes in every list
int bc1=0;
int bc2=0;
int bc3=0;
int wc1=0;
int wc2=0;
int wc3=0;

/***********************************************************************************/
//  this function from hach.c
//  didn't change content 


// Calculate hash value by string 
int cal_ipaddr_hash(char* ipaddr)
{
	int sum = 0;
	int i = 0;
	while (ipaddr[i])
	{
		sum += (int)ipaddr[i];
		i ++;
	}
	return sum;
}


/***********************************************************************************/
//  this function from hach.c
//  didn't change content 

// insert nodes to black&white list to use Hash Table
void black_add(struct node x)
{
	int hash_value = cal_ipaddr_hash(x.addr);
	switch(hash_value%3)
	{
		case 0:
			bc1++;
			b1_list[bc1-1].port_num=x.port_num;
			strcpy(b1_list[bc1-1].addr, x.addr);
			break;
		case 1:
			bc2++;
			b2_list[bc2-1].port_num=x.port_num;
			strcpy(b2_list[bc2-1].addr, x.addr);
			break;
		case 2:
			bc3++;
			b3_list[bc3-1].port_num=x.port_num;
			strcpy(b3_list[bc3-1].addr, x.addr);
			break;
		default:
			break;
	}
}

/***********************************************************************************/
//  

void white_add(struct node x)
{
	int hash_value = cal_ipaddr_hash(x.addr);
	switch(hash_value%3)
	{
		case 0:
			wc1++;
			w1_list[wc1-1].port_num=x.port_num;
			strcpy(w1_list[wc1-1].addr, x.addr);
			break;
		case 1:
			wc2++;
			w2_list[wc2-1].port_num=x.port_num;
			strcpy(w2_list[wc2-1].addr, x.addr);
			break;
		case 2:
			wc3++;
			w3_list[wc3-1].port_num=x.port_num;
			strcpy(w3_list[wc3-1].addr, x.addr);
			break;
		default:
			break;
	}
}



// Sorting of Hash_Table
void hashsort_blacklist()
{
	for(int i1=0; i1<bc1-1; i1++)
	{
		for(int j1=i1; j1<bc1; j1++)
		{
			struct node t;
			if (strcmp(b1_list[i1].addr, b1_list[j1].addr) > 0)
			{
				t=b1_list[i1];
				b1_list[i1]=b1_list[j1];
				b1_list[j1]=t;
			}
		}
	}
	for(int i2=0; i2<bc2-1; i2++)
	{
		for(int j2=i2; j2<bc2; j2++)
		{
			struct node t;
			if (strcmp(b2_list[i2].addr, b2_list[j2].addr) > 0)
			{
				t=b2_list[i2];
				b2_list[i2]=b2_list[j2];
				b2_list[j2]=t;
			}
		}
	}
	for(int i3=0; i3<bc3-1; i3++)
	{
		for(int j3=i3; j3<bc3; j3++)
		{
			struct node t;
			if (strcmp(b3_list[i3].addr, b3_list[j3].addr) > 0)
			{
				t=b3_list[i3];
				b3_list[i3]=b3_list[j3];
				b3_list[j3]=t;
			}
		}
	}
}
/***********************************************************************************/

void hashsort_whitelist()
{
	for(int i1=0; i1<wc1-1; i1++)
	{
		for(int j1=i1; j1<wc1; j1++)
		{
			struct node t;
			if (strcmp(w1_list[i1].addr, w1_list[j1].addr) > 0)
			{
				t=w1_list[i1];
				w1_list[i1]=w1_list[j1];
				w1_list[j1]=t;
			}
		}
	}
	for(int i2=0; i2<wc2-1; i2++)
	{
		for(int j2=i2; j2<wc2; j2++)
		{
			struct node t;
			if (strcmp(w2_list[i2].addr, w2_list[j2].addr) > 0)
			{
				t=w2_list[i2];
				w2_list[i2]=w2_list[j2];
				w2_list[j2]=t;
			}
		}
	}
	for(int i3=0; i3<wc3-1; i3++)
	{
		for(int j3=i3; j3<wc3; j3++)
		{
			struct node t;
			if (strcmp(w3_list[i3].addr, w3_list[j3].addr) > 0)
			{
				t=w3_list[i3];
				w3_list[i3]=w3_list[j3];
				w3_list[j3]=t;
			}
		}
	}
}

/***********************************************************************************/
// Decide whether port number is in the black or white list
// Using Hash Search Algorithm
int isInB(char* addr)
{
	int hash_value = cal_ipaddr_hash(addr);
	switch(hash_value%3)
	{
		case 0:
			for(int i=0; i<bc1; i++)
			{
				if(strcmp(b1_list[i].addr, addr) > 0)
					return 0;
				if(strcmp(b1_list[i].addr, addr) == 0)
					return 1;
			}
			break;
		case 1:
			for(int i=0; i<bc2; i++)
			{
				if(strcmp(b2_list[i].addr, addr) > 0)
					return 0;
				if(strcmp(b2_list[i].addr, addr) == 0)
					return 1;
			}
			break;
		case 2:
			for(int i=0; i<bc3; i++)
			{
				if(strcmp(b3_list[i].addr, addr) > 0)
					return 0;
				if(strcmp(b3_list[i].addr, addr) == 0)
					return 1;
			}
			break;
	}
	return 0;
}

/***********************************************************************************/


int isInW(char* addr)
{
	int hash_value = cal_ipaddr_hash(addr);
	switch(hash_value%3)
	{
		case 0:
			for(int i=0; i<wc1; i++)
			{
				if(strcmp(w1_list[i].addr, addr) > 0)
					return 0;
				if(strcmp(w1_list[i].addr, addr) == 0)
					return 1;
			}
			break;
		case 1:
			for(int i=0; i<wc2; i++)
			{
				if(strcmp(w2_list[i].addr, addr) > 0)
					return 0;
				if(strcmp(w2_list[i].addr, addr) == 0)
					return 1;
			}
			break;
		case 2:
			for(int i=0; i<wc3; i++)
			{
				if(strcmp(w3_list[i].addr, addr) > 0)
					return 0;
				if(strcmp(w3_list[i].addr, addr) == 0)
					return 1;
			}
			break;
	}
	return 0;
}
/***********************************************************************************/
//  this function copy from hach. and add   fflush(stream); in below 

/*
 * clienterror - returns an error message to the client
 */
void clienterror(FILE *stream, char *cause, char *errnum,
		char *shortmsg, char *longmsg) {
	fprintf(stream, "HTTP/1.1 %s %s\n", errnum, shortmsg);
	fprintf(stream, "Content-type: text/html\n");
	fprintf(stream, "\n");
	fprintf(stream, "<html><title>Tiny Error</title>");
	fprintf(stream, "<body bgcolor=""ffffff"">\n");
	fprintf(stream, "%s: %s\n", errnum, shortmsg);
	fprintf(stream, "<p>%s: %s\n", longmsg, cause);
	fprintf(stream, "<hr><em>The Tiny Web server</em>\n");
    fflush(stream);
}

int main(int argc, char **argv) {

  /* variables for connection management */
  int parentfd;          /* parent socket */
  int childfd;           /* child socket */
  int portno;            /* port to listen on */
  int clientlen;         /* byte size of client's address */
  struct hostent *hostp; /* client host info */
  char *hostaddrp;       /* dotted decimal host addr string */
  int optval;            /* flag value for setsockopt */
  struct sockaddr_in serveraddr; /* server's addr */
  struct sockaddr_in clientaddr; /* client addr */

  /* variables for connection I/O */
  FILE *stream;          /* stream version of childfd */
  char buf[BUFSIZE];     /* message buffer */
  char method[BUFSIZE];  /* request method */
  char uri[BUFSIZE];     /* request uri */
  char version[BUFSIZE]; /* request method */
  char filename[BUFSIZE];/* path derived from uri */
  char filetype[BUFSIZE];/* path derived from uri */
  char cgiargs[BUFSIZE]; /* cgi argument list */
  char *p;               /* temporary pointer */
  int is_static;         /* static request? */
  struct stat sbuf;      /* file status */
  int fd;                /* static content filedes */
  int pid;               /* process id from fork */
  int wait_status;       /* status from wait */

  // checking if checksums.txt exists yet or not
  char *textFile = "checksums.txt";
  if(!access(textFile, F_OK)){
	printf("%s was found\n", textFile);
  }else{ // if not found, create it
	printf("%s was not found\n", textFile);
	printf("Creating %s\n", textFile);
	system("cat > checksums.txt");
 	// get hash values for all supported file types in the server's directory
	system("md5sum *.html *.jpg *.gif > checksums.txt");
  }
  
  // run script that checks for and adds new files that have not yet been added to checksums.txt and also deletes files that are no longer in the directory
  system("bash newFiles.sh");

  printf("Verifying Files...\n");
  // check hash values for changes
  system("md5sum -c checksums.txt");
  printf("\n");

/***********************************************************************************/
// this section from hash.c. this is use for create blackList and whitelist from csv

	struct node temp;
	char IP_addr[60]; // account number
	char PortNumber[60]; // account name
    FILE* cfPtr, *cfPtr1;
	if ((cfPtr = fopen("blackList.csv", "r")) == NULL) {
		puts("blackList.csv could not be opened");
	} 
	else {
		while (!feof(cfPtr))
		{
			if (fscanf(cfPtr,"%s %s", IP_addr, PortNumber) != 2)
				break;
			temp.port_num=atoi(PortNumber);
			strcpy(temp.addr,IP_addr);
			black_add(temp);
		} 
		fclose(cfPtr); // fclose closes file   
	}
	hashsort_blacklist();
	
	if ((cfPtr1 = fopen("whiteList.csv", "r")) == NULL) {
		puts("whiteList.csv could not be opened");
	} 
	else {
		while (!feof(cfPtr1))
		{
			if (fscanf(cfPtr1,"%s %s", IP_addr, PortNumber) != 2)
				break; 
			temp.port_num=atoi(PortNumber);
			strcpy(temp.addr,IP_addr);
			white_add(temp);
		} 
		fclose(cfPtr1); // fclose closes file   
	}
	hashsort_whitelist();

/***********************************************************************************/

  /* check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }
  portno = atoi(argv[1]);

  /* open socket descriptor */
  parentfd = socket(AF_INET, SOCK_STREAM, 0);
  if (parentfd < 0) 
    error("ERROR opening socket");

  /* allows us to restart server immediately */
  optval = 1;
  setsockopt(parentfd, SOL_SOCKET, SO_REUSEADDR, 
	     (const void *)&optval , sizeof(int));

  /* bind port to socket */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)portno);
  if (bind(parentfd, (struct sockaddr *) &serveraddr, 
	   sizeof(serveraddr)) < 0) 
    error("ERROR on binding");

  /* get us ready to accept connection requests */
  if (listen(parentfd, 5) < 0) /* allow 5 requests to queue up */ 
    error("ERROR on listen");

  /* 
   * main loop: wait for a connection request, parse HTTP,
   * serve requested content, close connection.
   */
  clientlen = sizeof(clientaddr);

/***********************************************************************************/
//   create theread for eac request
  struct args *ARGS = (struct args *)malloc(sizeof(struct args)); 
  ARGS->requestno = 0;
  while (1) {

    /* wait for a connection request */
    childfd = accept(parentfd, (struct sockaddr *) &clientaddr, &clientlen);
    ARGS->childfd = childfd;
    ARGS->clientaddr = clientaddr;
    ARGS->requestno = ARGS->requestno + 1;
    pthread_t t1;
    pthread_create(&t1, NULL, thread_1, (void *)ARGS);
	pthread_join(t1, NULL);

  }
  /***********************************************************************************/
}

void *thread_1(void *arg)
{
	/***********************************************************************************/
	// pass accepted request into new thread to process request

    FILE *stream;          /* stream version of connfd */
	char buf[BUFSIZE];     /* message buffer */
	char method[BUFSIZE];  /* request method */
	char uri[BUFSIZE];     /* request uri */
	char version[BUFSIZE]; /* request method */
	char filename[BUFSIZE];/* path derived from uri */
	char filetype[BUFSIZE];/* path derived from uri */
	char cgiargs[BUFSIZE]; /* cgi argument list */
	char *p;               /* temporary pointer */
	int is_static;         /* static request? */
	struct stat sbuf;      /* file status */
	int fd;                /* static content filedes */
	int pid;               /* process id from fork */
    struct hostent *hostp; /* client host info */
    char *hostaddrp;       /* dotted decimal host addr string */


    struct args *data = arg;
	if (data->childfd < 0) 
      error("ERROR on accept");
    
    /* determine who sent the message */
    hostp = gethostbyaddr((const char *)&data->clientaddr.sin_addr.s_addr, 
			  sizeof(data->clientaddr.sin_addr.s_addr), AF_INET);
    if (hostp == NULL)
      error("ERROR on gethostbyaddr");
    hostaddrp = inet_ntoa(data->clientaddr.sin_addr);
    if (hostaddrp == NULL)
      error("ERROR on inet_ntoa\n");
 
	/* Display client IP address and port number*/
    printf("Client IP Address is %s\n", hostaddrp);
    printf("Client port is: %d\n", (int) ntohs(data->clientaddr.sin_port));


	/***********************************************************************************/
	// this section from hash.c
	// for check block list and whihte list
    if (isInB(hostaddrp)==1) {
			if(isInW(hostaddrp)==0)
			{
				printf("Blocked IP addr : %s!\n", hostaddrp);
        	    close(data->childfd);
                pthread_exit(&exit_thread);
			}
		}

	/***********************************************************************************/

	/***********************************************************************************/
	// this section   from tinyHttpServerKS.c
	// processing request
	// I didn't change this request prosessing methology 
	

    /* open the child socket descriptor as a stream */
    if ((stream = fdopen(data->childfd, "r+")) == NULL)
      error("ERROR on fdopen");

    /* get the HTTP request line */
    fgets(buf, BUFSIZE, stream);
    printf("%s", buf);
    sscanf(buf, "%s %s %s\n", method, uri, version);
    /* tiny only supports the GET method */
    if (strcasecmp(method, "GET")) {
      clienterror(stream, method, "501", "Not Implemented", 
	     "Tiny does not implement this method");
      fclose(stream);
      close(data->childfd);
	pthread_exit(&exit_thread);
    }

    /* read (and ignore) the HTTP headers */
    fgets(buf, BUFSIZE, stream);
    printf("%s", buf);
    while(strcmp(buf, "\r\n")) {
      fgets(buf, BUFSIZE, stream);
      printf("%s", buf);
    }

    /* parse the uri [crufty] */
    if (!strstr(uri, "cgi-bin")) { /* static content */
      is_static = 1;
      strcpy(cgiargs, "");
      strcpy(filename, ".");
      strcat(filename, uri);
      if (uri[strlen(uri)-1] == '/') 
	strcat(filename, "index.html");
    }
    else { /* dynamic content */
      is_static = 0;
      p = index(uri, '?');
      if (p) {
	strcpy(cgiargs, p+1);
	*p = '\0';
      }
      else {
	strcpy(cgiargs, "");
      }
      strcpy(filename, ".");
      strcat(filename, uri);
    }

    /* make sure the file exists */
    if (stat(filename, &sbuf) < 0) {
      clienterror(stream, filename, "404", "Not found", 
	     "Tiny couldn't find this file");
      fclose(stream);
      close(data->childfd);
	pthread_exit(&exit_thread);
    }

    /* serve static content */
    if (is_static) {
      if (strstr(filename, ".html"))
	strcpy(filetype, "text/html");
      else if (strstr(filename, ".gif"))
	strcpy(filetype, "image/gif");
      else if (strstr(filename, ".jpg"))
	strcpy(filetype, "image/jpg");
      else 
	strcpy(filetype, "text/plain");

      /* print response header */
      fprintf(stream, "HTTP/1.1 200 OK\n");
      fprintf(stream, "Server: Tiny Web Server\n");
      fprintf(stream, "Content-length: %d\n", (int)sbuf.st_size);
      fprintf(stream, "Content-type: %s\n", filetype);
      fprintf(stream, "\r\n"); 
      fflush(stream);

      /* Use mmap to return arbitrary-sized response body */
      fd = open(filename, O_RDONLY);
      p = mmap(0, sbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
      fwrite(p, 1, sbuf.st_size, stream);
      munmap(p, sbuf.st_size);
    }

    /* serve dynamic content */
    else {
      /* make sure file is a regular executable file */
      if (!(S_IFREG & sbuf.st_mode) || !(S_IXUSR & sbuf.st_mode)) {
	clienterror(stream, filename, "403", "Forbidden", 
	       "You are not allow to access this item");
      fclose(stream);
      close(data->childfd);
	pthread_exit(&exit_thread);
      }

      /* a real server would set other CGI environ vars as well*/
      setenv("QUERY_STRING", cgiargs, 1); 

      /* print first part of response header */
      sprintf(buf, "HTTP/1.1 200 OK\n");
      write(data->childfd, buf, strlen(buf));
      sprintf(buf, "Server: Tiny Web Server\n");
      write(data->childfd, buf, strlen(buf));

      /* create and run the child CGI process so that all child
         output to stdout and stderr goes back to the client via the
         childfd socket descriptor */
      pid = fork();
      if (pid < 0) {
	perror("ERROR in fork");
	exit(1);
      }
      else if (pid > 0) { /* parent process */
	wait(NULL);
      }
      else { /* child  process*/
	close(0); /* close stdin */
	dup2(data->childfd, 1); /* map socket to stdout */
	dup2(data->childfd, 2); /* map socket to stderr */
	if (execve(filename, NULL, environ) < 0) {
	  perror("ERROR in execve");
	}
      }
    }

    // notification of failed checksums
    system("md5sum --quiet --check checksums.txt");
    system("bash newFiles.sh");
    /* clean up */
    fclose(stream);
    close(data->childfd);
/***********************************************************************************/

}
