# NativeFormat Scripting

The nativeformat player has the ability to run scripts alongside its rendering of a graph in order to allow for dynamically changing the nodes/edges within a particular graph as well as interacting with outside messages.

## API

The Javascript API is broken down into a object-orientated architecture.

### Global

Here we have the ability to get the player object from the global scope:

```javascript
class NF {
    getPlayer(): Player;
}
```

Example:

```javascript
var player = global.NF.getPlayer()
```

### Player

After the player has been obtained from the global scope, we can use its API to modify it.

```javascript
class Player {
    let graphs: [Graph];
    let scripts: [Script];
    let loaded: bool;

    renderTime: number;
    playing: bool;

    registerMessage(message_identifier: string, sender_identifier: string, callback: function): int;
    sendMessage(message_identifier: string, message_type: int, payload: Object);
    unregisterMessage(handle: int);
    toJSON(): string;
    setJSON(json: string);
}
```

### Graph

A graph is a piece of information telling us how to render a particular piece of content.

```javascript
class Graph {
    let nodes: [Node];
    let edges: [Edge];
    let outputNode: Node;
    let identifier: string;
    let type: string;

    toJSON(): string;
    setJSON(json: string);
    addNode(node: Node);
    removeNode(identifier: string);
    valueAtPath(path: string): number;
    setValueAtPath(path: string, ...);
    addEdge(edge: Edge);
    removeEdge(identifier: string);

    constructor(json: string);
}
```

### Node

A node tells the graph which plugins to execute.

```javascript
class Node {
    let identifier: string;
    let type: string;
    let isOutputNode: bool;

    toJSON(): string;

    constructor(identifier: string, type: string);
}
```

### Edge

An edge tells the graph how to connect two nodes.

```javascript
class Edge {
    let identifier: string;
    let source: string;
    let target: string;

    toJSON(): string;

    constructor(identifier: string, source: string, target: string);
}
```

### Script

A script modifies the graph at runtime in response to messages.

```javascript
class Script {
    let name: string;
    let scope: int;

    close();
}
```
