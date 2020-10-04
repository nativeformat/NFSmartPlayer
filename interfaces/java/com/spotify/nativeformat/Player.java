package com.spotify.nativeformat;

import java.io.IOException;

public class Player {
   private boolean closed;
   private final DriverType driverType;
   private final String outputPath;
   private final PlayerListener listener;
   private long nPlayerPointer;

   static {
      try {
         if (OSValidator.isWindows()) {
            NativeUtils.loadLibraryFromJar("/resources/NFSmartPlayerJava.dll");
         } else if (OSValidator.isMac()) {
            NativeUtils.loadLibraryFromJar("/resources/libNFSmartPlayerJava.dylib");
         } else if (OSValidator.isUnix()) {
            NativeUtils.loadLibraryFromJar("/resources/libNFSmartPlayerJava.so");
         } else {
            NativeUtils.loadLibraryFromJar("/resources/libNFSmartPlayerJava");
         }
      } catch (IOException e) {
         e.printStackTrace();
      }
   }

   public Player(PlayerListener listener, DriverType driverType, String outputPath) {
      this.closed = false;
      this.driverType = driverType;
      this.outputPath = outputPath;
      this.listener = listener;
      init();
   }

   public native boolean getPlaying();
   public native void setPlaying(boolean playing);
   public native double getTime();
   public native void setTime(double time);
   public native boolean getLoaded();
   public native float getValueForPath(String path);
   public native void setValueForPath(String path, float... values);
   public native void setJson(String json);
   public native void sendMessage(String messageIdentifier, MessageType messageType, Object payload);

   public static native String getSpotifyPluginFactoryAccessTokenVariable();
   public static native String getSpotifyPluginFactoryTokenTypeVariable();
   public static native String getPlayerVersion();

   private native void init();
   public native void close();

   public boolean isClosed() {
      return this.closed;
   }

   protected void finalize() {
      close();
   }
}
