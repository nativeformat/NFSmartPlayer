#!/usr/bin/env python
'''
 * Copyright (c) 2018 Spotify AB.
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
 '''

from nfbuildosx import NFBuildOSX

from build_options import BuildOptions


def main():

    buildOptions = BuildOptions()
    buildOptions.addOption("debug", "Enable Debug Mode")
    buildOptions.addOption("deploy", "Do deployment")

    buildOptions.addOption("lintPython", "Lint python files")
    buildOptions.addOption("lintCmake", "Lint cmake files")
    buildOptions.addOption("lintCpp", "Lint CPP Files")
    buildOptions.addOption("lintCppWithInlineChange",
                           "Lint CPP Files and fix them")

    buildOptions.addOption("unitTests", "Run Unit Tests")
    buildOptions.addOption("integrationTests", "Run Integration Tests")

    buildOptions.addOption("makeBuildDirectory",
                           "Wipe existing build directory")
    buildOptions.addOption("generateProject", "Regenerate xcode project")
    buildOptions.addOption("addressSanitizer",
                           "Enable Address Sanitizer in generate project")
    buildOptions.addOption("codeCoverage",
                           "Enable code coverage in generate project")
    buildOptions.addOption("mockLoggingService",
                           "Enable logging service mock in generate project")

    buildOptions.addOption("buildTargetCLI", "Build Target: CLI")
    buildOptions.addOption("buildTargetFramework", "Build Target: Framework")

    buildOptions.addOption("staticAnalysis", "Run Static Analysis")

    buildOptions.addOption("makeCLI", "Deploy CLI Binary")
    buildOptions.addOption("makeOSXFramework", "Deploy OSX Framework Binary")
    buildOptions.addOption("makeOSXStaticLibrary",
                           "Deploy OSX Static Library Binary")
    buildOptions.addOption("makeJAR", "Deploy JAR")

    buildOptions.setDefaultWorkflow("Empty workflow", [])

    buildOptions.addWorkflow("local_unit", "Run local unit tests", [
        'debug',
        'generateProject',
        'buildTargetCLI',
        'unitTests'
    ])

    buildOptions.addWorkflow("local_it", "Run local integration tests", [
        'debug',
        'generateProject',
        'integrationTests'
    ])

    buildOptions.addWorkflow("lint", "Run lint workflow", [
        'lintPython',
        'lintCmake',
        'lintCppWithInlineChange'
    ])

    buildOptions.addWorkflow("address_sanitizer", "Run address sanitizer", [
        'debug',
        'lintPython',
        'lintCmake',
        'lintCpp',
        'makeBuildDirectory',
        'generateProject',
        'addressSanitizer',
        'mockLoggingService',
        'buildTargetCLI',
        'unitTests',
        'integrationTests'
    ])

    buildOptions.addWorkflow("code_coverage", "Collect code coverage", [
        'debug',
        'lintPython',
        'lintCmake',
        'lintCpp',
        'makeBuildDirectory',
        'generateProject',
        'codeCoverage',
        'mockLoggingService',
        'buildTargetCLI',
        'unitTests',
        'integrationTests'
    ])

    buildOptions.addWorkflow("build", "Production Build", [
        'lintPython',
        'lintCmake',
        'lintCpp',
        'makeBuildDirectory',
        'generateProject',
        'buildTargetCLI',
        'buildTargetFramework',
        'staticAnalysis',
        'makeCLI',
        'makeOSXFramework',
        'makeOSXStaticLibrary',
        'makeJAR'
    ])

    options = buildOptions.parseArgs()

    buildOptions.verbosePrintBuildOptions(options)

    target = {
        'buildTargetCLI': 'NFSmartPlayerCLI',
        'buildTargetFramework': 'NFSmartPlayerObjC',
    }

    nfbuild = NFBuildOSX()

    if buildOptions.checkOption(options, 'deploy'):
        nfbuild.deploy_artifacts = True

    if buildOptions.checkOption(options, 'debug'):
        nfbuild.build_type = 'Debug'

    if buildOptions.checkOption(options, 'lintPython'):
        nfbuild.lintPython()

    if buildOptions.checkOption(options, 'lintCmake'):
        nfbuild.lintCmake()

    if buildOptions.checkOption(options, 'makeBuildDirectory'):
        nfbuild.makeBuildDirectory()

    if buildOptions.checkOption(options, 'generateProject'):
        nfbuild.generateProject(
            address_sanitizer='addressSanitizer' in options,
            code_coverage='codeCoverage' in options,
            mock_logging_service='mockLoggingService' in options
        )

    if buildOptions.checkOption(options, 'lintCppWithInlineChange'):
        nfbuild.lintCPP(make_inline_changes=True)
    elif buildOptions.checkOption(options, 'lintCpp'):
        nfbuild.lintCPP(make_inline_changes=False)

    for option in ['buildTargetCLI',
                   'buildTargetFramework']:
        if buildOptions.checkOption(options, option):
            nfbuild.buildTarget(target[option])
            if buildOptions.checkOption(options, 'staticAnalysis'):
                nfbuild.staticallyAnalyse(target[option],
                                          include_regex='source/.*')

    # Test
    if buildOptions.checkOption(options, 'unitTests'):
        nfbuild.runUnitTests()

    if buildOptions.checkOption(options, 'integrationTests'):
        nfbuild.runIntegrationTests()

    if buildOptions.checkOption(options, 'codeCoverage'):
        nfbuild.collectCodeCoverage()

    # Makes
    if buildOptions.checkOption(options, 'makeCLI'):
        nfbuild.makeCLI()

    if buildOptions.checkOption(options, 'makeOSXFramework'):
        nfbuild.makeOSXFramework()

    if buildOptions.checkOption(options, 'makeOSXStaticLibrary'):
        nfbuild.makeOSXStaticLibrary()

    if buildOptions.checkOption(options, 'makeJAR'):
        nfbuild.makeJAR()


if __name__ == "__main__":
    main()
