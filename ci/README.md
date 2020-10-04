## CI Build Scripts
Each CI build is configured to run one of the scripts in this directory, sometimes with additional command line arguments. See the individual build configurations to view or edit the exact build pipeline commands. In general, each platform has a shell script that acts as an entry point (e.g. [osx.sh](osx.sh)) and invokes the appropriate python script (e.g. [osx.py](osx.py)). All of the platform-specific python scripts create build objects derived from [nfbuild.py](nfbuild.py). Some of the methods in the `NFBuild` python class are overriden for platform-specific functionality in the other `nfbuild*.py` files.

## Using CI Scripts Locally
The ci scripts can be used to download dependencies, generate build files with CMake, compile targets, and run tests in local build environments as well.

On OS X, the following commands may be useful. They all assume you have already cloned the project and all of its submodules, and that your current working directory is `NFSmartPlayer/`.

### Built-in Help

For a full-list of supported cmdline command, enter:

```
sh ci/osx.sh --help
```

### Running the linter
This will flag any style errors in python and CMake files, and edit C++ source files in place. It will not compile or run any targets.
```
sh ci/osx.sh lint
```

### Generating the project with debug symbols and address sanitizer hooks
This will also compile the `NFSmartPlayerCLI` and run unit and integration tests. 
NOTE: This will wipe out your `build` directory, including any arguments you may have configured for XCode targets.
If you have previously run this command and just need to re-run CMake, it is usually better to run `cmake .. -GXcode` in the existing `build` directory. 
```
sh ci/osx.sh address_sanitizer
```

### Running integration tests
This will run the integration tests locally.
If you want to skip project generation, add `-generateProject=0`.
This command will not destroy the contents of your `build` folder.
```
sh ci/osx.sh local_it
```

### Custom builds
The CI scripts allow enabling/disabling any step of the build process.
Options groups (or workflows) are specified without a preceding dash, and will activate a group of build steps.
Additionally individual build steps can be turned on or off. For example, the command below runs the address sanitizer
without wiping the build directory:
```
sh ci/osx.sh address_sanitizer -makeBuildDirectory=0
```

New build options can be added to the ci script by calling `buildOptions.addOption(optionName, optionHelpDescription)`.

New workflows can be added by calling `buildOptions.addWorkflow(workflow_name, workflow_help_description, array_of_optionName)`.

By convention, options should be in camel case, while workflows should use underscores.

## Tests
The player has two types of tests, both of which are invoked by the python scripts. The unit tests correspond to individual, executable CMake targets. The integration tests are also run in the python scripts (see the `runIntegrationTests()` method in [`nfbuild.py`](nfbuild.py)). See [`ci.yaml`](ci.yaml) for a list of all tests.  

### Adding unit tests
Once you have written your test(s) and added a CMake target, simply add the target to the list of `unit_tests` in [`ci.yaml`](ci.yaml). 

### Adding integration tests
To add an integration test you will need to commit a new test score, generate the expected audio output, and commit the expected audio.

- Add a new grapher score for your test in the [resources](../resources) directory. Let's call it `new-test.json`.
- Add a new entry to the `integration_tests` section of [ci.yaml](./ci.yaml).
- Run `python tools/generate-integration-audio.py` with appropriate arguments to generate audio. You will have to first build the `NFSmartPlayerCLI`. An example: `python tools/generate-integration-audio.py ./build/source/cli/Debug/NFSmartPlayerCLI ./ci/ci.yaml /tmp/nf-int-output.wav ./resources/smart-player-test-audio/`
- Commit the generated audio to the resources/smart-player-test-audio folder.