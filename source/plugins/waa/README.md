# WAA Factory
A set of plugins that mimic the web audio API nodes.

## Namespace

com.nativeformat.plugins.waa

## Plugins

A list of the plugins the WAA factory provides

### Biquad

`com.nativeformat.plugin.waa.biquad`

#### Commands

* `frequency` An [audio parameter](../..) controlling the frequency to modify.
* `detune` A readonly [audio parameter](../..) telling us the detune in the frequency.
* `Q` An [audio parameter](../..) defining the quality.
* `gain` An [audio parameter](../..) defining the volume to give to the frequency.
* `type: string type` Defines the type of biquad to use, the options are; lowpass, highpass, bandpass, lowshelf, highshelf, peaking, notch, allpass

### Convolver

`com.nativeformat.plugin.waa.convolver`

#### Commands

* `buffer: string buffer` A comma separated list of floating point numbers in the domain of [-1.0, 1.0] that correspond to the impulse response of the reverb to model
* `buffer.samplerate: number samplerate` The samplerate of the buffer
* `buffer.numberOfChannels: number channels` The number of channels in the buffer
* `normalize` Whether or not to normalise the reverb

### Delay

`com.nativeformat.plugin.waa.delay`

#### Commands

* `delayTime` An [audio parameter](../..) controlling the current delay time on the node.

### Dynamics Compressor

`com.nativeformat.plugin.waa.dynamicscompressor`

#### Commands

* `threshold` An [audio parameter](../..) specifying the threshold to compress the audio with.
* `knee` An [audio parameter](../..).
* `ratio` An [audio parameter](../..).
* `reduction` An [audio parameter](../..).
* `attack` An [audio parameter](../..).
* `release` An [audio parameter](../..).

### Gain

`com.nativeformat.plugin.waa.gain`

#### Commands

* `gain` An [audio parameter](../..) specifying the volume to change the incoming signal to.
