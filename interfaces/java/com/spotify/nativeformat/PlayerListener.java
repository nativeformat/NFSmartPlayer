package com.spotify.nativeformat;

interface PlayerListener {
   String nativeFormatResolveVariable(String pluginNamespace, String variableIdentifier);
   void nativeFormatDidLoad(boolean success, String errorMessage);
   void nativeFormatReceivedMessage(String messageIdentifier, String senderIdentifier, MessageType messageType, Object payload);
}