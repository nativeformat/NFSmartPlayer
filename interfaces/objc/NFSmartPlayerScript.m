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
#import <NFSmartPlayerObjC/NFSmartPlayerScript.h>

#import "NFSmartPlayerScript+Private.h"

@interface NFSmartPlayerScript ()

@property(nonatomic, assign, readonly) NF_SMART_PLAYER_SCRIPT_HANDLE scriptHandle;

@end

@implementation NFSmartPlayerScript

#pragma mark NFSmartPlayerScript

- (instancetype)initWithScript:(NF_SMART_PLAYER_SCRIPT_HANDLE)script {
  self = [super init];
  if (self) {
    _scriptHandle = script;
  }
  return self;
}

- (NSString *)name {
  return [NSString stringWithUTF8String:smartplayer_script_name(self.scriptHandle)];
}

- (NFSmartPlayerScriptScope)scope {
  return (NFSmartPlayerScriptScope)smartplayer_script_scope(self.scriptHandle);
}

#pragma mark NSObject

- (void)dealloc {
  smartplayer_script_close(self.scriptHandle);
}

@end

static_assert(NFSmartPlayerScriptScopeNone == NF_SMART_PLAYER_SCRIPT_SCOPE_NONE,
              "The ObjC enumeration must match the C enumeration");
static_assert(NFSmartPlayerScriptScopeGraph == NF_SMART_PLAYER_SCRIPT_SCOPE_GRAPH,
              "The ObjC enumeration must match the C enumeration");
static_assert(NFSmartPlayerScriptScopeSession == NF_SMART_PLAYER_SCRIPT_SCOPE_SESSION,
              "The ObjC enumeration must match the C enumeration");
