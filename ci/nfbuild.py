#!/usr/bin/env python

import fnmatch
import librosa
import os
import numpy
import pprint
import shutil
import subprocess
import sys
import yaml

try:
    from pathlib import Path
except ImportError:
    from pathlib2 import Path  # python 2 backport


class NFBuild(object):
    def __init__(self):
        ci_yaml_file = os.path.join('ci', 'ci.yaml')
        self.build_configuration = yaml.load(open(ci_yaml_file, 'r'))
        self.pretty_printer = pprint.PrettyPrinter(indent=4)
        self.current_working_directory = os.getcwd()
        self.build_directory = 'build'
        self.build_type = 'Release'
        self.jar_binary = 'jar'
        self.output_directory = os.path.join(self.build_directory, 'output')
        self.smart_player_test_audio_extraction_folder = 'resources'
        self.statically_analyzed_files = []
        self.deploy_artifacts = False

    def build_print(self, print_string):
        print(print_string)
        sys.stdout.flush()

    def makeBuildDirectory(self):
        if os.path.exists(self.build_directory):
            shutil.rmtree(self.build_directory)
        os.makedirs(self.build_directory)
        os.makedirs(self.output_directory)

    def generateProject(self,
                        code_coverage=False,
                        address_sanitizer=False,
                        thread_sanitizer=False,
                        undefined_behaviour_sanitizer=False,
                        debuild=False,
                        ios=False):
        assert True, "generateProject should be overridden by subclass"

    def buildTarget(self, target, sdk='macosx'):
        assert True, "buildTarget should be overridden by subclass"

    def staticallyAnalyse(self, target, include_regex=None):
        assert True, "staticallyAnalyse should be overridden by subclass"

    def targetBinary(self, target):
        assert True, "targetBinary should be overridden by subclass"

    def runTarget(self, target):
        target_file = self.targetBinary(target)
        target_result = subprocess.call([target_file])
        if target_result:
            sys.exit(target_result)

    def runUnitTests(self):
        for unit_test_target in self.build_configuration['unit_tests']:
            self.buildTarget(unit_test_target)
            self.runTarget(unit_test_target)

    def runIntegrationTests(self):
        cli_target_name = 'NFSmartPlayerCLI'
        integration_test_output_file = 'output.flac'
        self.buildTarget(cli_target_name)
        cli_binary = self.targetBinary(cli_target_name)
        smart_player_wav = 'nfsmartplayer.wav'
        input_wav = 'input.wav'
        output_wav = 'output.wav'
        loglevel = 'panic'
        os.environ['UBSAN_OPTIONS'] = 'print_stacktrace=1'
        os.environ['ASAN_OPTIONS'] = 'symbolize=1:detect_leaks=0'
        asan_path = 'llvm-symbolizer'
        os.environ['ASAN_SYMBOLIZER_PATH'] = asan_path
        self.build_print('CLI found at: ' + cli_binary)

        for integration_test in self.build_configuration['integration_tests']:
            self.build_print(
                'Running Integration Test: ' + integration_test['audio'])
            # Render the graph
            resolved_variables_string = ''
            resolved_variables_key = 'resolved_variables'
            if resolved_variables_key in integration_test:
                resolved_variables = integration_test[resolved_variables_key]
                for resolved_variable in resolved_variables:
                    if len(resolved_variables_string) > 0:
                        resolved_variables_string += ','
                    resolved_variables_string += resolved_variable + \
                        ',' + \
                        resolved_variables[resolved_variable]
            render_time = 0.0
            render_time_key = 'render_time'
            if render_time_key in integration_test:
                render_time = float(integration_test[render_time_key])
            cli_binary_call = [
                cli_binary,
                '--input-file=' + os.path.abspath(integration_test['graph']),
                '--driver-type=file',
                '--render-time=' + str(render_time)]
            if len(resolved_variables_string) > 0:
                cli_binary_call.append(
                    '--resolved-variables=' + resolved_variables_string)

            time_result = subprocess.call(cli_binary_call)
            if time_result:
                sys.exit(time_result)

            # Convert and prepare the input/output audio files
            audio_file = os.path.join(
                os.path.join(
                    self.smart_player_test_audio_extraction_folder,
                    'smart-player-test-audio'),
                integration_test['audio'])
            if os.path.isfile(integration_test_output_file):
                os.remove(integration_test_output_file)
            ffmpeg_result = subprocess.call([
                self.ffmpeg_binary,
                '-hide_banner',
                '-loglevel',
                loglevel,
                '-i',
                smart_player_wav,
                '-c:a',
                'flac',
                integration_test_output_file])
            if ffmpeg_result:
                self.build_print("Failed to convert render to flac")
                sys.exit(ffmpeg_result)
            if os.path.isfile(input_wav):
                os.remove(input_wav)
            ffmpeg_result = subprocess.call([
                self.ffmpeg_binary,
                '-hide_banner',
                '-loglevel',
                loglevel,
                '-i',
                integration_test_output_file,
                input_wav])
            if ffmpeg_result:
                self.build_print("Failed to convert render flac to wav")
                sys.exit(ffmpeg_result)
            if os.path.isfile(output_wav):
                os.remove(output_wav)
            ffmpeg_result = subprocess.call([
                self.ffmpeg_binary,
                '-hide_banner',
                '-loglevel',
                loglevel,
                '-i',
                audio_file,
                output_wav])
            if ffmpeg_result:
                self.build_print("Failed to convert recorded flac to wav")
                sys.exit(ffmpeg_result)
            # Check that the output is the same audio
            input_y, input_samplerate = librosa.load(
                input_wav, mono=True, sr=44100)
            output_y, output_samplerate = librosa.load(
                output_wav, mono=True, sr=44100)
            input_z = numpy.absolute(input_y.flatten())
            output_z = numpy.absolute(output_y.flatten())
            if abs(input_z.size - output_z.size) > 0:
                self.build_print(
                    "ERROR: Integration Test Fails on Length Check: " +
                    integration_test['audio'])
                self.build_print(
                    "Actual render length is: " +
                    str(input_z.size))
                self.build_print(
                    "Expected render length is: " +
                    str(output_z.size))
                sys.exit(1)
            input_z = numpy.resize(input_z, max(input_z.size, output_z.size))
            output_z = numpy.resize(output_z, max(input_z.size, output_z.size))
            audio_error = numpy.mean(
                numpy.absolute(
                    numpy.subtract(input_z, output_z)))
            if audio_error > integration_test['error']:
                self.build_print(
                    "ERROR: Integration Test Fails: " +
                    integration_test['audio'])
                self.build_print("Error rate: " + str(audio_error))
                sys.exit(1)
            else:
                self.build_print(
                    "Integration Test Passed: " + integration_test['audio'])

    def collectCodeCoverage(self):
        for root, dirnames, filenames in os.walk('build'):
            for filename in fnmatch.filter(filenames, '*.gcda'):
                full_filepath = os.path.join(root, filename)
                if full_filepath.startswith('build/source/') and \
                   '/tests/' not in full_filepath:
                    continue
                os.remove(full_filepath)
        llvm_run_script = os.path.join(
            os.path.join(
                self.current_working_directory,
                'ci'),
            'llvm-run.sh')
        cov_info = os.path.join('build', 'cov.info')
        lcov_result = subprocess.call([
            self.lcov_binary,
            '--directory',
            '.',
            '--base-directory',
            '.',
            '--gcov-tool',
            llvm_run_script,
            '--capture',
            '-o',
            cov_info])
        if lcov_result:
            sys.exit(lcov_result)
        coverage_output = os.path.join(self.output_directory, 'code_coverage')
        genhtml_result = subprocess.call([
            self.genhtml_binary,
            cov_info,
            '-o',
            coverage_output])
        if genhtml_result:
            sys.exit(genhtml_result)

    def lintCPPFile(self, filepath, make_inline_changes=False):
        # Do not lint machine generated code
        if os.path.basename(filepath) == 'com_spotify_nativeformat_Player.h':
            return True
        current_source = open(filepath, 'r').read()
        clang_format_call = [self.clang_format_binary]
        if make_inline_changes:
            clang_format_call.append('-i')
        clang_format_call.append(filepath)
        new_source = subprocess.check_output(clang_format_call)
        if current_source != new_source and not make_inline_changes:
            self.build_print(
                filepath + " failed C++ lint, file should look like:")
            self.build_print(new_source)
            return False
        return True

    def lintCPPDirectory(self, directory, make_inline_changes=False):
        passed = True
        for root, dirnames, filenames in os.walk(directory):
            for filename in filenames:
                if not filename.endswith(('.cpp', '.h', '.m', '.mm')):
                    continue
                full_filepath = os.path.join(root, filename)
                if not self.lintCPPFile(full_filepath, make_inline_changes):
                    passed = False
        return passed

    def lintCPP(self, make_inline_changes=False):
        lint_result = self.lintCPPDirectory('source', make_inline_changes)
        lint_result &= self.lintCPPDirectory('interfaces', make_inline_changes)
        lint_result &= self.lintCPPDirectory('include', make_inline_changes)
        if not lint_result:
            sys.exit(1)

    def lintPython(self):
        passed = True
        for root, dirnames, filenames in os.walk('ci'):
            for filename in filenames:
                if filename.endswith('.py'):
                    full_filepath = os.path.join(root, filename)
                    flake8_result = subprocess.check_output([
                        'flake8',
                        full_filepath,
                        '--exit-zero'])
                    if len(flake8_result) > 0:
                        self.build_print(flake8_result)
                        passed = False
        if not passed:
            sys.exit(1)

    def lintCmakeFile(self, filepath):
        self.build_print("Linting: " + filepath)
        return subprocess.call(['cmakelint', filepath]) == 0

    def lintCmakeDirectory(self, directory):
        passed = True
        for root, dirnames, filenames in os.walk(directory):
            for filename in filenames:
                if not filename.endswith('CMakeLists.txt'):
                    continue
                full_filepath = os.path.join(root, filename)
                if not self.lintCmakeFile(full_filepath):
                    passed = False
        return passed

    def lintCmake(self):
        lint_result = self.lintCmakeFile('CMakeLists.txt')
        lint_result &= self.lintCmakeDirectory('source')
        lint_result &= self.lintCmakeDirectory('interfaces')
        if not lint_result:
            sys.exit(1)

    def makeJAR(self):
        jni_target_name = 'NFSmartPlayerJava'
        self.buildTarget(jni_target_name)
        jni_target_binary = self.targetBinary(jni_target_name)
        jar_folder = os.path.join(self.output_directory, 'jar')
        os.makedirs(jar_folder)
        shutil.copytree(
            os.path.join('interfaces', 'java', 'com'),
            os.path.join(jar_folder, 'com'))
        resources_folder = os.path.join(
            self.output_directory,
            'jar',
            'resources')
        os.makedirs(resources_folder)
        shutil.copyfile(
            jni_target_binary,
            os.path.join(
                resources_folder,
                os.path.basename(jni_target_binary)))
        jar_file = os.path.join(self.output_directory, 'NFSmartPlayer.jar')
        target_result = subprocess.call([
            self.jar_binary,
            'cvf',
            jar_file,
            '-C',
            jar_folder,
            '.'])
        if target_result:
            sys.exit(target_result)

    def platform(self):
        assert True, "platform should be overridden by subclass"

    def makeCLI(self):
        cli_target_name = 'NFSmartPlayerCLI'
        self.buildTarget(cli_target_name)
        cli_target_binary = self.targetBinary(cli_target_name)
        cli_output_folder = os.path.join(
            self.output_directory,
            'cli')
        Path(cli_output_folder).mkdir(exist_ok=True)
        cli_output_file = os.path.join(
            cli_output_folder,
            os.path.basename(cli_target_binary))
        shutil.copyfile(cli_target_binary, cli_output_file)
        os.chmod(cli_output_file, 0o755)

    def find_file(self, directory, file_name, multiple_files=False):
        matches = []
        for root, dirnames, filenames in os.walk(directory):
            for filename in fnmatch.filter(filenames, file_name):
                matches.append(os.path.join(root, filename))
                if not multiple_files:
                    break
            if not multiple_files and len(matches) > 0:
                break
        return matches

    def make_archive(self, source, destination):
        base = os.path.basename(destination)
        name = base.split('.')[0]
        format = base.split('.')[1]
        archive_from = os.path.dirname(source)
        archive_to = os.path.basename(source.strip(os.sep))
        print(source, destination, archive_from, archive_to)
        shutil.make_archive(name, format, archive_from, archive_to)
        shutil.move('%s.%s' % (name, format), destination)
