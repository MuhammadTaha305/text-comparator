#include "simple_analyzer.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_map>
#include <filesystem>
#include <chrono>
#include <ctime>
#include <sstream>
#include <unordered_set>

using namespace std;

class FileHistory {
private:
    vector<string> history;
    static const int MAX_HISTORY = 10;
    unordered_map<string, pair<int, double>> fileStats; // filename -> (uses, last_similarity)
    // Folder history
    vector<string> folderHistory;

public:
    void addFile(const string& filename) {
        // Move to front
        auto it = find(history.begin(), history.end(), filename);
        if (it != history.end()) {
            history.erase(it);
        }
        
    // Insert
        history.insert(history.begin(), filename);
        
    // Trim
        if (history.size() > MAX_HISTORY) {
            history.resize(MAX_HISTORY);
        }
        
    // Update stats
        if (fileStats.find(filename) != fileStats.end()) {
            fileStats[filename].first++; // Increment usage count
        } else {
            fileStats[filename] = make_pair(1, 0.0); // First time use
        }
    }
    
    void addFolder(const string& folder) {
        auto it = find(folderHistory.begin(), folderHistory.end(), folder);
        if (it != folderHistory.end()) {
            folderHistory.erase(it);
        }
        folderHistory.insert(folderHistory.begin(), folder);
        if (folderHistory.size() > MAX_HISTORY) {
            folderHistory.resize(MAX_HISTORY);
        }
    }
    
    void updateFileSimilarity(const string& filename, double similarity) {
        // Update similarity score in hashmap
        if (fileStats.find(filename) != fileStats.end()) {
            fileStats[filename].second = similarity;
        }
    }
    
    const vector<string>& getHistory() const {
        return history;
    }
    const vector<string>& getFolderHistory() const { return folderHistory; }
    
    // File usage table
    void printFileStats() const {
        cout << "\n=== File Usage Statistics ===\n";
        if (fileStats.empty()) {
            cout << "No file statistics available.\n";
            return;
        }

    // Copy to vector
        vector<pair<string, pair<int, double>>> sortedStats;
        sortedStats.reserve(fileStats.size());
        for (const auto& stat : fileStats) {
            sortedStats.emplace_back(stat.first, stat.second);
        }

    // Sort by uses
        sort(sortedStats.begin(), sortedStats.end(),
             [](const auto& a, const auto& b) {
                 if (a.second.first == b.second.first) return a.first < b.first; // tie-break by name
                 return a.second.first > b.second.first;
             });

        auto baseName = [](const string& path) {
            size_t pos = path.find_last_of("/\\");
            if (pos == string::npos) return path;
            return path.substr(pos + 1);
        };

    // Width
        size_t maxName = 8;
        for (const auto& s : sortedStats) {
            maxName = max(maxName, baseName(s.first).size());
        }
        maxName = min<size_t>(maxName, 32);

        // Header
        cout << left << setw(6) << "#"
             << left << setw(static_cast<int>(maxName)+2) << "Filename"
             << right << setw(12) << "Uses"
             << right << setw(14) << "Last Sim" << "\n";
        cout << string(6 + (maxName+2) + 12 + 14, '-') << "\n";

        int index = 1;
        for (const auto& stat : sortedStats) {
            string name = baseName(stat.first);
            if (name.size() > maxName) {
                name = name.substr(0, maxName-1) + "~";
            }
            cout << left << setw(6) << index++
                 << left << setw(static_cast<int>(maxName)+2) << name
                 << right << setw(12) << stat.second.first;
            if (stat.second.second > 0.0) {
                cout << right << setw(14) << fixed << setprecision(2) << stat.second.second << "%";
            } else {
                cout << right << setw(14) << "N/A";
            }
            cout << "\n";
        }
    }
    
    // Most used files
    vector<string> getMostUsedFiles(int count = 5) const {
        vector<pair<string, int>> usageCounts;
        
        for (const auto& stat : fileStats) {
            usageCounts.push_back(make_pair(stat.first, stat.second.first));
        }
        
        sort(usageCounts.begin(), usageCounts.end(),
             [](const auto& a, const auto& b) {
                 return a.second > b.second;
             });
        
        vector<string> result;
        for (int i = 0; i < min(count, static_cast<int>(usageCounts.size())); ++i) {
            result.push_back(usageCounts[i].first);
        }
        
        return result;
    }
};

string getPathWithHistory(const string& prompt, FileHistory& history, bool isFolder);
static string trim(const string& s);
static bool isTextFile(const string& path);
struct DocVersion {
    string label;
    string path;
    vector<string> sentences;
};

struct VersionGroup {
    string name;
    vector<DocVersion> versions;
};

static VersionGroup* findVersionGroup(vector<VersionGroup>& groups, const string& name) {
    for (auto& g : groups) {
        if (g.name == name) return &g;
    }
    return nullptr;
}

static void listVersionGroups(const vector<VersionGroup>& groups) {
    if (groups.empty()) { cout << "No version groups defined.\n"; return; }
    for (size_t i=0;i<groups.size();++i) {
        cout << (i+1) << ") " << groups[i].name << " (" << groups[i].versions.size() << " versions)\n";
    }
}

static void addVersionToGroup(vector<VersionGroup>& groups, const string& groupName, const string& label, const string& path) {
    VersionGroup* grp = findVersionGroup(groups, groupName);
    if (!grp) { groups.push_back({groupName,{}}); grp = &groups.back(); }
    // load file
    string content = loadFile(path);
    if (content.empty()) { cout << "Error: could not load file.\n"; return; }
    vector<string> sentences = splitIntoSentences(content);
    grp->versions.push_back({label, path, sentences});
    cout << "Added version '" << label << "' with " << sentences.size() << " sentences.\n";
}

static void compareVersions(const DocVersion& original, const DocVersion& newer) {
    unordered_set<string> origSet(original.sentences.begin(), original.sentences.end());
    unordered_set<string> newSet(newer.sentences.begin(), newer.sentences.end());
    vector<string> added, removed;
    for (const auto& s : newer.sentences) if (!origSet.count(s)) added.push_back(s);
    for (const auto& s : original.sentences) if (!newSet.count(s)) removed.push_back(s);
    cout << "\n=== Version Comparison ===\n";
    cout << "Original: " << original.label << "  New: " << newer.label << "\n";
    cout << "Sentences in new NOT in original (added): " << added.size() << "\n";
    for (const auto& s : added) cout << "  + " << s << "\n";
    cout << "Sentences in original NOT in new (removed): " << removed.size() << "\n";
    for (const auto& s : removed) cout << "  - " << s << "\n";
}

static void manageVersionGroups(FileHistory& history, vector<VersionGroup>& groups) {
    while (true) {
        cout << "\n=== Version Groups ===\n";
        cout << "1. List groups\n";
        cout << "2. Add version to group\n";
        cout << "3. List versions in a group\n";
        cout << "4. Compare two versions in a group\n";
        cout << "5. Back to main menu\n";
        cout << "Enter choice: ";
        int c; if(!(cin>>c)){cin.clear();cin.ignore(10000,'\n'); continue;} cin.ignore();
        if (c==1) {
            listVersionGroups(groups);
        } else if (c==2) {
                string gName, label;
                cout << "Group name: "; getline(cin,gName);
                // Ensure label is unique within group
                while (true) {
                    cout << "Version label: "; getline(cin,label);
                    VersionGroup* existing = findVersionGroup(groups, gName);
                    bool dup = false;
                    if (existing) {
                        for (const auto& v : existing->versions) {
                            if (v.label == label) { dup = true; break; }
                        }
                    }
                    if (!dup) break;
                    cout << "Label already exists in this group. Enter a different label.\n";
                }
                string path = getPathWithHistory("Enter version file path", history, false);
                path = trim(path);
                if (!isTextFile(path)) { cout << "Error: must be .txt file.\n"; continue; }
                addVersionToGroup(groups, gName, label, path);
        } else if (c==3) {
            string gName; cout << "Group name: "; getline(cin,gName);
            VersionGroup* grp = findVersionGroup(groups, gName);
            if (!grp) { cout << "Group not found.\n"; continue; }
            if (grp->versions.empty()) { cout << "No versions in group.\n"; continue; }
            cout << "Versions in '"<<gName<<"':\n";
            for (size_t i=0;i<grp->versions.size();++i) {
                cout << (i+1) << ") " << grp->versions[i].label << " (" << grp->versions[i].sentences.size() << " sentences)\n";
            }
        } else if (c==4) {
            string gName; cout << "Group name: "; getline(cin,gName);
            VersionGroup* grp = findVersionGroup(groups, gName);
            if (!grp || grp->versions.size()<2) { cout << "Need at least 2 versions.\n"; continue; }
            for (size_t i=0;i<grp->versions.size();++i) {
                cout << (i+1) << ") " << grp->versions[i].label << '\n';
            }
            cout << "Select original index: "; int oi; cin>>oi; cout << "Select new index: "; int ni; cin>>ni; cin.ignore();
            if (oi<1||ni<1||oi>(int)grp->versions.size()||ni>(int)grp->versions.size()||oi==ni){ cout<<"Invalid indices.\n"; continue; }
            compareVersions(grp->versions[oi-1], grp->versions[ni-1]);
        } else if (c==5) {
            break;
        } else {
            cout << "Invalid choice.\n";
        }
    }
}

// trim whitespace
static string trim(const string& s){
    size_t a = s.find_first_not_of(" \t\r\n");
    if(a==string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a,b-a+1);
}

// case-insensitive check for .txt extension
static bool isTextFile(const string& path){
    if(path.size() < 4) return false;
    string ext = path.substr(path.size()-4);
    transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".txt";
}

void showMenu() {
    cout << "\n=== TEXT COMPARATOR ===\n";
    cout << "1. Compare Two Documents (Path vs Path)\n";
    cout << "2. Compare One Document Against Folder (1-to-many)\n";
    cout << "3. View File History\n";
    cout << "4. View File Statistics\n";
    cout << "5. View Analysis History\n";
    cout << "6. Compare with Analysis History\n";
    cout << "7. Manage Analysis History\n";
    cout << "8. Version Groups (Add/Compare)\n";
    cout << "9. Exit\n";
    cout << "Enter your choice: ";
}

string getFilePath(const string& prompt) {
    string filepath; 
    cout << prompt;
    getline(cin, filepath);
    return filepath;
}

string getPathWithHistory(const string& prompt, FileHistory& history, bool isFolder) {
    const vector<string>& list = isFolder ? history.getFolderHistory() : history.getHistory();
    if (!list.empty()) {
        cout << "Previously used " << (isFolder ? "folders" : "files") << ":\n";
        for (size_t i = 0; i < list.size(); ++i) {
            cout << "  " << (i + 1) << ") " << list[i] << "\n";
        }
    }
    cout << prompt << " (enter number to reuse or type a new path): ";
    string input;
    getline(cin, input);
    if (!input.empty() && all_of(input.begin(), input.end(), ::isdigit)) {
        int idx = stoi(input);
        if (idx >= 1 && idx <= static_cast<int>(list.size())) {
            return list[idx - 1];
        }
    }
    return input;
}

static string generateTimestampedFilename(const string& basePrefix) {
    using namespace std::chrono;
    auto now = system_clock::now();
    std::time_t t = system_clock::to_time_t(now);
    std::tm localTm;
#ifdef _WIN32
    localtime_s(&localTm, &t);
#else
    localTm = *std::localtime(&t);
#endif
    std::ostringstream oss;
    oss << basePrefix << "_"
        << std::put_time(&localTm, "%Y%m%d_%H%M%S")
        << ".txt";
    return oss.str();
}

void compareTwoDocuments(FileHistory& history, AnalysisHistory& analysisHistory) {
    string path1 = getPathWithHistory("Enter first document path", history, false);
    string path2 = getPathWithHistory("Enter second document path", history, false);
    path1 = trim(path1);
    path2 = trim(path2);

    if(!isTextFile(path1) || !isTextFile(path2)){
        cout << "Error: Both inputs must be .txt files.\n";
        return;
    }
    if(path1.empty() || path2.empty()){
        cout << "Error: Empty path provided.\n";
        return;
    }
    
    Document doc1, doc2;
    doc1.name = path1;
    doc2.name = path2;
    
    doc1.content = loadFile(path1);
    doc2.content = loadFile(path2);
    
    if (doc1.content.empty() || doc2.content.empty()) {
        cout << "Error: Could not load one or both files.\n";
        return;
    }
    
    analyzeDocument(doc1);
    analyzeDocument(doc2);
    
    double similarity = calculateSimilarity(doc1, doc2);
    printComparison(doc1, doc2);
    
    // Update file statistics using hashmap
    history.addFile(path1);
    history.addFile(path2);
    history.updateFileSimilarity(path1, similarity);
    history.updateFileSimilarity(path2, similarity);
    
    // Add to linked list analysis history
    addToAnalysisHistory(analysisHistory, doc1, similarity);
    addToAnalysisHistory(analysisHistory, doc2, similarity);
    
    // Automatically save comparison report
    string reportName = generateTimestampedFilename("comparison");
    saveReport(reportName, doc1, doc2);
    cout << "Results written to: " << reportName << "\n";
}

void compareOneToMany(FileHistory& history, AnalysisHistory& analysisHistory) {
    using std::filesystem::directory_iterator;
    using std::filesystem::is_directory;
    using std::filesystem::exists;

    string basePath = getPathWithHistory("Enter base document path", history, false);
    string folderPath = getPathWithHistory("Enter folder path to compare against", history, true);
    basePath = trim(basePath);
    folderPath = trim(folderPath);

    if(!isTextFile(basePath)){
        cout << "Error: Base document must be a .txt file.\n";
        return;
    }

    if (!exists(basePath)) {
        cout << "Error: Base document path does not exist.\n";
        return;
    }
    if (!exists(folderPath) || !is_directory(folderPath)) {
        cout << "Error: Folder path is invalid.\n";
        return;
    }

    Document baseDoc;
    baseDoc.name = basePath;
    baseDoc.content = loadFile(basePath);
    if (baseDoc.content.empty()) {
        cout << "Error: Could not load base document.\n";
        return;
    }
    analyzeDocument(baseDoc);

    vector<pair<string, double>> results;
    size_t comparedCount = 0;

    for (const auto& entry : directory_iterator(folderPath)) {
        try {
            if (!entry.is_regular_file()) continue;
            string candidatePath = entry.path().string();
            if (candidatePath == basePath) continue;
            if(!isTextFile(candidatePath)) continue; // skip non .txt

            Document cand;
            cand.name = candidatePath;
            cand.content = loadFile(candidatePath);
            if (cand.content.empty()) continue;
            analyzeDocument(cand);

            double sim = calculateSimilarity(baseDoc, cand);
            results.emplace_back(candidatePath, sim);

            history.addFile(candidatePath);
            history.updateFileSimilarity(candidatePath, sim);
            addToAnalysisHistory(analysisHistory, cand, sim);
            comparedCount++;
        } catch (...) {
            // Skip problematic entries
        }
    }

    if (results.empty()) {
        cout << "No comparable files found in folder.\n";
        return;
    }

    sort(results.begin(), results.end(), [](const auto& a, const auto& b){ return a.second > b.second; });

    cout << "\nCompared " << comparedCount << " file(s) in folder against base document.\n";
    cout << left << setw(6) << "#" << setw(8) << "Score" << "Path\n";
    cout << string(80, '-') << "\n";
    for (size_t i = 0; i < results.size(); ++i) {
        cout << left << setw(6) << (i+1)
             << setw(8) << fixed << setprecision(2) << results[i].second
             << results[i].first << "\n";
    }

    // Update history with base document and folder as well
    history.addFile(basePath);
    history.addFolder(folderPath);
    if (!results.empty()) history.updateFileSimilarity(basePath, results.front().second);
    addToAnalysisHistory(analysisHistory, baseDoc, results.front().second);

    // Automatically save batch comparison report
    {
        string reportName = generateTimestampedFilename("batch");
        ofstream out(reportName);
        if (!out.is_open()) {
            cout << "Error: Could not create report file.\n";
        } else {
            out << "BATCH COMPARISON REPORT\n";
            out << "Base Document: " << baseDoc.name << "\n";
            out << "Compared Files: " << results.size() << "\n\n";
            out << left << setw(6) << "#" << setw(8) << "Score" << "Path\n";
            out << string(80, '-') << "\n";
            for (size_t i = 0; i < results.size(); ++i) {
                out << left << setw(6) << (i+1)
                    << setw(8) << fixed << setprecision(2) << results[i].second
                    << results[i].first << "\n";
            }
            out.close();
            cout << "Results written to: " << reportName << "\n";
        }
    }
}

void manageAnalysisHistory(AnalysisHistory& analysisHistory) {
    cout << "\n=== Analysis History Management ===\n";
    cout << "1. Remove analysis entry\n";
    cout << "2. Clear all history\n";
    cout << "3. Find specific analysis\n";
    cout << "4. Back to main menu\n";
    cout << "Enter choice: ";
    
    int choice;
    cin >> choice;
    cin.ignore();
    
    switch (choice) {
        case 1: {
            // Simple direct input (no history selection needed here)
            string filename = getFilePath("Enter document name to remove: ");
            analysisHistory.removeAnalysis(filename);
            cout << "Analysis entry removed.\n";
            break;
        }
        case 2: {
            cout << "Are you sure you want to clear all history? (y/n): ";
            char confirm;
            cin >> confirm;
            if (confirm == 'y' || confirm == 'Y') {
                analysisHistory.clearHistory();
                cout << "All analysis history cleared.\n";
            }
            break;
        }
        case 3: {
            string filename = getFilePath("Enter document name to find: ");
            AnalysisNode* found = analysisHistory.findAnalysis(filename);
            if (found) {
                cout << "\nFound analysis:\n";
                cout << "Document: " << found->documentName << "\n";
                cout << "Words: " << found->wordCount << "\n";
                cout << "Sentences: " << found->sentenceCount << "\n";
                if (found->similarity > 0.0) {
                cout << "Similarity: " << fixed << setprecision(2) 
                    << found->similarity << "%\n";
                }
            } else {
                cout << "Analysis not found.\n";
            }
            break;
        }
        case 4:
            return;
        default:
            cout << "Invalid choice.\n";
    }
}

int main() {
    cout << "Welcome to TEXT COMPARATOR!\n";
    cout << "This tool compares text documents and analyzes their similarity.\n";
    cout << "Features: Arrays, Vectors, Linked Lists, Hashmaps, Sets, and more!\n";

    FileHistory history;
    AnalysisHistory analysisHistory; // Linked list for analysis history

    vector<VersionGroup> versionGroups; // in-memory version groups

    while (true) {
        showMenu();
        int choice;
        cin >> choice;
        cin.ignore(); 

        switch (choice) {
            case 1:
                compareTwoDocuments(history, analysisHistory);
                break;
            case 2:
                compareOneToMany(history, analysisHistory);
                break;
            case 3:
                cout << "\n=== File History ===\n";
                if (history.getHistory().empty()) {
                    cout << "No files in history.\n";
                } else {
                    for (size_t i = 0; i < history.getHistory().size(); ++i) {
                        cout << (i + 1) << ". " << history.getHistory()[i] << "\n";
                    }
                }
                break;
            case 4:
                history.printFileStats();
                break;
            case 5:
                analysisHistory.displayHistory();
                break;
            case 6: {
                string filename = getPathWithHistory("Enter document path to compare with history", history, false);
                Document doc;
                filename = trim(filename);
                if(!isTextFile(filename)){
                    cout << "Error: Must provide a .txt file.\n";
                    break;
                }
                doc.name = filename;
                doc.content = loadFile(filename);
                
                if (doc.content.empty()) {
                    cout << "Error: Could not load file.\n";
                } else {
                    analyzeDocument(doc);
                    compareWithHistory(doc, analysisHistory);
                }
                break;
            }
            case 7:
                manageAnalysisHistory(analysisHistory);
                break;
            case 8:
                manageVersionGroups(history, versionGroups);
                break;
            case 9:
                cout << "Goodbye!\n";
                return 0;
            default:
                cout << "Invalid choice. Please try again.\n";
        }

        cout << "\nPress Enter to continue...";
        cin.get();
    }
}