/*
 *format defination of protocol message
 */

struct dnshead
{
    unsigned short transaction_id;
    unsigned short flags;
    unsigned short questions;
    unsigned short answer_rrs;
    unsigned short authority_rrs; /* not support */
    unsigned short additional_rrs; /* not support */
};

/* defination of flags field */
/* X... .... .... .... */
#define QUERY    0
#define RESPONSE 1
#define GET_OP_TYPE(flags) (flags>>15)
#define SET_OP_TYPE(flags, bit)   (flags |= bit<<15)
/* .XXX X... .... .... */
#define OPCODE_STD_QUERY 0 /* standard query */
                           
#define GET_OP_CODE(flags) ((flags & 0x7800)>>11)
#define SET_OP_CODE(flags, code) (flags |= (code<<11))
/* .... .X.. .... .... */
#define ANTH_ANSWER
/* .... ..X. .... .... */ 
#define TRUNCATED_NO 0   /* message truncated */
#define TRUNCATED_YES 1 
/* .... ...X .... .... */
#define RECURSION_NEED 1  /* need recursion */
/* .... .... X... .... */
#define RECURSION_YES 1   /* recursion */
/* .... .... .XXX .... */

/* .... .... .... XXXX */ 
#define NO_ERROR   0      /* no error */
#define QUERY_ERROR 1     /*  */
#define SERVER_UNAVAILABLE 2 /* server unavailable */
#define NAME_NO_EXISTS     3 /* name not exist */
#define GET_RETCODE(flags) (flags & 0x000F)


/* query message */
struct query_seg
{
    char *name;
    unsigned short type;
    unsigned short class;
};


/* response message */
struct answer_seg 
{
    char *name;
    unsigned short type;
    unsigned short class;
    unsigned long  ttl;
    unsigned short dlen;
    unsigned char appdata[1];
};

/* quey type */
#define TYPE_A   (0x01) /* A type: get IP address of designated domain name */
#define TYPE_NS  (0x02) 

#define CLASS_IN  (0x01) /* internet class */

