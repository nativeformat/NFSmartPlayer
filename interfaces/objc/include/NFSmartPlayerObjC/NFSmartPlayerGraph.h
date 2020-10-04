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

#import <NFSmartPlayerObjC/NFSmartPlayerEdge.h>
#import <NFSmartPlayerObjC/NFSmartPlayerNode.h>
#import <NFSmartPlayerObjC/NFSmartPlayerParam.h>

typedef void (^NFSmartPlayerGraphLoad)(BOOL, NSString *_Nonnull);

@class NFSmartPlayerGraph;

@protocol NFSmartPlayerGraphDelegate <NSObject>

- (void)graphDidLoad:(nonnull NFSmartPlayerGraph *)graph
        errorMessage:(nonnull NSString *)errorMessage;

@end

@interface NFSmartPlayerGraph : NSObject

@property(nonatomic, strong, readonly, nonnull) NSString *identifier;
@property(nonatomic, assign, readwrite) NSTimeInterval renderTime;
@property(nonatomic, strong, readonly, nonnull) NSString *type;
@property(nonatomic, weak, readwrite, nullable) id<NFSmartPlayerGraphDelegate> delegate;
@property(nonatomic, assign, readonly, getter=isLoaded) BOOL loaded;

- (nonnull instancetype)init NS_UNAVAILABLE;
- (nonnull instancetype)initWithJSON:(nonnull NSString *)json
                           loadBlock:(nullable NFSmartPlayerGraphLoad)loadBlock;

- (float)valueForPath:(nonnull NSString *)path;
- (void)setValueForPath:(nonnull NSString *)path, ...;
- (nullable NFSmartPlayerParam *)paramForPath:(nonnull NSString *)path;
- (void)setJson:(nonnull NSString *)json;

@end
