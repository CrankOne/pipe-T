# include <string>
# include <list>
# include <set>
# include <vector>
# include <iostream>
# include <cassert>

//namespace ppt {
//namespace dag {

struct Node;

struct Link {
    Node * a, * b;
    Link( Node & n1, Node &n2 );
};

struct Node {
    std::string name;
    uint8_t mark;
    std::vector<Node *> children;
    Node( const char * n ) : name(n), mark(0x0) {}
};

Link::Link( Node & n1, Node &n2 ) : a(&n1), b(&n2) {}

//}  // namespace dag
//}  // namespace ppt

void
visit( Node * n, std::vector<Node *> & l ) {
    if( 0x1 & n->mark ) return;  // check temporary
    if( 0x2 & n->mark ) {
        std::cerr << "Revealed loop on \"" << n->name << "\"." << std::endl;
        throw std::runtime_error( "Not a DAG." );
    }
    n->mark |= 0x1;     // mark as temporary
    for( auto nPtr : n->children ) { if(! nPtr->mark) visit( nPtr, l ); }
    n->mark &= ~0x1;    // remove temporary
    n->mark |= 0x2;     // mark as permanent
    l.insert(l.begin(), n);
}

void
dfs( std::set<Node *> & s, std::vector<Node *> & l ) {
    for( auto nPtr : s ) {
        if( !nPtr->mark ) {
            visit( nPtr, l );
        }
    }
}

