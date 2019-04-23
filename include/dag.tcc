# include <string>
# include <list>
# include <set>
# include <map>
# include <vector>
# include <iostream>
# include <cassert>

//namespace ppt {
//namespace dag {

typedef uint16_t Depth;

struct Node;

struct Link {
    Node * a, * b;
    Link( Node & n1, Node &n2 );
};

struct Node {
    std::string name;
    uint8_t mark;
    int16_t depth;
    std::vector<Node *> children;
    Node( const char * n ) : name(n), mark(0x0), depth(0) {}
};

Link::Link( Node & n1, Node &n2 ) : a(&n1), b(&n2) {}

//}  // namespace dag
//}  // namespace ppt

void
visit( Node * n, std::map<Depth, std::set<Node *> > & l ) {
    if( 0x1 & n->mark ) return;  // check temporary
    if( 0x2 & n->mark ) {
        std::cerr << "Revealed loop on \"" << n->name << "\"." << std::endl;
        throw std::runtime_error( "Not a DAG." );
    }
    n->mark |= 0x1;     // mark as temporary
    for( auto nPtr : n->children ) {
        if(! nPtr->mark) {
            visit( nPtr, l );
        }
        if( nPtr->depth >= n->depth ) {
            n->depth = nPtr->depth + 1;
        }
    }
    n->mark &= ~0x1;    // remove temporary
    n->mark |= 0x2;     // mark as permanent
    auto it = l.find( n->depth );
    if( l.end() == it )
        it = l.emplace( n->depth, std::set<Node *>() ).first;
    it->second.insert(n);
}

/// Just performs a topological sort.
void
dfs( std::set<Node *> & s, std::map<Depth, std::set<Node *> > & l ) {
    for( auto nPtr : s ) {
        if( !nPtr->mark ) {
            visit( nPtr, l );
        }
    }
}

