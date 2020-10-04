# Audio Parameter

A meta type within other plugins that can define a value over time using complex transforms to figure out what its current value is. These are special parameters in the sense that are also interactable via OSC from the player as well as the get/setVariable interface the player provides.

## Parameter Path Format

`node-identifier/node-type.parameter-name`

* Node Identifier: The id of the node in graph (e.g. gain-17)
* Node Type: The type of the node (e.g. com.nativeformat.plugin.waa.gain)
* Parameter Name: The name of the parameter within the node (e.g. gain)

## Commands

* `setValueAtTime: number value, absolutetime time` A concrete value to set at a given time.
* `value: number value` Sets the value at time 0.
* `linearRampToValueAtTime: number value, absolutetime time` Specifies a linear ramp that will reach the given value at a certain time.
* `setTargetAtTime: number target, absolutetime time, number time_constant` Specifies a target to reach starting at a given time and gives a constant with which to guide the curve along.
* `exponentialRampToValueAtTime: number value, absolutetime time` Specifies an exponential ramp that will reach the given value at a certain time.
* `setValueAtCurveAtTime: string values, absolutetime start_time, relativetime duration` Specifies a curve to render based on the comma separated float values contained within the values string.
