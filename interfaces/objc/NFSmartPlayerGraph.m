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
#import <NFSmartPlayerObjC/NFSmartPlayerGraph.h>

#import "NFSmartPlayerEdge+Private.h"
#import "NFSmartPlayerGraph+Private.h"
#import "NFSmartPlayerNode+Private.h"
#import "NFSmartPlayerParam+Private.h"

void NFSmartPlayerGraphLoadInternal(void *context, int success, const char *error_message);

@interface NFSmartPlayerGraph ()

@property(nonatomic, assign, readonly) NF_SMART_PLAYER_GRAPH_HANDLE graphHandle;
@property(nonatomic, assign, readwrite, getter=isFirstLoadCalled) BOOL firstLoadCalled;
@property(nonatomic, strong, readwrite) NFSmartPlayerGraphLoad loadBlock;

@end

@implementation NFSmartPlayerGraph

#pragma mark NFSmartPlayerGraph

- (instancetype)initWithGraph:(NF_SMART_PLAYER_GRAPH_HANDLE)graph {
  self = [super init];
  if (self) {
    _graphHandle = graph;
  }
  return self;
}

- (instancetype)initWithJSON:(NSString *)json loadBlock:(NFSmartPlayerGraphLoad)loadBlock {
  self = [super init];

  if (self) {
    _loadBlock = loadBlock;

    _graphHandle = smartplayer_create_graph(json.UTF8String, (__bridge void *)self,
                                            NFSmartPlayerGraphLoadInternal);
  }

  return self;
}

- (void)setJson:(NSString *)json {
  smartplayer_graph_set_json(self.graphHandle, json.UTF8String);
}

- (NSString *)identifier {
  return [NSString stringWithUTF8String:smartplayer_graph_get_identifier(self.graphHandle)];
}

- (NSTimeInterval)renderTime {
  return smartplayer_graph_get_render_time(self.graphHandle);
}

- (void)setRenderTime:(NSTimeInterval)renderTime {
  smartplayer_graph_set_render_time(self.graphHandle, renderTime);
}

- (NSString *)type {
  return [NSString stringWithUTF8String:smartplayer_graph_type(self.graphHandle)];
}

- (float)valueForPath:(NSString *)path {
  return smartplayer_graph_value_for_path(self.graphHandle, path.UTF8String);
}

- (void)setValueForPath:(NSString *)path, ... {
  va_list args;
  va_start(args, path);
  smartplayer_graph_set_value_for_path(self.graphHandle, path.UTF8String, args);
  va_end(args);
}

- (NFSmartPlayerParam *)paramForPath:(NSString *)path {
  NF_SMART_PLAYER_PARAM_HANDLE paramHandle =
      smartplayer_graph_get_parameter(self.graphHandle, path.UTF8String);

  return paramHandle ? [[NFSmartPlayerParam alloc] initWithParam:paramHandle] : nil;
}

- (BOOL)isLoaded {
  return smartplayer_graph_loaded(self.graphHandle);
}

#pragma mark NSObject

- (void)dealloc {
  smartplayer_graph_close(self.graphHandle);
}

@end

void NFSmartPlayerGraphLoadInternal(void *context, int success, const char *error_message) {
  NFSmartPlayerGraph *graph = (__bridge NFSmartPlayerGraph *)context;
  NSString *message = [NSString stringWithUTF8String:error_message];
  if (!graph.firstLoadCalled) {
    graph.firstLoadCalled = YES;
    if (graph.loadBlock) {
      graph.loadBlock(success, message);
      graph.loadBlock = nil;
      return;
    }
  }
  if ([graph.delegate respondsToSelector:@selector(graphDidLoad:errorMessage:)]) {
    [graph.delegate graphDidLoad:graph errorMessage:message];
  }
}
