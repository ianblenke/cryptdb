#include <ope/merkle.hh>

#include <util/util.hh>
#include <util/ope-util.hh>
#include <crypto/sha.hh>

using namespace std;



ostream &
marshall_binary(ostream & o, const string & hash) {
    uint len = hash.size();
    
    o << len << " ";

    for (uint i = 0; i < len; i++) {
	o << (int)hash[i] << " ";
    }

    return o;
}

string
unmarshall_binary(istream & o) {
    uint size;
    o >> size;

    char h[size];
    
    for (uint i = 0 ; i < size; i++) {
	int c = 0;
	o >> c;
	h[i] = (char)c;
    }
    return string(h, size);    
}
/*
static ostream &
marshall_binary(ostream & o, string & hash) {
    uint len = hash.size();

    cerr << "binary len " << len <<"\n";
    o << len << " ";

      o.write(hash.c_str(), len);
    o << " ";
    
    
    return o;
}

static string
unmarshall_binary(istream & o) {
    uint size;
    o >> size;

    assert_s(size < 1024, "size too large");
    cerr << "read binary len " << size << "\n";

   char b[1];
    o.read(b, 1);
    assert_s(b[0] == ' ', "space expected");
    
    char h[size];
    o.read(h, size);


    return string(h, size);    
  
}
*/
/**** ElInfo ******/

ostream &
ElInfo::operator>>(std::ostream &out) {
    marshall_binary(out, key);
    marshall_binary(out, hash);
    return out;
}

istream &
ElInfo::operator<<(std::istream & is) {
    key = unmarshall_binary(is);
    hash = unmarshall_binary(is);
    return is;
}

string
ElInfo::pretty() const {
    return "(" + key + ", " +  read_short(short_string(hash)) + ")"; 
}

bool
ElInfo::operator==(const ElInfo & e) const {
    return key == e.key && hash == e.hash;
}


/**** Node Info ****/

NodeInfo::NodeInfo() {
    pos_in_parent = -1;
    key = "";
    childr.clear();
}

uint
NodeInfo::key_count() {
    assert_s(childr.size() >= 1, "invalid childr size");
    return childr.size() - 1;
}

static bool
operator==(const vector<ElInfo> & v1, const vector<ElInfo> & v2) {
    if (v1.size() != v2.size()) {
	return false;
    }
    for (uint i = 0; i < v1.size(); i++) {
	if (!(v1[i] == v2[i])) {
	    return false;
	}
    }

    return true;
}

   
std::ostream&
NodeInfo::operator>>(std::ostream &out) {

    marshall_binary(out, key);
    out << pos_in_parent << " ";

    out << childr.size() << " ";

    for (auto ei : childr) {
	ei >> out;
	out << " ";
    }

    return out;
    
}

std::istream&
NodeInfo::operator<<(std::istream & is) {

    key = unmarshall_binary(is);
    is >> pos_in_parent;

    int size;
    is >> size;

    childr = vector<ElInfo>(size);

    for (int i = 0; i < size; i++) {
	childr[i] << is;
    }

    return is;
}

string
NodeInfo::pretty() const {
    stringstream ss;
    ss << "(NodeInfo: key " <<  key << " pos in parent " << pos_in_parent << " childr ";

    for (uint i = 0; i < childr.size(); i++) {
	ss << childr[i].pretty() +  " ";
    }
    ss << ")";

    return ss.str();
 
}

string
format_concat(const string & key, uint key_size, const string & hash, uint hash_size) {
    string repr = string(key_size + hash_size, 0);

    assert_s(key.size() <= key_size, "size of key is larger than max allowed");

    repr.replace(0, key.size(), key);
    
    assert_s(hash.size() <= hash_size, "size of hash is larger than sha256::hashsize");

    repr.replace(key_size, hash.size(), hash);

    return repr;
}



string
NodeInfo::hash() const {

    uint repr_size = ElInfo::repr_size;
    string concat = string(childr.size() * repr_size, 0);

    uint count = 0;

    for (auto p : childr) {
	assert_s(p.hash != hash_not_extracted, "child with not extracted hash in NodeInfo::hash");
	concat.replace(repr_size * (count++), repr_size,
		       format_concat(p.key, b_key_size, p.hash, sha256::hashsize));
    }

    return sha256::hash(concat);
}


bool
NodeInfo::operator==(const NodeInfo & node) const {
    return (key == node.key) &&
	(pos_in_parent == node.pos_in_parent) &&
        (childr == node.childr);
}


/****** Merkle proof ***************/




std::ostream&
MerkleProof::operator>>(std::ostream &out) {
    out << path.size() << " ";

    for (auto ni : path) {
	ni >> out;
	out << " ";
    }

    return out;
}

std::istream&
MerkleProof::operator<<(std::istream &is) {
    uint size;
    is >> size;

    path.clear();
    
    for (uint i = 0; i < size; i++) {
	NodeInfo ni;
	ni << is;
	path.push_back(ni);
    }

    return is;
}
    
std::string
MerkleProof::pretty() const {
    string res = " [MerkleProof: ";
    for (auto ni : path) {
	res += ni.pretty() + " ";
    }
    res += "]";
    return res;
}

/********** UpInfo *******************/

bool
UpInfo::equals(UpInfo ui) {
    bool ret = (node == ui.node) &&
	(has_left_sib == ui.has_left_sib) &&
	(left_sib == ui.left_sib) &&
	(has_right_sib == ui.has_right_sib) &&
	(right_sib == ui.right_sib);

    if (!ret) {
	cerr << "differ; first \n";
	*this >> cerr;
	cerr << "\n second \n";
	ui >> cerr;
	cerr << "\n";
    }

    return ret;
}

std::ostream&
UpInfo::operator>>(std::ostream &out) {
    out << " ";
    node >> out;
    out << " ";
    out << has_left_sib << " ";
    left_sib >> out;
    out << " " << has_right_sib << " ";
    right_sib >> out;
    out << " ";
    return out;
}

std::istream&
UpInfo::operator<<(std::istream &is) {
    node << is;
    is >> has_left_sib;
    left_sib << is;
    is >> has_right_sib;
    right_sib << is;

    return is;
}

std::string
UpInfo::pretty() const {
    stringstream ss;
    ss << "{ UpInfo: node " << node.pretty() << "\n"
       << " \t\t has_left_sib " << has_left_sib
       << " left_sib " << left_sib.pretty() << "\n"
       << " \t\t has_right_sib " << has_right_sib
       << " right_sib " << right_sib.pretty()
       << " this del " << this_was_del
       << " right del " << right_was_del
       << " (left, node, right moved right, path): " << left_moved_right << node_moved_right << right_moved_right << " " << path_moved_right
       << "}";

    return ss.str();
}



/***** State  ********/

static ostream&
operator<<(ostream & out, const State & st) {
    out << " " << st.size() << " ";
    for (auto di : st) {
	di >> out;
	out << " ";
    }
    return out;
}

static istream&
operator>>(istream & is, State & st) {
    uint size;
    is >> size;
    st.clear();
    
    for (uint i = 0; i < size; i++) {
	UpInfo di;
	di << is;
	st.push_back(di);
    }

    return is;
}

std::string
PrettyPrinter::pretty(const State & st) {
    string res = " (State: ";
    for (auto di : st) {
	res += di.pretty() + " ";
    }
    res += ")";
    return res;
}


/***** Update Merkle Proof *********/


static void
hash_match(NodeInfo child, NodeInfo parent) {
    assert_s(child.hash() == parent.childr[child.pos_in_parent].hash, "hash mismatch parent and child");
}
static void
hash_match(const UpInfo & child, const UpInfo &  parent) {
    
    hash_match(child.node, parent.node);
    if (child.has_left_sib) {
	hash_match(child.left_sib, parent.node);
    }
    if (child.has_right_sib) {
	hash_match(child.right_sib, parent.node);
    }
    
}


string UpdateMerkleProof::new_hash() const {

    if (st_after.size() == 0) {
	return hash_empty_node;
    }
    UpInfo root = st_after[0];
    if (!root.this_was_del) {
	return root.node.hash();
    } else {
	assert_s(st_after.size() >= 2, "inconsistency");
	return st_after[1].node.hash();
    }
}



string
UpdateMerkleProof::old_hash() const {

    // first level
    auto it = st_before.begin();
    assert_s(it != st_before.end(), "levels in del proof empty");
    UpInfo di_parent = *it;
    it++;
    
    for (; it != st_before.end(); it++) {
	UpInfo di_child = *it;

	hash_match(di_child, di_parent);
	
	di_parent = di_child;
    }
    
    return st_before[0].node.hash();
}


ostream&
UpdateMerkleProof::operator>>(std::ostream &out){
    out << st_before << " " << st_after;
    return out;
}

istream&
UpdateMerkleProof::operator<<(std::istream &is) {
    is >> st_before >> st_after;
    return is;
}

string
UpdateMerkleProof::pretty() const {
    string res = "Update Proof: \n -- old " + PrettyPrinter::pretty(st_before) + "\n -- new  " + PrettyPrinter::pretty(st_after);
    return res;
}
  
