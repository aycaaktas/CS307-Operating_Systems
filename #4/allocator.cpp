#include <iostream>



using namespace std;

class HeapManager {
private:
    struct Node {
        int ID;
        int size;
        int index;
        Node* next;

        Node(int id, int sz, int idx) : ID(id), size(sz), index(idx), next(nullptr) {}
    };

    Node* head;
   
    

public:

    
    HeapManager() : head(nullptr) {}


    int initHeap(int size) {
        head = new Node(-1, size, 0);
        cout<<"Memory initialized\n";
        print(); 
        return 1;
    }

    int myMalloc(int ID, int size) {
        Node* current = head;
        Node* prev = nullptr;

        while (current != nullptr) {
            if (current->ID == -1 && current->size >= size) {
                int startIndex = current->index;
                if (current->size > size) {
                    Node* newNode = new Node(ID, size, startIndex);
                    newNode->next = current;
                    current->index += size;
                    current->size -= size;
                    if (prev != nullptr) {
                        prev->next = newNode;
                    } else {
                        head = newNode;
                    }
                } else {
                    current->ID = ID;
                }
                cout<<"Allocated for thread "<<ID<<"\n";
                print(); 
                return startIndex;
            }
            prev = current;
            current = current->next;
        }
        return -1;
    }

    int myFree(int ID, int index) {
        Node* current = head;
        Node* prev = nullptr;

        while (current != nullptr) {
            if (current->ID == ID && current->index == index) {
                current->ID = -1;
                // Coalescing adjacent free nodes
                if (prev != nullptr && prev->ID == -1) {
                    prev->size += current->size;
                    prev->next = current->next;
                    delete current;
                    current = prev;
                }

                Node* next = current->next;
                if (next != nullptr && next->ID == -1) {
                    current->size += next->size;
                    current->next = next->next;
                    delete next;
                }
                cout<<"Freed for thread "<<ID<<"\n";
                print(); 
                return 1;
            }
            prev = current;
            current = current->next;
        }
        return -1;
    }

    void print() {
        Node* current = head;
        while (current != nullptr) {
            cout << "[" << current->ID << "][" << current->size << "][" << current->index << "]";
            if (current->next != nullptr) {
                cout << "---";
            }
            current = current->next;
        }   
        cout << "\n";
    }

};


