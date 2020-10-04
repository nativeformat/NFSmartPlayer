# File Factory
A set of plugins that play different types of media files.

## Namespace

com.nativeformat.plugins.file

## Plugins

A list of the plugins the File factory provides

### OGG

`com.nativeformat.plugin.file.ogg`

#### Commands

* `setFile: string file_location` This tells the plugin where to pull the file from, it supports both local files and HTTP based files
* `setStartTime: absolutetime start_time` This tells the plugin when to start playing the OGG file
* `setDuration: relativetime duration` This tells the plugin how long to play the OGG for
* `setTrackStartTime: relativetime track_start_time` This tells the plugin when to start within the track
* `setChannel: number channel` This tells the plugin which channel to play (useful for MOGG files)
