/******************************************************************************************************************
* includes
******************************************************************************************************************/
#include "dynamote_ESP32.h"

/*
*  Here you can add your own application code for the custom code handlers.
*  This can be anything you want, not just IR remote specific.
*  However, below is an example relating to IR remotes.
*/

/******************************************************************************************************************
* parseCustomCommand
******************************************************************************************************************/
void parseCustomCommand(RemoteCommand command) {
  
  /*
  *  Many TV's have a source button that brings up the source menu before changing the source with a second press.
  *  This can make it impossible to change the source with the Google assistant, because you cannot send it twice fast enough.
  *  In the dynamote app, selecting a custom command AFTER recording an IR command still keeps the recorded IR command data,
  *  it just sets the useCustomCode flag to true. Therefore, you can still parse an IR command within a custom code handler.
  *  We can use this to create a custom command that sends the IR command multiple times to solve the described issue, see below.
  */
  if (command.customCode == "send_IR_code_twice") {
    // verify that an IR command is valid
    if (command.codeLength == 0) {
      return;
    }
    sendRemoteCommand(command);
    delay(2000);
    sendRemoteCommand(command);
  }
}