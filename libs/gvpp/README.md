# Graphviz ++

This is a small and simple C++ library to produce .gv files for GraphViz.
It support both narrow and wide character strings.

## Usage

#### Creating elements

*In the following, str means `string` if `chartype == char`,
or `wstring` if `chartype == wchar_t`*

Every class is templated by the char type of the strings it will hold, though
using <> will default to char. The only class instanciable by the user is Graph,
which has corresponding methods to create child elements. Those are:

* addNode(str id, str label = "", bool forcenew=false) creates a node with the corresponding id, or returns it if there already is one. If `forcenew == true`, an error will be thrown if the node already exists.
* addEdge(Node &node1, Node &node2, str label = "") creates an edge `node1` --> `node2`
* addSubGraph(str name, bool cluster = false, str label = "") creates a subgraph. If `cluster` is true and name doesn't start with "cluster_", it is prepended.

#### Setting attributes

All elements have a `set` method to set element values, as well as corresponding
`get` and `has` methods. `set` also always return a reference to the element it
was called upon, as to allow for method chaining.

Node and Edge only have one type of Element, so `set` just takes a key and a value:
 * `Node/Edge &set(str key, str value)`

Graph can have Graph attributes for itself, or Edge/Node attributes that act as
default for its children, thus it takes a type as first argument:
 * `Graph &set(AttrType type, str key, str value)` with `type` one of:
{`AttrType::GRAPH`, `AttrType::NODE`, `AttrType::EDGE`}

And finally, SubGraph being both a Graph (inherits AbstractGraph) and a graph
element, it has both signatures. The type-less one defaults to `AttrType::GRAPH`.

#### Printing

To output the graph as a .gv file, just pass it to an output stream through <<.
The stream's chartype has to match the Graph's, obviously. Additionally, gvpp
also provides `renderToFile(Graph &g, string layout, string format, string file)`
which calls graphviz to render the graph file directly, and save it as a file, or
display it if the format specifies it. `renderToScreen(Graph &g, string layout)`
is a shortcut for renderToFile with format="x11".
