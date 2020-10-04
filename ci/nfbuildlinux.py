#!/usr/bin/env python

import fnmatch
import os
import subprocess
import sys
import shutil
from distutils.dir_util import copy_tree

from nfbuild import NFBuild


class NFBuildLinux(NFBuild):
    def __init__(self):
        super(self.__class__, self).__init__()
        self.project_file = 'build.ninja'
        self.project_file = 'build.ninja'
        self.cmake_binary = 'cmake'
        self.android_ndk_folder = '~/ndk'
        self.clang_format_binary = 'clang-format'
        self.ffmpeg_binary = 'ffmpeg'

    def generateProject(self,
                        ios=False,
                        android=False,
                        android_arm=False,
                        address_sanitizer=False,
                        gcc=False):
        cmakeroot = '..'
        generator = '-GNinja'
        workingdir = self.build_directory
        cmake_call = [
            self.cmake_binary,
            cmakeroot,
            generator,
            '-DINCLUDE_LGPL=1',
            '-DUSE_FFMPEG=1',
            '-DCMAKE_BUILD_TYPE=Release']
        if address_sanitizer:
            cmake_call.append('-DUSE_ADDRESS_SANITIZER=1')
        else:
            cmake_call.append('-DUSE_ADDRESS_SANITIZER=0')
        if android or android_arm:
            android_abi = 'x86_64'
            android_toolchain_name = 'x86_64-llvm'
        if android_arm:
            android_abi = 'arm64-v8a'
            android_toolchain_name = 'arm64-llvm'
            cmake_call.extend([
                '-DANDROID=1',
                '-DCMAKE_TOOLCHAIN_FILE=' + self.android_ndk_folder +
                '/build/cmake/android.toolchain.cmake',
                '-DANDROID_NDK=' + self.android_ndk_folder,
                '-DANDROID_ABI=' + android_abi,
                '-DANDROID_NATIVE_API_LEVEL=21',
                '-DANDROID_TOOLCHAIN_NAME=' + android_toolchain_name,
                '-DANDROID_STL=c++_shared'])
        if gcc:
            cmake_call.extend(['-DLLVM_STDLIB=0'])
        else:
            cmake_call.extend(['-DLLVM_STDLIB=1'])
        cmake_result = subprocess.call(cmake_call, cwd=workingdir)
        if cmake_result != 0:
            sys.exit(cmake_result)

    def buildTarget(self, target, sdk='', arch='x86_64'):
        ninja_result = subprocess.call([
            'ninja',
            '-C',
            self.build_directory,
            '-f',
            self.project_file,
            target])
        if ninja_result != 0:
            sys.exit(ninja_result)

    def targetBinary(self, target, dylib=False):
        for root, dirnames, filenames in os.walk(self.build_directory):
            for filename in fnmatch.filter(filenames, target):
                full_target_file = os.path.join(root, filename)
                return full_target_file
        if not dylib:
            return self.targetBinary('lib' + target + '.so', dylib=True)
        return ''

    def addLibraryPath(self, library_path):
        library_path_environment_variable = "LD_LIBRARY_PATH"
        if library_path_environment_variable in os.environ:
            os.environ[library_path_environment_variable] +=\
                os.pathsep + library_path
        else:
            os.environ[library_path_environment_variable] =\
                library_path

    def platform(self):
        return 'linux'

    def packageArtifacts(self, android=False,
                         android_arm=False):
        lib_name = 'libNFSmartPlayer.a'
        cli_name = 'NFSmartPlayerCLI'
        output_folder = os.path.join(self.build_directory, 'output')
        artifacts_folder = os.path.join(output_folder, 'NFSmartPlayer')
        copy_tree('include', os.path.join(artifacts_folder, 'include'))
        source_folder = os.path.join(self.build_directory, 'source')
        lib_matches = self.find_file(source_folder, lib_name)
        cli_matches = self.find_file(source_folder, cli_name)
        shutil.copyfile(lib_matches[0], os.path.join(
            artifacts_folder, lib_name))
        if len(cli_matches) > 0:
            shutil.copyfile(cli_matches[0], os.path.join(
                artifacts_folder, cli_name))
        output_zip = os.path.join(output_folder, 'libNFSmartPlayer.zip')
        self.make_archive(artifacts_folder, output_zip)
