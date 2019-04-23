# include "include/dag.tcc"

int
main( int argc, char * argv[] ) {
    std::set<Node *> nodes;
    std::map< Depth, std::set<Node *> > order;
    Node a("A"), b("B"), c("C"), d("D"), e("E"), f("F"), g("G"), h("H");
    # define mklink( a, b ) { b.children.push_back(&a); }
    # if 0
    mklink(a, d)
    mklink(b, d)
    mklink(b, e)
    mklink(c, e)
    mklink(c, h)
    mklink(d, f)
    mklink(d, g)
    mklink(e, g)
    mklink(e, h)
    # else
    mklink(a, b)
    mklink(a, c)
    mklink(a, d)
    mklink(a, g)
    mklink(c, e)
    mklink(c, f)
    mklink(d, c)
    mklink(d, f)
    mklink(d, h)
    mklink(f, e)
    mklink(f, g)
    # endif
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
    for( auto p : order ) {
        std::cout << "Tier #" << p.first << ": {";
        for( auto pp: p.second ) {
            std::cout << pp->name << ", ";
        }
        std::cout << "}" << std::endl;
    }
    std::cout << std::endl;
}
