//
// Created by Ryan Vilim on 2/21/22.
//

#include<vector>

#ifndef PILES_TREI_H
#define PILES_TREI_H

using namespace std;

class Node {
public:
    Node* parent;
    vector<Node*> children;
    vector<int>* pile;
};

//
//struct Node{
//    Node* parent;
//    vector<Node*> children;
//    vector<int>* pile;
//};

Node add_child(Node &node, vector<int> &pile){
    Node child_node;

    child_node.pile=&pile;
    child_node.parent = &node;
    node.children.push_back(&child_node);

    return child_node;
}
/*
vector<int> compile_disallowed(Node &node){
    vector<int> disallowed((*node.pile).size(), false);

    auto n = node;
    do{
        for(int i=0; i<(*node.pile).size();i++){
            disallowed[i]|=(*n.pile)[i];
        }
        if(*n.parent == nullptr ) break;
        n = *n.parent;
    }while(true);

    return disallowed;
}*/
#endif //PILES_TREI_H
