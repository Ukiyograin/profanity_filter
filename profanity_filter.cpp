#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <regex>
#include <algorithm>
#include <unordered_map>
#include <memory>
#include <fstream>
#include <sstream>

/**
 * @brief 脏话屏蔽基类，定义统一接口
 */
class ProfanityFilter {
public:
    virtual ~ProfanityFilter() = default;
    
    /**
     * @brief 检测文本中是否包含脏话
     * @param text 待检测文本
     * @return true 如果包含脏话
     */
    virtual bool containsProfanity(const std::string& text) const = 0;
    
    /**
     * @brief 屏蔽文本中的脏话
     * @param text 待处理文本
     * @return 处理后的文本
     */
    virtual std::string censor(const std::string& text) const = 0;
    
    /**
     * @brief 添加脏话到过滤列表
     * @param word 脏话
     */
    virtual void addProfanity(const std::string& word) = 0;
    
    /**
     * @brief 从文件加载脏话列表
     * @param filename 文件名
     */
    virtual void loadFromFile(const std::string& filename) = 0;
};

/**
 * @brief 简单替换法 - 使用字符串查找和替换
 * 
 * 优点：简单直接，适用于小规模脏话列表
 * 缺点：效率较低，无法处理变体
 */
class SimpleReplacementFilter : public ProfanityFilter {
private:
    std::set<std::string> profanityList;
    char replacementChar;
    
public:
    explicit SimpleReplacementFilter(char replacementChar = '*') 
        : replacementChar(replacementChar) {
        // 默认脏话列表
        profanityList = {"shit", "fuck", "damn", "ass", "bitch", "bastard"};
    }
    
    bool containsProfanity(const std::string& text) const override {
        std::string lowerText = toLower(text);
        
        for (const auto& word : profanityList) {
            if (lowerText.find(word) != std::string::npos) {
                return true;
            }
        }
        return false;
    }
    
    std::string censor(const std::string& text) const override {
        std::string result = text;
        std::string lowerText = toLower(text);
        
        for (const auto& word : profanityList) {
            size_t pos = 0;
            while ((pos = lowerText.find(word, pos)) != std::string::npos) {
                // 替换脏话为指定字符
                for (size_t i = 0; i < word.length(); ++i) {
                    result[pos + i] = replacementChar;
                }
                pos += word.length();
            }
        }
        
        return result;
    }
    
    void addProfanity(const std::string& word) override {
        profanityList.insert(toLower(word));
    }
    
    void loadFromFile(const std::string& filename) override {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "无法打开文件: " << filename << std::endl;
            return;
        }
        
        std::string word;
        while (std::getline(file, word)) {
            if (!word.empty()) {
                addProfanity(word);
            }
        }
        file.close();
    }
    
private:
    static std::string toLower(const std::string& str) {
        std::string lower = str;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        return lower;
    }
};

/**
 * @brief 正则表达式法 - 使用正则表达式匹配脏话
 * 
 * 优点：可以处理复杂的模式匹配，支持变体
 * 缺点：正则表达式可能较难维护，性能中等
 */
class RegexFilter : public ProfanityFilter {
private:
    std::vector<std::regex> profanityPatterns;
    char replacementChar;
    
public:
    explicit RegexFilter(char replacementChar = '*') 
        : replacementChar(replacementChar) {
        // 默认正则表达式模式（支持一些变体）
        addPattern("shit");
        addPattern("fuck");
        addPattern("damn");
        addPattern("ass");
        addPattern("bitch");
        addPattern("bastard");
        // 支持常见变体，如 f*ck, f**k, sh*t 等
        addPattern("f[aeiou*]+ck");
        addPattern("sh[aeiou*]+t");
    }
    
    bool containsProfanity(const std::string& text) const override {
        std::string lowerText = toLower(text);
        
        for (const auto& pattern : profanityPatterns) {
            if (std::regex_search(lowerText, pattern)) {
                return true;
            }
        }
        return false;
    }
    
    std::string censor(const std::string& text) const override {
        std::string result = text;
        std::string lowerText = toLower(text);
        
        for (const auto& pattern : profanityPatterns) {
            std::smatch match;
            std::string::const_iterator searchStart = lowerText.cbegin();
            
            while (std::regex_search(searchStart, lowerText.cend(), match, pattern)) {
                size_t pos = match.position() + (searchStart - lowerText.cbegin());
                size_t length = match.length();
                
                // 替换脏话为指定字符
                for (size_t i = 0; i < length && pos + i < result.length(); ++i) {
                    result[pos + i] = replacementChar;
                }
                
                searchStart = match.suffix().first;
            }
        }
        
        return result;
    }
    
    void addProfanity(const std::string& word) override {
        addPattern(word);
    }
    
    void loadFromFile(const std::string& filename) override {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "无法打开文件: " << filename << std::endl;
            return;
        }
        
        std::string word;
        while (std::getline(file, word)) {
            if (!word.empty()) {
                addPattern(word);
            }
        }
        file.close();
    }
    
private:
    void addPattern(const std::string& pattern) {
        try {
            profanityPatterns.emplace_back(pattern, std::regex::icase);
        } catch (const std::regex_error& e) {
            std::cerr << "正则表达式错误: " << e.what() << " - 模式: " << pattern << std::endl;
        }
    }
    
    static std::string toLower(const std::string& str) {
        std::string lower = str;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        return lower;
    }
};

/**
 * @brief 字典树节点，用于高效存储和查找脏话
 */
class TrieNode {
public:
    std::unordered_map<char, std::shared_ptr<TrieNode>> children;
    bool isEndOfWord;
    
    TrieNode() : isEndOfWord(false) {}
};

/**
 * @brief 字典树法 - 使用字典树高效检测脏话
 * 
 * 优点：查找效率高，适合大规模脏话列表
 * 缺点：实现较复杂，内存占用较高
 */
class TrieFilter : public ProfanityFilter {
private:
    std::shared_ptr<TrieNode> root;
    char replacementChar;
    
public:
    explicit TrieFilter(char replacementChar = '*') 
        : root(std::make_shared<TrieNode>()), replacementChar(replacementChar) {
        // 默认脏话列表
        std::vector<std::string> defaultWords = {
            "shit", "fuck", "damn", "ass", "bitch", "bastard"
        };
        
        for (const auto& word : defaultWords) {
            addToTrie(word);
        }
    }
    
    bool containsProfanity(const std::string& text) const override {
        std::string lowerText = toLower(text);
        
        for (size_t i = 0; i < lowerText.length(); ++i) {
            auto node = root;
            for (size_t j = i; j < lowerText.length(); ++j) {
                char c = lowerText[j];
                if (node->children.find(c) == node->children.end()) {
                    break;
                }
                
                node = node->children[c];
                if (node->isEndOfWord) {
                    return true;
                }
            }
        }
        return false;
    }
    
    std::string censor(const std::string& text) const override {
        std::string result = text;
        std::string lowerText = toLower(text);
        
        for (size_t i = 0; i < lowerText.length(); ++i) {
            auto node = root;
            size_t wordEnd = std::string::npos;
            size_t wordLength = 0;
            
            for (size_t j = i; j < lowerText.length(); ++j) {
                char c = lowerText[j];
                if (node->children.find(c) == node->children.end()) {
                    break;
                }
                
                node = node->children[c];
                if (node->isEndOfWord) {
                    wordEnd = j;
                    wordLength = j - i + 1;
                }
            }
            
            if (wordEnd != std::string::npos) {
                // 替换脏话为指定字符
                for (size_t k = 0; k < wordLength; ++k) {
                    result[i + k] = replacementChar;
                }
                i = wordEnd; // 跳过已处理的字符
            }
        }
        
        return result;
    }
    
    void addProfanity(const std::string& word) override {
        addToTrie(toLower(word));
    }
    
    void loadFromFile(const std::string& filename) override {
        std::ifstream file(filename);
        if (!file.is_open()) {
            std::cerr << "无法打开文件: " << filename << std::endl;
            return;
        }
        
        std::string word;
        while (std::getline(file, word)) {
            if (!word.empty()) {
                addToTrie(toLower(word));
            }
        }
        file.close();
    }
    
private:
    void addToTrie(const std::string& word) {
        auto node = root;
        for (char c : word) {
            if (node->children.find(c) == node->children.end()) {
                node->children[c] = std::make_shared<TrieNode>();
            }
            node = node->children[c];
        }
        node->isEndOfWord = true;
    }
    
    static std::string toLower(const std::string& str) {
        std::string lower = str;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        return lower;
    }
};

/**
 * @brief 混合过滤器 - 结合多种过滤技术
 * 
 * 优点：结合多种技术的优点，提供更好的过滤效果
 * 缺点：实现复杂，资源消耗较大
 */
class HybridFilter : public ProfanityFilter {
private:
    std::unique_ptr<SimpleReplacementFilter> simpleFilter;
    std::unique_ptr<RegexFilter> regexFilter;
    std::unique_ptr<TrieFilter> trieFilter;
    
    // 使用哪种过滤器（可配置）
    bool useSimpleFilter = true;
    bool useRegexFilter = true;
    bool useTrieFilter = true;
    
public:
    explicit HybridFilter(char replacementChar = '*') {
        simpleFilter = std::make_unique<SimpleReplacementFilter>(replacementChar);
        regexFilter = std::make_unique<RegexFilter>(replacementChar);
        trieFilter = std::make_unique<TrieFilter>(replacementChar);
    }
    
    bool containsProfanity(const std::string& text) const override {
        if (useSimpleFilter && simpleFilter->containsProfanity(text)) {
            return true;
        }
        if (useRegexFilter && regexFilter->containsProfanity(text)) {
            return true;
        }
        if (useTrieFilter && trieFilter->containsProfanity(text)) {
            return true;
        }
        return false;
    }
    
    std::string censor(const std::string& text) const override {
        std::string result = text;
        
        if (useSimpleFilter) {
            result = simpleFilter->censor(result);
        }
        if (useRegexFilter) {
            result = regexFilter->censor(result);
        }
        if (useTrieFilter) {
            result = trieFilter->censor(result);
        }
        
        return result;
    }
    
    void addProfanity(const std::string& word) override {
        simpleFilter->addProfanity(word);
        regexFilter->addProfanity(word);
        trieFilter->addProfanity(word);
    }
    
    void loadFromFile(const std::string& filename) override {
        simpleFilter->loadFromFile(filename);
        regexFilter->loadFromFile(filename);
        trieFilter->loadFromFile(filename);
    }
    
    // 配置使用哪些过滤器
    void configureFilters(bool useSimple, bool useRegex, bool useTrie) {
        useSimpleFilter = useSimple;
        useRegexFilter = useRegex;
        useTrieFilter = useTrie;
    }
};

/**
 * @brief 示例使用和测试
 */
int main() {
    std::cout << "=== C++ 脏话屏蔽模板示例 ===\n\n";
    
    // 测试文本
    std::vector<std::string> testTexts = {
        "What the fuck are you doing?",
        "This is a shitty situation.",
        "You're such a bastard!",
        "I don't give a damn about it.",
        "He's a complete ass.",
        "She's being a real bitch today.",
        "This is f*cking amazing!",  // 变体
        "What a sh*tty day!",        // 变体
        "Hello, how are you today?"  // 清洁文本
    };
    
    // 创建不同过滤器实例
    std::cout << "1. 简单替换过滤器:\n";
    SimpleReplacementFilter simpleFilter('*');
    
    std::cout << "2. 正则表达式过滤器:\n";
    RegexFilter regexFilter('*');
    
    std::cout << "3. 字典树过滤器:\n";
    TrieFilter trieFilter('*');
    
    std::cout << "4. 混合过滤器:\n";
    HybridFilter hybridFilter('*');
    
    // 从文件加载额外脏话列表（如果存在）
    // simpleFilter.loadFromFile("profanity_list.txt");
    
    // 测试所有过滤器
    std::vector<std::pair<std::string, ProfanityFilter*>> filters = {
        {"简单替换", &simpleFilter},
        {"正则表达式", &regexFilter},
        {"字典树", &trieFilter},
        {"混合", &hybridFilter}
    };
    
    for (const auto& [name, filter] : filters) {
        std::cout << "\n=== " << name << "过滤器测试 ===\n";
        
        for (const auto& text : testTexts) {
            std::cout << "原始: " << text << "\n";
            std::cout << "检测到脏话: " << (filter->containsProfanity(text) ? "是" : "否") << "\n";
            std::cout << "处理后: " << filter->censor(text) << "\n";
            std::cout << "---\n";
        }
    }
    
    // 性能测试示例
    std::cout << "\n=== 性能测试示例 ===\n";
    
    // 创建一个长文本进行测试
    std::stringstream longText;
    for (int i = 0; i < 1000; ++i) {
        longText << "This is some text with fuck and shit in it. ";
    }
    std::string testString = longText.str();
    
    // 测试不同过滤器的性能
    auto timeFilter = [](ProfanityFilter& filter, const std::string& text) {
        auto start = std::chrono::high_resolution_clock::now();
        filter.censor(text);
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    };
    
    std::cout << "处理1000次重复文本的时间:\n";
    std::cout << "简单替换过滤器: " << timeFilter(simpleFilter, testString) << " ms\n";
    std::cout << "正则表达式过滤器: " << timeFilter(regexFilter, testString) << " ms\n";
    std::cout << "字典树过滤器: " << timeFilter(trieFilter, testString) << " ms\n";
    
    return 0;
}