#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <unistd.h>

#define QUEUESIZE 4096
#define BUFFSIZE  2048

unsigned char buffer[BUFFSIZE];
unsigned char buffer2[BUFFSIZE+4];
unsigned char queue[QUEUESIZE];
unsigned int queue_start;     /* FIFO pointers */
unsigned int queue_end;
size_t queue_size;

unsigned int zeroes[8]        = {0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000};
unsigned int ones[8]          = {0x01f, 0x03f, 0x07f, 0x0ff, 0x1ff, 0x3ff, 0x7ff, 0xfff};
unsigned int runs1[8]         = {0x00e, 0x01e, 0x03e, 0x07e, 0x0fe, 0x1fe, 0x3fe, 0x7fe};
unsigned int runs0[8]         = {0x011, 0x021, 0x041, 0x081, 0x101, 0x201, 0x401, 0x801};
unsigned int alternatings0[8] = {0x015, 0x015, 0x055, 0x055, 0x155, 0x155, 0x555, 0x555};
unsigned int alternatings1[8] = {0x00a, 0x02a, 0x02a, 0x0aa, 0x0aa, 0x2aa, 0x2aa, 0xaaa};
unsigned int limits0[8]       = {0x013, 0x033, 0x033, 0x033, 0x133, 0x333, 0x333, 0x333};
unsigned int limits1[8]       = {0x00c, 0x00c, 0x04c, 0x0cc, 0x0cc, 0x0cc, 0x4cc, 0xccc};

unsigned int buffer2_size;

unsigned int current_byte;
unsigned int bits_used_from_byte;
unsigned int got_byte;
int64_t      current_symbol;
unsigned int bits_in_current_symbol;
int outcount;

uint64_t    symbol_mask = 0;
int         symbol_length = 12;
int         fold = 1;
int         word_reverse = 0;
int         byte_reverse = 0;
int         hexmode = 0;
int         tbl_index=0;
uint64_t filebytes;
int pull_12_first = 1;
int pull_12_phase = 1;
int64_t x;
int64_t y;

// Node for the tree structure
typedef struct {
    int pattern;
    int pattern_length;
    int count;
    int link_0;
    int link_1; } t_node;

int line;
char *filename;
int filenumber;


const unsigned char byte_reverse_table[] = {
  0x00,0x80,0x40,0xC0,0x20,0xA0,0x60,0xE0,0x10,0x90,0x50,0xD0,0x30,0xB0,0x70,0xF0, 
  0x08,0x88,0x48,0xC8,0x28,0xA8,0x68,0xE8,0x18,0x98,0x58,0xD8,0x38,0xB8,0x78,0xF8, 
  0x04,0x84,0x44,0xC4,0x24,0xA4,0x64,0xE4,0x14,0x94,0x54,0xD4,0x34,0xB4,0x74,0xF4, 
  0x0C,0x8C,0x4C,0xCC,0x2C,0xAC,0x6C,0xEC,0x1C,0x9C,0x5C,0xDC,0x3C,0xBC,0x7C,0xFC, 
  0x02,0x82,0x42,0xC2,0x22,0xA2,0x62,0xE2,0x12,0x92,0x52,0xD2,0x32,0xB2,0x72,0xF2, 
  0x0A,0x8A,0x4A,0xCA,0x2A,0xAA,0x6A,0xEA,0x1A,0x9A,0x5A,0xDA,0x3A,0xBA,0x7A,0xFA,
  0x06,0x86,0x46,0xC6,0x26,0xA6,0x66,0xE6,0x16,0x96,0x56,0xD6,0x36,0xB6,0x76,0xF6, 
  0x0E,0x8E,0x4E,0xCE,0x2E,0xAE,0x6E,0xEE,0x1E,0x9E,0x5E,0xDE,0x3E,0xBE,0x7E,0xFE,
  0x01,0x81,0x41,0xC1,0x21,0xA1,0x61,0xE1,0x11,0x91,0x51,0xD1,0x31,0xB1,0x71,0xF1,
  0x09,0x89,0x49,0xC9,0x29,0xA9,0x69,0xE9,0x19,0x99,0x59,0xD9,0x39,0xB9,0x79,0xF9, 
  0x05,0x85,0x45,0xC5,0x25,0xA5,0x65,0xE5,0x15,0x95,0x55,0xD5,0x35,0xB5,0x75,0xF5,
  0x0D,0x8D,0x4D,0xCD,0x2D,0xAD,0x6D,0xED,0x1D,0x9D,0x5D,0xDD,0x3D,0xBD,0x7D,0xFD,
  0x03,0x83,0x43,0xC3,0x23,0xA3,0x63,0xE3,0x13,0x93,0x53,0xD3,0x33,0xB3,0x73,0xF3, 
  0x0B,0x8B,0x4B,0xCB,0x2B,0xAB,0x6B,0xEB,0x1B,0x9B,0x5B,0xDB,0x3B,0xBB,0x7B,0xFB,
  0x07,0x87,0x47,0xC7,0x27,0xA7,0x67,0xE7,0x17,0x97,0x57,0xD7,0x37,0xB7,0x77,0xF7, 
  0x0F,0x8F,0x4F,0xCF,0x2F,0xAF,0x6F,0xEF,0x1F,0x9F,0x5F,0xDF,0x3F,0xBF,0x7F,0xFF
};

// 12 total lengths (12 bit patterns) 
// 94 total patterns
int patterns[109]        = { 0,0,1,
                            0,1,2,3,
                            0,1,2,3,4,5,6,7,
                            0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,
                            0,0x01f,0x011,0x00e,0x015,0x00a,0x013,0x00c,
                            0,0x03f,0x021,0x01e,0x015,0x02a,0x033,0x00c,
                            0,0x07f,0x041,0x03e,0x055,0x02a,0x033,0x04c,
                            0,0x0ff,0x081,0x07e,0x055,0x0aa,0x033,0x0cc,
                            0,0x1ff,0x101,0x0fe,0x155,0x0aa,0x133,0x0cc,
                            0,0x3ff,0x201,0x1fe,0x155,0x2aa,0x333,0x0cc,
                            0,0x7ff,0x401,0x3fe,0x555,0x2aa,0x333,0x4cc,
                            0,0xfff,0x801,0x7fe,0x555,0xaaa,0x333,0xccc,
                              0x001,0x001,0x001,0x001,0x001,0x001,0x001,
                              0x01e,0x03e,0x07e,0x0fe,0x1fe,0x3fe,0x7fe};

int pattern_lengths[109] = { 0,1,1,
                            2,2,2,2,
                            3,3,3,3,3,3,3,3,
                            4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
                            5,5,5,5,5,5,5,5,
                            6,6,6,6,6,6,6,6,
                            7,7,7,7,7,7,7,7,
                            8,8,8,8,8,8,8,8,
                            9,9,9,9,9,9,9,9,
                            10,10,10,10,10,10,10,10,
                            11,11,11,11,11,11,11,11,
                            12,12,12,12,12,12,12,12,
                            5,6,7,8,9,10,11,
                            5,6,7,8,9,10,11};

t_node nodes[109];

uint64_t ipow(uint64_t base, uint64_t exp) {
    uint64_t result = 1;
    while (exp) {
        if (exp & 1)
            result *= base;
        exp >>= 1;
        base *= base;
    }
    return result;
}

// Walk the tree incrementing each node we visit.
void update_nodes(int symbol) {
    int bit;
    int node = 0;
    int i;
    for (i=0;i<12;i++) {
        bit = (symbol >> i) & 1;
        if (bit == 1) {
            node = nodes[node].link_1;
            if (node > 0)
                nodes[node].count++;
            else
                return;
        } else {
            node = nodes[node].link_0;
            if (node > 0)
                nodes[node].count++;
            else
                return;
        }
    }
}

void binary_pattern(int d,int length,int fieldwidth, char *s) {
    int i;
    for (i=0;i<(fieldwidth+1);i++) s[i]=0x00;
    for (i=0;i<(fieldwidth-length);i++) s[i]=' ';

    for(i=0;i<(length+1);i++) {
        if (i==length) s[i+fieldwidth-length] = 0x00;
        else if (((d >> i) & 0x1)==1) s[i+fieldwidth-length]='1';
        else s[i+fieldwidth-length]='0';
    }
}

void print_nodes() {
    int i;
    int j;
    char s[16];
    int length;
    int pattern;


    for(length=1;length<5;length++) {
        printf("%d BIT PATTERNS\n",length); 
        for(i=0;i<(ipow(2,length));i++) {
            for(j=1;j<109;j++) {
                if ((pattern_lengths[j]==length) && (nodes[j].pattern==i)) {
                    binary_pattern(nodes[j].pattern, length ,12,s);  // 1
                    printf("%02d node %02d  Pattern: %s  Length: %02d  Count: %d\n",tbl_index, j,s, pattern_lengths[j], nodes[j].count);
                    tbl_index++;
                }
            }
        }
    }
   
    // Lengths 5 through 12 
    for(length=5;length<13;length++) {
        printf("LENGTH %d\n",length);
        for (i=0;i<8;i++) {
            if      (i==0) pattern = zeroes[length-5];
            else if (i==1) pattern = ones[length-5];
            else if (i==2) pattern = runs0[length-5];
            else if (i==3) pattern = runs1[length-5];
            else if (i==4) pattern = alternatings0[length-5];
            else if (i==5) pattern = alternatings1[length-5];
            else if (i==6) pattern = limits0[length-5];
            else if (i==7) pattern = limits1[length-5];

            for(j=1;j<109;j++) {
                if ((nodes[j].pattern_length==length) && (nodes[j].pattern==pattern)) {
                    binary_pattern(nodes[j].pattern, length ,12,s);  // 1
                    printf("%02d node %02d  Pattern: %s  Length: %02d  Count: %d\n",tbl_index, j,s, pattern_lengths[j], nodes[j].count);
                    tbl_index++;
                }
            }
        }
    }
    
}

void print_nodes_csv(int chunkno) {
    int i;
    int j;
    char s[16];
    int length;
    int pattern;

    printf("%d,",chunkno);

    for(length=1;length<5;length++) {
        //printf("%d BIT PATTERNS\n",length); 
        for(i=0;i<(ipow(2,length));i++) {
            for(j=1;j<109;j++) {
                if ((pattern_lengths[j]==length) && (nodes[j].pattern==i)) {
                    binary_pattern(nodes[j].pattern, length ,12,s);  // 1
                    printf("%d,",nodes[j].count);
                    tbl_index++;
                }
            }
        }
    }
   
    // Lengths 5 through 12 
    for(length=5;length<13;length++) {
        //printf("LENGTH %d\n",length);
        for (i=0;i<8;i++) {
            if      (i==0) pattern = zeroes[length-5];
            else if (i==1) pattern = ones[length-5];
            else if (i==2) pattern = runs0[length-5];
            else if (i==3) pattern = runs1[length-5];
            else if (i==4) pattern = alternatings0[length-5];
            else if (i==5) pattern = alternatings1[length-5];
            else if (i==6) pattern = limits0[length-5];
            else if (i==7) pattern = limits1[length-5];

            for(j=1;j<109;j++) {
                if ((nodes[j].pattern_length==length) && (nodes[j].pattern==pattern)) {
                    binary_pattern(nodes[j].pattern, length ,12,s);  // 1
                    if (j == 94) {
                        printf("%d\n",nodes[j].count);
                    } else {
                        printf("%d,",nodes[j].count);
                    }
                    tbl_index++;
                }
            }
        }
    }
    
}

void populate_nodes(int nodeindex) {
    int length;
    int pattern;
    int newlength;
    int newpattern0;
    int newpattern1;
    int i;
    int found;
    int node0;
    int node1;
    nodes[nodeindex].count = 0;
    length = pattern_lengths[nodeindex];
    pattern = patterns[nodeindex];
    newlength = length+1;
    newpattern0 = pattern;
    newpattern1 = pattern + (1 << length);

    //if ((length==4) && (pattern==1)) {
    //    printf("FINDING LINKS FOR LEN 4, PATTERN 0x001\n");
    //}

    // Find the node with 0 added
    found = 0;
    i = 0;
    while (i<109) {
        //if ((patterns[i] == 0x001) && (pattern_lengths[i] == 5)) {
        //    printf("  pattern %d found with pattern = %03x and len = %d\n",i,patterns[i],pattern_lengths[i]);
        //}
        if ((patterns[i] == newpattern0) && (pattern_lengths[i] == newlength)) {
            node0 = i;
            found = 1;
            break;
        }
        i++;
    }
    if (found==0) node0=-1;

    // Find the node with 1 added
    found = 0;
    i = 0;
    while (i<109) {
        if ((patterns[i] == newpattern1) && (pattern_lengths[i] == newlength)) {
            node1 = i;
            found = 1;
            break;
        }
        i++;
    }
    if (found==0) node1=-1;
    
    nodes[nodeindex].link_0 = node0;
    nodes[nodeindex].link_1 = node1;
    nodes[nodeindex].pattern = pattern;
    nodes[nodeindex].pattern_length = length;
    //printf("line %02d populated node %02d link_0=%02d, link_1=%02d, pattern=%03x, len=%d\n",line++,nodeindex,node0,node1,pattern,length);
    fflush(stdout);
}

void display_usage() {
    printf("usage: pattern_count [-hvr] [-c <chunksize>] <filename>\n");
    printf("   Count multiple bit patterns within a binary file.\n");
    printf("   -r    --reverse      Reverse the bits in each byte to be big endian.\n");
    printf("   -v    --verbose      Print out diagnostics.\n");
    printf("   -h    --help         Print out this usage information.\n");
    printf("   -c n  --chunksize=n  Set the number of bits in each counted chunk. Default 8192.\n");
}

/* The queue
 *
 * This implements a FIFO into which bytes are pushed from a file and 
 * from which symbols (of the chosen size) are pulled from the other end.
 * Data from the file is read into buffer and that data is used to fill the
 * input side of the queue. The queue is twice as big as the buffer so the 
 * buffer read is done when the queue is less than half full.
 * It treats bits within bytes as big endian (I.E. MSB arrived first from ES).
 * There will be an option to switch to little endian at some point.
 */
 
 
void init_byte_queue() {
    int i;
    
    /* printf("Init Byte Queue\n"); */
    queue_start = 0;
    queue_end = 0;
    queue_size = 0;
    
    got_byte = 0;
    current_byte = 0;
    bits_used_from_byte = 0;
    current_symbol = 0;
    bits_in_current_symbol = 0;
   
    //printf("  Zeroing the queue, queuesize = %d\n",QUEUESIZE); 
    for (i=0;i<QUEUESIZE;i++) queue[i]=0;
    
    //printf("  Computing symbol mask with symbol_length=%d\n",symbol_length); 
    symbol_mask = ipow((uint64_t)2,(uint64_t)symbol_length)-1;
    //printf("  END symbol mask = %04x\n",symbol_mask);
}

int ishex(unsigned char c) {
    int result;
    result = 0;
    if      (c == '0') result = 1;
    else if (c == '1') result = 1;
    else if (c == '2') result = 1;
    else if (c == '3') result = 1;
    else if (c == '4') result = 1;
    else if (c == '5') result = 1;
    else if (c == '6') result = 1;
    else if (c == '7') result = 1;
    else if (c == '8') result = 1;
    else if (c == '9') result = 1;
    else if (c == 'a') result = 1;
    else if (c == 'b') result = 1;
    else if (c == 'c') result = 1;
    else if (c == 'd') result = 1;
    else if (c == 'e') result = 1;
    else if (c == 'f') result = 1;
    else if (c == 'A') result = 1;
    else if (c == 'B') result = 1;
    else if (c == 'C') result = 1;
    else if (c == 'D') result = 1;
    else if (c == 'E') result = 1;
    else if (c == 'F') result = 1;

    return result;
}

int ishexorx(unsigned char c) {
    int result;
    result = 0;
    if (ishex(c) == 1) result = 1;
    else if (c == 'x') result = 1;

    return result;
}

unsigned char hexpair[2];

int hexstate;

void init_hex2bin() {
    hexpair[0]=0x00;
    hexpair[1]=0x00;
    
    hexstate = 0;
}

/* 
 * This converts input hex text to binary. It uses a little
 * state machine to pull in 2 characters then convert them
 * to a byte. The state machine state is maintained across
 * calls so we do not lose values at read buffer boundaries.
 *
 * In the processing of the second character, an x will be
 * accepted if the first character is 0, so we get '0x'.
 * This allows the 0x prefixes to be eliminated without
 * accidentally treating the 0 as part of the data.
 */
 
size_t hex2bin(unsigned char *buffer, size_t len) {
    int outpos = 0;
    int scanpos = 0;
    unsigned char c;
    int byte;
    int nybble;
    /* Fetch characters until we get a hex 1.
     * Shift it into hexpair
     * If we have a valid hex pair put it in the buffer as binary
     * If we have 0x, drop it, including the 0.
     * If we have non hex, drop it.
     */

    do {
        if (hexstate == 0) {
            c = buffer[scanpos];
            if (ishex(c) == 1) {
                hexpair[0] = c;
                hexstate = 1;
            }
            scanpos++;
        }
        else if (hexstate == 1) {
            c = buffer[scanpos];
            if (((ishexorx(c) == 1) && (hexpair[0]=='0')) || (ishex(c)==1)){
                hexpair[1] = c;
                hexstate = 2;
            }
            scanpos++;
        }
        else if (hexstate == 2) {
            if ((hexpair[0]=='0') && (hexpair[1]=='x')) {
                hexstate = 0;
            }
            else { /* we have a valid hex pair */
                nybble = 0;                
                if ((((int)hexpair[0])>47) && (((int)hexpair[0])<58)){ /* 0-9 */
                    nybble = (int)hexpair[0] - 48;
                }
                else if ((((int)hexpair[0])>64) && (((int)hexpair[0])<71)){ /* A-F */
                    nybble = (int)hexpair[0] - 55;
                }
                else if ((((int)hexpair[0])>96) && (((int)hexpair[0])<103)){ /* a-f */
                    nybble = (int)hexpair[0] - 87;
                }

                nybble = nybble << 4;

                if ((((int)hexpair[1])>47) && (((int)hexpair[1])<58)){ /* 0-9 */
                    byte = nybble + (int)hexpair[1] - 48;
                }
                else if ((((int)hexpair[1])>64) && (((int)hexpair[1])<71)){ /* A-F */
                    byte = nybble + (int)hexpair[1] - 55;
                }
                else /*if ((((int)hexpair[1])>96) && (((int)hexpair[1])<103))*/ { /* a-f */
                    byte = nybble + (int)hexpair[1] - 87;
                }

                buffer[outpos++] = (unsigned char)byte;
                hexstate = 0;
            }
        }              
    } while (scanpos <= len);

    return outpos; /* return the number of bytes converted */
}

size_t fill_byte_queue(FILE *fp) {
    size_t len;
    size_t space;
    unsigned int i;
    unsigned int j;
    size_t total_len;
    unsigned int buff2_remaining;

    total_len = 0;
    /* Pull in a loop until there is less than BUFFSIZE space in thequeue */
    do {
        space = QUEUESIZE-queue_size; /* Dont pull more data than needed */
        if (space > BUFFSIZE) space = BUFFSIZE;
        
        //printf("  queue: SPACE=%d\n",space);
        
        len = fread((void *)buffer, (size_t)1,(size_t)space, fp);
        if (len==0) {
             //printf("  queue: LEN = %d\n",len);
            return total_len;
        }
        //printf("  queue: LEN = %d\n",len);
        //printf("  queue: ");
        //for(j=0;j<10;j++){
        //   printf("%02x ",buffer[j]);
        //}
        //printf("\n");
        /* Convert hex buffer to binary if we are in hex mode */
        //if (hexmode == 1) len = hex2bin(buffer,len); 
        
        /* Fold upper case to lower */
        //if (fold==1) {
        //    for (i=0;i<len;i++) {
        //        buffer[i]=tolower(buffer[i]);
        //    }
        //}

        /* If we aren't doing word reverse, just move the buffer to the queue */ 
        if (word_reverse == 0) {
            /*  Transfer buffer to queue */
            for (i=0;i<len;i++) {
                if (byte_reverse == 1) {
                    queue[(queue_end+i) % QUEUESIZE] = byte_reverse_table[buffer[i]];
                } else {
                    queue[(queue_end+i) % QUEUESIZE] = buffer[i];
                    //printf("  FROM BUFFER BYTE %02x\n",buffer[i]);
                }
                
                /* Call the monte carlo update that operated over bytes, not symbols */
                //update_monte_carlo(buffer[i]);
            }
            
            filebytes += len;
            
            queue_size += len;
            queue_end = ((queue_end + len) % QUEUESIZE);

            total_len += len;
        } else {  /* word_reverse == 1  so use the word reverse buffer */
            
            /* Transfer buffer to word reverse queue */
            /*printf(" Transfer buffer to word reverse queue \n");*/
            for (i=0;i<len;i++) {
                buffer2[buffer2_size++]=buffer[i];
            }

            /* Pad out to a 4 byte boundary if we are at the end */
            /* printf(" Pad out to a 4 byte boundary if we are at the end \n");*/
            /*if ((buffer2_size % 4 != 0)) {
                printf(" Padding! \n");
                for (i=0;i<(4-buffer2_size);i++) {
                    buffer2[buffer2_size++] = 0x00;
                }
                fprintf(stderr,"Warning: Padded %d extra zeroes to make 4 byte boundary for word reverse\n",(4-buffer2_size));
            }*/

            /* Transfer it back out to the queue in blocks of 4 bytes in reverse */
            /* printf(" Transfer it back out to the queue in blocks of 4 bytes in reverse\n");*/
            /*if ((buffer2_size % 4) != 0) {
                printf("ERROR: buffer2_size = %d\n",buffer2_size);
                printf("ERROR: buffer size not on 4 byte boundary"); 
                exit(1);
            }*/
            buff2_remaining = buffer2_size;
            i = 0;
            do {
                /* printf( "    i = %d,  buffer2_size = %d \n",i,buffer2_size);*/
                for (j=0;j<4;j++) {
                    /* printf( "    j = %d\n",j); */
                    if (byte_reverse == 1) {
                        queue[(queue_end+j) % QUEUESIZE] = byte_reverse_table[buffer2[(i*4)+(3-j)]];
                        //update_monte_carlo(byte_reverse_table[buffer2[(i*4)+(3-j)]]);
                    } else {
                        queue[(queue_end+j) % QUEUESIZE] = buffer2[(i*4)+(3-j)];
                        //update_monte_carlo(buffer2[(i*4)+j]);
                    }
                }
                buff2_remaining -= 4;
                buffer2_size -= 4;
                queue_size += 4;
                queue_end = (queue_end+4) % QUEUESIZE;
                total_len += 4;
                i++;
            } while (buffer2_size>3);

            /* Pad any leftover */
            if (buffer2_size != 0) {
                for (j=0;j<4;j++) {
                    if (j < buffer2_size) {
                        queue[(queue_end+j) % QUEUESIZE] = 0x00;
                        //update_monte_carlo(0x00);
                    } else {
                        if (byte_reverse == 1) {
                            queue[(queue_end+j) % QUEUESIZE] = byte_reverse_table[buffer2[(i*4)+(3-j)]];
                            //update_monte_carlo(byte_reverse_table[buffer2[(i*4)+(3-j)]]);
                        } else {
                            queue[(queue_end+j) % QUEUESIZE] = buffer2[(i*4)+(3-j)];
                            //update_monte_carlo(buffer2[(i*4)+(3-j)]);
                        }
                    }
                }
                fprintf(stderr,"Warning: Padded %d extra zeroes to make 4 byte boundary for word reverse\n",buffer2_size);
            }
            buffer2_size = 0;
        }
    } while ((QUEUESIZE-queue_size) > BUFFSIZE);
    return total_len;
}

/* pull symbol length bits off the start of the queue */
int64_t get_symbol(uint64_t symbol_length) {

    unsigned int temp;
    
    current_symbol = 0;
    
    /* Get a byte if we don't have one */
    if (got_byte == 0) {
        if (((queue_size*8) < symbol_length)) return -1; /* Uh oh. Empty */
        
        current_byte = queue[queue_start];
            //printf("      BYTE  %02x\n",current_byte);
        queue_start = (queue_start+1) % QUEUESIZE;
        queue_size -= 1;
        bits_used_from_byte = 0;
        got_byte=1;
    }
    
    /* Move bits from current byte pulled from queue to current symbol */
    if (symbol_length == 1) { /* Optimize for the single bit size case */
        current_symbol = (current_byte & 0x80) >> 7;
        current_byte <<= 1;
        bits_used_from_byte++;
        if (bits_used_from_byte == 8) {
            got_byte = 0;
            bits_used_from_byte = 0;
        }
        return current_symbol;
    } else if (symbol_length == 8) { /* optimize for the byte size case */
        current_symbol = current_byte;
        got_byte = 0;
        return current_symbol;
    } else {  /* Symbol Length != 8 or 1, do it bit by bit */
              /* Later maybe optimize when > 7 bits needed */
        /* Take upper symbol_length bits */
        bits_in_current_symbol = 0;
        do {
            temp = (current_byte & 0x80) >> 7;
            current_byte = (current_byte << 1) & 0xff;
            //printf("      BYTE  %02x\n",current_byte);
            bits_used_from_byte++;
            if (bits_used_from_byte == 8) {
                got_byte = 0;
                bits_used_from_byte = 0;
            }
            current_symbol = ((current_symbol << 1) | temp) & symbol_mask;
            bits_in_current_symbol++;
            
            /* fetch a new byte from queue if we aren't done yet */
            if (got_byte == 0) {
                if (((queue_size*8) < symbol_length)) return -1; /* Uh oh. Empty */
        
                current_byte = queue[queue_start];
                //printf("      BYTE  %02x\n",current_byte);
                queue_start = (queue_start+1) % QUEUESIZE;
                queue_size -= 1;
                bits_used_from_byte = 0;
                got_byte=1;
            }
        } while (bits_in_current_symbol < symbol_length);
        return current_symbol;
    }
}

// Return 12 bits, in a moving window bases.
// Pull symbols from the queue to do that and feed out bitwise.

// 48 a5 b9 f2 3e 9a 8d 4f e4 e2 77 b0 f2 bf f2 6f
//
int pull_12_bitwise() {
    //int64_t x;
    //int64_t y;
    int  result;
    //char s[32];
    //char t[32];
    if (pull_12_first==1) {
        //printf("  PULL_12_FIRST\n");
        pull_12_first = 0;
        x = get_symbol(12);
        //printf("   FIRST SYM %03x\n",x);
        if (x < 0) return -1;
        y = get_symbol(12);
        //printf("   SECND SYM %03x\n",y);
        if (y < 0) return -1;
        x = x+(y << 12);
        //printf("   CMBND SYM %03x\n",x);
        pull_12_phase=1;
        //printf("   RETURNING %03x\n",x & 0xfff);
        return (x & 0xfff);
    } else {
        if (pull_12_phase > 11) {
           //printf("   PHASE REACHED 12\n");
           y = get_symbol(12);
           //printf("   NEXT  SYM %03x",x);
           //printf("   A\n");
           if (y<0) return -1;
           x = ((x >> 12)&0xfff) + ((y << 12)&0x0fff000);
           pull_12_phase=0;
           //printf("   B\n");
           //binary_pattern(x,24,24,s);
           //printf("   C\n");
           //printf("   X = %s\n",s);
        }
        result = (x>>pull_12_phase) & 0xfff;
           //printf("   D\n");
           //binary_pattern(result,12,14,s);
           //printf("   E\n");
           //binary_pattern(x,24,26,t);
           //printf("   F\n");
          //printf("     RETURNING %s, phase = %d x = %s\n",s,pull_12_phase,t);
        pull_12_phase++;
        return result;
    }

}

void clear_counts() {
    int i;
    for(i=0;i<109;i++) {
        nodes[i].count=0;
    }
}

int main(int argc, char *argv[]) {
    int i;

    tbl_index=0;
    // Populate the nodes from the patterns and lengths.
    
    // Root node - null pattern
    nodes[0].pattern        = 0;
    nodes[0].pattern_length = 0; 
    nodes[0].count = 0; 
    nodes[0].link_0 = 1; 
    nodes[0].link_1 = 2; 

    line =0;

    //printf("START Populating Nodes\n");
    for (i=1;i<109;i++)
        populate_nodes(i);    

    //printf("END Populating Nodes\n");

    int chunksize=8192;
    int verbose=0;
    int use_stdin;
    FILE *fp;
    int opt;
    int longIndex;

    // Parse the command line arguments and options
    char optString[] = "vhrc:";
    static const struct option longOpts[] = {
        { "help", no_argument, NULL, 'h' },
        { "chunksize", required_argument, NULL, 'c' },
        { "reverse", no_argument, NULL, 'r' },
        { "verbose", no_argument, NULL, 'v' },
        { NULL, no_argument, NULL, 0 }
    }; 

    //printf("START Parsing options\n");
    opt = getopt_long( argc, argv, optString, longOpts, &longIndex );
    while( opt != -1 ) {
        //fprintf(stderr,"OPT = %c\n",opt);
        switch( opt ) {
            case 'v':
                verbose = 1;
                break;
            case 'r':
                byte_reverse = 1;
                break;
            case 'c':
                chunksize = atoi(optarg);
                break;
            case 'h':   /* fall-through is intentional */
            case '?':
                display_usage();
                exit(0);
                 
            default:
                /* You won't actually get here. */
                break;
        }
         
        opt = getopt_long( argc, argv, optString, longOpts, &longIndex );
    } // end while
               
    /* Loop through the filenames */
	if ((optind==argc)) {
		use_stdin = 1;
	}
	else {
		use_stdin = 0;
	}

	if (use_stdin==1) {
		use_stdin = 1;
		fp = stdin;
	}
	else {
            filenumber=optind;
		    filename = argv[filenumber];
            fp = fopen(filename,"rb");
    }
    //printf("END Parsing options\n");
    if (fp == NULL) {
        fprintf(stderr," can't open %s for reading\n",filename);
        exit(1);
    }
    if (verbose>0) {
        printf(" opened %s for reading\n",filename);
    }
    // Start reading the file
    //int chunk=0;
    //int bit_count=0;
    //int head  = 0;
    //int len;
    int not_eof = 1;
    int symbol_count = 0;
    int symbol;
    int chunkno = 1;
    //char s[16];
    if (verbose>0) {
        fprintf(stderr,"START Init byte queue\n");
    }
    init_byte_queue();
    if (verbose>0) {
        fprintf(stderr,"END Init byte queue\n");
    }
    // CSV header
    printf("chunk, 0, 1, 00, 10, 01, 11, 000, 100, 010, 110, 001, 101, 011, 111,");
    printf("0000, 1000, 0100, 1100, 0010, 1010, 0110, 1110, 0001, 1001, 0101, 1101, 0011, 1011, 0111, 1111,");
    printf("00000, 11111, 10001, 01110, 10101, 01010, 11001, 00110,");
    printf("000000, 111111, 100001, 011110, 101010, 010101, 110011, 001100,");
    printf("0000000, 1111111, 1000001, 0111110, 1010101, 0101010, 1100110, 0011001,");
    printf("00000000, 11111111, 10000001, 01111110, 10101010, 01010101, 11001100, 00110011,");
    printf("000000000, 111111111, 100000001, 011111110, 101010101, 010101010, 110011001, 001100110,");
    printf("0000000000, 1111111111, 1000000001, 0111111110, 1010101010, 0101010101, 1100110011, 0011001100,");
    printf("00000000000, 11111111111, 10000000001, 01111111110, 10101010101, 01010101010, 11001100110, 00110011001,");
    printf("000000000000, 111111111111, 100000000001, 011111111110, 101010101010, 010101010101, 110011001100, 001100110011\n");
    //printf("START Loop\n");
    do {
        if (queue_size < 3) {
            not_eof = (int)fill_byte_queue(fp);
            if (not_eof==0) {
                if (verbose>0) {
                    fprintf(stderr,"break due to no_eof==1\n");
                }
                break;
            }
        }

        symbol = pull_12_bitwise(symbol_length);
        symbol_count++;

        if (symbol ==-1){
            if (verbose>0) {
                fprintf(stderr,"Break due to -1\n");
            }
            break;
        }

        update_nodes(symbol);

        //printf("symbol count = %d\n",symbol_count);
        if (symbol_count == chunksize) {
            print_nodes_csv(chunkno);
            clear_counts();
            symbol_count = 0;
            chunkno++;
        }

    } while (1==1);


    fclose(fp);
    return(0);
}


