/*
 *  sctpclnt.c
 *
 *  SCTP multi-stream client.
 *
 *  M. Tim Jones <mtj@mtjones.com>
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include "common.h"
#include <sys/time.h>

#define BUFSIZE 1024*1024

int main()
{
  int connSock, in, i, ret, flags;
  struct sockaddr_in servaddr;
  struct sctp_status status;
  struct sctp_sndrcvinfo sndrcvinfo;
  struct sctp_event_subscribe events;
  struct sctp_initmsg initmsg;
  //char buffer[MAX_BUFFER+1];
  char buf[BUFSIZE];
  struct timeval s, e;
 
  /* Create an SCTP TCP-Style Socket */
  connSock = socket( AF_INET, SOCK_STREAM, IPPROTO_SCTP );

  /* Specify that a maximum of 5 streams will be available per socket */
  memset( &initmsg, 0, sizeof(initmsg) );
  initmsg.sinit_num_ostreams = 9;
  initmsg.sinit_max_instreams = 9;
  initmsg.sinit_max_attempts = 4;
  ret = setsockopt( connSock, IPPROTO_SCTP, SCTP_INITMSG,
                     &initmsg, sizeof(initmsg) );

  /* Specify the peer endpoint to which we'll connect */
  bzero( (void *)&servaddr, sizeof(servaddr) );
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(8000);
  servaddr.sin_addr.s_addr = inet_addr( "192.168.211.138" );

  /* Connect to the server */
  ret = connect( connSock, (struct sockaddr *)&servaddr, sizeof(servaddr) );

  /* Enable receipt of SCTP Snd/Rcv Data via sctp_recvmsg */
  memset( (void *)&events, 0, sizeof(events) );
  events.sctp_data_io_event = 1;
  ret = setsockopt( connSock, SOL_SCTP, SCTP_EVENTS,
                     (const void *)&events, sizeof(events) );

  /* Read and emit the status of the Socket (optional step) */
  in = sizeof(status);
  ret = getsockopt( connSock, SOL_SCTP, SCTP_STATUS,
                     (void *)&status, (socklen_t *)&in );

  printf("assoc id  = %d\n", status.sstat_assoc_id );
  printf("state     = %d\n", status.sstat_state );
  printf("instrms   = %d\n", status.sstat_instrms );
  printf("outstrms  = %d\n", status.sstat_outstrms );

  /* Expect two messages from the peer */


  gettimeofday(&s, NULL);


  while(1){

    in = sctp_recvmsg( connSock, (void *)buf, sizeof(buf),
                        (struct sockaddr *)NULL, 0, &sndrcvinfo, &flags );
	printf("in = %d\n",in);

    if (in > 0) {
	buf[in] = 0;
	//printf("data:\n%s",buf);
    }else{
	break;
    }

 }


  gettimeofday(&e, NULL);
  printf("time = %lf\n", (e.tv_sec - s.tv_sec) + (e.tv_usec - s.tv_usec)*1.0E-6);


  /* Close our socket and exit */
  close(connSock);

  return 0;
}

