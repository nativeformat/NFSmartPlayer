# EQ Factory
A set of plugins that interface to the PeqBank library.
Currently provides a 3-band EQ with configurable frequency cutoffs and gains for each band. 
The default frequency cutoffs are 264 HZ for the low band and 1000 Hz for the mid band. These can be changed with the commands below if desired, but a more typical usage would be to set `lowGain`, `midGain`, and `highGain` over time in a graph. 0 dB is the default gain for all bands, and will result in a relatively flat (unchanged) frequency response compared to the original track. 

## Namespace

com.nativeformat.plugins.eq

## Plugins

A list of the plugins the EQ factory provides

### EQ

`com.nativeformat.plugin.eq.eq3band`

A 3-band EQ consisting of a low shelf, a mid-range peaking filter, and high shelf. The gain and cutoff or center frequency of each section are controlled by [audio parameters](../..).

#### Commands

* `lowCutoff` An [audio parameter](../..) specifying the cutoff frequency (Hz) for the low shelf filter, below which the `lowGain` will be applied. Defaults to 264 Hz and ranges from 0 to 22000 Hz. 
* `midFrequency` An [audio parameter](../..) specifying the center frequency (Hz) of the peq filter, at which `midGain` will be applied. Defaults to 1000 Hz and ranges from 0 to 22000 Hz. 
* `highCutoff` An [audio parameter](../..) specifying the cutoff frequency for the high shelf filter, above which the `highGain` will be applied. Defaults to 3300 Hz and ranges from 0 to 22000 Hz. 
* `lowGain` An [audio parameter](../..) specifying the gain (dB) for the low shelf. Defaults to 0 dB and ranges from -24 to 24 dB.
* `midGain` An [audio parameter](../..) specifying the gain (dB) for the mid range filter. Defaults to 0 dB and ranges from -24 to 24 dB.
* `highGain` An [audio parameter](../..) specifying the gain (dB) for the high shelf. Defaults to 0 dB and ranges from -24 to 24 dB.
