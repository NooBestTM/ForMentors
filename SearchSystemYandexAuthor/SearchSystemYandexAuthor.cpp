#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <numeric>
#include <stdexcept>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
//погрешность измерения
const double EPSILON = 1e-6;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    int id = 0;
    double relevance = 0;
    int rating = 0;

    Document() = default;

    Document(int doc_id, double doc_relevance, int doc_rating)
        : id(doc_id), relevance(doc_relevance), rating(doc_rating) {}

};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};


class SearchServer {
public:
    SearchServer() = default;

    explicit SearchServer(const string& stop_words)
        : stop_words_(SplitStringIntoSet(stop_words))
    {
        CheckStopWordsOnValid();
    }

    template<typename Сontainer>
    explicit SearchServer(const Сontainer& stop_words) {
        for (const string& word : stop_words) {
            if (!word.empty()) {
                stop_words_.insert(word);
            }
        }
        CheckStopWordsOnValid();
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status,
        const vector<int>& ratings) {
        if (document_id < 0) {
            string exception_msg = "Document \"" + document
                + "\" was not added. Negative ID: "s + to_string(document_id) + "."s;
            throw invalid_argument(exception_msg);
        }
        if (documents_.count(document_id) != 0) {
            string exception_msg = "Document \""s + document
                + "\" was not added. ID "s + to_string(document_id) + " already exists."s;
            throw invalid_argument(exception_msg);
        }

        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
        added_ids_sequence_.push_back(document_id);
    }

    template<typename KeyMapper>
    vector<Document> FindTopDocuments(const string& raw_query, KeyMapper filter) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, filter);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
            if (abs(lhs.relevance - rhs.relevance) < EPSILON) {
                return lhs.rating > rhs.rating;
            }
            return lhs.relevance > rhs.relevance;
        });

        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status_only) const {
        return FindTopDocuments(raw_query,
            [status_only](int document_id, DocumentStatus status, int rating) {
            return status == status_only;
        });
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    int GetDocumentCount() const {
        return static_cast<int>(documents_.size());
    }

    int GetDocumentId(int index) const {
        if (index < 0 || index > GetDocumentCount() - 1) {
            throw out_of_range("There are no such number of documents.");
        }
        return added_ids_sequence_[index];
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query,
        int document_id) const {
        //Добавил исключение, если данного документа не существует
        //В задании о нём не говорится
        if (documents_.count(document_id) == 0) {
            string except_msg = "There is no document with ID: " + to_string(document_id);
            throw invalid_argument(except_msg);
        }

        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;

        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return { matched_words, documents_.at(document_id).status };
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_ = {};
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;
    vector<int> added_ids_sequence_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }



    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsValidWord(word)) {
                string except_msg = "Word \""s + word + "\" contains special symbol."s;
                throw invalid_argument(except_msg);
            }

            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }

        const int rating_sum = accumulate(ratings.begin(), ratings.end(), 0);

        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    static bool IsValidWord(const string& word) {
        //A valid word must not contain special characters
        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
        });
    }

    void CheckStopWordsOnValid() const {
        for (const string& word : stop_words_) {
            if (!IsValidWord(word)) {
                throw invalid_argument("Some of stop words contain special symbols");
            }
        }
    }

    static set<string> SplitStringIntoSet(const string& str) {
        vector<string> splited_words = SplitIntoWords(str);
        set<string> s(splited_words.begin(), splited_words.end());

        return s;
    }

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        // Word shouldn't be empty
        if (text[0] == '-') {
            text = text.substr(1);
            if (text.empty()) {
                string except_msg = "There is no word after \'-\' symbol in query."s;
                throw invalid_argument(except_msg);
            }
            if (text[0] == '-') {
                string except_msg = "There are many \'-\' symbol in a row in query."s;
                throw invalid_argument(except_msg);
            }
            if (!IsValidWord(text)) {
                string except_msg = "In query word \""s + text + "\" contains special symbol";
                throw invalid_argument(except_msg);
            }
            is_minus = true;
        }
        return { text, is_minus, IsStopWord(text) };
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                }
                else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    // Existence required
    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template<typename KeyMapper>
    vector<Document> FindAllDocuments(const Query& query, KeyMapper filter) const {
        map<int, double> document_to_relevance;

        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }

            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (filter(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back(
                { document_id, relevance, documents_.at(document_id).rating });
        }
        return matched_documents;
    }
};

template<typename Iterator>
class IteratorRange {
public:
    IteratorRange(Iterator begin, Iterator end) :
        begin_(begin), end_(end),
        page_documents_(vector<Document>(begin, end))
    {}

    Iterator begin() const {
        return begin_;
    }

    Iterator end() const {
        return end_;
    }

    size_t size() const {
        return distance(begin_, end_);
    }

private:
    vector<Document> page_documents_;
    Iterator begin_, end_;
};

template <typename Iterator>
ostream& operator<<(ostream& os, const IteratorRange<Iterator>& content) {
    for (Iterator it = content.begin(); it != content.end(); ++it) {
        os << *it;
    }
    return os;
}

template<typename Iterator>
class Paginator {
public:
    Paginator(const Iterator begin,
        const Iterator end,
        const size_t page_size) :
        begin_(begin), end_(end)
    {
        for (Iterator step = begin; step < end; ) {
            if (static_cast<size_t>(distance(step, end)) < page_size) {
                pages_.push_back({ step, end });
                break;
            }
            Iterator temp = step;
            advance(step, page_size);
            pages_.push_back({ temp, step });
        }
    }

    Iterator begin() const {
        return (*pages_.begin()).begin();
    }
    Iterator end() const {
        return (*pages_.rbegin()).end();
    }
    size_t size() const {
        return pages_.size();
    }

private:
    vector<IteratorRange<Iterator>> pages_;
    Iterator begin_, end_;
};

template<typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}

template <typename ElementType>
ostream& operator<<(ostream& out, vector<ElementType> elements) {
    bool is_first = true;
    out << '[';
    for (const auto& element : elements) {
        if (!is_first) {
            out << ", "s;
        }
        out << element;
        is_first = false;
    }
    out << ']';
    return out;
}
ostream& operator<<(ostream& out, const Document& document) {
    out << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating << " }"s;
    return out;
}

ostream& operator<<(ostream& output, const DocumentStatus status) {
    switch (status)
    {
    case DocumentStatus::ACTUAL:
        output << "ACTUAL"s;
        break;
    case DocumentStatus::IRRELEVANT:
        output << "IRRELEVANT"s;
        break;
    case DocumentStatus::BANNED:
        output << "BANNED"s;
        break;
    case DocumentStatus::REMOVED:
        output << "REMOVED"s;
    }
    return output;
}


int main() {
    try {
        SearchServer search_server("and with"s);
        search_server.AddDocument(1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
        search_server.AddDocument(2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
        search_server.AddDocument(3, "big cat nasty hair"s, DocumentStatus::ACTUAL, { 1, 2, 8 });
        search_server.AddDocument(4, "big dog cat Vladislav"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
        search_server.AddDocument(5, "big dog hamster Borya"s, DocumentStatus::ACTUAL, { 1, 1, 1 });
        const auto search_results = search_server.FindTopDocuments("curly dog"s);
        int page_size = 2;
        const auto pages = Paginate(search_results, page_size);
        for (auto page = pages.begin(); page != pages.end(); ++page) {
            cout << *page << endl;
            cout << "Page break"s << endl;
        }
    }
    catch (const invalid_argument& e) {
        cout << e.what() << endl;
    }
    catch (const out_of_range& e) {
        cout << e.what() << endl;
    }
    return 0;
}}