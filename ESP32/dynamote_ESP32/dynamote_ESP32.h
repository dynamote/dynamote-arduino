#ifndef DYNAMOTE_ESP32_H
#define DYNAMOTE_ESP32_H

/******************************************************************************************************************
* includes
******************************************************************************************************************/
#include "LinkedList.h"

/******************************************************************************************************************
* structs
******************************************************************************************************************/
typedef struct
{
  uint8_t codeProtocol;         // The type of IR code
  uint32_t codeValue;           // The data bits if IR type is not raw
  LinkedList codeValueRaw;      // The data bits if IR type is raw
  uint8_t codeLength;           // The length of the IR code in bits
  String customCode;            // custom code specified by the user
  bool useCustomCode;           // whether to use the received custom code over the IR code
} RemoteCommand;

/******************************************************************************************************************
* functions
******************************************************************************************************************/
void sendRemoteCommand(RemoteCommand command);
void sendRemoteCommandFromWifi(String command);

#endif
