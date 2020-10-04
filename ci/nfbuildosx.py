#!/usr/bin/env python

import fnmatch
import os
import plistlib
import re
import shutil
import subprocess
import sys

from distutils import dir_util
from nfbuild import NFBuild


class NFBuildOSX(NFBuild):
    def __init__(self):
        super(self.__class__, self).__init__()
        self.project_file = os.path.join(
            self.build_directory,
            'NFSmartPlayer.xcodeproj')
        self.cmake_binary = 'cmake'
        self.clang_format_binary = 'clang-format'
        self.android_ndk_folder = '/usr/local/share/android-ndk'
        self.ninja_binary = 'ninja'
        self.ios = False
        self.android = False
        self.android_arm = False

    def generateProject(self,
                        code_coverage=False,
                        address_sanitizer=False,
                        thread_sanitizer=False,
                        undefined_behaviour_sanitizer=False,
                        mock_logging_service=False,
                        debuild=False,
                        ios=False,
                        android=False,
                        android_arm=False,
                        gcc=False):
        self.use_ninja = android or android_arm
        self.android = android
        self.android_arm = android_arm
        self.ios = ios

        cmake_call = [self.cmake_binary, '..']

        if android or android_arm:
            android_abi = 'x86_64'
            android_toolchain_name = 'x86_64-llvm'
            if android_arm:
                android_abi = 'arm64-v8a'
                android_toolchain_name = 'arm64-llvm'
            cmake_call.extend([
                '-GNinja',
                '-DANDROID=1',
                '-DCMAKE_TOOLCHAIN_FILE=' + self.android_ndk_folder +
                '/build/cmake/android.toolchain.cmake',
                '-DANDROID_NDK=' + self.android_ndk_folder,
                '-DANDROID_ABI=' + android_abi,
                '-DANDROID_NATIVE_API_LEVEL=21',
                '-DANDROID_TOOLCHAIN_NAME=' + android_toolchain_name,
                '-DANDROID_STL=c++_shared'])
            self.project_file = 'build.ninja'
        else:
            cmake_call.append('-GXcode')
        if ios:
            cmake_call.append('-DIOS=1')
        else:
            cmake_call.append('-DIOS=0')

        if self.build_type == 'Release':
            cmake_call.append('-DCREATE_RELEASE_BUILD=1')
        else:
            cmake_call.append('-DCREATE_RELEASE_BUILD=0')
        if code_coverage:
            cmake_call.append('-DCODE_COVERAGE=1')
        else:
            cmake_call.append('-DCODE_COVERAGE=0')
        if address_sanitizer:
            cmake_call.append('-DUSE_ADDRESS_SANITIZER=1')
        else:
            cmake_call.append('-DUSE_ADDRESS_SANITIZER=0')
        if mock_logging_service:
            cmake_call.append('-DTEST_LOG_SINK=1')
        else:
            cmake_call.append('-DTEST_LOG_SINK=0')

        if thread_sanitizer:
            cmake_call.append('-DUSE_THREAD_SANITIZER=1')
        else:
            cmake_call.append('-DUSE_THREAD_SANITIZER=0')
        if undefined_behaviour_sanitizer:
            cmake_call.append('-DUSE_UNDEFINED_BEHAVIOUR_SANITIZER=1')
        else:
            cmake_call.append('-DUSE_UNDEFINED_BEHAVIOUR_SANITIZER=0')
        cmake_result = subprocess.call(cmake_call, cwd=self.build_directory)
        if cmake_result != 0:
            sys.exit(cmake_result)

    def buildTarget(self, target, sdk='macosx', arch='x86_64'):
        result = 0
        build_log = '%s.build.log' % target
        print('Building %s (see %s for build log)...' % (target, build_log))
        f = open(os.path.join(self.output_directory, build_log), 'w')
        if self.use_ninja:
            result = subprocess.call([
                self.ninja_binary,
                '-C',
                self.build_directory,
                '-f',
                self.project_file,
                target],
                stdout=f,
                stderr=f)
        else:
            result = subprocess.call([
                'xcodebuild',
                '-project',
                self.project_file,
                '-target',
                target,
                '-sdk',
                sdk,
                '-arch',
                arch,
                '-configuration',
                self.build_type,
                'build'],
                stdout=f,
                stderr=f)
        f.close()
        if result != 0:
            sys.exit(result)

    def addLibraryPath(self, library_path):
        library_path_environment_variable = "DYLD_FALLBACK_LIBRARY_PATH"
        if library_path_environment_variable in os.environ:
            os.environ[library_path_environment_variable] += os.pathsep
            + library_path
        else:
            os.environ[library_path_environment_variable] = library_path

    def staticallyAnalyse(self, target, include_regex=None):
        diagnostics_key = 'diagnostics'
        files_key = 'files'
        exceptions_key = 'static_analyzer_exceptions'
        static_file_exceptions = []
        static_analyzer_result = subprocess.check_output([
            'xcodebuild',
            '-project',
            self.project_file,
            '-target',
            target,
            '-sdk',
            'macosx',
            '-configuration',
            self.build_type,
            '-dry-run',
            'analyze'])
        analyze_command = '--analyze'
        for line in static_analyzer_result.splitlines():
            if analyze_command not in line:
                continue
            static_analyzer_line_words = line.split()
            analyze_command_index = static_analyzer_line_words.index(
                analyze_command)
            source_file = static_analyzer_line_words[analyze_command_index + 1]
            if source_file.startswith(self.current_working_directory):
                source_file = source_file[
                    len(self.current_working_directory)+1:]
            if include_regex is not None:
                if not re.match(include_regex, source_file):
                    continue
            if source_file in self.statically_analyzed_files:
                continue
            self.build_print('Analysing ' + source_file)
            stripped_command = line.strip()
            clang_result = subprocess.call(stripped_command, shell=True)
            if clang_result:
                sys.exit(clang_result)
            self.statically_analyzed_files.append(source_file)
        static_analyzer_check_passed = True
        for root, dirnames, filenames in os.walk(self.build_directory):
            for filename in fnmatch.filter(filenames, '*.plist'):
                full_filepath = os.path.join(root, filename)
                static_analyzer_result = plistlib.readPlist(full_filepath)
                if 'clang_version' not in static_analyzer_result \
                        or files_key not in static_analyzer_result \
                        or diagnostics_key not in static_analyzer_result:
                    continue
                if len(static_analyzer_result[files_key]) == 0:
                    continue
                for static_analyzer_file in static_analyzer_result[files_key]:
                    if static_analyzer_file in static_file_exceptions:
                        continue
                    normalised_file = static_analyzer_file[
                        len(self.current_working_directory)+1:]
                    if normalised_file in \
                            self.build_configuration[exceptions_key]:
                        continue
                    self.build_print('Issues found in: ' + normalised_file)
                    for static_analyzer_issue in \
                            static_analyzer_result[diagnostics_key]:
                        self.pretty_printer.pprint(static_analyzer_issue)
                        sys.stdout.flush()
                    static_analyzer_check_passed = False
        if not static_analyzer_check_passed:
            print('Static analyzer check did not pass. Continuing...')

    def targetBinary(self, target, dylib=False):
        for root, dirnames, filenames in os.walk(self.build_directory):
            for filename in fnmatch.filter(filenames, target):
                full_target_file = os.path.join(root, filename)
                return full_target_file
        if not dylib:
            return self.targetBinary('lib' + target + '.dylib', dylib=True)
        return ''

    def makeUniversalBinary(self):
        library_name = 'libNFSmartPlayer.a'
        source_binaries = os.path.join(self.build_directory, 'source')
        universal_release_folder = os.path.join(
            source_binaries,
            self.build_type + '-universal')
        if not os.path.exists(universal_release_folder):
            os.makedirs(universal_release_folder)
        static_library_simulator = os.path.join(
            os.path.join(
                source_binaries,
                self.build_type + '-iphonesimulator'),
            library_name)
        static_library_device = os.path.join(
            os.path.join(
                source_binaries,
                self.build_type + '-iphoneos'),
            library_name)
        self.universal_binary_path = os.path.join(
            universal_release_folder,
            library_name)
        lipo_result = subprocess.call([
            'lipo',
            '-create',
            static_library_simulator,
            static_library_device,
            '-output',
            self.universal_binary_path])
        if lipo_result:
            sys.exit(lipo_result)

    def createFramework(self,
                        name,
                        library_paths,
                        library_name,
                        interface_headers_directory):
        headers_directory_name = 'Headers'
        a_version_directory_name = 'A'
        versions_directory_name = 'Versions'
        framework_directory = os.path.join(self.output_directory, name)
        if os.path.exists(framework_directory):
            shutil.rmtree(framework_directory)
        os.makedirs(framework_directory)
        versions_directory = os.path.join(
            framework_directory,
            versions_directory_name)
        os.makedirs(versions_directory)
        a_directory = os.path.join(
            versions_directory,
            a_version_directory_name)
        os.makedirs(a_directory)
        headers_directory = os.path.join(
            a_directory,
            headers_directory_name)
        dir_util.copy_tree(interface_headers_directory, headers_directory)
        new_library_path = os.path.join(a_directory, library_name)
        if len(library_paths) == 1:
            shutil.copyfile(library_paths[0], new_library_path)
        else:
            lipo_call = ['lipo', '-create']
            for library_path in library_paths:
                lipo_call.append(library_path)
            lipo_call.extend(['-output', new_library_path])
            lipo_result = subprocess.call(lipo_call)
            if lipo_result:
                sys.exit(lipo_result)
        top_level_headers_directory = os.path.join(
            framework_directory,
            headers_directory_name)
        top_level_library = os.path.join(framework_directory, library_name)
        current_directory = os.path.join(versions_directory, 'Current')
        os.symlink(os.path.join(
            versions_directory_name,
            os.path.join(
                a_version_directory_name,
                headers_directory_name)
        ), top_level_headers_directory)
        os.symlink(os.path.join(
            versions_directory_name,
            os.path.join(
                a_version_directory_name,
                library_name)
        ), top_level_library)
        os.symlink(a_version_directory_name, current_directory)
        zip_result = subprocess.call([
            'zip',
            '-r',
            '-y',
            name + ".zip",
            name], cwd=self.output_directory)
        if zip_result:
            sys.exit(zip_result)
        shutil.rmtree(framework_directory)

    def createStaticLibraryPackage(self,
                                   static_library,
                                   include_headers_directory,
                                   library_directory_name):
        library_name = 'libNFSmartPlayer.a'
        library_directory = os.path.join(
            self.output_directory,
            library_directory_name)
        if os.path.exists(library_directory):
            shutil.rmtree(library_directory)
        os.makedirs(library_directory)
        build_static_library = os.path.join(library_directory, library_name)
        shutil.copyfile(static_library, build_static_library)
        headers_directory = os.path.join(library_directory, 'include')
        dir_util.copy_tree(include_headers_directory, headers_directory)
        zip_result = subprocess.call([
            'zip',
            '-r',
            '-y',
            library_directory_name + ".zip",
            library_directory_name], cwd=self.output_directory)
        if zip_result:
            sys.exit(zip_result)
        shutil.rmtree(library_directory)

    def makeOSXFramework(self):
        library_name = 'libNFSmartPlayerObjC.a'
        library_path = os.path.join(
            os.path.join(
                os.path.join(
                    os.path.join(
                        'build',
                        'interfaces'),
                    'objc'),
                self.build_type),
            library_name)
        interface_headers_directory = os.path.join(
            os.path.join(
                os.path.join(
                    'interfaces',
                    'objc'),
                'include'),
            'NFSmartPlayerObjC')
        self.createFramework(
            'NFSmartPlayerObjC.framework',
            [library_path],
            'NFSmartPlayerObjC',
            interface_headers_directory)

    def makeIOSFramework(self):
        library_name = 'libNFSmartPlayerObjC.a'
        library_path_simulator = os.path.join(
            os.path.join(
                os.path.join(
                    os.path.join(
                        'build',
                        'interfaces'),
                    'objc'),
                self.build_type + '-iphonesimulator'),
            library_name)
        library_path_device = os.path.join(
            os.path.join(
                os.path.join(
                    os.path.join(
                        'build',
                        'interfaces'),
                    'objc'),
                self.build_type + '-iphoneos'),
            library_name)
        interface_headers_directory = os.path.join(
            os.path.join(
                os.path.join(
                    'interfaces',
                    'objc'),
                'include'),
            'NFSmartPlayerObjC')
        self.createFramework(
            'NFSmartPlayerObjC.framework',
            [library_path_simulator, library_path_device],
            'NFSmartPlayerObjC',
            interface_headers_directory)

    def makeOSXStaticLibrary(self):
        library_name = 'libNFSmartPlayer.a'
        static_library = os.path.join(
            os.path.join(
                os.path.join(
                    self.build_directory,
                    'source'),
                self.build_type),
            library_name)
        self.createStaticLibraryPackage(
            static_library,
            'include',
            'libNFSmartPlayer')

    def makeIOSStaticLibrary(self):
        self.createStaticLibraryPackage(
            self.universal_binary_path,
            'include',
            'libNFSmartPlayer-iOS')

    def platform(self):
        return 'osx'
