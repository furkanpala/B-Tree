#include <iostream>
#include <vector>

using namespace std;

char key; // 0 -> x, 1 -> y, 2 -> z

struct Data
{
    int x, y;
    char z;

    Data(int, int, char);
    bool operator<(const Data &) const;
    bool operator==(const Data &) const;
    friend ostream &operator<<(ostream &, const Data &);
};

Data::Data(int x, int y, char z) : x(x), y(y), z(z) {}

bool Data::operator<(const Data &data) const
{
    switch (key)
    {
    case 0:
        return x < data.x;
        break;
    case 1:
        return y < data.y;
        break;
    case 2:
        return z < data.z;
        break;
    default:
        return false;
    }
}
bool Data::operator==(const Data &data) const
{
    switch (key)
    {
    case 0:
        return x == data.x;
        break;
    case 1:
        return y == data.y;
        break;
    case 2:
        return z == data.z;
        break;
    default:
        return false;
    }
}

ostream &operator<<(ostream &out, const Data &data)
{
    out << "(" << data.x << "," << data.y << "," << data.z << ")";
    return out;
}

struct BTreeNode
{
    vector<Data> values; // Instead of keys, I keep Data objcets in which one of the attributes is the key
    vector<BTreeNode *> children;
    bool is_leaf;

    BTreeNode(bool, size_t);
    void Delete(const Data &, int);
    int SearchByKey(const Data &);
    Data GetPred(int);
    Data GetSucc(int);
    void Merge(int, int);
    void Fill(int, int);
    void BorrowFromPrev(int);
    void BorrowFromNext(int);
};

BTreeNode::BTreeNode(bool is_leaf, size_t size) : is_leaf(is_leaf)
{
    values.reserve(size);
    children.reserve(size + 1UL);
}

// Returns the index of the first greater or equal key
int BTreeNode::SearchByKey(const Data &data)
{
    int i = 0;
    while ((size_t)i < values.size() && values[i] < data)
        i++;

    return i;
}

// Returns the predecessor value of values[index]
Data BTreeNode::GetPred(int index)
{
    BTreeNode *traverse = children[index];
    while (!(traverse->is_leaf))
        traverse = traverse->children[traverse->children.size() - 1];

    return traverse->values[traverse->values.size() - 1];
}

// Returns the successor value of values[index]
Data BTreeNode::GetSucc(int index)
{
    BTreeNode *traverse = children[index + 1];
    while (!(traverse->is_leaf))
        traverse = traverse->children[0];

    return traverse->values[0];
}

/*
    this : current node
    child : ith child of current node
    sibling : (i+1)th child of current node

    Move ith value of parent to end of the values of the child
    Also move all the values of sibling to end of the child

*/
void BTreeNode::Merge(int index, int t)
{
    BTreeNode *child = children[index];
    BTreeNode *sibling = children[index + 1];

    child->values.push_back(values[index]);

    for (int i = 0; (size_t)i < sibling->values.size(); i++)
        child->values.push_back(sibling->values[i]);

    // If exists, move children too
    for (int i = 0; (size_t)i < sibling->children.size(); i++)
        child->children.push_back(sibling->children[i]);

    values.erase(values.begin() + index);
    children.erase(children.begin() + index + 1);

    delete sibling;
}

/*
    Insert from prev_sibling to current node
    Insert from current node to child
*/
void BTreeNode::BorrowFromPrev(int index)
{
    BTreeNode *child = children[index];
    BTreeNode *prev_sibling = children[index - 1];

    child->values.insert(child->values.begin(), values[index - 1]);

    if (!(child->is_leaf))
        child->children.insert(child->children.begin(), prev_sibling->children[prev_sibling->children.size() - 1]);

    values[index - 1] = prev_sibling->values[prev_sibling->values.size() - 1];

    prev_sibling->values.pop_back();

    if (!(prev_sibling->is_leaf))
        prev_sibling->children.pop_back();
}

/*
    Insert from child to current node
    Insert from current node to next_sibling
*/
void BTreeNode::BorrowFromNext(int index)
{
    BTreeNode *child = children[index];
    BTreeNode *next_sibling = children[index + 1];

    child->values.push_back(values[index]);

    if (!(child->is_leaf))
        child->children.push_back(next_sibling->children[0]);

    values[index] = next_sibling->values[0];

    next_sibling->values.erase(next_sibling->values.begin());

    if (!(next_sibling->is_leaf))
        next_sibling->children.erase(next_sibling->children.begin());
}

/*
    Inverse of SplitChild

    Fills the ith child of the current node
    because it has less than t - 1 keys
*/
void BTreeNode::Fill(int index, int t)
{
    if (index != 0 && children[index - 1]->values.size() >= (size_t)t)
        BorrowFromPrev(index);

    else if ((size_t)index != values.size() && children[index + 1]->values.size() >= (size_t)t)
        BorrowFromNext(index);

    else if ((size_t)index != values.size())
        Merge(index, t);

    else
        Merge(index - 1, t);
}

void BTreeNode::Delete(const Data &data, int t)
{
    int index = SearchByKey(data);

    // If key is in this node
    if ((size_t)index < values.size() && values[index] == data)
    {
        if (is_leaf)
            values.erase(values.begin() + index);
        else
        {
            if (children[index]->values.size() >= (size_t)t)
            {
                Data pred = GetPred(index);
                values[index] = pred;
                children[index]->Delete(pred, t);
            }
            else if (children[index + 1]->values.size() >= (size_t)t)
            {
                Data succ = GetSucc(index);
                values[index] = succ;
                children[index + 1]->Delete(succ, t);
            }
            else
            {
                Merge(index, t);
                children[index]->Delete(data, t);
            }
        }
    }
    else
    {
        if (is_leaf)
            cout << "Key does not exist" << endl;
        else
        {
            bool is_last_child;
            if ((size_t)index == values.size())
                is_last_child = true;
            else
                is_last_child = false;

            if (children[index]->values.size() < (size_t)t)
                Fill(index, t);

            if (is_last_child && (size_t)index > values.size())
                children[index - 1]->Delete(data, t);
            else
                children[index]->Delete(data, t);
        }
    }
}

struct BTree
{
    BTreeNode *root;
    int t; // Degree of B-Tree

    BTree(int);
    void SplitChild(BTreeNode *, int);
    void InsertNonFull(BTreeNode *, const Data &);
    void Insert(const Data &);
    void PreorderTraverse(BTreeNode *);
    void Delete(const Data &);
};

BTree::BTree(int t) : root(nullptr), t(t) {}

void BTree::SplitChild(BTreeNode *parent, int child_index)
{
    // Child to be splitted
    BTreeNode *child = parent->children[child_index];

    // Allocate a node that will be a new child of the parent
    // and will store t - 1 keys of child
    BTreeNode *new_child = new BTreeNode(child->is_leaf, t - 1);

    // Copy last t - 1 keys of child to new_child
    for (int i = 0; i < t - 1; i++)
        new_child->values.push_back(child->values[t + i]);

    child->values.erase(child->values.end() - (t - 1), child->values.end());

    // If child is not a leaf, then copy last t children too
    if (!(child->is_leaf))
    {
        for (int i = 0; i < t; i++)
            new_child->children.push_back(child->children[t + i]);

        child->children.erase(child->children.end() - t, child->children.end());
    }

    parent->values.insert(parent->values.begin() + child_index, child->values[t - 1]);
    child->values.erase(child->values.end() - 1);
    parent->children.insert(parent->children.begin() + child_index + 1, new_child);
}

void BTree::InsertNonFull(BTreeNode *node, const Data &data)
{
    int i = 0;
    while ((size_t)i < node->values.size() && node->values[i] < data)
        i++;

    if (node->is_leaf)
        node->values.insert(node->values.begin() + i, data);
    else
    {
        //Check if the child is full
        if (node->children[i]->values.size() == (size_t)(2 * t - 1))
        {
            // Split the child
            SplitChild(node, i);

            if (node->values[i] < data)
                i++;
        }
        InsertNonFull(node->children[i], data);
    }
}

void BTree::Insert(const Data &data)
{
    // If tree is empty
    if (root == nullptr)
    {
        // Allocate a new node
        // It is leaf
        // Maximum 2*t-1 keys
        root = new BTreeNode(true, 2 * t - 1);
        root->values.insert(root->values.begin(), data);
    }
    // If root is full
    else if (root->values.size() == (size_t)(2 * t - 1))
    {
        // Allocate a node for the new root
        BTreeNode *new_root = new BTreeNode(false, 2 * t - 1);

        // Current root becomes a child of the new root
        new_root->children.insert(new_root->children.begin(), root);

        // Make it new root
        root = new_root;

        // Split the old root
        SplitChild(root, 0);

        // Insert new data to one of the non-full childeren
        InsertNonFull(root, data);
    }
    // If root is not full
    else
        // Insert to non-full node
        InsertNonFull(root, data);
}

void BTree::PreorderTraverse(BTreeNode *node)
{
    for (auto &value : node->values)
        cout << value;

    cout << endl;

    for (auto &child : node->children)
        PreorderTraverse(child);
}

void BTree::Delete(const Data &data)
{
    if (root == nullptr)
    {
        cout << "Empty tree" << endl;
        return;
    }

    root->Delete(data, t);

    if (root->values.size() == 0)
    {
        BTreeNode *old_root = root;
        if (root->is_leaf)
            root = nullptr;
        else
            root = root->children[0];

        delete old_root;
    }
}

int main()
{
    int node_count, t;
    char _key;

    cin >> node_count;
    cin >> t;
    cin >> _key;

    BTree b_tree(t);

    if (_key == 'x')
        key = 0;
    else if (_key == 'y')
        key = 1;
    else if (_key == 'z')
        key = 2;

    int x, y;
    char z;
    int i = 0;
    while (i < node_count && cin >> x >> y >> z)
    {
        Data data(x, y, z);
        b_tree.Insert(data);
        i++;
    }

    if (key == 0)
    {
        int key_to_delete;
        cin >> key_to_delete;

        // Create a Data object in which the only important attribute is key
        // others are random
        Data data(key_to_delete, 9, 'a');

        b_tree.Delete(data);
    }
    else if (key == 1)
    {
        int key_to_delete;
        cin >> key_to_delete;

        // Create a Data object in which the only important attribute is key
        // others are random
        Data data(0, key_to_delete, 'a');

        b_tree.Delete(data);
    }
    else if (key == 2)
    {
        char key_to_delete;
        cin >> key_to_delete;

        // Create a Data object in which the only important attribute is key
        // others are random
        Data data(0, 0, key_to_delete);

        b_tree.Delete(data);
    }

    b_tree.PreorderTraverse(b_tree.root);
    return 0;
}