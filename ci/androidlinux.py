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

from nfbuildlinux import NFBuildLinux
from build_options import BuildOptions


def main():
    buildOptions = BuildOptions()

    buildOptions.addOption("makeBuildDirectoryX86",
                           "Wipe existing build directory for X86 build.")
    buildOptions.addOption("generateProjectX86",
                           "Regenerate project for X86 build")

    buildOptions.addOption("buildTargetLibraryX86",
                           "Build Target: Library (X86)")

    buildOptions.addOption("makeBuildDirectoryArm64",
                           "Wipe existing build directory for ARM64 build.")
    buildOptions.addOption("generateProjectArm64",
                           "Regenerate project for ARM64 build")
    buildOptions.addOption(
        "packageArtifactsArm64", "Package the artifacts produced by the build")
    buildOptions.addOption(
        "packageArtifactsX86", "Package the artifacts produced by the build")
    buildOptions.addOption("buildTargetLibraryArm64",
                           "Build Target: Library (ARM64)")
    buildOptions.addOption("makeCLI", "Deploy CLI Binary")

    buildOptions.setDefaultWorkflow("Empty workflow", [])

    buildOptions.addWorkflow("build", "Production Build (Android Linux)", [
        'makeBuildDirectoryX86',
        'generateProjectX86',
        'buildTargetLibraryX86',
        'packageArtifactsX86',
        'makeBuildDirectoryArm64',
        'generateProjectArm64',
        'buildTargetLibraryArm64',
        'packageArtifactsArm64'
    ])

    buildOptions.addWorkflow("buildX86", "Production Build (X86)", [
        'makeBuildDirectoryX86',
        'generateProjectX86',
        'buildTargetLibraryX86',
        'packageArtifactsX86'
    ])

    buildOptions.addWorkflow("buildArm64", "Production Build (ARM64)", [
        'makeBuildDirectoryArm64',
        'generateProjectArm64',
        'buildTargetLibraryArm64',
        'packageArtifactsArm64'
    ])

    options = buildOptions.parseArgs()
    buildOptions.verbosePrintBuildOptions(options)

    library_target = 'NFSmartPlayer'
    nfbuild = NFBuildLinux()

    if buildOptions.checkOption(options, 'makeBuildDirectoryX86'):
        nfbuild.makeBuildDirectory()

    if buildOptions.checkOption(options, 'generateProjectX86'):
        nfbuild.generateProject(android=True, android_arm=False)

    if buildOptions.checkOption(options, 'buildTargetLibraryX86'):
        nfbuild.buildTarget(library_target)

    if buildOptions.checkOption(options, 'packageArtifactsX86'):
        nfbuild.packageArtifacts(android=True, android_arm=False)

    if buildOptions.checkOption(options, 'packageArtifactsArm64'):
        nfbuild.packageArtifacts(android=True, android_arm=True)

    if buildOptions.checkOption(options, 'makeBuildDirectoryArm64'):
        nfbuild.makeBuildDirectory()

    if buildOptions.checkOption(options, 'generateProjectArm64'):
        nfbuild.generateProject(android=True, android_arm=True)

    if buildOptions.checkOption(options, 'buildTargetLibraryArm64'):
        nfbuild.buildTarget(library_target)

    if buildOptions.checkOption(options, 'makeCLI'):
        nfbuild.makeCLI()


if __name__ == "__main__":
    main()
