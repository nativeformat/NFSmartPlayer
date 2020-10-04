#!/usr/bin/env python

import os

from nfbuildlinux import NFBuildLinux
from build_options import BuildOptions


def main():

    buildOptions = BuildOptions()
    buildOptions.addOption("deploy", "Do deployment")

    buildOptions.addOption("integrationTests", "Run Integration Tests")

    buildOptions.addOption("makeBuildDirectory",
                           "Wipe existing build directory")
    buildOptions.addOption("generateProject", "Regenerate xcode project")
    buildOptions.addOption("addressSanitizer",
                           "Enable Address Sanitizer in generate project")
    buildOptions.addOption("mockLoggingService",
                           "Enable logging service mock in generate project")

    buildOptions.addOption("buildTargetLibrary", "Build Target: Library")
    buildOptions.addOption("gnuToolchain", "Build with gcc and libstdc++")
    buildOptions.addOption("llvmToolchain", "Build with clang and libc++")

    buildOptions.addOption("makeCLI", "Deploy CLI Binary")
    buildOptions.addOption("makeJAR", "Deploy JAR")

    buildOptions.setDefaultWorkflow("Empty workflow", [])

    buildOptions.addWorkflow("clang_build", "Production Clang Build", [
        'llvmToolchain',
        'makeBuildDirectory',
        'generateProject',
        'buildTargetLibrary',
        'makeCLI',
        'integrationTests'
    ])

    buildOptions.addWorkflow("gcc_build", "Production build with gcc", [
        'gnuToolchain',
        'makeBuildDirectory',
        'generateProject',
        'buildTargetLibrary',
        'makeCLI',
        'integrationTests',
    ])

    buildOptions.addWorkflow("address_sanitizer", "Run address sanitizer", [
        'makeBuildDirectory',
        'generateProject',
        'addressSanitizer',
        'mockLoggingService',
        'buildTargetLibrary',
        'makeCLI',
        'integrationTests'
    ])

    buildOptions.addWorkflow("build", "Production Build", [
        'makeBuildDirectory',
        'generateProject',
        'buildTargetLibrary',
        'makeCLI',
        'makeJAR'
    ])

    options = buildOptions.parseArgs()

    buildOptions.verbosePrintBuildOptions(options)

    library_target = 'NFSmartPlayer'
    nfbuild = NFBuildLinux()

    if buildOptions.checkOption(options, 'deploy'):
        nfbuild.deploy_artifacts = True

    if buildOptions.checkOption(options, 'makeBuildDirectory'):
        nfbuild.makeBuildDirectory()

    if buildOptions.checkOption(options, 'generateProject'):
        if buildOptions.checkOption(options, 'gnuToolchain'):
            os.environ['CC'] = 'gcc'
            os.environ['CXX'] = 'g++'
            nfbuild.generateProject(
                address_sanitizer='addressSanitizer' in options,
                gcc=True)
        elif buildOptions.checkOption(options, 'llvmToolchain'):
            os.environ['CC'] = 'clang'
            os.environ['CXX'] = 'clang++'
            nfbuild.generateProject(
                address_sanitizer='addressSanitizer' in options,
                gcc=False)
        else:
            nfbuild.generateProject(
                address_sanitizer='addressSanitizer' in options)

    if buildOptions.checkOption(options, 'buildTargetLibrary'):
        nfbuild.buildTarget(library_target)

    if buildOptions.checkOption(options, 'makeCLI'):
        nfbuild.makeCLI()

    if buildOptions.checkOption(options, 'makeJAR'):
        nfbuild.makeJAR()

    if buildOptions.checkOption(options, 'integrationTests'):
        nfbuild.runIntegrationTests()


if __name__ == "__main__":
    main()
