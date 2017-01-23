/*
 *  sctpsrvr.c
 *
 *  SCTP multi-stream server.
 *
 *  M. Tim Jones <mtj@mtjones.com>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include "common.h"

// get file data size
long get_file_size(const char *file[]){
        fpos_t fsize;
        long sz = 0;

        FILE *fp = fopen(file,"rb");

        fseek(fp,0,SEEK_END);
//      fgetpos(fp,&fsize); 
        sz = ftell(fp);

        printf("FILESIZE = %lo\n",sz);

        fclose(fp);

        return sz;
}


int main()
{
  int listenSock, connSock, ret;
  struct sockaddr_in servaddr;
  struct sctp_initmsg initmsg;
  char buffer[MAX_BUFFER+1];
  time_t currentTime;

  FILE *fp;
  char *file = "./sample2.txt";
  long flen = 0;

// get file data into the buffer
//--------------------------------------
  fp = fopen(file,"r");
  if(!fp){
      printf("file open err \n");
      return 0;
  }

  // get file size
  flen = get_file_size(file);
  printf("%s=%d\n",file,flen);

  // crate buffer
  char buf[flen+1];
  memset(buf,'\0',sizeof(buf));

  // file data into buffer
  fread(buf,flen,1,fp);

//  printf("%s",buf);
//---------------------------------------


  /* Create SCTP TCP-Style Socket */
  listenSock = socket( AF_INET, SOCK_STREAM, IPPROTO_SCTP );

  /* Accept connections from any interface */
  bzero( (void *)&servaddr, sizeof(servaddr) );
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl( INADDR_ANY );
  servaddr.sin_port = htons(8000);

  ret = bind( listenSock, (struct sockaddr *)&servaddr, sizeof(servaddr) );

  /* Specify that a maximum of 5 streams will be available per socket */
  memset( &initmsg, 0, sizeof(initmsg) );
  initmsg.sinit_num_ostreams = 9;
  initmsg.sinit_max_instreams = 9;
  initmsg.sinit_max_attempts = 4;
  ret = setsockopt( listenSock, IPPROTO_SCTP, SCTP_INITMSG, 
                     &initmsg, sizeof(initmsg) );

  /* Place the server socket into the listening state */
  listen( listenSock, 5 );

  /* Server loop... */
  while( 1 ) {

    /* Await a new client connection */
    printf("Awaiting a new connection\n");
    connSock = accept( listenSock, (struct sockaddr *)NULL, (int *)NULL );

    /* New client socket has connected */

    /* Grab the current time */
    currentTime = time(NULL);

    /* Send local time on stream 0 (local time stream) */
//    snprintf( buffer, MAX_BUFFER, "%s\n", ctime(&currentTime) );
//    ret = sctp_sendmsg( connSock, (void *)buffer, (size_t)strlen(buffer),
//                         NULL, 0, 0, 0, LOCALTIME_STREAM, 0, 0 );
//    printf("ret = %d\n",ret);

  int i;
  for(i=0;i<9;i++){

    /* Send GMT on stream 1 (GMT stream) */
    snprintf( buffer, MAX_BUFFER, "%s\n", asctime( gmtime( &currentTime ) ) );
    ret = sctp_sendmsg( connSock, (void *)buf, (size_t)flen,
                         NULL, 0, 0, 0, i/*GMT_STREAM*/, 0, 0 );

    printf("ret = %d\n",ret);

  }

    /* Close the client connection */
    close( connSock );

  }

  return 0;
}

