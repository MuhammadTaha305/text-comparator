#ifndef SIMPLE_ANALYZER_H
#define SIMPLE_ANALYZER_H

#include <string>
#include <vector>
#include <set>
#include <map>
#include <unordered_map>
#include <array>

using namespace std;

// Node in analysis history list
struct AnalysisNode {
    string documentName;
    int wordCount;
    int sentenceCount;
    double similarity;
    AnalysisNode* next;
    
    AnalysisNode(const string& name, int words, int sentences, double sim = 0.0)
        : documentName(name), wordCount(words), sentenceCount(sentences), 
          similarity(sim), next(nullptr) {}
};

// Analysis history list
class AnalysisHistory {
private:
    AnalysisNode* head;
    int size;
    
public:
    AnalysisHistory() : head(nullptr), size(0) {}
    ~AnalysisHistory();
    
    void addAnalysis(const string& name, int words, int sentences, double sim = 0.0);
    void removeAnalysis(const string& name);
    void displayHistory() const;
    int getSize() const { return size; }
    AnalysisNode* findAnalysis(const string& name) const;
    void clearHistory();
    AnalysisNode* getHead() const { return head; }
};

// Document stats
struct Document {
    string name;
    string content;
    vector<string> words;
    set<string> uniqueWords;
    unordered_map<string, int> wordFrequency;  // Word -> freq
    int wordCount;
    int sentenceCount;
    double avgSentenceLength;
};

// Core
string loadFile(const string& filename);
vector<string> splitIntoWords(const string& text);
vector<string> splitIntoSentences(const string& text);
void analyzeDocument(Document& doc);
double calculateSimilarity(const Document& doc1, const Document& doc2);
set<string> findCommonWords(const Document& doc1, const Document& doc2);
set<string> findUniqueWords(const Document& doc1, const Document& doc2);

// Hashmap helpers
unordered_map<string, int> findWordOverlap(const Document& doc1, const Document& doc2);
vector<pair<string, int>> getTopWordsByFrequency(const Document& doc, int topN = 10);
void analyzeWordPatterns(const Document& doc);

// History helpers
void addToAnalysisHistory(AnalysisHistory& history, const Document& doc, double similarity = 0.0);
void compareWithHistory(const Document& doc, const AnalysisHistory& history);

// Array helpers
array<int, 5> getDocumentMetrics(const Document& doc);
void analyzeDocumentCategories(const Document& doc);

// Output
void printDocumentStats(const Document& doc);
void printComparison(const Document& doc1, const Document& doc2);
void saveReport(const string& filename, const Document& doc1, const Document& doc2);

#endif
