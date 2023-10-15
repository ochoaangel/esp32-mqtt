#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "mqtt_client.h"
#include "cJSON.h"
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Datos Wifi
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define SSID_NAME "SHAW-EEAD50"
#define SSID_PASS "0MGSM253KPJ9"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Datos MQTT
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define MQTT_BROKER_URI "mqtt://192.168.0.4:1883"
#define MQTT_CLIENT_ID "bomba"
#define MQTT_TOPIC_ESP_SUBSCRIPTION "tanque"
#define MQTT_TOPIC_BROKER_DESTINATION "server"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Keys importantes en el json enviado por mqtt, siempre en minusculas, node lo pone en minuscula, si no, se desborda memoria
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define MQTT_JSON_KEY_SUBTOPIC "subtopic"
#define MQTT_JSON_KEY_DESTINATION "destination"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// subTopicos ó acciones
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define MQTT_SUBTOPIC_ALL "all"
#define MQTT_SUBTOPIC_RESET_PLC "resetPlc"
#define MQTT_SUBTOPIC_NIVEL "nivel"
#define MQTT_SUBTOPIC_MOTORES_TEMPERATURAS "motoresTemperaturas"
#define MQTT_SUBTOPIC_MOTORES_SHOW_ON_OFF "motoresShowOnOff"
#define MQTT_SUBTOPIC_ENTRA_AGUA_CALLE "entraAguaCalle"
#define MQTT_SUBTOPIC_ENTRA_AGUA_TANQUE "entraAguaTanque"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// variables
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int retry_num = 0;
esp_mqtt_client_handle_t mqtt_client;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Funciones de acciones
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static void hacer_todo(void)
{
  printf("---Ejecutando la función hacer_todo()\n");
}

static void resetear_plc(void)
{
  printf("---Ejecutando la función resetear_plc()\n");
}

static void medir_nivel_tanque(void)
{
  printf("---Ejecutando la función medir_nivel_tanque()\n");
}

static void medir_temperaturas_motores(void)
{
  printf("---Ejecutando la función medir_temperaturas_motores()\n");
}

static void mostrar_motores_on_off(void)
{
  printf("---Ejecutando la función mostrar_motores_on_off()\n");
}

static void entra_agua_calle(void)
{
  printf("---Ejecutando la función entra_agua_calle()\n");
}

static void entra_agua_tanque(void)
{
  printf("---Ejecutando la función entra_agua_tanque()\n");
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// funciones generales
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
  esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

  switch (event->event_id)
  {
  case MQTT_EVENT_CONNECTED:
    printf("=====> MQTT ESP conectado al server\n");
    esp_mqtt_client_subscribe(event->client, MQTT_TOPIC_ESP_SUBSCRIPTION, 0);
    break;

  case MQTT_EVENT_DISCONNECTED:
    printf("=====> MQTT ESP desconectado del server\n");
    break;

  case MQTT_EVENT_DATA:
    const char *data = event->data;
    int data_len = event->data_len;
    printf("=====> MQTT ESP Transmited in/out: %.*s\n", data_len, data);
    cJSON *jsonData = cJSON_Parse(data);
    if (jsonData != NULL)
    {
      cJSON *destination = cJSON_GetObjectItemCaseSensitive(jsonData, "destination");
      if (cJSON_IsString(destination) && (destination->valuestring != NULL))
      {

        if (strcmp(destination->valuestring, MQTT_TOPIC_ESP_SUBSCRIPTION) == 0)
        {
          // acciones a ejecutar cuando recibo un evento en el tópico al que estoy suscrito
          cJSON *subTopic = cJSON_GetObjectItemCaseSensitive(jsonData, MQTT_JSON_KEY_SUBTOPIC);

          // creando una acción para cada "#define" o subTopico
          if (strcmp(subTopic->valuestring, MQTT_SUBTOPIC_ALL) == 0)
          {
            hacer_todo();
          }
          else if (strcmp(subTopic->valuestring, MQTT_SUBTOPIC_RESET_PLC) == 0)
          {
            resetear_plc();
          }
          else if (strcmp(subTopic->valuestring, MQTT_SUBTOPIC_NIVEL) == 0)
          {
            medir_nivel_tanque();
          }
          else if (strcmp(subTopic->valuestring, MQTT_SUBTOPIC_MOTORES_TEMPERATURAS) == 0)
          {
            medir_temperaturas_motores();
          }
          else if (strcmp(subTopic->valuestring, MQTT_SUBTOPIC_MOTORES_SHOW_ON_OFF) == 0)
          {
            mostrar_motores_on_off();
          }
          else if (strcmp(subTopic->valuestring, MQTT_SUBTOPIC_ENTRA_AGUA_CALLE) == 0)
          {
            entra_agua_calle();
          }
          else if (strcmp(subTopic->valuestring, MQTT_SUBTOPIC_ENTRA_AGUA_TANQUE) == 0)
          {
            entra_agua_tanque();
          }
          else
          {
            // No se encontró un subtopic coincidente
            printf("No se encuentra acciones para ese subtopic: %s\n", subTopic->valuestring);
          }
        }
      }
      else
      {
        printf("No se encontró la llave 'destination' o no es un valor de cadena válido\n");
      }
      cJSON_Delete(jsonData);
    }
    else
    {
      printf("Error al analizar el JSON\n");
    }
    break;

  default:
    break;
  }
}

static void mqtt_app_start(void)
{
  esp_mqtt_client_config_t mqtt_cfg = {.broker.address.uri = MQTT_BROKER_URI, .credentials.client_id = MQTT_CLIENT_ID};
  mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
  esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, mqtt_client);
  esp_mqtt_client_subscribe(mqtt_client, MQTT_TOPIC_ESP_SUBSCRIPTION, 0);
  esp_mqtt_client_start(mqtt_client);
}

static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  if (event_id == WIFI_EVENT_STA_START)
  {
    printf("=====>  CONNECTANDO AL WIFI....\n");
  }
  else if (event_id == WIFI_EVENT_STA_CONNECTED)
  {
    printf("=====>  YA CONNECTADO AL WIFI ==LISTO==\n");
  }
  else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
  {
    printf("=====>  Perdió la conexión al wifi\n");
    if (retry_num < 5)
    {
      esp_wifi_connect();
      retry_num++;
      printf("=====> Intentando conectar al Wifi...\n");
    }
  }
  else if (event_id == IP_EVENT_STA_GOT_IP)
  {
    printf("=====>  IP obtenido...\n\n");
  }
}

void wifi_connection()
{
  esp_netif_init();
  esp_event_loop_create_default();
  esp_netif_create_default_wifi_sta();
  wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&wifi_initiation);
  esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
  esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);
  wifi_config_t wifi_configuration = {.sta = { .ssid = SSID_NAME, .password = SSID_PASS }};
  strcpy((char *)wifi_configuration.sta.ssid, SSID_NAME);
  strcpy((char *)wifi_configuration.sta.password, SSID_PASS);
  esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration);
  esp_wifi_start();
  esp_wifi_set_mode(WIFI_MODE_STA);
  printf("=====> Conectado al WIFI con SSID:%s  password:%s", SSID_NAME, SSID_PASS);
  esp_wifi_connect();
}

void app_main(void)
{
  nvs_flash_init();
  wifi_connection();
  mqtt_app_start();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// #include <stdio.h>
// #include <string.h>
// #include "freertos/FreeRTOS.h" 
// #include "esp_system.h"
// #include "esp_wifi.h"
// #include "esp_log.h"
// #include "esp_event.h"
// #include "nvs_flash.h"
// #include "lwip/err.h"
// #include "lwip/sys.h"
// #include "mqtt_client.h"
// #include "cJSON.h"
// #include <stdio.h>
// #include <string.h>
// #include "freertos/FreeRTOS.h"
// #include "esp_system.h"
// #include "esp_wifi.h"
// #include "esp_log.h"
// #include "esp_event.h"
// #include "nvs_flash.h"
// #include "lwip/err.h"
// #include "lwip/sys.h"
// #include "mqtt_client.h"
// #include "cJSON.h"
// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// // Datos Wifi
// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// #define SSID_NAME "SHAW-EEAD50"
// #define SSID_PASS "0MGSM253KPJ9"

// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// // Datos MQTT
// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// #define MQTT_BROKER_URI "mqtt://192.168.0.4:1883"
// #define MQTT_CLIENT_ID "bomba"
// #define MQTT_TOPIC_ESP_SUBSCRIPTION "tanque"
// #define MQTT_TOPIC_BROKER_DESTINATION "server"

// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// // Keys importantes en el json enviado por mqtt, siempre en minusculas, node lo pone en minuscula, si no, se desborda memoria
// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// #define MQTT_JSON_KEY_SUBTOPIC "subtopic"
// #define MQTT_JSON_KEY_DESTINATION "destination"

// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// // subTopicos ó acciones
// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// #define MQTT_SUBTOPIC_ALL "all"
// #define MQTT_SUBTOPIC_RESET_PLC "resetPlc"
// #define MQTT_SUBTOPIC_NIVEL "nivel"
// #define MQTT_SUBTOPIC_MOTORES_TEMPERATURAS "motoresTemperaturas"
// #define MQTT_SUBTOPIC_MOTORES_SHOW_ON_OFF "motoresShowOnOff"
// #define MQTT_SUBTOPIC_ENTRA_AGUA_CALLE "entraAguaCalle"
// #define MQTT_SUBTOPIC_ENTRA_AGUA_TANQUE "entraAguaTanque"

// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// // variables
// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// int retry_num = 0;
// esp_mqtt_client_handle_t mqtt_client;

// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// // Funciones de acciones
// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// static void hacer_todo(void)
// {
//   printf("---Ejecutando la función hacer_todo()\n");
// }

// static void resetear_plc(void)
// {
//   printf("---Ejecutando la función resetear_plc()\n");
// }

// static void medir_nivel_tanque(void)
// {
//   printf("---Ejecutando la función medir_nivel_tanque()\n");
// }

// static void medir_temperaturas_motores(void)
// {
//   printf("---Ejecutando la función medir_temperaturas_motores()\n");
// }

// static void mostrar_motores_on_off(void)
// {
//   printf("---Ejecutando la función mostrar_motores_on_off()\n");
// }

// static void entra_agua_calle(void)
// {
//   printf("---Ejecutando la función entra_agua_calle()\n");
// }

// static void entra_agua_tanque(void)
// {
//   printf("---Ejecutando la función entra_agua_tanque()\n");
// }

// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// // funciones generales
// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
// {
//   esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

//   switch (event->event_id)
//   {
//   case MQTT_EVENT_CONNECTED:
//     printf("=====> MQTT ESP conectado al server\n");
//     esp_mqtt_client_subscribe(event->client, MQTT_TOPIC_ESP_SUBSCRIPTION, 0);
//     break;

//   case MQTT_EVENT_DISCONNECTED:
//     printf("=====> MQTT ESP desconectado del server\n");
//     break;

//   case MQTT_EVENT_DATA:
//     const char *data = event->data;
//     int data_len = event->data_len;
//     printf("=====> MQTT ESP Transmited in/out: %.*s\n", data_len, data);
//     cJSON *jsonData = cJSON_Parse(data);
//     if (jsonData != NULL)
//     {
//       cJSON *destination = cJSON_GetObjectItemCaseSensitive(jsonData, "destination");
//       if (cJSON_IsString(destination) && (destination->valuestring != NULL))
//       {

//         if (strcmp(destination->valuestring, MQTT_TOPIC_ESP_SUBSCRIPTION) == 0)
//         {
//           // acciones a ejecutar cuando recibo un evento en el tópico al que estoy suscrito
//           cJSON *subTopic = cJSON_GetObjectItemCaseSensitive(jsonData, MQTT_JSON_KEY_SUBTOPIC);

//           // creando una acción para cada "#define" o subTopico
//           if (strcmp(subTopic->valuestring, MQTT_SUBTOPIC_ALL) == 0)
//           {
//             hacer_todo();
//           }
//           else if (strcmp(subTopic->valuestring, MQTT_SUBTOPIC_RESET_PLC) == 0)
//           {
//             resetear_plc();
//           }
//           else if (strcmp(subTopic->valuestring, MQTT_SUBTOPIC_NIVEL) == 0)
//           {
//             medir_nivel_tanque();
//           }
//           else if (strcmp(subTopic->valuestring, MQTT_SUBTOPIC_MOTORES_TEMPERATURAS) == 0)
//           {
//             medir_temperaturas_motores();
//           }
//           else if (strcmp(subTopic->valuestring, MQTT_SUBTOPIC_MOTORES_SHOW_ON_OFF) == 0)
//           {
//             mostrar_motores_on_off();
//           }
//           else if (strcmp(subTopic->valuestring, MQTT_SUBTOPIC_ENTRA_AGUA_CALLE) == 0)
//           {
//             entra_agua_calle();
//           }
//           else if (strcmp(subTopic->valuestring, MQTT_SUBTOPIC_ENTRA_AGUA_TANQUE) == 0)
//           {
//             entra_agua_tanque();
//           }
//           else
//           {
//             // No se encontró un subtopic coincidente
//             printf("No se encuentra acciones para ese subtopic: %s\n", subTopic->valuestring);
//           }
//         }
//       }
//       else
//       {
//         printf("No se encontró la llave 'destination' o no es un valor de cadena válido\n");
//       }
//       cJSON_Delete(jsonData);
//     }
//     else
//     {
//       printf("Error al analizar el JSON\n");
//     }
//     break;

//   default:
//     break;
//   }
// }

// static void mqtt_app_start(void)
// {
//   esp_mqtt_client_config_t mqtt_cfg = {.broker.address.uri = MQTT_BROKER_URI, .credentials.client_id = MQTT_CLIENT_ID};
//   mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
//   esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, mqtt_client);
//   esp_mqtt_client_subscribe(mqtt_client, MQTT_TOPIC_ESP_SUBSCRIPTION, 0);
//   esp_mqtt_client_start(mqtt_client);
// }

// static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
// {
//   if (event_id == WIFI_EVENT_STA_START)
//   {
//     printf("=====>  CONNECTANDO AL WIFI....\n");
//   }
//   else if (event_id == WIFI_EVENT_STA_CONNECTED)
//   {
//     printf("=====>  YA CONNECTADO AL WIFI ==LISTO==\n");
//   }
//   else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
//   {
//     printf("=====>  Perdió la conexión al wifi\n");
//     if (retry_num < 5)
//     {
//       esp_wifi_connect();
//       retry_num++;
//       printf("=====> Intentando conectar al Wifi...\n");
//     }
//   }
//   else if (event_id == IP_EVENT_STA_GOT_IP)
//   {
//     printf("=====>  IP obtenido...\n\n");
//   }
// }

// void wifi_connection()
// {
//   esp_netif_init();
//   esp_event_loop_create_default();
//   esp_netif_create_default_wifi_sta();
//   wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();
//   esp_wifi_init(&wifi_initiation);
//   esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
//   esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);
//   wifi_config_t wifi_configuration = {.sta = { .ssid = SSID_NAME, .password = SSID_PASS }};
//   strcpy((char *)wifi_configuration.sta.ssid, SSID_NAME);
//   strcpy((char *)wifi_configuration.sta.password, SSID_PASS);
//   esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration);
//   esp_wifi_start();
//   esp_wifi_set_mode(WIFI_MODE_STA);
//   printf("=====> Conectado al WIFI con SSID:%s  password:%s", SSID_NAME, SSID_PASS);
//   esp_wifi_connect();
// }

// void app_main(void)
// {
//   nvs_flash_init();
//   wifi_connection();
//   mqtt_app_start();
// }