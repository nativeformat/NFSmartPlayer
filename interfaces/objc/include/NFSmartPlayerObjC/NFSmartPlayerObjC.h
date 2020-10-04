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

#import <NFSmartPlayerObjC/NFSmartPlayerGraph.h>
#import <NFSmartPlayerObjC/NFSmartPlayerParam.h>
#import <NFSmartPlayerObjC/NFSmartPlayerScript.h>

@class NFSmartPlayerObjC;

typedef NS_ENUM(NSInteger, NFSmartPlayerMessageType) {
  NFSmartPlayerMessageTypeGeneric,
  NFSmartPlayerMessageTypeNone
};

@protocol NFSmartPlayerObjCDelegate <NSObject>

@optional

- (nonnull NSString *)smartPlayer:(nonnull NFSmartPlayerObjC *)smartPlayer
               requiresResolution:(nonnull NSString *)pluginNamespace
                      forVariable:(nonnull NSString *)variableIdentifier;
- (void)smartPlayer:(nonnull NFSmartPlayerObjC *)smartPlayer
            didLoad:(BOOL)success
       errorMessage:(nonnull NSString *)errorMessage;
- (void)smartPlayer:(nonnull NFSmartPlayerObjC *)smartPlayer
    receivedMessage:(nonnull NSString *)messageIdentifier
             sender:(nonnull NSString *)senderIdentifier
        messageType:(NFSmartPlayerMessageType)messageType
            payload:(nullable id)payload;

@end

@interface NFSmartPlayerObjC : NSObject

@property(nonatomic, assign, readwrite, getter=isPlaying) BOOL playing;
@property(nonatomic, assign, readwrite) NSTimeInterval time;
@property(nonatomic, strong, readonly, nonnull) NSArray<NFSmartPlayerGraph *> *graphs;
@property(nonatomic, weak, readwrite, nullable) id<NFSmartPlayerObjCDelegate> delegate;
@property(nonatomic, strong, readonly, nonnull) NSArray<NFSmartPlayerScript *> *scripts;
@property(nonatomic, assign, readonly, getter=isLoaded) BOOL loaded;

+ (nonnull NSString *)spotifyPluginFactoryAccessTokenVariable;
+ (nonnull NSString *)spotifyPluginFactoryTokenTypeVariable;
+ (nonnull NSString *)playerVersion;

- (nonnull instancetype)init;

- (nullable NFSmartPlayerGraph *)graphForIdentifier:(nonnull NSString *)identifier;
- (void)addGraph:(nonnull NFSmartPlayerGraph *)graph;
- (void)removeGraph:(nonnull NSString *)graphIdentifier;
- (void)sendMessage:(nonnull NSString *)messageIdentifier
        messageType:(NFSmartPlayerMessageType)messageType
            payload:(nullable id)payload;
- (nonnull NFSmartPlayerScript *)openScript:(nonnull NSString *)name code:(nonnull NSString *)code;
- (nullable NFSmartPlayerScript *)scriptForIdentifier:(nonnull NSString *)identifier;
- (float)valueForPath:(nonnull NSString *)path;
- (void)setValueForPath:(nonnull NSString *)path, ...;
- (void)setJson:(nonnull NSString *)json;

@end
