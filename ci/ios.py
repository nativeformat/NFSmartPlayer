#!/usr/bin/env python

from nfbuildosx import NFBuildOSX

from build_options import BuildOptions


def main():

    buildOptions = BuildOptions()
    buildOptions.addOption("debug", "Enable Debug Mode")
    buildOptions.addOption("lintPython", "Lint python files")
    buildOptions.addOption("lintCmake", "Lint cmake files")
    buildOptions.addOption("lintCpp", "Lint CPP Files")
    buildOptions.addOption("lintCppWithInlineChange",
                           "Lint CPP Files and fix them")

    buildOptions.addOption("makeBuildDirectory",
                           "Wipe existing build directory")
    buildOptions.addOption("generateProject", "Regenerate xcode project")

    buildOptions.addOption("buildTargetIphoneSimulator",
                           "Build Target: iPhone Simulator")
    buildOptions.addOption("buildTargetIphoneOS", "Build Target: iPhone OS")

    buildOptions.addOption("makeUniversalBinary", "Make Universal Binary")
    buildOptions.addOption("makeIOSFramework", "Make IOS Framework")
    buildOptions.addOption("makeIOSStaticLibrary", "Make IOS Static Library")

    buildOptions.setDefaultWorkflow("Empty workflow", [])

    buildOptions.addWorkflow("lint", "Run lint workflow", [
        'lintPython',
        'lintCmake',
        'lintCppWithInlineChange'
    ])

    buildOptions.addWorkflow("build", "Production Build", [
        'lintPython',
        'lintCmake',
        'lintCpp',
        'makeBuildDirectory',
        'generateProject',
        'buildTargetIphoneSimulator',
        'buildTargetIphoneOS',
        'makeUniversalBinary',
        'makeIOSFramework',
        'makeIOSStaticLibrary'
    ])

    framework_target = 'NFSmartPlayerObjC'
    nfbuild = NFBuildOSX()

    options = buildOptions.parseArgs()
    buildOptions.verbosePrintBuildOptions(options)

    if buildOptions.checkOption(options, 'debug'):
        nfbuild.build_type = 'Debug'

    if buildOptions.checkOption(options, 'lintPython'):
        nfbuild.lintPython()

    if buildOptions.checkOption(options, 'lintCmake'):
        nfbuild.lintCmake()

    if buildOptions.checkOption(options, 'makeBuildDirectory'):
        nfbuild.makeBuildDirectory()

    if buildOptions.checkOption(options, 'generateProject'):
        nfbuild.generateProject(ios=True)

    if buildOptions.checkOption(options, 'lintCppWithInlineChange'):
        nfbuild.lintCPP(make_inline_changes=True)
    elif buildOptions.checkOption(options, 'lintCpp'):
        nfbuild.lintCPP(make_inline_changes=False)

    if buildOptions.checkOption(options, 'buildTargetIphoneSimulator'):
        nfbuild.buildTarget(framework_target,
                            sdk='iphonesimulator',
                            arch='x86_64')

    if buildOptions.checkOption(options, 'buildTargetIphoneOS'):
        nfbuild.buildTarget(framework_target, sdk='iphoneos', arch='arm64')

    if buildOptions.checkOption(options, 'makeUniversalBinary'):
        nfbuild.makeUniversalBinary()

    if buildOptions.checkOption(options, 'makeIOSFramework'):
        nfbuild.makeIOSFramework()

    if buildOptions.checkOption(options, 'makeIOSStaticLibrary'):
        nfbuild.makeIOSStaticLibrary()


if __name__ == "__main__":
    main()
