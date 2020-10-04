#!/bin/bash
# Copyright (c) 2018 Spotify AB.
#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

# Exit on any non-zero status
set -e
set -x

# Install system dependencies
apt-get -q update
apt-get install -y -q --no-install-recommends apt-utils \
    ninja-build \
    clang \
    clang-format-10 \
    libc++-dev \
    libcurl4-openssl-dev \
    libssl-dev \
    libc++-dev \
    wget \
    libsndfile1 \
    git \
    unzip \
    make \
    libyaml-dev \
    libssl-dev \
    python-dev \
    python3-dev \
    python-virtualenv \
    build-essential \
    autoconf \
    libtool \
    libffi-dev \
    libffi6 \
    libblas3 \
    libblas-dev \
    liblapack3 \
    liblapack-dev \
    libatlas3-base \
    libatlas-base-dev \
    x264 \
    lame \
    python-pip \
    python3-pip \
    virtualenv \
    cmake \
    libidn11 \
    libidn11-dev \
    libavformat-dev \
    libavcodec-dev \
    libavutil-dev \
    libswscale-dev \
    libavresample-dev \
    libavfilter-dev \
    libavdevice-dev \
    libavcodec-extra \
    libc++abi-dev \
    libc++-dev \
    gobjc++ \
    ffmpeg \
    llvm \
    libasound2-dev \
    default-jdk \
    default-jre \
    default-jre-headless

# Update submodules
git submodule sync
git submodule update --init --recursive

export CC=clang
export CXX=clang++

# Install virtualenv
virtualenv --python=$(which python2) smartplayer_env
. smartplayer_env/bin/activate

# Install Python Packages
pip install six
# pip install --upgrade setuptools pip
pip install -r ${PWD}/ci/requirements.txt

# Install gyp
cd tools/gyp
python setup.py install
cd ../..

# Execute our python build tools
if [ -n "$BUILD_ANDROID" ]; then

    if [ ! -d ~/ndk ]; then
        # Install Android NDK
        NDK='android-ndk-r21d-linux-x86_64'
        ZIP='zip'
        wget -nc https://dl.google.com/android/repository/${NDK}.${ZIP} -O ${PWD}/${NDK}.${ZIP}
        unzip -o -q ${NDK}.${ZIP}
        rm -rf ~/ndk
        mv android-ndk-r21d ~/ndk

        chmod +x -R ~/ndk
    fi

    python ci/androidlinux.py "$@"
else
    python ci/linux.py "$@"
fi
