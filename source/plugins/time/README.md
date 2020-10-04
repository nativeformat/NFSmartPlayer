# Time Factory
A set of plugins that can manipulate time in the graph.

## Namespace

com.nativeformat.plugins.time

## Plugins

A list of the plugins the Time factory provides

### Loop

`com.nativeformat.plugin.time.loop`

#### Commands

* `node.start: absolutetime start_time, relativetime duration, optional number loops` Activates the loop node.

### Stretch

`com.nativeformat.plugin.time.stretch`

A plugin that can independently stretch time and shift the pitch of an audio stream. 

#### Commands

* `stretch` An [audio parameter](../..) specifying the time stretch multiplier. For example, a stretch value of 2.0 will make audio play at half the original speed. Defaults to 1.0, which leaves the time unchanged.
* `pitchRatio` An [audio parameter](../..) specifying the pitch multiplier. For example, a pitchRatio value of 2.0 will double the original audio frequencies. Defaults to 1.0, which will leave the pitch unchanged. 

Note that this node can be used to alter just the time, just the pitch, or both. 