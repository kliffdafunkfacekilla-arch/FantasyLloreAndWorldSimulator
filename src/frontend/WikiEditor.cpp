#include "../../include/WikiEditor.hpp"
#include "../../deps/imgui/imgui.h"
#include "../../deps/imgui/misc/cpp/imgui_stdlib.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <regex>

std::map<int, LoreArticle> WikiEditor::s_articles;
int WikiEditor::s_currentArticleId = -1;
std::string WikiEditor::s_vaultPath = "";

void WikiEditor::Initialize() {
    LoreArticle welcome;
    welcome.id = 1;
    welcome.title = "Welcome to the World Wiki";
    welcome.content = "# World Wiki\nThis is a foundational wiki editor. You can link articles, use basic markdown, and manage world building lore here.\n\n*This will eventually support Obsidian Markdown exporting.*";
    welcome.tags = {"system", "guide"};
    s_articles[welcome.id] = welcome;
    s_currentArticleId = 1;
}

void WikiEditor::OpenArticle(int id) {
    if (s_articles.find(id) != s_articles.end()) {
        s_currentArticleId = id;
    }
}

void WikiEditor::ImportFromObsidian(const std::string& vaultPath) {
    s_articles.clear();
    s_currentArticleId = -1;
    s_vaultPath = vaultPath;

    if (!std::filesystem::exists(vaultPath) || !std::filesystem::is_directory(vaultPath)) {
        std::cerr << "[WIKI] Invalid vault path: " << vaultPath << std::endl;
        return;
    }

    int nextId = 1;
    std::map<std::string, int> titleToId; // For resolving links

    // Pass 1: Read all files and create articles
    for (const auto& entry : std::filesystem::recursive_directory_iterator(vaultPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".md") {
            LoreArticle article;
            article.id = nextId++;
            article.title = entry.path().stem().string(); // Filename without extension

            std::ifstream file(entry.path());
            if (file.is_open()) {
                std::stringstream buffer;
                buffer << file.rdbuf();
                std::string content = buffer.str();

                // Extremely basic frontmatter tag parsing (YAML list)
                std::regex tagsRegex(R"(tags:\s*\[(.*?)\])");
                std::smatch match;
                if (std::regex_search(content, match, tagsRegex)) {
                    std::string tagsStr = match[1].str();
                    std::stringstream ss(tagsStr);
                    std::string tag;
                    while (std::getline(ss, tag, ',')) {
                        // Trim whitespace
                        tag.erase(0, tag.find_first_not_of(" \t\r\n'\""));
                        tag.erase(tag.find_last_not_of(" \t\r\n'\"") + 1);
                        if (!tag.empty()) {
                            article.tags.push_back(tag);
                        }
                    }
                }

                article.content = content;
                s_articles[article.id] = article;
                titleToId[article.title] = article.id;
            }
        }
    }

    // Pass 2: Resolve [[Wiki Links]]
    std::regex linkRegex(R"(\[\[(.*?)\]\])");
    for (auto& pair : s_articles) {
        LoreArticle& article = pair.second;
        std::sregex_iterator next(article.content.begin(), article.content.end(), linkRegex);
        std::sregex_iterator end;
        while (next != end) {
            std::smatch match = *next;
            std::string linkedTitle = match[1].str();
            // Handle aliases [[Title|Alias]]
            size_t pipePos = linkedTitle.find('|');
            if (pipePos != std::string::npos) {
                linkedTitle = linkedTitle.substr(0, pipePos);
            }

            if (titleToId.find(linkedTitle) != titleToId.end()) {
                article.linkedArticles.push_back(titleToId[linkedTitle]);
            }
            next++;
        }
    }

    if (!s_articles.empty()) {
        s_currentArticleId = s_articles.begin()->first;
        std::cout << "[WIKI] Imported " << s_articles.size() << " articles from " << vaultPath << std::endl;
    }
}


void WikiEditor::DrawEditor(bool* p_open) {
    if (!*p_open) return;

    ImGui::SetNextWindowSize(ImVec2(900, 600), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Wiki & Lore Editor", p_open)) {
        ImGui::End();
        return;
    }

    // Top Bar for Importing
    static std::string inputPath = "./vault";
    ImGui::InputText("Vault Path", &inputPath);
    ImGui::SameLine();
    if (ImGui::Button("Import Obsidian Vault")) {
        ImportFromObsidian(inputPath);
    }

    ImGui::Separator();

    // Left Pane: Article List
    ImGui::BeginChild("ArticleList", ImVec2(250, 0), true);
    if (ImGui::Button("New Article", ImVec2(-1, 0))) {
        int newId = s_articles.empty() ? 1 : s_articles.rbegin()->first + 1;
        LoreArticle newArticle;
        newArticle.id = newId;
        newArticle.title = "Untitled Article";
        s_articles[newId] = newArticle;
        s_currentArticleId = newId;
    }

    ImGui::Separator();

    for (const auto& pair : s_articles) {
        bool isSelected = (s_currentArticleId == pair.first);
        if (ImGui::Selectable(pair.second.title.c_str(), isSelected)) {
            s_currentArticleId = pair.first;
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // Right Pane: Editor / Viewer
    ImGui::BeginChild("ArticleContent", ImVec2(0, 0), false);

    if (s_currentArticleId != -1 && s_articles.find(s_currentArticleId) != s_articles.end()) {
        LoreArticle& article = s_articles[s_currentArticleId];

        ImGui::InputText("Title", &article.title);

        ImGui::Separator();
        ImGui::Text("Content");

        // Simple InputTextMultiline for markdown editing
        ImGui::InputTextMultiline("##source", &article.content, ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 16), ImGuiInputTextFlags_AllowTabInput);

        // Tags
        ImGui::Separator();
        ImGui::Text("Tags");
        std::string tagStr = "";
        for (size_t i = 0; i < article.tags.size(); ++i) {
            tagStr += article.tags[i];
            if (i < article.tags.size() - 1) tagStr += ", ";
        }
        ImGui::TextWrapped("Tags: %s", tagStr.empty() ? "None" : tagStr.c_str());

        // Links
        ImGui::Separator();
        ImGui::Text("Linked Articles");
        if (article.linkedArticles.empty()) {
            ImGui::TextDisabled("No links.");
        } else {
            for (int linkedId : article.linkedArticles) {
                if (s_articles.find(linkedId) != s_articles.end()) {
                    if (ImGui::Button(s_articles[linkedId].title.c_str())) {
                        s_currentArticleId = linkedId;
                    }
                    ImGui::SameLine();
                }
            }
            ImGui::NewLine();
        }
    } else {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "Select an article or create a new one.");
    }

    ImGui::EndChild();

    ImGui::End();
}
