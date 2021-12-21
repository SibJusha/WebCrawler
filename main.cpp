#include <iostream>
#include <fstream>
#include <chrono>
#include <stack>
#include <set>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>

class wrong_template : std::exception {
    std::string what_str;
public:
    wrong_template() : what_str("Wrong template") {}

    const char* what() const noexcept override {
        return what_str.c_str();
    }
};

class WebCrawler {

    std::string _template_;
    std::set<std::string> visited_files;
    std::stack<std::string> not_visited_files;
    std::recursive_mutex mutex_stack;
    std::condition_variable_any empty_stack;
    std::vector<std::thread> threads;
    int count;
    unsigned int waiting_threads;

    std::string CopyFile (const std::string& from_directory) {
        std::ifstream source(from_directory);
        std::string copy = from_directory.substr(from_directory.find_last_of('/') + 1,
                                                 from_directory.size());
        std::ofstream dest(copy);
        dest << source.rdbuf();

        source.close();
        dest.close();
        return copy;
    }

    std::string GetFileName (std::istream& input) {
        char ch = 1;
        for (int i = 0; i < _template_.size() || ch == EOF; i++) {
            input >> ch;
            if (ch != _template_[i]) {
                throw wrong_template();
            }
        }
        std::string result;
        char x;
        input >> x;
        result += x;
        for (int i = 0; i < 255 && x != '"'; i++) {
            input >> x;
            result += x;
        }
        if (x != '"') {
            throw wrong_template();
        }
        return result;
    }


public:

    explicit WebCrawler(const std::string& protocol) :
        _template_("a href=\"" + protocol + "://"),
        count(0),
        waiting_threads(0)
    {}
    ~WebCrawler() = default;

    int Analysis (const std::string& start_file, bool first) {
        std::ifstream source;
        if (first) {
            std::ifstream temp(CopyFile(start_file));
            std::swap(source, temp);
        } else {
            std::unique_lock<std::recursive_mutex> lock(mutex_stack);
            if (not_visited_files.empty()) {
                waiting_threads++;
                if (waiting_threads == threads.size()) {
                    return 0;
                }
                empty_stack.wait(lock);
                waiting_threads--;
            }
            not_visited_files.top();
        }
        char x;
        source >> x;
        while (source.eof()) {
            while (x != '<' && source.eof()) {
                source >> x;
            }
            if (x == '<') {
                std::string found_file;
                try {
                    found_file = GetFileName(source);
                } catch (...) {
                    continue;
                }
                std::lock_guard<std::recursive_mutex> lock(mutex_stack);
                if (visited_files.find(found_file) == visited_files.end()) {
                    count++;
                    not_visited_files.push(found_file);
                    empty_stack.notify_one();
                }
            }
        }
        return 1;
    }

    void LoopedAnalysis (const std::string& start_file, bool first) {
        int n = 1;
        n = Analysis(start_file, first);
        while (n != 0) {
            n = Analysis(start_file, false);
        }
    }

    int GetFilesCount() const {
        return count;
    }
};


int main() {
    int threads_count;
    std::string start_file;
    std::cin >> start_file >> threads_count;
    auto start = std::chrono::steady_clock::now();
    WebCrawler fileReader("file");
    //fileReader.RunAnalysis(threads_count, start_file);
    fileReader.Analysis(start_file, true);
    for (int i = 0; i < threads_count; i++) {
        std::thread thread(&WebCrawler::LoopedAnalysis, start_file, false);
        threads.push_back(thread);
    }
    for (auto& th : threads) {
        th.join();
    }
    int files_count = fileReader.GetFilesCount();
    if (files_count == -1) {
        std::cout << "Error";
        return 0;
    }
    std::thread th(foo, 3);
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_time = end - start;
    std::cout << files_count << elapsed_time.count() << std::endl;
    return 0;
}
