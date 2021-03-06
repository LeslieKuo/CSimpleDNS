/*
 * simple dns server
 */
/* server_udp.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <unistd.h> 
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include "dns.h"
#include "hash.h"

/* defines */
#define DEFAULT_PORT 53
#define MAX_EVENTS   10
#define MAX_FDS      10
#define HASH_SIZE    1024
#define LINE_MAXLEN 256
#define DOMAIN_MAXLEN 64
#define IP_MAXLEN  16 
#define FALSE 0
#define TRUE 1

/* global varibles */
static hash_t g_ht;

/* function */
static int do_it(int sSocket);
static int process_std_query(const char *rev_buf, const unsigned int len, char *send_buf, unsigned int *plen);

static struct query_seg *construct_query_seg(const unsigned int nlen);
static void destroy_query_seg(struct query_seg *qr);
static unsigned int get_dotname_len(const char *buf);
static int get_query_name(const char *buf, char *pname);
static unsigned int get_next_query_seg(const char *buf, struct query_seg **pqs);

static int socket_init(void);
static int init_db(void);
static char* GetOneLine(FILE* fp);
static int IsNameValid(const char *name);

int main(int argc, char** argv)
{
    int sSocket = 0;
    /* use epoll */
    struct epoll_event ev;
    struct epoll_event events[MAX_EVENTS];
    int epollfd;
    int nfds;
    int n;
    int i;
    
    
    printf("Server waiting...\n");

    sSocket = socket_init();
    if (sSocket== -1)
    {
        return -1;
    }

    /* USE epoll  */
    epollfd = epoll_create(MAX_FDS);
    if (epollfd == -1)
    {
        perror("epoll_create");
        return -1;
    }
    
    ev.events = EPOLLIN;
    ev.data.fd = sSocket;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, sSocket,&ev) == -1)
    {
        perror("epoll_ctl");
        return -1;
    }

    init_db();

    for(;;)
    {
        nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
        if (nfds <= 0)
        {
			continue;
        }

        for (n = 0; n < nfds; ++n)
        {
    
            if (events[n].data.fd == sSocket)
            {
		        if (do_it(sSocket) != 0)
                {
                    break;
                }
            }
	    }
    }

    hash_destroy(&g_ht);
    close(sSocket);
    
    return 0;
}

/*create UDP socket*/
static int socket_init()
{
    int sPort = DEFAULT_PORT;
    int sSocket = 0;
    struct sockaddr_in ser;

    sSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if(sSocket < 0)
    {
        perror("socket");
        return -1;
    }
    
    ser.sin_family = AF_INET;
    ser.sin_port = htons(sPort);
    ser.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if(bind(sSocket, (struct sockaddr*)&ser, sizeof(ser)) < 0)
    {
        perror("bind");
        return -1; 
    }
    
    return sSocket;
}


static int init_db()
{
	/*read data from /etc/host, and insert data into hash table*/
	FILE* fp = NULL;
	char acCmd[LINE_MAXLEN] = {0};
	char *p;
	char acName[DOMAIN_MAXLEN] = {0};
	char acIP[IP_MAXLEN] = {0};
	struct in_addr strIPAddr;

    hash_init(&g_ht, HASH_SIZE);

	fp = fopen("/etc/hosts", "r");
	if (NULL == fp)
		return -1;

	while ((p = GetOneLine(fp)) != NULL)
	{
		//skip comment lines
		if ((*p == ' ') || (*p == '#'))
		{
			free(p);
			continue;
		}

        /*  do not support alias name */
		sscanf(p, "%s%s", acIP, acName);

		if ((IsNameValid(acName)) && (0 != inet_aton(acIP, &strIPAddr)))
		{
			if (HASH_OK != hash_insert(&g_ht, acName, strIPAddr.s_addr))
            {
				printf("\nPut %s %d into Hash table failed.\n", acName, strIPAddr.s_addr);
            }

		}

		free(p);
	}	

	fclose(fp);

}

static int do_it(int sSocket)
{
    int sRecv = 0;
    int sLen = 0;
    int sSend = 0;
    char recv_buf[4096] = {0};
    struct sockaddr_in cli;
    char send_buf[4096]={0};
    unsigned int sendlen =0;

    
    sLen = sizeof(cli);
    memset(recv_buf, 0, sizeof(recv_buf));

    sRecv = recvfrom(sSocket, recv_buf, 4096, 0, (struct sockaddr*)&cli, (unsigned int*)&sLen);
    if((sRecv < 0)||(sRecv == 0))
    {
        perror("recvfrom");
		return -1;
    }
    else
    {
        printf("recvfrom(): [%s]\n", recv_buf);
        printf("recvfrom() client IP: [%s]\n", inet_ntoa(cli.sin_addr));
        printf("recvfrom() client PORT: [%d]\n", ntohs(cli.sin_port));
		/*process standard query*/
        process_std_query(recv_buf, sRecv, send_buf, &sendlen);
    }

	/*send response packet to client*/
    sSend = sendto(sSocket, send_buf, sendlen, 0, (struct sockaddr*)&cli, sizeof(cli));
    if((sSend < 0)||(sSend == 0))
    {
        perror("perror");
        return -1;
    }
    printf("sendto() Succeeded!\n");

    return 0;
}

/* process standard query  */
int process_std_query(const char *recv_buf, const unsigned int len, char *send_buf, unsigned int *plen)
{
    struct dnshead *h = (struct dnshead *)recv_buf;
    struct dnshead *sh = (struct dnshead *)send_buf;
    char *name = NULL;
    char *querys= (char*)(recv_buf + sizeof(struct dnshead));
    char *answer= (char*)(send_buf + len);
    struct query_seg *pqs = NULL;
    unsigned int rlen = 0;
    unsigned int totallen = 0;
    unsigned int n = 0;
    int data = 0;

    if (!recv_buf || !send_buf || len<sizeof(struct dnshead))
    {
        return -1;
    }

    /* check message type */
    if (GET_OP_TYPE(ntohs(h->flags)) != QUERY ||
        GET_OP_CODE(ntohs(h->flags)) != OPCODE_STD_QUERY)
    {
        return -1;
    }

    if (ntohs(h->questions) == 0 || 
        ntohs(h->answer_rrs) != 0 ||
        ntohs(h->authority_rrs) != 0 ||
        ntohs(h->additional_rrs) != 0)
    {
        return -1;
    }

    if (!plen)
        return -1;
    *(plen) = 0;

    /*copy query message */
    memcpy(send_buf, recv_buf, len);
    sh->flags = ntohs(sh->flags);
    SET_OP_TYPE(sh->flags, RESPONSE); /*modify to response  message*/
    SET_OP_CODE(sh->flags, OPCODE_STD_QUERY);
    sh->flags = htons(sh->flags);

    /* look up IP address according to domain name in query message*/
	for (n = 0; n < ntohs(h->questions); n++)
    {
        rlen = get_next_query_seg(querys, &pqs);
        totallen += rlen;
        if (totallen > len-sizeof(struct dnshead) || rlen == 0)
        {
            destroy_query_seg(pqs);
            return -1;
        }

        /* check query type, just support A type & internet class query */
        if (pqs->type != TYPE_A || pqs->class != CLASS_IN)
        {
            destroy_query_seg(pqs);
            return -1;
        }

        /* get hash value from hash table, and assemble response message */
        data = hash_lookup(&g_ht, pqs->name);
        if (data == HASH_FAIL)
        {
            continue;
        }

        printf("find data = %d\n, sizeof(data) = %d\n",data, sizeof(unsigned long));

        sh->answer_rrs += 1; /* answer  number increase*/		
        
        memcpy(answer, querys, rlen);  /* copy  name, type and class*/
        *(unsigned long*)(answer+rlen) = htonl(1800); /* ttl */
        *(unsigned short*)(answer+rlen+sizeof(data)) = htons(sizeof(data)); /*data length*/
        *(unsigned long*)(answer+rlen+sizeof(data)+sizeof(unsigned short)) = (data); /*data*/

        /*  increase answer and query pointer */
        querys += rlen;
        answer += rlen + 2*sizeof(data) + sizeof(unsigned short);

        *(plen) = answer - send_buf;

        destroy_query_seg(pqs);
    }
    sh->answer_rrs = htons(sh->answer_rrs);     

	return 0;
}

/* generate query_seg from buffer*/
 static unsigned int get_next_query_seg(const char *buf, struct query_seg **ppqs)
 {
	 const char *tmp = buf;
	 unsigned int pos = 0;
	 unsigned int nlen = 0;
	 struct query_seg *pqs = NULL;
		 
	 nlen = get_dotname_len(buf);
	 if (nlen == 0)
	 {
		 return 0;
	 }
 
	 pqs = construct_query_seg(nlen);
	 if (!pqs)
	 {
		 return 0;
	 }
 
	 /* get query name */
	 if (get_query_name(buf, pqs->name) == -1)
	 {
		 destroy_query_seg(pqs);
		 return 0;
	 }
 
	 pqs->type = ntohs(*(unsigned short*)(tmp + strlen(buf)+1));
	 pqs->class = ntohs(*(unsigned short*)(tmp + strlen(buf)+1+sizeof(unsigned short)));
 
	 *ppqs = pqs;
	 return strlen(buf)+1+2*sizeof(unsigned short);
 
 }
 
 static struct query_seg *construct_query_seg(const unsigned int nlen)
 {
	 struct query_seg *pqs = NULL;
	 pqs = malloc(sizeof(struct query_seg));
	 if (!pqs)
	 {
		 return NULL;
	 }
 
	 pqs->name = NULL;
	 pqs->name = (char*)malloc(nlen+1);
	 if (pqs->name == NULL)
	 {
		 free(pqs);
		 return NULL;
	 }
	 memset(pqs->name, 0, nlen+1);
 
	 return pqs;
 }
 
 /* free query */
 static void destroy_query_seg(struct query_seg *qr)
 {
	 free(qr->name);
	 free(qr);
 }
 
 /* return length of name 
  * length of x.x.com 
  * */
 static unsigned int get_dotname_len(const char *buf)
 {
	 return strlen(buf)-1;
 }
 
 
 /* get query name*/
 static int get_query_name(const char *buf, char *pname)
 {
	 char offset = *buf;
	 char tmp = 0;
 
	 strcpy(pname, buf+1);
	 while(offset <= strlen(pname))
	 {
		 tmp += pname[offset] + 1;
		 if (pname[offset] == '\0')
			 break;
		 pname[offset] = '.';		 
		 offset += tmp;
	 }
 }
 
 
 /*Letters in domain name must be in the range below:
	 a--z, A--Z, 0--9, . */
 static int IsNameValid(const char *name)
 {
	 char *c = name;
 
	 if (!name)
		 return FALSE;
 
	 if (strlen(name) == 0)
		 return FALSE;
 
	 do
	 {
		 if ((*c>='a' && *c<='z') || (*c >= 'A' && *c <= 'Z') ||
			 (*c>='0' && *c<='9') || (*c == '.') || (*c == '-'))
		 {
			 c++;
			 continue;
		 }
		 else
			 return FALSE;
 
	 }while(*c != '\0');
 
	 return TRUE;
 }
 
 
 /*Get one line from host file. 
	 Note:the invoker is responsible for free of memory*/
 static char* GetOneLine(FILE* fp)
 {
	 int c,n;
	 int size = LINE_MAXLEN;
	 char* buf;
	 char* p;
 
	 n = 0;
	 buf = (char*)malloc(size * sizeof(char));
 
	 if (NULL == buf)
		 return NULL;
	 else
	 {
		 p = buf;
		 while ((c = fgetc(fp)) != EOF)
		 {
			 if ('\n' == c)
				 break;
 
			 *p++ = c;			 
			 n++;
		 }
		 *p = '\0';
 
		 if ((EOF == c) && (0 == n))
			 return NULL;
		 else
			 return buf;
	 }
 
 }
 
 
 

