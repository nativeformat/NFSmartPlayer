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
#import <NFSmartPlayerObjC/NFSmartPlayerEdge.h>

#import "NFSmartPlayerEdge+Private.h"

@interface NFSmartPlayerEdge ()

@property(nonatomic, assign, readonly) NF_SMART_PLAYER_EDGE_HANDLE edgeHandle;

@end

@implementation NFSmartPlayerEdge

#pragma mark NFSmartPlayerEdge

- (instancetype)initWithEdge:(NF_SMART_PLAYER_EDGE_HANDLE)edge {
  self = [super init];
  if (self) {
    _edgeHandle = edge;
  }
  return self;
}

- (instancetype)initWithSource:(NSString *)source target:(NSString *)target {
  self = [super init];
  if (self) {
    _edgeHandle = smartplayer_create_edge(source.UTF8String, target.UTF8String);
  }
  return self;
}

- (instancetype)initWithJSON:(NSString *)json {
  self = [super init];
  if (self) {
    _edgeHandle = smartplayer_create_edge_json(json.UTF8String);
  }
  return self;
}

- (NSString *)source {
  return [NSString stringWithUTF8String:smartplayer_edge_get_source(self.edgeHandle)];
}

- (NSString *)target {
  return [NSString stringWithUTF8String:smartplayer_edge_get_target(self.edgeHandle)];
}

- (NSString *)json {
  return [NSString stringWithUTF8String:smartplayer_edge_get_json(self.edgeHandle)];
}

- (NSString *)identifier {
  return [NSString stringWithUTF8String:smartplayer_edge_get_identifier(self.edgeHandle)];
}

#pragma mark NSObject

- (void)dealloc {
  smartplayer_edge_close(self.edgeHandle);
}

@end
