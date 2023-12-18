//
//  main.cpp
//  addressContactBook
//
//  Created by Avinash Sai Sriramoju on 16/12/23.
//

#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <cstdlib>
#include <limits>
#include <algorithm>
#include <unordered_map>

class TrieNode {
public:
    std::unordered_map<char, TrieNode*> children;
    long offset; // Store the offset in the file for quick access

    TrieNode() : offset(-1) {}

    ~TrieNode() {
        for (auto& pair : children) {
            delete pair.second;
        }
    }
};

class Contact {
public:
    std::string firstName;
    std::string lastName;
    std::string address;
    std::string phoneNumber;

    Contact(const std::string& firstName, const std::string& lastName,
            const std::string& address, const std::string& phoneNumber)
        : firstName(firstName), lastName(lastName), address(address), phoneNumber(phoneNumber) {}

    std::string toString() const {
        std::string s = std::string("Contact{") +
            "firstName='" + firstName + '\'' +
            ", lastName='" + lastName + '\'' +
            ", address='" + address + '\'' +
            ", phoneNumber='" + phoneNumber + '\'' +
            '}';
        return s;
    }

    friend std::istream& operator>>(std::istream& is, Contact& contact) {
        std::getline(is >> std::ws, contact.firstName, ',');
        std::getline(is >> std::ws, contact.lastName, ',');
        std::getline(is >> std::ws, contact.address, ',');
        std::getline(is >> std::ws, contact.phoneNumber);
        return is;
    }

    friend std::ostream& operator<<(std::ostream& os, const Contact& contact) {
        os << contact.firstName << "," << contact.lastName << "," << contact.address << "," << contact.phoneNumber;
        return os;
    }
};

class AddressBook {
private:
    std::string filename;  // Store the filename/path separately
    mutable std::fstream storage;
    long nextOffset;
    TrieNode* phoneIndexRoot;
    TrieNode* nameIndexRoot;

    long writeContact(const Contact& contact) {
        long offset = nextOffset;
        storage.seekp(offset, storage.beg);

        std::ostringstream oss;
        oss << contact;

        std::string data = oss.str();
        int length = static_cast<int>(data.length());

        storage.write(reinterpret_cast<const char*>(&length), sizeof(length));
        storage.write(data.c_str(), length);

        nextOffset = storage.tellp();

        return offset;
    }

    Contact readContact(long offset) const {
        std::fstream fileCopy(filename, std::ios::in | std::ios::binary);

        fileCopy.seekg(offset);
        int length;
        fileCopy.read(reinterpret_cast<char*>(&length), sizeof(length));

        char* data = new char[length + 1];
        data[length] = '\0'; // Null-terminate the string
        fileCopy.read(data, length);

        std::string dataString(data);

        // Use a std::istringstream to extract values from the string
        std::istringstream iss(dataString);
        Contact contact("", "", "", "");
        iss >> contact;

        delete[] data;

        return contact;
    }
    
    

    void insertIntoTrie(TrieNode* root, const std::string& key, long offset) {
        TrieNode* current = root;
        for (char c : key) {
            if (current->children.find(c) == current->children.end()) {
                current->children[c] = new TrieNode();
            }
            current = current->children[c];
        }
        current->offset = offset;
    }

    long searchInTrie(const TrieNode* root, const std::string& key) const {
        const TrieNode* current = root;
        for (char c : key) {
            auto it = current->children.find(c);
            if (it == current->children.end()) {
                return -1; // Not found
            }
            current = it->second;
        }
        return current->offset;
    }
    


public:
    AddressBook(const std::string& filename)
        : filename(filename) {  // Initialize the filename
        storage.open(filename, std::ios::in | std::ios::out | std::ios::binary);
        if (!storage.is_open()) {
            storage.open(filename, std::ios::out | std::ios::binary);
        }

        storage.seekg(0, storage.end);
        nextOffset = storage.tellg();
        storage.seekg(0, storage.beg);

        // Initialize trie roots
        phoneIndexRoot = new TrieNode();
        nameIndexRoot = new TrieNode();
    }

    ~AddressBook() {
        delete phoneIndexRoot;
        delete nameIndexRoot;
    }

    void addContact(const Contact& contact) {
        long offset = writeContact(contact);
        insertIntoTrie(phoneIndexRoot, contact.phoneNumber, offset);
        insertIntoTrie(nameIndexRoot, contact.firstName + " " + contact.lastName, offset);
    }

    Contact searchByPhoneNumber(const std::string& phoneNumber) const {
        long offset = searchInTrie(phoneIndexRoot, phoneNumber);
        if (offset != -1) {
            return readContact(offset);
        }
        return Contact("", "", "", ""); // Return an empty contact if not found
    }

    Contact searchByName(const std::string& name) const {
        std::string lowercaseName = toLowerCase(name);
        long offset = searchInTrie(nameIndexRoot, lowercaseName);
        if (offset != -1) {
            return readContact(offset);
        }
        return Contact("", "", "", ""); // Return an empty contact if not found
    }

    
    std::string toLowerCase(const std::string& str) const {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }

    void close() {
        storage.close();
    }
};

int main() {
    try {
        AddressBook addressBook("address_book.dat");

        // Mock data
        addressBook.addContact(Contact("Avinash", "test", "Bengaluru", "9676806379"));
        addressBook.addContact(Contact("first", "last", "test address ", "1234567890"));

        // CLI
        while (true) {
            std::cout << "\n1. Add Contact\n"
                      << "2. Search by Phone Number\n"
                      << "3. Search by Name\n"
                      << "4. Exit\n"
                      << "Choose an option: ";

            int choice;
            std::cin >> choice;

            if (std::cin.fail()) {
                // Handle invalid input (non-integer)
                std::cin.clear(); // Clear the error flag
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Discard invalid input
                std::cout << "Invalid choice. Please enter a number.\n";
                continue;
            }

            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Consume the newline

            switch (choice) {
                case 1: {
                    std::string firstName, lastName, address, phoneNumber;
                    std::cout << "Enter First Name: ";
                    std::getline(std::cin, firstName);

                    std::cout << "Enter Last Name: ";
                    std::getline(std::cin, lastName);

                    std::cout << "Enter Address: ";
                    std::getline(std::cin, address);

                    std::cout << "Enter Phone Number: ";
                    std::getline(std::cin, phoneNumber);

                    addressBook.addContact(Contact(firstName, lastName, address, phoneNumber));
                    std::cout << "Contact added successfully!\n";
                    break;
                }

                case 2: {
                    std::string searchPhoneNumber;
                    std::cout << "Enter Phone Number to Search: ";
                    std::getline(std::cin, searchPhoneNumber);

                    long startTime = std::chrono::high_resolution_clock::now().time_since_epoch().count();
                    Contact resultByPhoneNumber = addressBook.searchByPhoneNumber(searchPhoneNumber);
                    long endTime = std::chrono::high_resolution_clock::now().time_since_epoch().count();

                    if (resultByPhoneNumber.firstName != "") {
                        std::cout << "Search Result: " << resultByPhoneNumber << "\n";
                    } else {
                        std::cout << "Contact not found.\n";
                    }

                    std::cout << "Search time: " << (endTime - startTime) / 1e6 << " milliseconds\n";
                    break;
                }

                case 3: {
                    std::string searchName;
                    std::cout << "Enter Name to Search: ";
                    std::getline(std::cin, searchName);

                    long nameStartTime = std::chrono::high_resolution_clock::now().time_since_epoch().count();
                    Contact resultByName = addressBook.searchByName(searchName);
                    long nameEndTime = std::chrono::high_resolution_clock::now().time_since_epoch().count();

                    if (resultByName.firstName != "") {
                        std::cout << "Search Result: " << resultByName << "\n";
                    } else {
                        std::cout << "Contact not found.\n";
                    }

                    std::cout << "Search time: " << (nameEndTime - nameStartTime) / 1e6 << " milliseconds\n";
                    break;
                }

                case 4:
                    addressBook.close();
                    std::cout << "Exiting Address Book. Goodbye!\n";
                    std::exit(0);
                    break;

                default:
                    std::cout << "Invalid choice. Please try again.\n";
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
