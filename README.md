# Simple Text Document Analyzer

A simplified C++ text analysis tool that analyzes text documents and compares their similarity using the Jaccard coefficient.

## Features

- **Single Document Analysis**: Analyze word count, sentence count, unique words, and word frequency
- **Two Document Comparison**: Compare two documents (path vs path) and calculate similarity
- **1-to-Many Comparison**: Compare one document against multiple files in a folder (path vs folder)
- **File History**: Remember previously used files for easy access
- **Report Generation**: Save detailed analysis reports to text files

## Requirements

- C++17 compatible compiler (GCC, Clang, or MSVC)
- Standard C++ libraries only (no external dependencies)

## Building the Project

### Using Makefile (Linux/Mac/Windows with MinGW)
```bash
make all
```

### Manual Compilation
```bash
g++ -std=c++17 -Wall -Wextra -O2 -o text_analyzer main.cpp simple_analyzer.cpp
```

## Running the Program

```bash
./text_analyzer
```

## Usage

The program provides a simple menu-driven interface:

1. Compare Two Documents (Path vs Path)
2. Compare One Document Against Folder (1-to-many)
3. View File History
4. View File Statistics
5. View Analysis History (Linked List)
6. Compare with Analysis History
7. Manage Analysis History
8. Exit

## How It Works

### Text Analysis
- **Word Tokenization**: Splits text into individual words, removing punctuation
- **Sentence Detection**: Identifies sentences using period, exclamation mark, and question mark
- **Statistics Calculation**: Counts words, sentences, unique words, and calculates averages
- **Frequency Analysis**: Tracks how often each word appears

### Similarity Calculation
Uses the **Jaccard Similarity Coefficient** only:
```
Similarity = |A ∩ B| / |A ∪ B|
```
Where:
- A ∩ B = Common words between documents
- A ∪ B = All unique words from both documents

### File Processing
- Supports any text file format (.txt, .md, etc.)
- Accepts full paths for files and folders
- Maintains a history of recently used files
- Generates detailed reports in text format (including batch 1-to-many report)

## Output Files

The program can generate several types of reports:
- `comparison_report.txt`: Detailed comparison between two documents
- `batch_report.txt`: Summary of 1-to-many comparison results
- `file_history.txt`: List of recently used files

## Example Output

```
=== Document: sample.txt ===
Word Count: 150
Sentence Count: 12
Unique Words: 89
Average Sentence Length: 12.50 words
Top 5 Most Frequent Words:
  1. the (15 times)
  2. and (12 times)
  3. is (8 times)
  4. in (7 times)
  5. to (6 times)

=== Comparison Results ===
Similarity (Jaccard): 45.67%
Common Words: 23
Words only in doc1.txt: 45
Words only in doc2.txt: 38
```

## Technical Details

### Data Structures Used
- **std::vector**: Store words, sentences, and comparison results
- **std::set**: Store unique words and perform set operations
- **std::unordered_map**: Track word frequencies efficiently
- **std::string**: Handle text content and file paths

### Algorithms
- **Set Operations**: Intersection and union for similarity calculation
- **Sorting**: Rank words by frequency and results by similarity
- **File I/O**: Read text files and write reports

## Limitations

- Basic text processing (no advanced NLP features)
- Simple sentence detection (periods, exclamation marks, question marks)
- ASCII text support (basic Unicode handling)
- Memory usage scales with document size

## Future Enhancements

- Support for more document formats (PDF, DOCX)
- Advanced text preprocessing (stemming, stop words)
- Multiple similarity metrics (Cosine, Euclidean)
- GUI interface
- Database storage for large document collections

## Troubleshooting

### Common Issues
1. **Compilation Error**: Ensure you have a C++17 compatible compiler
2. **File Not Found**: Check file paths and permissions
3. **Memory Issues**: Very large files may cause memory problems

### Getting Help
- Check that all source files are present
- Verify compiler supports C++17 standard
- Ensure text files are readable and not corrupted

## License

This is a simple educational project. Feel free to modify and use as needed.

## Authors

Built collaboratively by:
- **[Muhammad Taha](https://github.com/MuhammadTaha305)**
- **Maier Ali**
- **Aytesam Sohail**
