Scene graph
===========

**WARNING: USE AT YOUR OWN RISK. The Scene graph is currently in Alpha
mode and will change frequently. It is not yet recommended for critical
production work.**

The scene graph is the basis of our Example Viewer which consists of a
superset of OSPRay objects represented in a graph hierarchy (currently a
tree). This graph functions as a hierarchical specification for scene
properties and a self-managed update graph. The scene graph
infrastructure includes many convenience functions for templated
traversals, queries of state and child state, automated updates, and
timestamped modifications to underlying state.

The scene graph nodes closely follow the dependencies of existing OSPRay
API internals, i.e., a `sg::Renderer` has a "`model`" child, which in
turn has a "`TriangleMesh`", which in turn has a child named "`vertex`"
similar to how you may set the "`vertex`" parameter on the
`osp::TriangleMesh` which in turn is added to an `OSPModel` object which
is set as the model on the `OSPRenderer`. The scene graph is a superset
of OSPRay functionality so there is no direct 1:1 mapping between the
scene graph hierarchy in all cases, however it is kept as close as
possible. This makes the scene graph viewer in the [Example Viewer] a
great way to understand OSPRay state.


Hierarchy Structure
-------------------

The root of the scene graph is based on `sg::Renderer`. The scene graph can
be created by

    auto renderer = sg::createNode("renderer", "Renderer");

which automatically creates child nodes for necessary OSPRay state. To
update and commit all state and render a single function is provided
which can be called with:

    renderer.renderFrame(renderer["frameBuffer"].nodeAs<sg::FrameBuffer>);

Values can be set using:

    renderer["spp"] = 16;

The explore the full set of nodes, simply launch the Example Viewer and
traverse through the GUI representation of all scene graph nodes.


Traversals
---------

The scene graph contains a set of builtin traversals as well as modular
visitor functors for implementing custom passes over the scene graph. The
required traversals are handled for you by default within the
renderFrame function on the renderer. For any given node there are two
phases to a traversal operation, pre and post traversal of the nodes
children. `preTraversal` initializes node state and objects and sets the
current traversal context with appropriate state. For instance,
`sg::Model` will create a new `OSPModel` object, set its value to that
object, and set `sg::RenderContext.currentOSPModel` to its own value.
After `preTraversal` is finished, the children of `sg::Model` are
processed in a similar fashion and now use the modified context. In
`postTraversal`, `sg::Model` will commit the changes that its children
have potentially set and it will pop its modifications from the current
context. This behavior is replicated for every scene graph node and
enables children to act on parent state without specific implementations
from the parent node. An example of this are the `sg::NodeParam` nodes
which are containers for values to be set on `OSPObject`s, such as a
float value. This is put on the scene graph with a call to:

    renderer["lights"]["sun"].createChild("intensity", "float", 0.3f);

This call accesses the child named "`lights`" on the renderer, and in
turn the child named "`sun`". This child then gets its own child of a
newly created node with the name "`intensity"` of type `float` with a
value of `0.3f`. When committed, this node will call `ospSet1f `with the
node value on the current `OSPObject` on the context which is set by the
parent. If you were to create a custom light called "`MyLight`" and had
a float parameter called "`flickerFreq`", a similar line would be used
without requiring any additional changes in the scene graph internals
beyond registering the new light class. Known parameters such as floats
will also show up in the Example Viewer GUI without requiring any
additional code beyond adding them to the scene graph and the internal
implementation in OSPRay.

The base passes required to use the scene graph include `verification`,
`commit`, and `render` traversals. Every node in the scene graph has a valid
state which needs to be set before operating on the node. Nodes may have
custom qualifications for validity, but by default they are set through
`valid_` flags on the scene graph `Node` for things like whitelists and range
checks. Once verified, `commit` traverses the scene graph and commits
scene graph state to OSPRay. Commits are timestamped, so re-committing
will only have any affect if a dependent child has been modified
requiring a new commit. Because of this, each node does not have to
track if it is valid or if anything in the scene has been modified, as
commit will only be called on that node if those are already true. By
default invalid nodes with throw exceptions, however this can be turned
off which enables the program to keep running. In the Example Viewer GUI,
invalid nodes will be marked in red but the previously committed state
will keep rendering until the invalid state is corrected.

For examples of implementing custom traversals, see the `sg/visitors`
folder. Here is an example of a visitor that collects all nodes with a
given name:

    struct GatherNodesByName : public Visitor
    {
      GatherNodesByName(const std::string &_name);

      bool operator()(Node &node, TraversalContext &ctx) override;

      std::vector<std::shared_ptr<Node>> results();

    private:
      std::string name;
      std::vector<std::shared_ptr<Node>> nodes;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    inline GatherNodesByName::GatherNodesByName(const std::string &_name)
        : name(_name)
    {
    }

    inline bool GatherNodesByName::operator()(Node &node, TraversalContext &)
    {
      if (utility::longestBeginningMatch(node.name(), this->name) == this->name) {
        auto itr = std::find_if(
          nodes.begin(),
          nodes.end(),
          [&](const std::shared_ptr<Node> &nodeInList) {
            return nodeInList.get() == &node;
          }
        );

        if (itr == nodes.end())
          nodes.push_back(node.shared_from_this());
      }

      return true;
    }

    inline std::vector<std::shared_ptr<Node>> GatherNodesByName::results()
    {
      return nodes;// TODO: should this be a move (i.e., reader 'consumes')?
    }


Thread Safety
-------------

The scene graph is only thread safe for accessing and setting values on
nodes. More advanced operations like adding or removing nodes are not
thread safe. At some point we hope to add transactions to handle these,
but for now the scene graph nodes must be added/removed on the same
thread that is committing and rendering.
