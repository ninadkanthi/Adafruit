// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <stdlib.h>

#include <stdio.h>
#include <stdint.h>

/* This sample uses the _LL APIs of iothub_client for example purposes.
  That does not mean that HTTP only works with the _LL APIs.
  Simply changing the using the convenience layer (functions not having _LL)
  and removing calls to _DoWork will yield the same results. */

#include "AzureIoTHub.h"
#include "iot_logging.h"

static const char* deviceId = "nk_arduino_device_1";
static const char* connectionString = "[Insert your Key]";

// /* .... New Code
// Define the Model

BEGIN_NAMESPACE(Contoso);

DECLARE_STRUCT(SystemProperties,
               ascii_char_ptr, DeviceID,
               _Bool, Enabled
              );

DECLARE_STRUCT(DeviceProperties,
               ascii_char_ptr, DeviceID,
               _Bool, HubEnabledState
              );

DECLARE_MODEL(Thermostat,

              // Event data (temperature, external temperature and humidity)
              WITH_DATA(int, Temperature),
              WITH_DATA(int, ExternalTemperature),
              WITH_DATA(int, Humidity),
              WITH_DATA(ascii_char_ptr, DeviceId),

              // Device Info - This is command metadata + some extra fields
              WITH_DATA(ascii_char_ptr, ObjectType),
              WITH_DATA(_Bool, IsSimulatedDevice),
              WITH_DATA(ascii_char_ptr, Version),
              WITH_DATA(DeviceProperties, DeviceProperties),
              WITH_DATA(ascii_char_ptr_no_quotes, Commands),

              // Commands implemented by the device
              WITH_ACTION(SetTemperature, int, temperature),
              WITH_ACTION(SetHumidity, int, humidity)
             );

END_NAMESPACE(Contoso);


EXECUTE_COMMAND_RESULT SetTemperature(Thermostat* thermostat, int temperature)
{
  LogInfo("Received temperature %d\r\n", temperature);
  thermostat->Temperature = temperature;
  return EXECUTE_COMMAND_SUCCESS;
}

EXECUTE_COMMAND_RESULT SetHumidity(Thermostat* thermostat, int humidity)
{
  LogInfo("Received humidity %d\r\n", humidity);
  thermostat->Humidity = humidity;
  return EXECUTE_COMMAND_SUCCESS;
}

// .... New Code */

// Define the Model
/*
  BEGIN_NAMESPACE(WeatherStation);

  DECLARE_MODEL(ContosoAnemometer,
              WITH_DATA(ascii_char_ptr, DeviceId),
              WITH_DATA(int, WindSpeed),
              WITH_ACTION(TurnFanOn),
              WITH_ACTION(TurnFanOff),
              WITH_ACTION(SetAirResistance, int, Position)
             );

  END_NAMESPACE(WeatherStation);
*/
DEFINE_ENUM_STRINGS(IOTHUB_CLIENT_CONFIRMATION_RESULT, IOTHUB_CLIENT_CONFIRMATION_RESULT_VALUES)
/*
  EXECUTE_COMMAND_RESULT TurnFanOn(ContosoAnemometer* device)
  {
  (void)device;
  LogInfo("Turning fan on.\r\n");
  return EXECUTE_COMMAND_SUCCESS;
  }

  EXECUTE_COMMAND_RESULT TurnFanOff(ContosoAnemometer* device)
  {
  (void)device;
  LogInfo("Turning fan off.\r\n");
  return EXECUTE_COMMAND_SUCCESS;
  }

  EXECUTE_COMMAND_RESULT SetAirResistance(ContosoAnemometer* device, int Position)
  {
  (void)device;
  LogInfo("Setting Air Resistance Position to %d.\r\n", Position);
  return EXECUTE_COMMAND_SUCCESS;
  }
*/
void sendCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void* userContextCallback)
{
  int messageTrackingId = (intptr_t)userContextCallback;

  LogInfo("Message Id: %d Received.\r\n", messageTrackingId);

  LogInfo("Result Call Back Called! Result is: %s \r\n", ENUM_TO_STRING(IOTHUB_CLIENT_CONFIRMATION_RESULT, result));
}

static void sendMessage(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, const unsigned char* buffer, size_t size)
{
  static unsigned int messageTrackingId;
  IOTHUB_MESSAGE_HANDLE messageHandle = IoTHubMessage_CreateFromByteArray(buffer, size);
  if (messageHandle == NULL)
  {
    LogInfo("unable to create a new IoTHubMessage\r\n");
  }
  else
  {
    if (IoTHubClient_LL_SendEventAsync(iotHubClientHandle, messageHandle, sendCallback, (void*)(uintptr_t)messageTrackingId) != IOTHUB_CLIENT_OK)
    {
      LogInfo("failed to hand over the message to IoTHubClient");
    }
    else
    {
      LogInfo("IoTHubClient accepted the message for delivery\r\n");
    }
    IoTHubMessage_Destroy(messageHandle);
  }
  free((void*)buffer);
  messageTrackingId++;
}

/*this function "links" IoTHub to the serialization library*/
static IOTHUBMESSAGE_DISPOSITION_RESULT IoTHubMessage(IOTHUB_MESSAGE_HANDLE message, void* userContextCallback)
{
  IOTHUBMESSAGE_DISPOSITION_RESULT result;
  const unsigned char* buffer;
  size_t size;
  if (IoTHubMessage_GetByteArray(message, &buffer, &size) != IOTHUB_MESSAGE_OK)
  {
    LogInfo("unable to IoTHubMessage_GetByteArray\r\n");
    result = EXECUTE_COMMAND_ERROR;
  }
  else
  {
    /*buffer is not zero terminated*/
    char* temp = malloc(size + 1);
    if (temp == NULL)
    {
      LogInfo("failed to malloc\r\n");
      result = EXECUTE_COMMAND_ERROR;
    }
    else
    {
      memcpy(temp, buffer, size);
      temp[size] = '\0';
      EXECUTE_COMMAND_RESULT executeCommandResult = EXECUTE_COMMAND(userContextCallback, temp);
      result =
        (executeCommandResult == EXECUTE_COMMAND_ERROR) ? IOTHUBMESSAGE_ABANDONED :
        (executeCommandResult == EXECUTE_COMMAND_SUCCESS) ? IOTHUBMESSAGE_ACCEPTED :
        IOTHUBMESSAGE_REJECTED;
      free(temp);
    }
  }
  return result;
}

void simplesample_http_run(void)
{
  LogInfo("simplesample_http_run...\r\n");
  // why do initBme() in each and every loop!!!
  initBme();

  if (serializer_init(NULL) != SERIALIZER_OK)
  {
    LogInfo("Failed on serializer_init\r\n");
  }
  else
  {
    IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(connectionString, HTTP_Protocol);
    srand((unsigned int)time(NULL));
    // int avgWindSpeed = 10;

    if (iotHubClientHandle == NULL)
    {
      LogInfo("Failed on IoTHubClient_LL_CreateFromConnectionString\r\n");
    }
    else
    {
      unsigned int minimumPollingTime = 9; /*because it can poll "after 9 seconds" polls will happen effectively at ~10 seconds*/
      if (IoTHubClient_LL_SetOption(iotHubClientHandle, "MinimumPollingTime", &minimumPollingTime) != IOTHUB_CLIENT_OK)
      {
        LogInfo("failure to set option \"MinimumPollingTime\"\r\n");
      }

#ifdef MBED_BUILD_TIMESTAMP
      // For mbed add the certificate information
      if (IoTHubClient_LL_SetOption(iotHubClientHandle, "TrustedCerts", certificates) != IOTHUB_CLIENT_OK)
      {
        LogInfo("failure to set option \"TrustedCerts\"\r\n");
      }
#endif // MBED_BUILD_TIMESTAMP

      Thermostat* thermostat = CREATE_MODEL_INSTANCE(Contoso, Thermostat);
      if (thermostat == NULL)
      {
        LogInfo("Failed on CREATE_MODEL_INSTANCE\r\n");
      }
      else
      {
        if (IoTHubClient_LL_SetMessageCallback(iotHubClientHandle, IoTHubMessage, thermostat) != IOTHUB_CLIENT_OK)
        {
          LogInfo("unable to IoTHubClient_SetMessageCallback\r\n");
        }
        else
        {
          thermostat->DeviceId = (char*)deviceId;
          int sendCycle = 10;
          int currentCycle = 0;
          while (1)
          {
            if (currentCycle >= sendCycle) 
            {
              float Temp;
              float Humi;
              getNextSample(&Temp, &Humi);
              //thermostat->Temperature = 50 + (rand() % 10 + 2);
              thermostat->Temperature = (int)round(Temp);
              thermostat->ExternalTemperature = 55 + (rand() % 5 + 2);
              //thermostat->Humidity = 50 + (rand() % 8 + 2);
              thermostat->Humidity = (int)round(Humi);
              currentCycle = 0;
              unsigned char*buffer;
              size_t bufferSize;

              LogInfo("Sending sensor value Temperature = %d, Humidity = %d\r\n", thermostat->Temperature, thermostat->Humidity);

              if (SERIALIZE(&buffer, &bufferSize, thermostat->DeviceId, thermostat->Temperature, thermostat->Humidity, thermostat->ExternalTemperature) != IOT_AGENT_OK)
              {
                LogInfo("Failed sending sensor value\r\n");
              }
              else
              {

                IOTHUB_MESSAGE_HANDLE messageHandle = IoTHubMessage_CreateFromByteArray(buffer, bufferSize);
                if (messageHandle == NULL)
                {
                  LogInfo("unable to create a new IoTHubMessage\r\n");
                }
                else
                {
                  if (IoTHubClient_LL_SendEventAsync(iotHubClientHandle, messageHandle, sendCallback, (void*)1) != IOTHUB_CLIENT_OK) 
                  {
                    LogInfo("failed to hand over the message to IoTHubClient");
                  }
                  else
                  {
                    LogInfo("IoTHubClient accepted the message for delivery\r\n");
                  }

                  IoTHubMessage_Destroy(messageHandle);
                }
                free(buffer);
              } // if (SERIALIZE
            }// if (currentCycle >= sendCycle) 
            IoTHubClient_LL_DoWork(iotHubClientHandle);
            ThreadAPI_Sleep(100);
            currentCycle++;
          }// while (1)
        }// if else if (IoTHubClient_LL_SetMessageCallback
      }
      DESTROY_MODEL_INSTANCE(thermostat);
    }
    IoTHubClient_LL_Destroy(iotHubClientHandle);
  }
  serializer_deinit();
}
