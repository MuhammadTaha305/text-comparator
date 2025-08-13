#include "simple_analyzer.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <iomanip>
#include <array>
#include <cmath> 
#include <unordered_set>

using namespace std;

// Stop words
const char* STOP_WORD_LIST[] = {
    "the","and","a","to","of","in","is","it","that","for",
    "on","with","as","this","by"
};
const size_t STOP_WORD_COUNT = sizeof(STOP_WORD_LIST)/sizeof(STOP_WORD_LIST[0]);

// Stop-word check
inline bool isStopWord(const string& w) {
    for (size_t i = 0; i < STOP_WORD_COUNT; ++i) {
        if (w == STOP_WORD_LIST[i]) return true;
    }
    return false;
}

// Destructor
AnalysisHistory::~AnalysisHistory() {
    clearHistory();
}

// Add analysis (front insertion)
void AnalysisHistory::addAnalysis(const string& name, int words, int sentences, double sim) {
    AnalysisNode* newNode = new AnalysisNode(name, words, sentences, sim);
    newNode->next = head;
    head = newNode;
    size++;
}

// Remove an analysis entry
void AnalysisHistory::removeAnalysis(const string& name) {
    if (!head) return;
    
    // If head node matches
    if (head->documentName == name) {
        AnalysisNode* temp = head;
        head = head->next;
        delete temp;
        size--;
        return;
    }
    
    // Search for node to remove
    AnalysisNode* current = head;
    while (current->next && current->next->documentName != name) {
        current = current->next;
    }
    
    if (current->next) {
        AnalysisNode* temp = current->next;
        current->next = temp->next;
        delete temp;
        size--;
    }
}

// Print history
void AnalysisHistory::displayHistory() const {
    if (!head) {
        cout << "No analysis history available.\n";
        return;
    }
    
    // Collect entries first to compute dynamic width (base filename only)
    vector<tuple<string,int,int,double>> rows;
    rows.reserve(size);
    AnalysisNode* current = head;
    size_t maxName = 8;
    while (current) {
        string full = current->documentName;
        size_t pos = full.find_last_of("/\\");
        string base = (pos==string::npos)? full : full.substr(pos+1);
        if (base.size() > 32) base = base.substr(0,31) + "~";
        maxName = max(maxName, base.size());
        rows.emplace_back(base, current->wordCount, current->sentenceCount, current->similarity);
        current = current->next;
    }
    maxName = min<size_t>(maxName, 32);

    cout << "\n=== Analysis History (" << rows.size() << " entries) ===\n";
    cout << left << setw(4) << "#"
        << left << setw(static_cast<int>(maxName)+2) << "File"
        << right << setw(8) << "Words"
        << right << setw(11) << "Sentences"
        << right << setw(11) << "Similarity" << "\n";
    cout << string(4 + (maxName+2) + 8 + 11 + 11, '-') << "\n";

    int idx = 1;
    for (const auto& row : rows) {
        const string& name = get<0>(row);
        int wc = get<1>(row);
        int sc = get<2>(row);
        double sim = get<3>(row);
        cout << left << setw(4) << idx++
             << left << setw(static_cast<int>(maxName)+2) << name
             << right << setw(8) << wc
             << right << setw(11) << sc;
        if (sim > 0.0) {
            cout << right << setw(10) << fixed << setprecision(2) << sim << "%";
        } else {
            cout << right << setw(11) << "N/A";
        }
        cout << "\n";
    }
}

// Find analysis node
AnalysisNode* AnalysisHistory::findAnalysis(const string& name) const {
    AnalysisNode* current = head;
    while (current) {
        if (current->documentName == name) {
            return current;
        }
        current = current->next;
    }
    return nullptr;
}

// Clear list
void AnalysisHistory::clearHistory() {
    while (head) {
        AnalysisNode* temp = head;
        head = head->next;
        delete temp;
    }
    size = 0;
}

// Add document summary to history
void addToAnalysisHistory(AnalysisHistory& history, const Document& doc, double similarity) {
    history.addAnalysis(doc.name, doc.wordCount, doc.sentenceCount, similarity);
}

// Compare current doc with history
void compareWithHistory(const Document& doc, const AnalysisHistory& history) {
    cout << "\n=== Comparison with Analysis History ===\n";
    
    if (history.getSize() == 0) {
        cout << "No previous analyses to compare with.\n";
        return;
    }
    
    // Find similar documents in history
    vector<pair<string, double>> similarities;
    AnalysisNode* current = history.getHead();
    
    while (current) {
    // Similarity based on word & sentence counts
        double wordCountDiff = std::abs(static_cast<double>(doc.wordCount - current->wordCount)) / std::max(doc.wordCount, current->wordCount);
        double sentenceCountDiff = std::abs(static_cast<double>(doc.sentenceCount - current->sentenceCount)) / std::max(doc.sentenceCount, current->sentenceCount);
        
    // 1.0 = identical
        double similarity = 1.0 - (wordCountDiff + sentenceCountDiff) / 2.0;
        
        similarities.push_back(make_pair(current->documentName, similarity));
        current = current->next;
    }
    
    // Sort by similarity
    sort(similarities.begin(), similarities.end(),
         [](const auto& a, const auto& b) {
             return a.second > b.second;
         });
    
    cout << "Most similar documents in history:\n";
    for (size_t i = 0; i < min(similarities.size(), size_t(5)); ++i) {
        cout << "  " << (i + 1) << ". " << similarities[i].first 
             << " (similarity: " << fixed << setprecision(2) 
             << (similarities[i].second * 100) << "%)\n";
    }
}

string loadFile(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        return "";
    }
    
    stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

vector<string> splitIntoWords(const string& text) {
    vector<string> words;
    stringstream ss(text);
    string word;
    
    while (ss >> word) {
    // Strip non-alphanumeric
        string cleanWord;
        for (char c : word) {
            if (isalnum(c)) {
                cleanWord += tolower(c);
            }
        }
        if (!cleanWord.empty() && !isStopWord(cleanWord)) {
            words.push_back(cleanWord);
        }
    }
    
    return words;
}

vector<string> splitIntoSentences(const string& text) {
    vector<string> sentences;
    string currentSentence;
    
    for (char c : text) {
        currentSentence += c;
        if (c == '.' || c == '!' || c == '?') {
            // Clean sentence
            string cleanSentence;
            for (char sc : currentSentence) {
                if (isalnum(sc) || isspace(sc) || sc == '.' || sc == '!' || sc == '?') {
                    cleanSentence += tolower(sc);
                }
            }
            
            // Trim whitespace
            while (!cleanSentence.empty() && isspace(cleanSentence.front())) {
                cleanSentence.erase(cleanSentence.begin());
            }
            while (!cleanSentence.empty() && isspace(cleanSentence.back())) {
                cleanSentence.pop_back();
            }
            
            if (!cleanSentence.empty()) {
                sentences.push_back(cleanSentence);
            }
            currentSentence.clear();
        }
    }
    
    // Trailing text
    if (!currentSentence.empty()) {
        string cleanSentence;
        for (char sc : currentSentence) {
            if (isalnum(sc) || isspace(sc)) {
                cleanSentence += tolower(sc);
            }
        }
        
        while (!cleanSentence.empty() && isspace(cleanSentence.front())) {
            cleanSentence.erase(cleanSentence.begin());
        }
        while (!cleanSentence.empty() && isspace(cleanSentence.back())) {
            cleanSentence.pop_back();
        }
        
        if (!cleanSentence.empty()) {
            sentences.push_back(cleanSentence);
        }
    }
    
    return sentences;
}

// Longest sentence by word count
static pair<string,int> longestSentence(const Document& doc) {
    vector<string> sentences = splitIntoSentences(doc.content);
    string best;
    int bestCount = 0;
    for (const auto& s : sentences) {
        // Count words by reusing splitIntoWords
        int wc = static_cast<int>(splitIntoWords(s).size());
        if (wc > bestCount) {
            bestCount = wc;
            best = s;
        }
    }
    return {best, bestCount};
}

void analyzeDocument(Document& doc) {
    // Words & sentences
    doc.words = splitIntoWords(doc.content);
    vector<string> sentences = splitIntoSentences(doc.content);
    
    // Counts
    doc.wordCount = doc.words.size();
    doc.sentenceCount = sentences.size();
    
    // Average sentence length
    doc.avgSentenceLength = (doc.sentenceCount > 0) ? 
        static_cast<double>(doc.wordCount) / doc.sentenceCount : 0.0;
    
    // Build unique set & frequency map
    doc.uniqueWords.clear();
    doc.wordFrequency.clear();
    
    for (const string& word : doc.words) {
        doc.uniqueWords.insert(word);
    // Frequency
        doc.wordFrequency[word]++;
    }
}

double calculateSimilarity(const Document& doc1, const Document& doc2) {
    set<string> intersection;
    set<string> union_set;

    // Intersection
    set_intersection(
        doc1.uniqueWords.begin(), doc1.uniqueWords.end(),
        doc2.uniqueWords.begin(), doc2.uniqueWords.end(),
        inserter(intersection, intersection.begin())
    );

    // Union
    set_union(
        doc1.uniqueWords.begin(), doc1.uniqueWords.end(),
        doc2.uniqueWords.begin(), doc2.uniqueWords.end(),
        inserter(union_set, union_set.begin())
    );

    if (union_set.empty()) return 0.0;
    return (static_cast<double>(intersection.size()) / union_set.size()) * 100.0;
}


set<string> findCommonWords(const Document& doc1, const Document& doc2) {
    set<string> common;
    set_intersection(
        doc1.uniqueWords.begin(), doc1.uniqueWords.end(),
        doc2.uniqueWords.begin(), doc2.uniqueWords.end(),
        inserter(common, common.begin())
    );
    return common;
}

set<string> findUniqueWords(const Document& doc1, const Document& doc2) {
    set<string> unique;
    set_difference(
        doc1.uniqueWords.begin(), doc1.uniqueWords.end(),
        doc2.uniqueWords.begin(), doc2.uniqueWords.end(),
        inserter(unique, unique.begin())
    );
    return unique;
}

// Word overlap with combined frequency
unordered_map<string, int> findWordOverlap(const Document& doc1, const Document& doc2) {
    unordered_map<string, int> overlap;
    
    for (const auto& wordPair : doc1.wordFrequency) {
        const string& word = wordPair.first;
        int freq1 = wordPair.second;
        
    // Check if word exists in doc2
        auto it = doc2.wordFrequency.find(word);
        if (it != doc2.wordFrequency.end()) {
            // Word exists in both docs
            overlap[word] = freq1 + it->second;
        }
    }
    
    return overlap;
}

// Top words by frequency
vector<pair<string, int>> getTopWordsByFrequency(const Document& doc, int topN) {
    vector<pair<string, int>> sortedWords;
    
    // Copy map to vector
    for (const auto& wordPair : doc.wordFrequency) {
        sortedWords.push_back(wordPair);
    }
    
    // Sort by frequency
    sort(sortedWords.begin(), sortedWords.end(), 
         [](const pair<string, int>& a, const pair<string, int>& b) {
             return a.second > b.second;
         });
    
    // Truncate
    if (sortedWords.size() > static_cast<size_t>(topN)) {
        sortedWords.resize(topN);
    }
    
    return sortedWords;
}

// Basic word pattern stats
void analyzeWordPatterns(const Document& doc) {
    cout << "\n=== Word Pattern Analysis ===\n";
    
    // Group by frequency
    unordered_map<int, vector<string>> frequencyGroups;
    
    for (const auto& wordPair : doc.wordFrequency) {
        int freq = wordPair.second;
        frequencyGroups[freq].push_back(wordPair.first);
    }
    
    // Most common frequency
    int maxFreq = 0;
    for (const auto& freqGroup : frequencyGroups) {
        maxFreq = max(maxFreq, freqGroup.first);
    }
    
    cout << "Most frequent words (appearing " << maxFreq << " times):\n";
    for (const string& word : frequencyGroups[maxFreq]) {
        cout << "  " << word << "\n";
    }
    
    // Words appearing once
    if (frequencyGroups.find(1) != frequencyGroups.end()) {
        cout << "\nWords appearing only once (" << frequencyGroups[1].size() << " words):\n";
        int count = 0;
        for (const string& word : frequencyGroups[1]) {
            if (count++ >= 10) break; // Limit output
            cout << "  " << word << "\n";
        }
        if (frequencyGroups[1].size() > 10) {
            cout << "  ... and " << (frequencyGroups[1].size() - 10) << " more\n";
        }
    }
    
    // Vocabulary richness
    double vocabularyRichness = static_cast<double>(doc.uniqueWords.size()) / doc.wordCount;
    cout << "\nVocabulary richness: " << fixed << setprecision(3) 
         << vocabularyRichness << " (higher = more diverse vocabulary)\n";
}

void printDocumentStats(const Document& doc) {
    cout << "\n=== Document Analysis: " << doc.name << " ===\n";
    cout << "Word count: " << doc.wordCount << "\n";
    cout << "Sentence count: " << doc.sentenceCount << "\n";
    cout << "Unique words: " << doc.uniqueWords.size() << "\n";
    cout << "Average sentence length: " << fixed << setprecision(2) 
         << doc.avgSentenceLength << " words\n";
    
    // Top words
    vector<pair<string, int>> topWords = getTopWordsByFrequency(doc, 10);
    cout << "\nTop 10 most frequent words:\n";
    for (const auto& word : topWords) {
        cout << "  " << word.first << ": " << word.second << " times\n";
    }
    
    // Word pattern analysis
    analyzeWordPatterns(doc);
    
    // Category analysis
    analyzeDocumentCategories(doc);
}

void printComparison(const Document& doc1, const Document& doc2) {
    cout << "\nTEXT COMPARATOR - RESULTS SUMMARY\n";
    cout << "==================================\n\n";
    
    // IDs
    cout << "Document A: " << doc1.name << "\n";
    cout << "Document B: " << doc2.name << "\n\n";
    
    // Basic stats
    cout << "Basic Statistics:\n";
    cout << setw(25) << left << "Metric" 
         << setw(15) << left << "Document A" 
         << setw(15) << left << "Document B" << "\n";
    cout << string(55, '-') << "\n";
    
    cout << setw(25) << left << "Word Count" 
         << setw(15) << left << doc1.wordCount 
         << setw(15) << left << doc2.wordCount << "\n";
    
    cout << setw(25) << left << "Sentence Count" 
         << setw(15) << left << doc1.sentenceCount 
         << setw(15) << left << doc2.sentenceCount << "\n";
    
    cout << setw(25) << left << "Unique Words" 
         << setw(15) << left << doc1.uniqueWords.size() 
         << setw(15) << left << doc2.uniqueWords.size() << "\n";
    
    cout << setw(25) << left << "Avg Sentence Length" 
         << setw(15) << left << fixed << setprecision(1) << doc1.avgSentenceLength 
         << setw(15) << left << fixed << setprecision(1) << doc2.avgSentenceLength << "\n\n";

    // Longest sentence
    auto ls1 = longestSentence(doc1);
    auto ls2 = longestSentence(doc2);
    cout << "Longest Sentence Word Count:\n";
    cout << setw(25) << left << "Metric" 
         << setw(15) << left << "Document A" 
         << setw(15) << left << "Document B" << "\n";
    cout << string(55,'-') << "\n";
    cout << setw(25) << left << "Longest Sentence Words" 
         << setw(15) << left << ls1.second
         << setw(15) << left << ls2.second << "\n\n";
    if (!ls1.first.empty()) {
        cout << "Longest Sentence (A): " << ls1.first << "\n";
    } else {
        cout << "Longest Sentence (A): N/A\n";
    }
    if (!ls2.first.empty()) {
        cout << "Longest Sentence (B): " << ls2.first << "\n";
    } else {
        cout << "Longest Sentence (B): N/A\n";
    }
    cout << "\n";
    
    // Top 5 words
    cout << "Top 5 Frequent Words:\n";
    cout << setw(25) << left << "Document A" 
         << setw(25) << left << "Document B" << "\n";
    cout << string(50, '-') << "\n";
    
    vector<pair<string, int>> topWords1 = getTopWordsByFrequency(doc1, 5);
    vector<pair<string, int>> topWords2 = getTopWordsByFrequency(doc2, 5);
    
                    for (size_t i = 0; i < 5; ++i) {
                    string word1 = (i < topWords1.size()) ? topWords1[i].first : "";
                    string word2 = (i < topWords2.size()) ? topWords2[i].first : "";
                    
                    cout << setw(25) << left << word1 
                         << setw(25) << left << word2 << "\n";
                }
    cout << "\n";
    
                    // Common words
                set<string> common = findCommonWords(doc1, doc2);
                cout << "Common Words (" << common.size() << "): ";
                size_t idx = 0;
                for (const string& word : common) {
                    cout << word;
                    if (++idx < common.size()) cout << ", ";
                }
    cout << "\n\n";

    // Exclusive words
    set<string> onlyA = findUniqueWords(doc1, doc2);
    set<string> onlyB = findUniqueWords(doc2, doc1);
    cout << "Words Exclusive to Document A (" << onlyA.size() << "): ";
    size_t eidx = 0;
    for (const string& w : onlyA) { cout << w; if (++eidx < onlyA.size()) cout << ", "; }
    cout << "\n";
    cout << "Words Exclusive to Document B (" << onlyB.size() << "): ";
    eidx = 0;
    for (const string& w : onlyB) { cout << w; if (++eidx < onlyB.size()) cout << ", "; }
    cout << "\n\n";
    
    // Similarity metrics
    double jaccardSimilarity = calculateSimilarity(doc1, doc2);
    
    cout << "Similarity Metrics:\n";
    cout << "Jaccard Similarity: " << fixed << setprecision(2) << jaccardSimilarity << "%\n\n";
}

void saveReport(const string& filename, const Document& doc1, const Document& doc2) {
    ofstream file(filename);
    if (!file.is_open()) {
        cout << "Error: Could not create report file.\n";
        return;
    }
    
    file << "TEXT COMPARATOR - RESULTS SUMMARY\n";
    file << "==================================\n\n";
    
    // IDs
    file << "Document A: " << doc1.name << "\n";
    file << "Document B: " << doc2.name << "\n\n";
    
    // Basic stats
    file << "Basic Statistics:\n";
    file << setw(25) << left << "Metric" 
         << setw(15) << left << "Document A" 
         << setw(15) << left << "Document B" << "\n";
    file << string(55, '-') << "\n";
    
    file << setw(25) << left << "Word Count" 
         << setw(15) << left << doc1.wordCount 
         << setw(15) << left << doc2.wordCount << "\n";
    
    file << setw(25) << left << "Sentence Count" 
         << setw(15) << left << doc1.sentenceCount 
         << setw(15) << left << doc2.sentenceCount << "\n";
    
    file << setw(25) << left << "Unique Words" 
         << setw(15) << left << doc1.uniqueWords.size() 
         << setw(15) << left << doc2.uniqueWords.size() << "\n";
    
    file << setw(25) << left << "Avg Sentence Length" 
         << setw(15) << left << fixed << setprecision(1) << doc1.avgSentenceLength 
         << setw(15) << left << fixed << setprecision(1) << doc2.avgSentenceLength << "\n\n";

    // Longest sentence
    auto ls1 = longestSentence(doc1);
    auto ls2 = longestSentence(doc2);
    file << "Longest Sentence Word Count:\n";
    file << setw(25) << left << "Metric" 
        << setw(15) << left << "Document A" 
        << setw(15) << left << "Document B" << "\n";
    file << string(55,'-') << "\n";
    file << setw(25) << left << "Longest Sentence Words" 
        << setw(15) << left << ls1.second
        << setw(15) << left << ls2.second << "\n\n";
    file << "Longest Sentence (A): " << (ls1.first.empty()?"N/A":ls1.first) << "\n";
    file << "Longest Sentence (B): " << (ls2.first.empty()?"N/A":ls2.first) << "\n\n";
    
    // Top 5 words
    file << "Top 5 Frequent Words:\n";
    file << setw(25) << left << "Document A" 
         << setw(25) << left << "Document B" << "\n";
    file << string(50, '-') << "\n";
    
    vector<pair<string, int>> topWords1 = getTopWordsByFrequency(doc1, 5);
    vector<pair<string, int>> topWords2 = getTopWordsByFrequency(doc2, 5);
    
                    for (size_t i = 0; i < 5; ++i) {
                    string word1 = (i < topWords1.size()) ? topWords1[i].first : "";
                    string word2 = (i < topWords2.size()) ? topWords2[i].first : "";
                    
                    file << setw(25) << left << word1 
                         << setw(25) << left << word2 << "\n";
                }
    file << "\n";
    
                    // Common words
                set<string> common = findCommonWords(doc1, doc2);
                file << "Common Words (" << common.size() << "): ";
                size_t idx2 = 0;
                for (const string& word : common) {
                    file << word;
                    if (++idx2 < common.size()) file << ", ";
                }
    file << "\n\n";

    // Exclusive words
    set<string> onlyA = findUniqueWords(doc1, doc2);
    set<string> onlyB = findUniqueWords(doc2, doc1);
    file << "Words Exclusive to Document A (" << onlyA.size() << "): ";
    size_t ridx = 0;
    for (const string& w : onlyA) { file << w; if (++ridx < onlyA.size()) file << ", "; }
    file << "\n";
    file << "Words Exclusive to Document B (" << onlyB.size() << "): ";
    ridx = 0;
    for (const string& w : onlyB) { file << w; if (++ridx < onlyB.size()) file << ", "; }
    file << "\n\n";
    
    // Similarity metrics
    double jaccardSimilarity = calculateSimilarity(doc1, doc2);
    
    file << "Similarity Metrics:\n";
    file << "Jaccard Similarity: " << fixed << setprecision(2) << jaccardSimilarity << "%\n\n";
    
    file.close();
}

// Document metrics as array
array<int, 5> getDocumentMetrics(const Document& doc) {
    array<int, 5> metrics;
    metrics[0] = doc.wordCount;           // Words
    metrics[1] = doc.sentenceCount;       // Sentences
    metrics[2] = doc.uniqueWords.size();  // Unique
    metrics[3] = doc.wordFrequency.size(); // Types
    metrics[4] = static_cast<int>(doc.avgSentenceLength); // Avg sentence length
    
    return metrics;
}

// Category analysis (array-based)
void analyzeDocumentCategories(const Document& doc) {
    cout << "\n=== Document Category Analysis (Array-based) ===\n";
    
    // Category thresholds
    array<int, 4> thresholds = {100, 500, 1000, 2000}; // Word count thresholds
    array<string, 4> categories = {"Short", "Medium", "Long", "Very Long"};
    
    // Collect metrics
    array<int, 5> metrics = getDocumentMetrics(doc);
    
    cout << "Document Metrics:\n";
    cout << "  Total words: " << metrics[0] << "\n";
    cout << "  Total sentences: " << metrics[1] << "\n";
    cout << "  Unique words: " << metrics[2] << "\n";
    cout << "  Word types: " << metrics[3] << "\n";
    cout << "  Avg sentence length: " << metrics[4] << " words\n";
    
    // Derive category
    int categoryIndex = 0;
    for (int i = 0; i < 4; ++i) {
        if (metrics[0] <= thresholds[i]) {
            categoryIndex = i;
            break;
        }
        categoryIndex = 3; // Very Long
    }
    
    cout << "\nDocument Category: " << categories[categoryIndex] << "\n";
    
    // Readability scores
    array<double, 3> readabilityScores;
    readabilityScores[0] = static_cast<double>(metrics[2]) / metrics[0]; // Vocabulary diversity
    readabilityScores[1] = static_cast<double>(metrics[0]) / metrics[1]; // Words per sentence
    readabilityScores[2] = static_cast<double>(metrics[3]) / metrics[2]; // Word type variety
    
    cout << "\nReadability Analysis:\n";
    cout << "  Vocabulary diversity: " << fixed << setprecision(3) << readabilityScores[0] << "\n";
    cout << "  Words per sentence: " << fixed << setprecision(1) << readabilityScores[1] << "\n";
    cout << "  Word type variety: " << fixed << setprecision(3) << readabilityScores[2] << "\n";
    
    // Complexity level
    array<string, 3> complexityLevels = {"Simple", "Moderate", "Complex"};
    int complexityIndex = 0;
    
    if (readabilityScores[0] > 0.7 && readabilityScores[1] < 15) {
        complexityIndex = 0; // Simple
    } else if (readabilityScores[0] > 0.5 && readabilityScores[1] < 25) {
        complexityIndex = 1; // Moderate
    } else {
        complexityIndex = 2; // Complex
    }
    
    cout << "  Complexity level: " << complexityLevels[complexityIndex] << "\n";
}
