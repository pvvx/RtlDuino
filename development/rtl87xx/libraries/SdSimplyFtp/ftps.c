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

#include "Arduino.h"
#include "cmsis_os.h"

#include "ftps.h"

#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"


//  Stack area for the ftp server thread.
//WORKING_AREA( wa_ftp_server, FTPS_THREAD_STACK_SIZE );

//  HTTP server thread.
// osThreadId ftp_tid = 0;

void ftp_server(const void *p)
{
  struct netconn *ftpsrvconn, *newconn, *datsrvconn;

//  (void) p;
//  chRegSetThreadName( "ftpserver" );

  INFO_PRINT("Start ftp server...\r\n");

  //  Create new TCP connections handle
  ftpsrvconn = netconn_new( NETCONN_TCP );
  LWIP_ERROR( "ftp_server: invalid conn", ( ftpsrvconn != NULL ), return; );
  datsrvconn = netconn_new( NETCONN_TCP );
  LWIP_ERROR( "ftp_server: invalid conn", ( datsrvconn != NULL ), return; );

  //  Bind ftpsrvconn to port 21 (FTP_SERV_PORT) with default IP address
  //    and put the connection into LISTEN state
  netconn_bind( ftpsrvconn, IP_ADDR_ANY, FTP_SERV_PORT );
  netconn_listen( ftpsrvconn );

  //  Bind datasrvconn to port FTP_DATA_PORT with default IP address
  //    and put the connection into LISTEN state
  netconn_bind( datsrvconn, IP_ADDR_ANY, FTP_DATA_PORT );
  netconn_listen( datsrvconn );

  //  Goes to the final priority after initialization
//  chThdSetPriority( FTPS_THREAD_PRIORITY );

  while( TRUE )
  {
    if( netconn_accept( ftpsrvconn, &newconn ) == ERR_OK )
    {
      ftp_server_service( newconn, datsrvconn );
      netconn_delete( newconn );
    }
  }
}
