/**
  ******************************************************************************
  * @file    LwIP/LwIP_HTTP_Server_Netconn_RTOS/Src/httpser-netconn.c
  * @author  MCD Application Team
  * @brief   Basic http server implementation using LwIP netconn API
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2016 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

#include "lwip/opt.h"
#include "lwip/arch.h"
#include "lwip/api.h"
#include "lwip/apps/fs.h"
#include "string.h"
#include "httpserver-netconn.h"
#include "cmsis_os.h"

#include <stdio.h>

#define WEBSERVER_THREAD_PRIO    ( osPriorityAboveNormal )


/**
  * @brief serve tcp connection
  * @param conn: pointer on connection structure
  * @retval None
  */
static void http_server_serve(struct netconn *conn)
{
  struct netbuf *inbuf;
  err_t recv_err;
  char* buf;
  u16_t buflen;
  struct fs_file file;

  /* Read the data from the port, blocking if nothing yet there.
   We assume the request (the part we care about) is in one netbuf */
  recv_err = netconn_recv(conn, &inbuf);

  if (recv_err == ERR_OK)
  {
    if (netconn_err(conn) == ERR_OK)
    {
      netbuf_data(inbuf, (void**)&buf, &buflen);

      /* Is this an HTTP GET command? (only check the first 5 chars, since
      there are other formats for GET, and we're keeping it very simple )*/
      if ((buflen >=5) && (strncmp(buf, "GET /", 5) == 0))
      {
        if(strncmp(buf, "GET /gateway.html", 17) == 0)
        {
          /* Load gateway page */
          fs_open(&file, "/gateway.html");
          netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_NOCOPY);
          fs_close(&file);
        }
        else if(strncmp(buf, "GET /Fs/config.json", 19) == 0)
        {
           /* Load dynamic page */
           DynJson(conn);
        }
        else
        {
          /* Load Error page */
          fs_open(&file, "/404.html");
          netconn_write(conn, (const unsigned char*)(file.data), (size_t)file.len, NETCONN_NOCOPY);
          fs_close(&file);
        }
      }
      /* POST segment */
      else if ((buflen >= 6) && (strncmp(buf, "POST /", 6) == 0))
      {
        if (strncmp((char const *)buf,"POST /save", 10) == 0)
        {
        	portCHAR ipAddress[16], netmask[16], port[16];
        	memset(ipAddress, 0, 16);
        	memset(netmask, 0, 16);
        	memset(port, 0, 16);

        	char *retstart_index = strstr((const char *) buf, (const char *) "txt_ipAddress=");
        	char *retend_index = strstr((const char *) retstart_index, (const char *) "&");
        	/* The value 14 implies the length of the string, namely txt_ipAddress= */
        	strncpy((char *)ipAddress, (retstart_index + 14), (int)(retend_index-(retstart_index + 14)));

        	/* Skip the char '&' */
        	retstart_index = strstr((const char *) (retend_index + 1), (const char *) "txt_netmask=");
        	retend_index = strstr((const char *) retstart_index, (const char *) "&");
        	/* The value 12 implies the length of the string, namely txt_netmask= */
        	strncpy((char *)netmask, (retstart_index + 12), (int)(retend_index-(retstart_index + 12)));

        	/* Skip the char '&' */
        	retstart_index = strstr((const char *) (retend_index + 1), (const char *) "txt_port=");
			    retend_index = strstr((const char *) retstart_index, (const char *) "&");
			    /* The value 9 implies the length of the string, namely txt_port= */
			    strncpy((char *)port, (retstart_index + 9), (int)(retend_index-(retstart_index + 9)));

			    /* Obtaining the ipAddress, netmask and port*/
        	printf("%p", ipAddress);
        	printf("%p", netmask);
        	printf("%p", port);

        	/* Displaying alert*/
        	DynRedirect(conn);
        }
      }
    }
  }
  /* Close the connection (server closes in HTTP) */
  netconn_close(conn);

  /* Delete the buffer (netconn_recv gives us ownership,
   so we have to make sure to deallocate the buffer) */
  netbuf_delete(inbuf);
}


/**
  * @brief  http server thread
  * @param arg: pointer on argument(not used here)
  * @retval None
  */
static void http_server_netconn_thread(void *arg)
{
  struct netconn *conn, *newconn;
  err_t err, accept_err;

  /* Create a new TCP connection handle */
  conn = netconn_new(NETCONN_TCP);

  if (conn!= NULL)
  {
    /* Bind to port 80 (HTTP) with default IP address */
    err = netconn_bind(conn, NULL, 80);

    if (err == ERR_OK)
    {
      /* Put the connection into LISTEN state */
      netconn_listen(conn);

      while(1)
      {
        /* accept any icoming connection */
        accept_err = netconn_accept(conn, &newconn);
        if(accept_err == ERR_OK)
        {
          /* serve connection */
          http_server_serve(newconn);

          /* delete connection */
          netconn_delete(newconn);
        }
      }
    }
  }
}

/**
  * @brief  Initialize the HTTP server (start its thread)
  * @param  none
  * @retval None
  */
void http_server_netconn_init()
{
  sys_thread_new("HTTP", http_server_netconn_thread, NULL, DEFAULT_THREAD_STACKSIZE, WEBSERVER_THREAD_PRIO);
}

/**
  * @brief  Create and send a dynamic Json file.
  * @param  conn pointer on connection structure
  * @retval None
  */
void DynJson(struct netconn *conn)
{
  portCHAR PAGE_BODY[512];
  memset(PAGE_BODY, 0, 512);

  /* Generate the json from data */
  strcat((char *)PAGE_BODY, "{\"gatewayName\":\"");
  strcat((char *)PAGE_BODY, (char *) "DHCP");
  strcat((char *)PAGE_BODY, (char *) "\",\"ipAddress\":\"");
  strcat((char *)PAGE_BODY, (char *) "192.168.1.101");
  strcat((char *)PAGE_BODY, (char *) "\",\"netmask\":\"");
  strcat((char *)PAGE_BODY, (char *) "255.255.255.0");
  strcat((char *)PAGE_BODY, (char *) "\",\"port\":\"");
  strcat((char *)PAGE_BODY, (char *) "80");
  strcat((char *)PAGE_BODY, (char *) "\"}");

  /* Send the dynamically generated json */
  netconn_write(conn, PAGE_BODY, strlen(PAGE_BODY), NETCONN_COPY);
}

/**
  * @brief  Create and send a dynamic Json file.
  * @param  conn pointer on connection structure
  * @retval None
  */
void DynRedirect(struct netconn *conn)
{
  portCHAR PAGE_BODY[250];
  memset(PAGE_BODY, 0, 250);

  /* Generate the redirection */
  strcat((char *)PAGE_BODY, "<!DOCTYPE html>");
  strcat((char *)PAGE_BODY, "<html lang=\"en\">");
  strcat((char *)PAGE_BODY, "<head>");
  strcat((char *)PAGE_BODY, "    <meta charset=\"utf-8\">");
  strcat((char *)PAGE_BODY, "    <title></title>");
  strcat((char *)PAGE_BODY, "</head>");
  strcat((char *)PAGE_BODY, "<body>");
  strcat((char *)PAGE_BODY, "</body>");
  strcat((char *)PAGE_BODY, "<script>");
  strcat((char *)PAGE_BODY, "    alert('ok');");
  strcat((char *)PAGE_BODY, "    window.location = '/gateway.html';");
  strcat((char *)PAGE_BODY, "</script>");
  strcat((char *)PAGE_BODY, "</html>");

  /* Send the dynamically generated json */
  netconn_write(conn, PAGE_BODY, strlen(PAGE_BODY), NETCONN_COPY);
}
