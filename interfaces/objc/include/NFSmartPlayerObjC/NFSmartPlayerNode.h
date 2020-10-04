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
#import <Foundation/Foundation.h>

#import <NFSmartPlayerObjC/NFSmartPlayerParam.h>

@interface NFSmartPlayerNode : NSObject

@property(nonatomic, strong, readonly, nonnull) NSString *identifier;
@property(nonatomic, strong, readonly, nonnull) NSString *type;
@property(nonatomic, assign, readonly, getter=isOutputNode) BOOL outputNode;
@property(nonatomic, strong, readonly, nonnull) NSString *json;
@property(nonatomic, strong, readonly, nonnull) NSArray<NFSmartPlayerParam *> *params;

- (nonnull instancetype)initWithJSON:(nonnull NSString *)json;
- (nonnull instancetype)init NS_UNAVAILABLE;
- (nullable NFSmartPlayerParam *)paramForIdentifier:(nonnull NSString *)identifier;

@end
