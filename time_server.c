

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
//#include <winsock.h>
#include <time.h>

#define HOST_NAME "pool.ntp.org"
#define PORT_NUMBER 123
#define NTP_TIMESTAMP_DELTA 2208988800ull

#define LI(packet)   (uint8_t) ((packet.li_vn_mode & 0xC0) >> 6) // (li   & 11 000 000) >> 6
#define VN(packet)   (uint8_t) ((packet.li_vn_mode & 0x38) >> 3) // (vn   & 00 111 000) >> 3
#define MODE(packet) (uint8_t) ((packet.li_vn_mode & 0x07) >> 0) // (mode & 00 000 111) >> 0

/***************************************************************
 * @Description
 * from * (C) 2014 David Lettier.
 * http://www.lettier.com/
 * https://github.com/lettier/ntpclient/blob/master/source/c/main.c
 * NTP client.
 *
 *
get_time function is used to get the current time from a Network Time Protocol (NTP) server.
It starts by creating a socket and trying to connect to a given NTP server. Upon successful connection, it sends an NTP time request to the server and waits for the response. Once the response is received, it extracts the timestamp, converts it into the UNIX local time format and then prints it.
This function is helpful in scenarios where the accurate time is needed to be synchronized with a global standard.
@Arguments
None.
Returns
It returns 0 when the function successfully retrieves and prints the time.
 In case of any error while opening the socket, finding the host,
 establishing the connection or any I/O operations,
 it returns 1 representing the failure of the operation.
 * @return
 ***********************************************/


int get_time(void) {
    struct sockaddr_in server_address;
    struct hostent *server;
    char buffer[256];
    int sockfd, bytes, n;

    typedef struct
    {

        uint8_t li_vn_mode;      // Eight bits. li, vn, and mode.
        // li.   Two bits.   Leap indicator.
        // vn.   Three bits. Version number of the protocol.
        // mode. Three bits. Client will pick mode 3 for client.

        uint8_t stratum;         // Eight bits. Stratum level of the local clock.
        uint8_t poll;            // Eight bits. Maximum interval between successive messages.
        uint8_t precision;       // Eight bits. Precision of the local clock.

        uint32_t rootDelay;      // 32 bits. Total round trip delay time.
        uint32_t rootDispersion; // 32 bits. Max error aloud from primary clock source.
        uint32_t refId;          // 32 bits. Reference clock identifier.

        uint32_t refTm_s;        // 32 bits. Reference time-stamp seconds.
        uint32_t refTm_f;        // 32 bits. Reference time-stamp fraction of a second.

        uint32_t origTm_s;       // 32 bits. Originate time-stamp seconds.
        uint32_t origTm_f;       // 32 bits. Originate time-stamp fraction of a second.

        uint32_t rxTm_s;         // 32 bits. Received time-stamp seconds.
        uint32_t rxTm_f;         // 32 bits. Received time-stamp fraction of a second.

        uint32_t txTm_s;         // 32 bits and the most important field the client cares about. Transmit time-stamp seconds.
        uint32_t txTm_f;         // 32 bits. Transmit time-stamp fraction of a second.

    } ntp_packet;              // Total: 384 bits or 48 bytes.

    // Create and zero out the packet. All 48 bytes worth.

    ntp_packet packet = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    memset( &packet, 0, sizeof( ntp_packet ) );
    *( ( char * ) &packet + 0 ) = 0x1b; // Represents 27 in base 10 or 00011011 in base 2.


    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd < 0) {
        printf("ERROR opening socket\r\n");
        return 1;
    }
    printf("socket opened\r\n");


    server = gethostbyname(HOST_NAME);
    if (server == NULL) {
        printf("ERROR, no such host\n");
        return 1;
    }

    printf("host found : %s\r\n", HOST_NAME);

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    memmove((char *)&server_address.sin_addr.s_addr, server->h_addr, server->h_length);
    server_address.sin_port = htons(PORT_NUMBER);

    if(connect(sockfd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("ERROR connecting to server");
        return 1;
    }

    printf("connected to time server\r\n", HOST_NAME);

    // Send it the NTP packet it wants. If n == -1, it failed.

    n = write( sockfd, ( char* ) &packet, sizeof( ntp_packet ) );

    if ( n < 0 )
        printf( "ERROR writing to socket\n" );
    else
        printf("wrote to ntp\n");

    // Wait and receive the packet back from the server. If n == -1, it failed.

    n = read( sockfd, ( char* ) &packet, sizeof( ntp_packet ) );

    if ( n < 0 )
        printf( "ERROR reading from socket\n" );
    else
        printf("read from ntp\n");

    // These two fields contain the time-stamp seconds as the packet left the NTP server.
    // The number of seconds correspond to the seconds passed since 1900.
    // ntohl() converts the bit/byte order from the network's to host's "endianness".

    packet.txTm_s = ntohl( packet.txTm_s ); // Time-stamp seconds.
    packet.txTm_f = ntohl( packet.txTm_f ); // Time-stamp fraction of a second.

    // Extract the 32 bits that represent the time-stamp seconds (since NTP epoch) from when the packet left the server.
    // Subtract 70 years worth of seconds from the seconds since 1900.
    // This leaves the seconds since the UNIX epoch of 1970.
    // (1900)------------------(1970)**************************************(Time Packet Left the Server)

    time_t txTm = ( time_t ) ( packet.txTm_s - NTP_TIMESTAMP_DELTA );

    // Print the time we got from the server, accounting for local timezone and conversion from UTC time.

    printf( "Time: %s", ctime( ( const time_t* ) &txTm ) );

    close(sockfd);

    return 0;
}

