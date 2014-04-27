/*

void Trampoline::jump() {
  switch(msg->messageType) {
    case SearchCommandMessageType:
      construct_reply(do_search(dp, (SearchMessage*) msg));
      //LogicalPlan lp= 
      construct_reply(dp.internalSearchCommand(lp));
      //create message
      break;
    case InsertUpdateMessageType:
      //extract Record
      construct_reply(dp.internalInsertUpdateMessageType(record));
      //create message
      break;
    case DeleteInfoMessageType:
      //extract DeleteCommandInfo
      construct_reply(dp.internalDeleteCommand(delCommand));
      break;
    case SerializeInfoMessageType:
      //extract SerializeCommandInput
      construct_reply(dp.internalGetInfoCommand(serializeCommand));
      break;
    case GetInfoMessageType:
      //extract GetInfoCommandInput
      dp.internalGetInfoCommand(getInfo);
      break;
    case ResettingLogMessageType:
      //extract ResetLogCommandInfo
      dp.internalResetLogCommand(resetLog);
      break;
  }
}

*/
