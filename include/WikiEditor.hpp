#pragma once
#include <string>
#include <vector>
#include <map>

// Represents a single Wiki Article/Entry
struct LoreArticle {
    int id;
    std::string title;
    std::string content;
    std::vector<std::string> tags;
    std::vector<int> linkedArticles; // IDs of other articles
};

// Wiki Editor Subsystem
class WikiEditor {
public:
    static void Initialize();
    static void DrawEditor(bool* p_open);
    static void OpenArticle(int id);
    static void ImportFromObsidian(const std::string& vaultPath);

private:
    static std::map<int, LoreArticle> s_articles;
    static int s_currentArticleId;
    static std::string s_vaultPath;
};
