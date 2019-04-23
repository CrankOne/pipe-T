# include "include/dag.tcc"

int
main( int argc, char * argv[] ) {
    std::set<Node *> nodes;
    std::vector<Node *> order;
    Node a("A"), b("B"), c("C"), d("D"), e("E"), f("F"), g("G"), h("H");
    # define mklink( a, b ) { a.children.push_back(&b); }
    mklink(a, d)
    mklink(b, d)
    mklink(b, e)
    mklink(c, e)
    mklink(c, h)
    mklink(d, f)
    mklink(d, g)
    mklink(e, g)
    mklink(e, h)
    # undef mklink
    # define add_node(n) { nodes.insert(&n); }
    add_node( a );
    add_node( b );
    add_node( c );
    add_node( d );
    add_node( e );
    add_node( f );
    add_node( g );
    add_node( h );
    # undef add_node
    dfs( nodes, order );
    for( auto nPtr : order ) {
        std::cout << nPtr->name << ", ";
    }
    std::cout << std::endl;
}
