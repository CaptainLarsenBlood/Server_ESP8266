#include "wifi.h"
//------------------------------------------------
wifi_state_cb_t on_station_connect = NULL;
wifi_disco_cb_t on_station_disconnect = NULL;
wifi_state_cb_t on_station_first_connect = NULL;
wifi_state_cb_t on_client_connect = NULL;
wifi_state_cb_t on_client_disconnect = NULL;
//------------------------------------------------
volatile bool wifi_station_is_connected = false;
volatile bool wifi_station_static_ip = false;
//------------------------------------------------
const char http_header[] = {"HTTP/1.1 200 OK\r\nServer: nginx\r\nContent-Type: text/html\r\nConnection: keep-alive\r\n\r\n"};
//------------------------------------------------
const uint8_t index_htm[] = {
0x3c,0x68,0x74,0x6d,0x6c,0x3e,0x3c,0x62,0x6f,0x64,0x79,0x3e,0x3c,0x68,0x31,0x20,
0x73,0x74,0x79,0x6c,0x65,0x3d,0x22,0x74,0x65,0x78,0x74,0x2d,0x61,0x6c,0x69,0x67,
0x6e,0x3a,0x20,0x63,0x65,0x6e,0x74,0x65,0x72,0x3b,0x22,0x3e,0x45,0x73,0x70,0x20,
0x38,0x32,0x36,0x36,0x3c,0x62,0x72,0x3e,0x3c,0x62,0x72,0x3e,0x48,0x54,0x54,0x50,
0x20,0x53,0x65,0x72,0x76,0x65,0x72,0x3c,0x2f,0x68,0x31,0x3e,0x0d,0x0a,0x3c,0x70,
0x3e,0x3c,0x73,0x70,0x61,0x6e,0x20,0x73,0x74,0x79,0x6c,0x65,0x3d,0x22,0x66,0x6f,
0x6e,0x74,0x2d,0x66,0x61,0x6d,0x69,0x6c,0x79,0x3a,0x20,0x54,0x69,0x6d,0x65,0x73,
0x20,0x4e,0x65,0x77,0x20,0x52,0x6f,0x6d,0x61,0x6e,0x2c,0x54,0x69,0x6d,0x65,0x73,
0x2c,0x73,0x65,0x72,0x69,0x66,0x3b,0x22,0x3e,0x57,0x69,0x2d,0x46,0x69,0x20,0x53,
0x6f,0x43,0x3c,0x2f,0x73,0x70,0x61,0x6e,0x3e,0x20,0x3c,0x2f,0x70,0x3e,0x0d,0x0a,
0x3c,0x2f,0x62,0x6f,0x64,0x79,0x3e,0x3c,0x2f,0x68,0x74,0x6d,0x6c,0x3e};
//------------------------------------------------
static os_timer_t timer;
//------------------------------------------------
#define GPIO_LED 2
//------------------------------------------------
void ICACHE_FLASH_ATTR tcp_task(void *pvParameters)
{
  portTickType xLastWakeTime;
  char str1[21];
  char addr_str[25];
  int buflen = 512;
  uint8_t buf[512] = {};
  int sockfd, accept_sock, ret;
  socklen_t sockaddrsize;
  os_printf("Create socket...\n");
  struct sockaddr_in servaddr, cliaddr, remotehost;
  xTaskHandle recv_handle = NULL;
  if ( (sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP)) < 0 ) {
    os_printf("socket not created\n");
    vTaskDelete(NULL);
  }
  memset(&servaddr, 0, sizeof(servaddr));
  memset(&cliaddr, 0, sizeof(cliaddr));
  //Заполнение информации о сервере
  servaddr.sin_family    = AF_INET; // IPv4
  servaddr.sin_addr.s_addr = INADDR_ANY;
  servaddr.sin_port = htons(SERVER_PORT);
  //Свяжем сокет с адресом сервера
  if (bind(sockfd, (const struct sockaddr *)&servaddr,  sizeof(struct sockaddr_in)) < 0 )
  {
    os_printf("socket not binded\n");
    vTaskDelete(NULL);
  }
  os_printf("socket binded\n");
  listen(sockfd, 1);
  for(;;)
  {
    accept_sock = accept(sockfd, (struct sockaddr *)&cliaddr, (socklen_t *)&sockaddrsize);
    os_printf(" socket: %d\n", accept_sock);
    if(accept_sock >= 0)
    {
      ret = recvfrom(accept_sock, buf, buflen, 0, (struct sockaddr *)&remotehost, &sockaddrsize);
      if(ret > 0)
      {
        if ((ret >=5 ) && (strncmp(buf, "GET /", 5) == 0))
        {
          if ((strncmp((char const *)buf,"GET / ",6)==0)||(strncmp((char const *)buf,"GET /index.html",15)==0))
          {
            strcpy((char*)buf,http_header);
            memcpy((void*)(buf + strlen(http_header)),(void*)index_htm,sizeof(index_htm));
            write(accept_sock, (const unsigned char*)buf, strlen(http_header) + sizeof(index_htm));
          }
        }
      }
      close(accept_sock);
    }
  }
  close(sockfd);
  vTaskDelete(NULL);
}
//------------------------------------------------
void ICACHE_FLASH_ATTR wifi_event_handler_cb(System_Event_t *event)
{
  static bool station_was_connected = false;
  if (event == NULL)
  {
      return;
  }
  os_printf("[WiFi] event %u\n", event->event_id);
  switch (event->event_id)
  {
    case EVENT_STAMODE_DISCONNECTED:
      wifi_station_is_connected = false;
      GPIO_OUTPUT_SET(GPIO_LED, 1);
      Event_StaMode_Disconnected_t *ev = (Event_StaMode_Disconnected_t *)&event->event_info;
      if(on_station_disconnect)
      {
          on_station_disconnect(ev->reason);
      }
      break;
    case EVENT_STAMODE_CONNECTED:
      if(wifi_station_static_ip)
      {
          os_printf("STA IP is static\n");
      }
      wifi_station_is_connected = true;
      if(!station_was_connected)
      {
        station_was_connected = true;
        if(on_station_first_connect)
        {
            on_station_first_connect();
        }
      }
      if(on_station_connect){
          on_station_connect();
      }
      break;
    case EVENT_STAMODE_DHCP_TIMEOUT:
      if(wifi_station_is_connected)
      {
          wifi_station_is_connected = false;
          if(on_station_disconnect)
          {
              on_station_disconnect(REASON_UNSPECIFIED);
              if(on_station_first_connect)
              {
                  on_station_first_connect();
              }
          }
          if(on_station_connect){
              on_station_connect();
          }
      }
      break;
    case EVENT_STAMODE_GOT_IP:
      wifi_station_is_connected = true;
      GPIO_OUTPUT_SET(GPIO_LED, 0);
      xTaskCreate(tcp_task, "tcp_task", 4096, NULL, 5, NULL);
      if(!station_was_connected)
      {
          station_was_connected = true;
      }
      break;
    case EVENT_SOFTAPMODE_STACONNECTED:
      if(on_client_connect){
          on_client_connect();
      }
      break;
    case EVENT_SOFTAPMODE_STADISCONNECTED:
      if(on_client_disconnect)
      {
          on_client_disconnect();
      }
      break;
  default:
      break;
  }
}
//------------------------------------------------
bool ICACHE_FLASH_ATTR wifi_station_connected()
{
    if(!wifi_station_is_connected){
        return false;
    }
    WIFI_MODE mode = wifi_get_opmode();
    if((mode & STATION_MODE) == 0){
        return false;
    }
    STATION_STATUS wifistate = wifi_station_get_connect_status();
    wifi_station_is_connected = (wifistate == STATION_GOT_IP ||
        (wifi_station_static_ip && wifistate == STATION_CONNECTING));
    return wifi_station_is_connected;
}
//------------------------------------------------
LOCAL void ICACHE_FLASH_ATTR wait_for_connection_ready(uint8 flag)
{
    os_timer_disarm(&timer);
    if(wifi_station_connected()) {
        os_printf("connected\n");
    } else {
      os_printf("reconnect after 2s\n");
      os_timer_setfn(&timer, (os_timer_func_t *)wait_for_connection_ready, NULL);
      os_timer_arm(&timer, 2000, 0);
    }
}
//------------------------------------------------
void ICACHE_FLASH_ATTR set_on_station_connect(wifi_state_cb_t cb)
{
    on_station_connect = cb;
}
//----------------------------------------------------------------
void ICACHE_FLASH_ATTR set_on_station_disconnect(wifi_disco_cb_t cb)
{
    on_station_disconnect = cb;
}
//------------------------------------------------
void ICACHE_FLASH_ATTR set_on_client_connect(wifi_state_cb_t cb){
    on_client_connect = cb;
}
//------------------------------------------------
void ICACHE_FLASH_ATTR set_on_client_disconnect(wifi_state_cb_t cb)
{
    on_client_disconnect = cb;
}
//------------------------------------------------
LOCAL void ICACHE_FLASH_ATTR on_wifi_connect()
{
    os_printf("on_wifi_connect();\n");
    os_timer_disarm(&timer);
    os_timer_setfn(&timer, (os_timer_func_t *)wait_for_connection_ready, NULL);
    os_timer_arm(&timer, 100, 0);
}
//------------------------------------------------
LOCAL void ICACHE_FLASH_ATTR on_wifi_disconnect(uint8_t reason)
{
    os_printf("disconnect %d\n", reason);
}
//------------------------------------------------
LOCAL void ICACHE_FLASH_ATTR on_wifi_client_connect()
{
    os_printf("on_wifi_client_connect();\n");
    uint32 station_num = wifi_softap_get_station_num();
    os_printf("number station: %d\n", station_num);
    GPIO_OUTPUT_SET(GPIO_LED, 0);
}
//------------------------------------------------
LOCAL void ICACHE_FLASH_ATTR on_wifi_client_disconnect()
{
    os_printf("client disconnect\n");
    uint32 station_num = wifi_softap_get_station_num();
    os_printf("number station: %d\n", station_num);
    if  (!station_num) GPIO_OUTPUT_SET(GPIO_LED, 1);
}
//------------------------------------------------
bool ICACHE_FLASH_ATTR wifi_set_mode(WIFI_MODE mode)
{
    if(!mode){
        bool s = wifi_set_opmode(mode);
        wifi_fpm_open();
        wifi_fpm_set_sleep_type(MODEM_SLEEP_T);
        wifi_fpm_do_sleep(0xFFFFFFFF);
        return s;
    }
    wifi_fpm_close();
    return wifi_set_opmode(mode);
}
//------------------------------------------------
bool ICACHE_FLASH_ATTR start_wifi_station(const char * ssid, const char * pass)
{
    WIFI_MODE mode = wifi_get_opmode();
    if((mode & STATION_MODE) == 0)
    {
        mode |= STATION_MODE;
        if(!wifi_set_mode(mode))
        {
            os_printf("Failed to enable Station mode!\n");
            return false;
        }
    }
    if(!ssid)
    {
        os_printf("No SSID Given. Will connect to the station saved in flash\n");
        return true;
    }
    struct station_config config;
    memset(&config, 0, sizeof(struct station_config));
    strcpy(config.ssid, ssid);
    if(pass)
    {
        strcpy(config.password, pass);
    }
    if(!wifi_station_set_config(&config)){
        os_printf("Failed to set Station config!\n");
        return false;
    }
    wifi_set_sleep_type(NONE_SLEEP_T);
    if(!wifi_station_dhcpc_status()) {
        os_printf("DHCP is not started. Starting it...\n");
        if(!wifi_station_dhcpc_start()) {
            os_printf("DHCP start failed!\n");
            return false;
        }
    }
    if(wifi_get_phy_mode() != PHY_MODE_11N)
      wifi_set_phy_mode(PHY_MODE_11N);
    return wifi_station_connect();
}
//------------------------------------------------
bool ICACHE_FLASH_ATTR start_wifi_ap(const char * ssid, const char * pass)
{
  struct ip_info ipinfo;
  uint32_t ip;
  char ip_char[15];
  uint8 macadr[6];
  if(!wifi_set_mode(SOFTAP_MODE))
  {
      os_printf("Failed to enable Access Point mode!\n");
      return false;
  }
  struct softap_config *config = (struct softap_config *)zalloc(sizeof(struct softap_config));
  wifi_get_macaddr(SOFTAP_IF, macadr);
  wifi_softap_get_config(config);
  memset(config->ssid, 0, sizeof(config->ssid));
  sprintf(config->ssid, WIFI_APSSID);
  memset(config->password, 0, sizeof(config->password));
  sprintf(config->password, WIFI_APPASSWORD);
  if (wifi_get_opmode() == SOFTAP_MODE)
  {
    os_printf("SOFTAP_MODE\n");
    config->authmode = AUTH_WPA_WPA2_PSK;
    config->channel = 7;
    config->max_connection = 255;
    config->ssid_hidden = 0;
    wifi_softap_set_config(config);
  }
  if (wifi_get_opmode() == SOFTAP_MODE)
  {
    wifi_softap_get_config(config);
    os_printf("OPMODE: %u, SSID: %s, PASSWORD: %s, CHANNEL: %d, AUTHMODE: %d, MACADDRESS: %02x:%02x:%02x:%02x:%02x:%02x\n",
          wifi_get_opmode(),
          config->ssid,
          config->password,
          config->channel,
          config->authmode,
          macadr[0],macadr[1],macadr[2],macadr[3],macadr[4],macadr[5]);
  }
  free(config);
  wifi_set_sleep_type(NONE_SLEEP_T);
  wifi_softap_dhcps_stop();
  snprintf(ip_char, sizeof(ip_char), "%s", WIFI_AP_IP);
  ip = ipaddr_addr(ip_char);
  memcpy(&ipinfo.ip, &ip, 4);
  snprintf(ip_char, sizeof(ip_char), "%s", WIFI_AP_GW);
  ip = ipaddr_addr(ip_char);
  memcpy(&ipinfo.gw, &ip, 4);
  snprintf(ip_char, sizeof(ip_char), "%s", WIFI_AP_NETMASK);
  ip = ipaddr_addr(ip_char);
  memcpy(&ipinfo.netmask, &ip, 4);
  wifi_set_ip_info(SOFTAP_IF, &ipinfo);
  struct dhcps_lease dhcp_lease;
  snprintf(ip_char, sizeof(ip_char), "%s", WIFI_AP_IP_CLIENT_START);
  ip = ipaddr_addr(ip_char);
  memcpy(&dhcp_lease.start_ip, &ip, 4);
  snprintf(ip_char, sizeof(ip_char), "%s", WIFI_AP_IP_CLIENT_END);
  ip = ipaddr_addr(ip_char);
  memcpy(&dhcp_lease.end_ip, &ip, 4);
  wifi_softap_set_dhcps_lease(&dhcp_lease);
  wifi_softap_dhcps_start();
  if(wifi_get_phy_mode() != PHY_MODE_11N)
    wifi_set_phy_mode(PHY_MODE_11N);
  return true;
}
//------------------------------------------------
bool ICACHE_FLASH_ATTR stop_wifi_ap()
{
    WIFI_MODE mode = wifi_get_opmode();
    mode &= ~SOFTAP_MODE;
    if(!wifi_set_mode(mode))
    {
        os_printf("Failed to disable AP mode!\n");
        return false;
    }
    return true;
}
//------------------------------------------------
bool ICACHE_FLASH_ATTR stop_wifi_station()
{
    WIFI_MODE mode = wifi_get_opmode();
    mode &= ~STATION_MODE;
    if(!wifi_set_mode(mode))
    {
        os_printf("Failed to disable STA mode!\n");
        return false;
    }
    return true;
}
//------------------------------------------------
WIFI_MODE ICACHE_FLASH_ATTR init_esp_wifi()
{
  //set_on_client_connect(on_wifi_client_connect);
  //set_on_client_disconnect(on_wifi_client_disconnect);
  set_on_station_connect(on_wifi_connect);
  set_on_station_disconnect(on_wifi_disconnect);
  wifi_set_event_handler_cb(wifi_event_handler_cb);
  WIFI_MODE mode = wifi_get_opmode_default();
  wifi_set_mode(mode);
  //stop_wifi_station();
  stop_wifi_ap();
  return mode;
}
//------------------------------------------------
