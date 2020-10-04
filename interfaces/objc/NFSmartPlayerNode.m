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
#import <NFSmartPlayerObjC/NFSmartPlayerNode.h>

#import "NFSmartPlayerNode+Private.h"
#import "NFSmartPlayerParam+Private.h"

@interface NFSmartPlayerNode ()

@property(nonatomic, assign, readonly) NF_SMART_PLAYER_NODE_HANDLE nodeHandle;

@end

@implementation NFSmartPlayerNode

#pragma mark NFSmartPlayerNode

- (instancetype)initWithNode:(NF_SMART_PLAYER_NODE_HANDLE)node {
  self = [super init];
  if (self) {
    _nodeHandle = node;
  }
  return self;
}

- (instancetype)initWithJSON:(NSString *)json {
  self = [super init];
  if (self) {
    _nodeHandle = smartplayer_create_node(json.UTF8String);
  }
  return self;
}

- (NSString *)identifier {
  return [NSString stringWithUTF8String:smartplayer_node_identifier(self.nodeHandle)];
}

- (NSString *)type {
  return [NSString stringWithUTF8String:smartplayer_node_type(self.nodeHandle)];
}

- (BOOL)isOutputNode {
  return smartplayer_node_is_output_node(self.nodeHandle);
}

- (NSString *)json {
  return [NSString stringWithUTF8String:smartplayer_node_json(self.nodeHandle)];
}

- (NSArray<NFSmartPlayerParam *> *)params {
  NSMutableArray<NFSmartPlayerParam *> *params = [NSMutableArray new];
  for (NSInteger i = 0; i < smartplayer_node_get_params(self.nodeHandle); ++i) {
    [params addObject:[[NFSmartPlayerParam alloc]
                          initWithParam:smartplayer_node_get_param_at_index(self.nodeHandle, i)]];
  }
  return params.copy;
}

- (NFSmartPlayerParam *)paramForIdentifier:(NSString *)identifier {
  NF_SMART_PLAYER_PARAM_HANDLE paramHandle =
      smartplayer_node_get_param(self.nodeHandle, identifier.UTF8String);

  return paramHandle ? [[NFSmartPlayerParam alloc] initWithParam:paramHandle] : nil;
}

#pragma mark NSObject

- (void)dealloc {
  smartplayer_node_close(self.nodeHandle);
}

@end
