/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2019 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                             www.st.com/SLA0044
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */     
#include "fatfs.h"
#include "lwip.h"
#include "usb_host.h"
#include "stm32f4xx_nucleo_144.h"
#include "lwip/api.h"
#include "lwip/prot/dhcp.h"
#include "term_io.h"

#include "ftpd.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
   
/* USER CODE END FunctionPrototypes */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];
  
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}                   
/* USER CODE END GET_IDLE_TASK_MEMORY */

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
extern struct netif gnetif;

/*
fragmenty kodu dla serwera WWW
opracowano na podstawie dostepnych przykladow
*/
void displayOwnIp(void)
{
  xprintf(
    "My IP: %d.%d.%d.%d\n",
    ip4_addr1_16(netif_ip4_addr(&gnetif)),
    ip4_addr2_16(netif_ip4_addr(&gnetif)),
    ip4_addr3_16(netif_ip4_addr(&gnetif)),
    ip4_addr4_16(netif_ip4_addr(&gnetif))
    );
}

static char response[500];

//based on available code examples
static void http_server_serve(struct netconn *conn)
{
  struct netbuf *inbuf;
  err_t recv_err;
  char* buf;
  u16_t buflen;

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
        response[0] = 0;

        strcpy(response, "HTTP/1.1 200 OK\r\n\
          Content-Type: text/html\r\n\
          Connnection: close\r\n\r\n\
          <!DOCTYPE HTML>\r\n");

        strcat(response,"<title>Prosta strona WWW</title>");
        strcat(response,"<h1>H1 Header</h1>");

        strcat(response,"A to jest tekst na stronie");
          netconn_write(conn, response, sizeof(response), NETCONN_NOCOPY);
      }
    }
  }
  /* Close the connection (server closes in HTTP) */
  netconn_close(conn);

  /* Delete the buffer (netconn_recv gives us ownership,
   so we have to make sure to deallocate the buffer) */
  netbuf_delete(inbuf);
}


//based on available code examples
static void http_server_netconn_thread(void const *arg)
{
  struct netconn *conn, *newconn;
  err_t err, accept_err;

  xprintf("http_server_netconn_thread\n");

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
     
 void StartDefaultTask(void const * argument)
{
	 // Glowny task aplikacji
 /* init code for USB_HOST */
   MX_USB_HOST_Init();

  /* init code for FATFS */
  MX_FATFS_Init();

  /* init code for LWIP */
  MX_LWIP_Init();



  xprintf("DefaultTask!\n\r");

  /* Infinite loop */

  xprintf("Obtaining address with DHCP...\n");
  struct dhcp *dhcp = netif_dhcp_data(&gnetif);
  u8_t lState = 0;
  do {
	  if (dhcp->state != lState) {
		  lState = dhcp->state;
		  xprintf("State changed to: %02X\r\n", lState);
		  switch (dhcp->state) {
		  	  case DHCP_STATE_OFF:
		  		  xprintf("DHCP_STATE_OFF");
		  		  break;
		  	case DHCP_STATE_REQUESTING:
				  xprintf("DHCP_STATE_REQUESTING");
				  break;
		  	case DHCP_STATE_INIT:
				  xprintf("DHCP_STATE_INIT");
				  break;
		  	case DHCP_STATE_REBOOTING:
				  xprintf("DHCP_STATE_REBOOTING");
				  break;
		  	case DHCP_STATE_REBINDING:
				  xprintf("DHCP_STATE_REBINDING");
				  break;
		  	case DHCP_STATE_RENEWING:
				  xprintf("DHCP_STATE_RENEWING");
				  break;
		  	case DHCP_STATE_SELECTING:
				  xprintf("DHCP_STATE_SELECTING");
				  break;
		  	case DHCP_STATE_INFORMING:
				  xprintf("DHCP_STATE_INFORMING");
				  break;
		  	case DHCP_STATE_CHECKING:
				  xprintf("DHCP_STATE_CHECKING");
				  break;
		  	case DHCP_STATE_PERMANENT:
				  xprintf("DHCP_STATE_PERMANENT");
				  break;
		  	case DHCP_STATE_BOUND:
				  xprintf("DHCP_STATE_BOUND");
				  break;
		  	case DHCP_STATE_RELEASING:
				  xprintf("DHCP_STATE_RELEASING");
				  break;
		  	case DHCP_STATE_BACKING_OFF:
				  xprintf("DHCP_STATE_BACKING_OFF");
				  break;
		  }
		  xprintf("\r\n");
	  }

	  vTaskDelay(250);
  } while(dhcp->state != DHCP_STATE_BOUND);
  xprintf("DHCP bound\n");
  displayOwnIp();
//  extern ApplicationTypeDef Appli_state;
//
//  while(Appli_state != APPLICATION_READY);
//  FIL file;
//  char buffer[32];
//  uint signs_read;
//
//  puts("File contents:\n");
//  f_open(&file, "0:/test.txt", FA_READ);
//  while(f_eof(&file) == 0) {
//	  f_read(&file, buffer, 32, &signs_read);
//	  printf("%.*s", signs_read, buffer);
//  }
//  f_close(&file);
//  puts("\n");

  ftpd_init();


  for(;;)
  {
//	  BSP_LED_Toggle(LED_GREEN);
//	  BSP_LED_Toggle(LED_RED);
//	  BSP_LED_Toggle(LED_BLUE);
    osDelay(200);
  }
}

/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
