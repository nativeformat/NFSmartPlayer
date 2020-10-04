/*
 * Copyright (c) 2017 Spotify AB.
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#import <NFSmartPlayerObjC/NFSmartPlayerObjC.h>

#import "NFSmartPlayerGraph+Private.h"
#import "NFSmartPlayerParam+Private.h"
#import "NFSmartPlayerScript+Private.h"

const char *NFSmartPlayerObjCResolverInternal(void *context, const char *plugin_namespace,
                                              const char *variable_name);
void NFSmartPlayerObjCLoadInternal(void *context, int success, const char *error_message);
void NFSmartPlayerObjCMessageCallback(void *context, const char *message_identifier,
                                      const char *sender_identifier,
                                      NF_SMART_PLAYER_MESSAGE_TYPE message_type,
                                      const void *payload);

@interface NFSmartPlayerObjC ()

@property(nonatomic, assign, readonly) NF_SMART_PLAYER_HANDLE handle;

@end

@implementation NFSmartPlayerObjC

#pragma mark NFSmartPlayerObjC

+ (NSString *)playerVersion {
  return [NSString stringWithUTF8String:NF_SMART_PLAYER_VERSION];
}

- (instancetype)init {
  self = [super init];

  if (self) {
    _handle = smartplayer_open(NFSmartPlayerObjCResolverInternal, (__bridge void *)self,
                               NF_SMART_PLAYER_DEFAULT_SETTINGS,
                               NF_SMART_PLAYER_DRIVER_TYPE_SOUNDCARD, "");
    smartplayer_set_message_callback(_handle, NFSmartPlayerObjCMessageCallback);
  }

  return self;
}

- (BOOL)isPlaying {
  return smartplayer_is_playing(_handle);
}

- (void)setPlaying:(BOOL)playing {
  smartplayer_set_playing(_handle, playing);
}

- (NSTimeInterval)time {
  return smartplayer_render_time(_handle);
}

- (void)setTime:(NSTimeInterval)time {
  smartplayer_set_render_time(_handle, time);
}

- (NSArray<NFSmartPlayerGraph *> *)graphs {
  NSMutableArray<NFSmartPlayerGraph *> *graphs = [NSMutableArray new];
  for (NSInteger i = 0; i < smartplayer_get_graphs(self.handle); ++i) {
    [graphs addObject:[[NFSmartPlayerGraph alloc]
                          initWithGraph:smartplayer_get_graph_at_index(self.handle, i)]];
  }
  return graphs.copy;
}

- (void)setJson:(NSString *)json {
  smartplayer_set_json(self.handle, json.UTF8String, &NFSmartPlayerObjCLoadInternal);
}

- (NSArray<NFSmartPlayerScript *> *)scripts {
  NSMutableArray<NFSmartPlayerScript *> *scripts = [NSMutableArray new];
  for (NSInteger i = 0; i < smartplayer_get_scripts(self.handle); ++i) {
    [scripts addObject:[[NFSmartPlayerScript alloc]
                           initWithScript:smartplayer_get_script_at_index(self.handle, i)]];
  }
  return scripts.copy;
}

- (BOOL)isLoaded {
  return smartplayer_is_loaded(self.handle);
}

- (NFSmartPlayerGraph *)graphForIdentifier:(NSString *)identifier {
  NF_SMART_PLAYER_GRAPH_HANDLE graphHandle =
      smartplayer_get_graph(self.handle, identifier.UTF8String);

  return graphHandle ? [[NFSmartPlayerGraph alloc] initWithGraph:graphHandle] : nil;
}

- (void)addGraph:(NFSmartPlayerGraph *)graph {
  smartplayer_add_graph(self.handle, graph.graphHandle);
}

- (void)removeGraph:(NSString *)graphIdentifier {
  smartplayer_remove_graph(self.handle, graphIdentifier.UTF8String);
}

- (void)sendMessage:(NSString *)messageIdentifier
        messageType:(NFSmartPlayerMessageType)messageType
            payload:(id)payload {
  switch (messageType) {
    case NFSmartPlayerMessageTypeGeneric:
    case NFSmartPlayerMessageTypeNone:
      smartplayer_send_message(self.handle, messageIdentifier.UTF8String, messageType,
                               [payload UTF8String]);
      break;
  }
}

- (NFSmartPlayerScript *)openScript:(NSString *)name code:(NSString *)code {
  return [[NFSmartPlayerScript alloc]
      initWithScript:smartplayer_open_script(self.handle, name.UTF8String, code.UTF8String)];
}

- (NFSmartPlayerScript *)scriptForIdentifier:(NSString *)identifier {
  NF_SMART_PLAYER_SCRIPT_HANDLE scriptHandle =
      smartplayer_get_script(self.handle, identifier.UTF8String);

  return scriptHandle ? [[NFSmartPlayerScript alloc] initWithScript:scriptHandle] : nil;
}

- (float)valueForPath:(NSString *)path {
  return smartplayer_get_value_for_path(self.handle, path.UTF8String);
}

- (void)setValueForPath:(NSString *)path, ... {
  va_list args;
  va_start(args, path);
  smartplayer_vset_value_for_path(self.handle, path.UTF8String, args);
  va_end(args);
}

#pragma mark NSObject

- (void)dealloc {
  smartplayer_close(_handle);
}

@end

const char *NFSmartPlayerObjCResolverInternal(void *context, const char *plugin_namespace,
                                              const char *variable_name) {
  NFSmartPlayerObjC *smartPlayer = (__bridge NFSmartPlayerObjC *)context;
  if ([smartPlayer.delegate respondsToSelector:@selector(smartPlayer:
                                                   requiresResolution:forVariable:)]) {
    return [smartPlayer.delegate smartPlayer:smartPlayer
                          requiresResolution:[NSString stringWithUTF8String:plugin_namespace]
                                 forVariable:[NSString stringWithUTF8String:variable_name]]
        .UTF8String;
  }
  return "";
}

void NFSmartPlayerObjCLoadInternal(void *context, int success, const char *error_message) {
  NFSmartPlayerObjC *smartPlayer = (__bridge NFSmartPlayerObjC *)context;
  if ([smartPlayer.delegate respondsToSelector:@selector(smartPlayer:didLoad:errorMessage:)]) {
    [smartPlayer.delegate smartPlayer:smartPlayer
                              didLoad:success
                         errorMessage:[NSString stringWithUTF8String:error_message]];
  }
}

void NFSmartPlayerObjCMessageCallback(void *context, const char *message_identifier,
                                      const char *sender_identifier,
                                      NF_SMART_PLAYER_MESSAGE_TYPE message_type,
                                      const void *payload) {
  NFSmartPlayerObjC *smartPlayer = (__bridge NFSmartPlayerObjC *)context;
  if ([smartPlayer.delegate
          respondsToSelector:@selector(smartPlayer:receivedMessage:sender:messageType:payload:)]) {
    [smartPlayer.delegate smartPlayer:smartPlayer
                      receivedMessage:[NSString stringWithUTF8String:message_identifier]
                               sender:[NSString stringWithUTF8String:sender_identifier]
                          messageType:message_type
                              payload:nil];
  }
}

static_assert(NFSmartPlayerMessageTypeGeneric == NF_SMART_PLAYER_MESSAGE_TYPE_GENERIC,
              "The ObjC enumeration must match the C enumeration");
static_assert(NFSmartPlayerMessageTypeNone == NF_SMART_PLAYER_MESSAGE_TYPE_NONE,
              "The ObjC enumeration must match the C enumeration");
