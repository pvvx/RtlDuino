/*
 *
 *  FTP Server on STM32-E407 with ChibiOs
 *
 *  Copyright (c) 2014-2015 by Jean-Michel Gallego
 *
 *  Please read file ReadMe.txt for instructions
 *
 *  This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <ctype.h>
#include <string.h>

#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"

#include "ff.h"
#include "ftps.h"

#define CH_FREQUENCY 1000

//extern bool_t blinkFast;

struct    netconn *conn, *dataconn, *datasrvconn;
struct    netbuf  *inbuf;
struct    ip_addr ipclient;
struct    ip_addr ipserver;

FIL       file;
FILINFO   finfo;
char      lfn[ FTP_MAX_LFN + 1 ];    // Buffer to store the LFN

uint16_t  dataPort;
int8_t    cmdStatus;                    // status of ftp command connexion
char      command[5];                 // command sent by client
char      parameters[ FTP_PARAM_SIZE ]; // parameters sent by client
char      cwdName[ FTP_CWD_SIZE ];      // name of current directory
char      cwdRNFR[ FTP_CWD_SIZE ];      // name of origin directory for Rename command
char      path[ FTP_CWD_SIZE ];
bool      dataPassiveConn;
char      str[64];
time_t timeBeginTrans;
uint32_t  bytesTransfered;
int8_t    err;
uint8_t   buf[ SD_BUF_SIZE ];   // data buffer for writing to SD card

// =========================================================
//              Send a response to the client
// =========================================================

void netconnWrite( const char * s )
{
  netconn_write( conn, s, strlen( s ), NETCONN_COPY);
  CONSOLE_PRINT( s );
}

void netconnWriteNl( const char * s )
{
  netconnWrite( s );
  netconnWrite( "\r\n" );
}

char * i2strZ( char * s, uint32_t i, uint8_t z )
{
  char * psi = s + ( z > 0 ? z : sizeof( s ));

  * -- psi = 0;
  if( i == 0 )
    * -- psi = '0';
  for( ; i; i /= 10 )
    * -- psi = '0' + i% 10;
  if( z > 0 )
    while( psi > s )
      * -- psi = '0';
  return psi;
}

//  Convert an integer to string (must be <= 9999999999)
//
//  Return pointer to string

char * i2str( uint32_t i )
{
  return i2strZ( str, i, 0 );
}

// Create string YYYYMMDDHHMMSS from date and time
//
// parameters:
//    date, time
//
// return:
//    pointer to string

char * makeDateTimeStr( uint16_t date, uint16_t time )
{
  i2strZ( str, (( date & 0xFE00 ) >> 9 ) + 1980, 5 );
  i2strZ( str + 4, ( date & 0x01E0 ) >> 5, 3 );
  i2strZ( str + 6, date & 0x001F, 3 );
  i2strZ( str + 8, ( time & 0xF800 ) >> 11, 3 );
  i2strZ( str + 10, ( time & 0x07E0 ) >> 5, 3 );
  i2strZ( str + 12, ( time & 0x001F ) << 1, 3 );
  return str;
}

#if LINUX_COMPATIBLE
typedef struct _msftm_td {
	unsigned short day :5;
	unsigned short month :4;
	unsigned short year :7;
} msftm_d;

typedef struct _msftm_tt {
	unsigned short sec :5;
	unsigned short min :6;
	unsigned short hour :5;
} msftm_t;

typedef union {
	unsigned short w;
	msftm_d d;
} msftm_td;

typedef union {
	unsigned short w;
	msftm_t t;
} msftm_tt;

typedef struct _mmm
{
	uint8_t str[4];
}smonth;

smonth month[12] = { "Jan" , "Feb" , "Mar" , "Apr" , "May" , "Jun" , "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

/*
 * MS files times
 */
uint8_t * ux_ftime(unsigned short td, unsigned short tt) {
	msftm_td d;
	msftm_tt t;
	d.w = td;
	t.w = tt;
	if(d.d.year >= 2017-1980)
		rtl_sprintf(str, "%s %02u %02u:%02u", &month[d.d.month], d.d.day, t.t.hour, t.t.min);	
	else
		rtl_sprintf(str, "%s %02u %u", &month[d.d.month], d.d.day, d.d.year + 1980);
	return str;
}
/*
 * Linux files attr
 */
uint8_t * ux_fattr(uint8_t attr) {
	memset(str, '-', 10);
	if (attr & AM_DIR) {
		str[0] = 'd';
//   		str[3] = 'x';
//   		str[6] = 'x';
//   		str[9] = 'x';
	}
	if (!(attr & AM_VOL)) {
//   	 {
		str[1] = 'r';
		str[4] = 'r';
		str[7] = 'r';
		if (!(attr & AM_RDO)) {
			str[2] = 'w';
			str[5] = 'w';
			str[8] = 'w';
		}
	}
//   	if(attr & AM_VOL) str[3] = 'x';
	if (!(attr & AM_SYS))
		str[3] = 'x';
	if (!(attr & AM_HID))
		str[6] = 'x';
	if (!(attr & AM_ARC))
		str[9] = 'x';
	str[10] = 0;
	return str;
}

#endif

// =========================================================
//             Get a command from the client
// =========================================================

// update variables command and parameters
//
// return: -4 time out
//         -3 error receiving data
//         -2 command line too long
//         -1 syntax error
//          0 command without parameters
//          >0 length of parameters

int8_t readCommand( void )
{
  char   * pbuf;
  uint16_t buflen;
  int8_t   rc = 0;
  int8_t   i;
  char     car;

  command[ 0 ] = 0;
  parameters[ 0 ] = 0;
  err = netconn_recv( conn, & inbuf );
  if( err == ERR_TIMEOUT )
    return -4;
  if( err != ERR_OK )
    return -3;
  netbuf_data( inbuf, (void **) & pbuf, & buflen );
  if( buflen == 0 )
    goto deletebuf;
  i = 0;
  car = pbuf[ 0 ];
  do
  {
    if( ! isalpha( car ))
      break;
    command[ i ++ ] = car;
    car = pbuf[ i ];
  }
  while( i < buflen && i < 4 );
  command[ i ] = 0;
  if( car != ' ' )
    goto deletebuf;
  do
    if( i > buflen + 2 )
      goto deletebuf;
  while( pbuf[ i ++ ] == ' ' );
  rc = i;
  do
    car = pbuf[ rc ++ ];
  while( car != '\n' && car != '\r' && rc < buflen );
  if( rc == buflen )
  {
    rc = -1;
    goto deletebuf;
  }
  if( rc - i - 1 >= FTP_PARAM_SIZE )
  {
    rc = -2;
    goto deletebuf;
  }
  strncpy( parameters, pbuf + i - 1, rc - i );
  parameters[ rc - i ] = 0;
  rc = rc - i;

  deletebuf:
  CONSOLE_PRINT( "<<%s %s\r\n", command, parameters );
  netbuf_delete( inbuf );
  return rc;
}

// =========================================================
//               Functions for data connection
// =========================================================

bool dataConnect( void )
{
  CONSOLE_PRINT( "Connecting in %s mode\r\n",
                 ( dataPassiveConn ? "passive" : "active" ));
  if( dataPassiveConn )
  {
    // Wait for connexion form client during 500 ms
    time_t time = xTaskGetTickCount() + 500;
    while( netconn_accept( datasrvconn, & dataconn ) != ERR_OK )
      if( time < xTaskGetTickCount())
        goto error;
  }
  else
  {
    //  Create a new TCP connection handle
    dataconn = netconn_new( NETCONN_TCP );
    if( dataconn == NULL )
    {
      CONSOLE_PRINT( "Error in dataConnect(): netconn_new\r\n" );
      goto delete;
    }
    netconn_bind( dataconn, IP_ADDR_ANY, dataPort );
    //  Connect to data port with client IP address
    err = netconn_connect( dataconn, & ipclient, dataPort );
    if( err != ERR_OK )
    {
      CONSOLE_PRINT( "Error %u in dataConnect(): netconn_connect\r\n", err );
      goto delete;
    }
  }
  return true;

  delete:
  netconn_delete( dataconn );
  error:
  netconnWriteNl( "425 No data connection" );
  return false;
}

void dataClose( void )
{
  if( dataconn == NULL )
    return;
  netconn_close( dataconn );
  netconn_delete( dataconn );
  dataconn = NULL;
}

err_t dataWrite( const char * data )
{
  return netconn_write( dataconn, data, strlen( data ), NETCONN_COPY );
}

// =========================================================
//                  Functions on files
// =========================================================

// Make complete path/name from cwdName and parameters
//
// 3 possible cases:
//   parameters can be absolute path, relative path or only the name
//
// parameters:
//   fullName : where to store the path/name
//
// return:
//   true, if done

bool makePathFrom( char * fullName, char * param )
{
  // Root or empty?
  if( ! strcmp( param, "/" ) || strlen( param ) == 0 )
  {
    strcpy( fullName, "/" );
    return true;
  }
  // If relative path, concatenate with current dir
  if( param[0] != '/' )
  {
    strcpy( fullName, cwdName );
    if( fullName[ strlen( fullName ) - 1 ] != '/' )
      strncat( fullName, "/", FTP_CWD_SIZE );
    strncat( fullName, param, FTP_CWD_SIZE );
  }
  else
    strcpy( fullName, param );
  // If ends with '/', remove it
  uint16_t strl = strlen( fullName ) - 1;
  if( fullName[ strl ] == '/' && strl > 1 )
    fullName[ strl ] = 0;
  if( strlen( fullName ) < FTP_CWD_SIZE )
    return true;
  netconnWriteNl( "500 Command line too long" );
  return false;
}

bool makePath( char * fullName )
{
  return makePathFrom( fullName, parameters );
}

// Return true if a file or directory exists
//   path : absolute name of file or directory

bool fs_exists( char * path )
{
  if( ! strcmp( path, "/" ) )
    return true;

  char *  path0 = path;

  return f_stat( path0, & finfo ) == FR_OK;
}

bool fs_opendir( DIR * pdir, char * dirName )
{
  char * dirName0 = dirName;
  uint8_t ffs_result;

  ffs_result = f_opendir( pdir, dirName0 );
  return ffs_result == FR_OK;
//  return f_opendir( pdir, dirName0 ) == FR_OK;
}

void closeTransfer( void )
{
  uint32_t deltaT = (uint32_t) ( xTaskGetTickCount() - timeBeginTrans );
  netconnWriteNl( "226-File successfully transferred" );
  if( deltaT > 0 && bytesTransfered > 0 )
  {
    netconnWrite( "226 " );
    netconnWrite( i2str( deltaT ));
    netconnWrite( " ms, " );
    uint32_t bps;
    if( bytesTransfered < 0x7fffffff / CH_FREQUENCY )
      bps = ( bytesTransfered * CH_FREQUENCY ) / deltaT;
    else
      bps = ( bytesTransfered / deltaT ) * CH_FREQUENCY;
    if(bps < 10000)
    {
      netconnWrite( i2str( bps ));
      netconnWriteNl( " bytes/s" );
    }
    else
    {
      netconnWrite( i2str( bps / 1000 ));
      netconnWriteNl( " kbytes/s" );
    }
  }

  f_close( & file );
}

// =========================================================
//                   Process a command
// =========================================================

bool ftp_server_command( char * command, char * parameters )
{
//  blinkFast = TRUE;
  ///////////////////////////////////////
  //                                   //
  //      ACCESS CONTROL COMMANDS      //
  //                                   //
  ///////////////////////////////////////

  //
  //  QUIT
  //

  if( ! strcmp( command, "QUIT" ))
    return FALSE;
  //
  //  PWD - Print Directory
  //
  else if( ! strcmp( command, "PWD" ) ||
           ( ! strcmp( command, "CWD" ) && ! strcmp( parameters, "." )))  // 'CWD .' is the same as PWD command
  {
    netconnWrite( "257 \"" );
    netconnWrite( cwdName );
    netconnWriteNl( "\" is your current directory" );
  }
  //
  //  CWD - Change Working Directory
  //
  else if( ! strcmp( command, "CWD" ))
  {
 
    CONSOLE_PRINT("Current directory %s\r\n", cwdName);
    if( makePath( path ))
      if(!strcmp(parameters, "..")) {
       	char * ptr = strrchr(path, '/');
        if(ptr) {
          if(ptr != path) {
	        *ptr = 0;
            ptr = strrchr(path, '/');
	        if(ptr)	{
	          if(ptr != path) *ptr = 0;
	          else ptr[1] = 0;
			}	         
    	  }
    	  else ptr[1] = 0;
        }
	  }
      if( fs_exists( path ))
      {
        strcpy( cwdName, path );
        netconnWriteNl( "250 Directory successfully changed." );
      }
      else
        netconnWriteNl( "550 Failed to change directory." );
      CONSOLE_PRINT("New directory %s\r\n", cwdName);
  }
#if LINUX_COMPATIBLE  
  
  else if(( ! strcmp( command, "SYST" )) || ( ! strcmp( command, "syst" )))
  {
	netconnWriteNl("215 UNIX Type: L8");  
  }
  else if( ! strcmp( command, "REST" ))
  {
	netconnWriteNl("200");  
  }
#endif  
  //
  //  CDUP - Change to Parent Directory
  //
  else if( ! strcmp( command, "CDUP" ))
  {
    bool ok = false;

    if( strlen( cwdName ) > 1 )  // do nothing if cwdName is root
    {
      // if cwdName ends with '/', remove it (must not append)
      if( cwdName[ strlen( cwdName ) - 1 ] == '/' )
        cwdName[ strlen( cwdName ) - 1 ] = 0;
      // search last '/'
      char * pSep = strrchr( cwdName, '/' );
      ok = pSep > cwdName;
      // if found, ends the string on its position
      if( ok )
      {
        * pSep = 0;
        ok = fs_exists( cwdName );
      }
    }
    // if an error appends, move to root
    if( ! ok )
      strcpy( cwdName, "/" );
    netconnWrite( "200 Ok. Current directory is " );
    netconnWriteNl( cwdName );
  }

  ///////////////////////////////////////
  //                                   //
  //    TRANSFER PARAMETER COMMANDS    //
  //                                   //
  ///////////////////////////////////////

  //
  //  MODE - Transfer Mode
  //
  else if( ! strcmp( command, "MODE" ))
  {
    if( ! strcmp( parameters, "S" ))
      netconnWriteNl( "200 S Ok" );
    // else if( ! strcmp( parameters, "B" ))
    //  netconnWriteNl( "200 B Ok" );
    else
      netconnWriteNl( "504 Only S(tream) is suported" );
  }
  //
  //  STRU - File Structure
  //
  else if( ! strcmp( command, "STRU" ))
  {
    if( ! strcmp( parameters, "F" ))
      netconnWriteNl( "200 F Ok" );
    // else if( ! strcmp( parameters, "R" ))
    //  client << "200 B Ok\r\n";
    else
      netconnWriteNl( "504 Only F(ile) is suported" );
  }
  //
  //  TYPE - Data Type
  //
  else if( ! strcmp( command, "TYPE" ))
  {
    if( ! strcmp( parameters, "A" ))
      netconnWriteNl( "200 TYPE is now ASII" );
    else if( ! strcmp( parameters, "I" ))
      netconnWriteNl( "200 TYPE is now 8-bit binary" );
    else
      netconnWriteNl( "504 Unknow TYPE" );
  }
  //
  //  PASV - Passive Connection management
  //
  else if( ! strcmp( command, "PASV" ))
  {
    dataPassiveConn = true;
    dataClose();
    dataPort = FTP_DATA_PORT;
    CONSOLE_PRINT( "Connection management set to passive\r\n" );
    CONSOLE_PRINT( "Data port set to %U\r\n", dataPort );
    netconnWrite( "227 Entering Passive Mode (" );
    netconnWrite( i2str( ip4_addr1( & ipserver ))); netconnWrite( "," );
    netconnWrite( i2str( ip4_addr2( & ipserver ))); netconnWrite( "," );
    netconnWrite( i2str( ip4_addr3( & ipserver ))); netconnWrite( "," );
    netconnWrite( i2str( ip4_addr4( & ipserver ))); netconnWrite( "," );
    netconnWrite( i2str( dataPort >> 8 )); netconnWrite( "," );
    netconnWrite( i2str( dataPort & 255 )); netconnWriteNl( ")." );
  }
  //
  //  PORT - Data Port
  //
  else if( ! strcmp( command, "PORT" ))
  {
    uint8_t ip[4];
    uint8_t i;
    dataPassiveConn = false;
    dataClose();
    // get IP of data client
    char * p = parameters;
    for( i = 0; i < 4; i ++ )
    {
      ip[ i ] = atoi( p );
      p = strchr( p, ',' ) + 1;
    }
    // get port of data client
    dataPort = 256 * atoi( p );
    p = strchr( p, ',' ) + 1;
    dataPort += atoi( p );
    if( p == NULL )
      netconnWriteNl( "501 Can't interpret parameters" );
    else
    {
      IP4_ADDR( & ipclient, ip[0], ip[1],ip[2],ip[3] );
      netconnWriteNl( "200 PORT command successful" );
      CONSOLE_PRINT( "Data IP set to %u:%u:%u:%u\r\n", ip[0], ip[1], ip[2], ip[3] );
      CONSOLE_PRINT( "Data port set to %U\r\n", dataPort );
    }
  }

  ///////////////////////////////////////
  //                                   //
  //        FTP SERVICE COMMANDS       //
  //                                   //
  ///////////////////////////////////////

  //
  //  LIST - List
  //
  else if( ! strcmp( command, "LIST" ))
  {
    uint16_t nm = 0;
    DIR dir;
//	if(cwdName[0] = 0) { cwdName[0]='/'; cwdName[1]=0; };    

    if( ! fs_opendir( & dir, cwdName ))
    {
      netconnWrite( "550 Can't open directory " );
      netconnWriteNl( cwdName );
    }
    else if( dataConnect())
    {
      netconnWriteNl( "150 Accepted data connection" );
      for( ; ; )
      {
        if( f_readdir( & dir, & finfo ) != FR_OK ||
            finfo.fname[0] == 0 )
          break;
        if( finfo.fname[0] == '.' )
          continue;
#if LINUX_COMPATIBLE
					dataWrite(ux_fattr(finfo.fattrib));
					dataWrite(" 1");
					if (finfo.fattrib & AM_VOL)
						dataWrite(" volume ");
					else if (finfo.fattrib & AM_SYS)
						dataWrite(" system ");
					else
						dataWrite(" none   ");
					if (finfo.fattrib & AM_HID)
						dataWrite(" hidden ");
					else
						dataWrite(" none   ");
					dataWrite( i2str( finfo.fsize ));
					dataWrite(" ");
          			dataWrite(ux_ftime(finfo.fdate, finfo.ftime));
					dataWrite(" ");
  	                dataWrite( *finfo.lfname == 0 ? finfo.fname : finfo.lfname );
			        dataWrite( "\r\n" );
#else
        if( finfo.fattrib & AM_DIR )
          dataWrite( "+/" );
        else
        {
          dataWrite( "+r,s" );
          dataWrite( i2str( finfo.fsize ));
        }
        dataWrite( ",\t" );
//        dataWrite( lfn[0] == 0 ? finfo.fname : lfn );
        dataWrite( *finfo.lfname == 0 ? finfo.fname : finfo.lfname );
        dataWrite( "\r\n" );
#endif
        nm ++;
      }
      netconnWriteNl( "226 Directory send OK." );
      dataClose();
    }
  }
  //
  //  MLSD - Listing for Machine Processing (see RFC 3659)
  //
  else if( ! strcmp( command, "MLSD" ))
  {
    if( dataConnect())
    {
      DIR dir;

      netconnWriteNl( "150 Accepted data connection" );
      if( ! fs_opendir( & dir, cwdName ))
      {
        netconnWrite( "550 Can't open directory " );
        netconnWriteNl( parameters );
      }
      else
      {
        uint16_t nm = 0;

        for( ; ; )
        {
          if( f_readdir( & dir, & finfo ) != FR_OK ||
              finfo.fname[0] == 0 )
            break;
//          if( finfo.fname[0] == '.' )
//            continue;

          dataWrite( "Type=" );
          dataWrite( finfo.fattrib & AM_DIR ? "dir" : "file" );
          dataWrite( ";Size=" );
          dataWrite( i2str( finfo.fsize ));
          dataWrite( ";Modify=" );
          dataWrite( makeDateTimeStr( finfo.fdate, finfo.ftime ));
          dataWrite( "; " );
          dataWrite( *finfo.lfname == 0 ? finfo.fname : finfo.lfname );
          dataWrite( "\r\n" );
          nm ++;
        }
        netconnWriteNl( "226-options: -a -l" );
        netconnWrite( "226 " );
        netconnWrite( i2str( nm ));
        netconnWriteNl( " matches total" );
      }
      dataClose();
    }
  }
  //
  //  NLST - Name List
  //
  else if( ! strcmp( command, "NLST" ))
  {
    if( dataConnect())
    {
      DIR dir;

      netconnWriteNl( "150 Accepted data connection" );
      if( ! fs_opendir( & dir, cwdName ))
      {
        netconnWrite( "550 Can't open directory " );
        netconnWriteNl( parameters );
      }
      else
      {
        uint16_t nm = 0;

        for( ; ; )
        {
          dataWrite( *finfo.lfname == 0 ? finfo.fname : finfo.lfname );
          dataWrite( "\r\n" );
          nm ++;
        }
        netconnWriteNl( "226 Directory send OK." );
      }
      dataClose();
    }
  }
  //
  //  DELE - Delete a File
  //
  else if( ! strcmp( command, "DELE" ))
  {
    if( strlen( parameters ) == 0 )
      netconnWriteNl( "501 No file name" );
    else if( makePath( path ))
    {
      if( ! fs_exists( path ))
      {
        netconnWrite( "550 File " );
        netconnWrite( parameters );
        netconnWriteNl( " not found" );
      }
      else
      {
        if( f_unlink( path ) == FR_OK )
        {
          netconnWrite( "250 Deleted " );
          netconnWriteNl( parameters );
        }
        else
        {
          netconnWrite( "450 Can't delete " );
          netconnWriteNl( parameters );
        }
      }
    }
  }
  //
  //  NOOP
  //
  else if( ! strcmp( command, "NOOP" ))
  {
    netconnWriteNl( "200 Zzz..." );
  }
  //
  //  RETR - Retrieve
  //
  else if( ! strcmp( command, "RETR" ))
  {
    if( strlen( parameters ) == 0 )
      netconnWriteNl( "501 No file name" );
    else if( makePath( path ))
    {
      if( ! fs_exists( path ))
      {
        netconnWrite( "550 File " );
        netconnWrite( parameters );
        netconnWriteNl( " not found" );
      }
      else if( f_open( & file, path, FA_READ ) != FR_OK )
      {
        netconnWrite( "450 Can't open " );
        netconnWriteNl( parameters );
      }
      else if( dataConnect())
      {
        uint16_t nb;

        CONSOLE_PRINT( "Sending %s\r\n", parameters );
        netconnWrite( "150-Connected to port " );
        netconnWriteNl( i2str( dataPort ));
        netconnWrite( "150 " );
        netconnWrite( i2str( f_size( & file )));
        netconnWriteNl( " bytes to download" );
        timeBeginTrans = xTaskGetTickCount();
        bytesTransfered = 0;

///        CONSOLE_PRINT( "Start transfert\r\n" );
        while( f_read( & file, buf, SD_BUF_SIZE, (UINT *) & nb ) == FR_OK && nb > 0 )
        {
          netconn_write( dataconn, buf, nb, NETCONN_COPY ); 
          bytesTransfered += nb;
///          CONSOLE_PRINT( "Sent %u bytes\r", bytesTransfered );
//          blinkFast = TRUE;
        }
///        CONSOLE_PRINT( "\n" );
        closeTransfer();
        dataClose();
      }
    }
  }
  //
  //  STOR - Store
  //
  else if( ! strcmp( command, "STOR" ))
  {
    if( strlen( parameters ) == 0 )
      netconnWriteNl( "501 No file name" );
    else if( makePath( path ))
    {
      if( f_open( & file, path, FA_CREATE_ALWAYS | FA_WRITE ) != FR_OK )
      {
        netconnWrite( "451 Can't open/create " );
        netconnWriteNl( parameters );
      }
      else if( ! dataConnect())
        f_close( & file );
      else
      {
        struct   pbuf * rcvbuf = NULL;
        void   * prcvbuf;
        uint16_t buflen = 0;
        uint16_t off = 0;
        uint16_t copylen;
        int8_t   ferr = 0;
        UINT     nb;

        CONSOLE_PRINT( "Receiving %s\r\n", parameters );
        netconnWrite( "150 Connected to port " );
        netconnWriteNl( i2str( dataPort ));
        timeBeginTrans = xTaskGetTickCount();
        bytesTransfered = 0;
        do
        {
          err = netconn_recv_tcp_pbuf( dataconn, & rcvbuf );
          if( err != ERR_OK )
            break;
          uint16_t pboff = 0;
          prcvbuf = rcvbuf->payload;
          buflen = rcvbuf->tot_len;
          bytesTransfered += buflen;
          while( buflen > 0 && ferr == 0 )
          {
            if( buflen <= SD_BUF_SIZE - off )
              copylen = buflen;
            else
              copylen = SD_BUF_SIZE - off;
            buflen -= copylen;
            if(pbuf_copy_partial(rcvbuf, buf + off, copylen, pboff) != copylen) {
	          CONSOLE_PRINT("Buffer error!\r\n");
              err = -1;
              break;
            }
            pboff += copylen; 
            prcvbuf += copylen;
            off += copylen;
            if( off == SD_BUF_SIZE )
            {
              ferr = f_write( & file, buf, SD_BUF_SIZE, (UINT *) & nb );
              // chThdSleepMilliseconds( 2 );
              off = 0;
            }
          }
          pbuf_free( rcvbuf );
//          CONSOLE_PRINT( "Received %u bytes\r", bytesTransfered );
//          blinkFast = TRUE;
        }
        while( ferr == 0 );
        if( off > 0 && ferr == 0 )
          ferr = f_write( & file, buf, off, (UINT *) & nb );
///        CONSOLE_PRINT( "\n" );
        //netconn_recved( dataconn, (u32_t) buflen );
        CONSOLE_PRINT( "Received %u bytes\r\n", bytesTransfered );
        dataClose();
        closeTransfer();
        //netbuf_delete( rcvbuf );
      }
    }
  }
  //
  //  MKD - Make Directory
  //
  else if( ! strcmp( command, "MKD" ))
  {
    if( strlen( parameters ) == 0 )
      netconnWriteNl( "501 No directory name" );
    else if( makePath( path ))
    {
      if( fs_exists( path ))
      {
        netconnWrite( "521 \"" );
        netconnWrite( parameters );
        netconnWriteNl( "\" directory already exists" );
      }
      else
      {
        CONSOLE_PRINT(  "Creating directory %s\r\n", parameters );
        if( f_mkdir( path ) == FR_OK )
        {
          netconnWrite( "257 \"" );
          netconnWrite( parameters );
          netconnWriteNl( "\" created" );
        }
        else
        {
          netconnWrite( "550 Can't create \"" );
          netconnWrite( parameters );
          netconnWriteNl( "\"" );
        }
      }
    }
  }
  //
  //  RMD - Remove a Directory
  //
  else if( ! strcmp( command, "RMD" ))
  {
    if( strlen( parameters ) == 0 )
      netconnWriteNl( "501 No directory name" );
    else if( makePath( path ))
    {
      CONSOLE_PRINT(  "Deleting %s\r\n", path );
      if( ! fs_exists( path ))
      {
        netconnWrite( "550 Directory " );
        netconnWrite( parameters );
        netconnWriteNl( " not found" );
      }
      else if( f_unlink( path ) == FR_OK)
      {
        netconnWrite( "250 \"" );
        netconnWrite( parameters );
        netconnWriteNl( "\" removed" );
      }
      else
      {
        netconnWrite( "501 Can't delete \"" );
        netconnWrite( parameters );
        netconnWriteNl( "\"" );
      }
    }
  }
  //
  //  RNFR - Rename From
  //
  else if( ! strcmp( command, "RNFR" ))
  {
    cwdRNFR[ 0 ] = 0;
    if( strlen( parameters ) == 0 )
      netconnWriteNl( "501 No file name" );
    else if( makePath( cwdRNFR ))
    {
      if( ! fs_exists( cwdRNFR ))
      {
        netconnWrite( "550 File " );
        netconnWrite( parameters );
        netconnWriteNl( " not found" );
      }
      else
      {
        CONSOLE_PRINT( "Renaming %s\r\n", cwdRNFR );
        netconnWriteNl( "350 RNFR accepted - file exists, ready for destination" );
      }
    }
  }
  //
  //  RNTO - Rename To
  //
  else if( ! strcmp( command, "RNTO" ))
  {
    char dir[ FTP_CWD_SIZE ];
    if( strlen( cwdRNFR ) == 0 )
      netconnWriteNl( "503 Need RNFR before RNTO" );
    else if( strlen( parameters ) == 0 )
      netconnWriteNl( "501 No file name" );
    else if( makePath( path ))
    {
      if( fs_exists( path ))
      {
        netconnWrite( "553 " );
        netconnWrite( parameters );
        netconnWriteNl( " already exists" );
      }
      else
      {
        strcpy( dir, path );
        char * psep = strrchr( dir, '/' );
        bool fail = psep == NULL;
        if( ! fail )
        {
          if( psep == dir )
            psep ++;
          * psep = 0;
          fail = ! ( fs_exists( dir ) &&
                     ( finfo.fattrib & AM_DIR || ! strcmp( dir, "/")));
          if( fail )
          {
            netconnWrite( "550 \"" );
            netconnWrite( dir );
            netconnWriteNl( "\" is not directory" );
          }
          else
          {
            CONSOLE_PRINT(  "Renaming %s to %s\r\n", cwdRNFR, path );
            if( f_rename( cwdRNFR, path ) == FR_OK )
              netconnWriteNl( "250 File successfully renamed or moved" );
            else
              fail = true;
          }
        }
        if( fail )
          netconnWriteNl( "451 Rename/move failure" );
      }
    }
  }

  ///////////////////////////////////////
  //                                   //
  //   EXTENSIONS COMMANDS (RFC 3659)  //
  //                                   //
  ///////////////////////////////////////

  //
  //  FEAT - New Features
  //
  else if( ! strcmp( command, "FEAT" ))
  {
    netconnWriteNl( "211-Extensions suported:") ;
    // netconnWriteNl( " MDTM\r\n";
    netconnWriteNl( " MLSD" );
    netconnWriteNl( " SIZE" );
    netconnWriteNl( " SITE FREE" );
    netconnWriteNl( "211 End." );
  }
  //
  //  SIZE - Size of the file
  //
  else if( ! strcmp( command, "SIZE" ))
  {
    if( strlen( parameters ) == 0 )
      netconnWriteNl( "501 No file name" );
    else if( makePath( path ))
    {
      if( ! fs_exists( path ) || finfo.fattrib & AM_DIR )
        netconnWriteNl( "550 No such file" );
      else
      {
        netconnWrite( "213 " );
        netconnWriteNl( i2str( finfo.fsize ));
        f_close( & file );
      }
    }
  }
  //
  //  SITE - System command
  //
  else if( ! strcmp( command, "SITE" ))
  {
    if( ! strcmp( parameters, "FREE" ))
    {
      FATFS * fs;
      uint32_t free_clust;
      f_getfree( "0:", & free_clust, & fs );
      netconnWrite( "200 " );
      netconnWrite( i2str( free_clust * fs->csize >> 11 ));
      netconnWrite( " MB free of " );
      netconnWrite( i2str(  (fs->n_fatent - 2) * fs->csize >> 11 ));
      netconnWriteNl( " MB capacity" );
    }
    else
    {
      netconnWrite( "500 Unknow SITE command " );
      netconnWriteNl( parameters );
    }
  }
  //
  //  Unknow command
  //
  else
    netconnWriteNl( "500 Unknow command" );
  return TRUE;
}

// =========================================================
//                       Ftp server
// =========================================================

void ftp_server_service( struct netconn *cn, struct netconn *dscn )
{
  uint16_t dummy;

  // variables initialization
  strcpy( cwdName, "/" );  // Set the root directory
  cwdRNFR[ 0 ] = 0;
  conn = cn;
  datasrvconn = dscn;
  dataconn = NULL;
  cmdStatus = 0;
  dataPassiveConn = false;
  finfo.lfname = lfn;
  finfo.lfsize = _MAX_LFN + 1;

  //  Get the local Ip
  netconn_addr( conn, & ipserver, & dummy );

  netconnWriteNl( "220---   Welcome to FTP Server!   ---" );
///  netconnWriteNl( "220---  for ChibiOs & STM32-E407  ---" );
///  netconnWriteNl( "220---   by Jean-Michel Gallego   ---" );
  netconnWrite( "220 --   Version " );
  netconnWrite( FTP_VERSION );
  netconnWriteNl( "   --" );

  INFO_PRINT( "Client connected!\r\n" );

  //  Wait for user name during 10 seconds
  netconn_set_recvtimeout( conn, 10 * 1000 );
  if( readCommand() < 0 )
    goto close;
  if( strcmp( command, "USER" ))
  {
    netconnWriteNl( "500 Syntax error" );
    goto close;
  }
#ifdef FTP_USER
  if( strcmp( parameters, FTP_USER ))
  {
    netconnWriteNl( "530 " );
    goto close;
  }
#endif  
  netconnWriteNl( "331 OK. Password required" );

  //  Wait for password during 10 seconds
  if( readCommand() < 0 )
    goto close;
  if( strcmp( command, "PASS" ))
  {
    netconnWriteNl( "500 Syntax error" );
    goto close;
  }
#ifdef FTP_PASS
  if( strcmp( parameters, FTP_PASS ))
  {
    netconnWriteNl( "530 " );
    goto close;
  }
#endif  
  netconnWriteNl( "230 OK." );

  //  Wait for user commands
  //  Disconnect if FTP_TIME_OUT minutes of inactivity
  netconn_set_recvtimeout( conn, FTP_TIME_OUT * 1000 );
  while( true )
  {
    err = readCommand();
    if( err == -4 ) // time out
      goto close;
    if( err < 0 )
      goto close;
    if( ! ftp_server_command( command, parameters ))
      goto bye;
  }

  bye:
  netconnWriteNl( "221 Goodbye" );

  close:
  //  Close the connections
  dataClose();
  netconn_close( conn );
  //  Delete the buffer (netconn_recv gives us ownership,
  //    so we have to make sure to deallocate the buffer)
  // netbuf_delete( inbuf );
  INFO_PRINT( "Client disconnected\r\n" );
}
